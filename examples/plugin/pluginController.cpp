// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "base/source/fstring.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

#include "pluginController.h"
#include "pluginEditorView.h"
#include "pluginParameters.h"

// param definitions for this plugin
#include "parameters.h"

#include <cmath>
#include <iostream>

#include "MLSerialization.h"


namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

FUID PluginController::uid(0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA);

const char* vstBinaryAttrID{"b"};

//-----------------------------------------------------------------------------
// PluginController implementation

PluginController::PluginController()
{
  // start timers in main thread.
  _timers->start(true);
}

PluginController::~PluginController()
{
  // don't stop the master Timers-- there may be other plugin instances using it!
}

#pragma mark ComponentBase

tresult PLUGIN_API PluginController::notify(IMessage* message)
{
  if (!message) return kInvalidArgument;

  const char * signalName = message->getMessageID();
  
  // could add signal header if we want to send any other messages
  
  const void* messageData;
  uint32 messageSizeInBytes;
    
  if (message->getAttributes()->getBinary(vstBinaryAttrID, messageData, messageSizeInBytes) == kResultOk)
  {
    //std::cout  << "notify: received " << signalName << ", " << messageSizeInBytes << " bytes\n";
    
    const float * pFloatData = reinterpret_cast< const float* >(messageData);
    
    if(messageSizeInBytes > sizeof(float))
    {
      // get signal width
      size_t signalWidthInChannels = pFloatData[0];
      if(signalWidthInChannels)
      {
        // lazily make DSPBuffer if needed
        if(!_signalsFromProcessor[signalName])
        {
            _signalsFromProcessor[signalName] = ml::make_unique< DSPBuffer >();
            
            // set buffer size in frames
            _signalsFromProcessor[signalName]->resize(kPublishedSignalBufferSize*signalWidthInChannels);
            
            // set signal width info
            //_processorSignalWidths[signalName] = VECTORS;
           std::cout  << "        made buffer for " << signalName << ", " << signalWidthInChannels << " channels\n";
        }
      
        // write message data to DSPBuffer
        DSPBuffer* sigBuf = _signalsFromProcessor[signalName].get();
        size_t samplesInMessage = (messageSizeInBytes - sizeof(float))/sizeof(float);

        sigBuf->write(pFloatData + 1, samplesInMessage);
    
        //std::cout  << "        writing " << signalName << ", " << samplesInMessage << " samples\n";
      }
    }
    
    return kResultOk;
  }

  return EditController::notify(message);
}

#pragma mark EditController

tresult PLUGIN_API PluginController::initialize(FUnknown* context)
{
  tresult result = EditController::initialize(context);
  if(result != kResultOk)
  {
    return result;
  }
  
  // read parameter descriptions from parameters.h
  createPluginParameters(_parameterDescriptions);
  
  // make normalized <-> real projections
  createProjections(_parameterDescriptions);

  // set initial param values, allocating any needed memory
  for(auto & paramPtr : _parameterDescriptions)
  {
    ParameterDescription& param = *paramPtr;
    
    auto paramName = param.getProperty("name").getTextValue();
    auto paramDefault = param.getProperty("default").getFloatValue();
    std::cout << " setting " << paramName << " to default " << paramDefault << "\n";
    
    //auto paramPath = Path(param.getProperty("name").getTextValue());
    
    // create and set the default value
    _paramValues.push_back ( param.getProperty("default") );
  }
  
  // store param ids by name
  for(int i=0; i < _parameterDescriptions.size(); ++i)
  {
    ParameterDescription& param = *_parameterDescriptions[i];
    Path paramName = param.getProperty("name").getTextValue();
    _paramIDsByName[paramName] = i;
  }

  // generate Steinberg parameters for each parameter
  // the info is needed in this form by some Steinberg API elsewhere (auwrapper is one place)
  // it might not be necessary to save these: if we override getParameterInfo() we could generate them on the fly.
  for(int i=0; i < _parameterDescriptions.size(); ++i)
  {
    ParameterDescription& param = *_parameterDescriptions[i];
    bool isAutomatable = param.getProperty("automatable").getBoolValueWithDefault(true);
    bool isReadOnly = param.getProperty("read_only").getBoolValueWithDefault(false);
    const char *name = param.getProperty("name").getTextValue().getText();
    double defaultVal = param.getProperty("default").getFloatValue();
    int stepCount = param.getProperty("stepcount").getFloatValueWithDefault(0);
    bool isBypass = param.getProperty("name").getTextValue() == "bypass";
    
    int32 flags = 0;
    if(isReadOnly) flags |= ParameterInfo::kIsReadOnly;
    if(isAutomatable && !isReadOnly) flags |= ParameterInfo::kCanAutomate;
    if(isBypass) flags |= ParameterInfo::kIsBypass;

    EditController::parameters.addParameter (Steinberg::String(name), nullptr, stepCount, defaultVal, flags, i);
  }

  return result;
}

tresult PLUGIN_API PluginController::terminate()
{
	return EditController::terminate();
}

tresult PLUGIN_API PluginController::setComponentState(IBStream* state)
{
  // receive the current state of the processor
  if(!state) return kResultFalse;
  
  IBStreamer streamer(state, kLittleEndian);
  
  for(auto& p : _parameterDescriptions)
  {
    Path paramName = p->getTextProperty("name");
    float fTemp;
    streamer.readFloat(fTemp);
    int id = getParamIDByName(paramName);
    float fNorm = plainParamToNormalized(id, fTemp);
    setParamNormalized(id, fNorm);
  }
  
  return kResultOk;
}

tresult PLUGIN_API PluginController::setState (IBStream* state)
{
  if(!state) return kResultFalse;
  IBStreamer streamer(state, kLittleEndian);
  
  std::vector<unsigned char> dataVec;
  
  int32 size{};
  streamer.readInt32(size);
  
  // TODO errors!
  dataVec.resize(size);
  
  auto bytesRead = streamer.readRaw(dataVec.data(), size);
  std::cout << "PluginController::setState: " << bytesRead << " bytes read \n";
  
  overwrite(binaryToPropertyTree(dataVec));
    dump();
  return kResultOk;
}

tresult PLUGIN_API PluginController::getState (IBStream* state)
{
  if(!state) return kResultFalse;
  IBStreamer streamer(state, kLittleEndian);
  
  auto binaryData = propertyTreeToBinary();
  streamer.writeInt32(binaryData.size());
  streamer.writeRaw(binaryData.data(), binaryData.size());

  std::cout << "\n\nPluginController::getState: \n";
  dump();
  return kResultOk;
}

int32 PLUGIN_API PluginController::getParameterCount()
{
  int32 c = EditController::getParameterCount();
  return c;
}

tresult PLUGIN_API PluginController::getParamStringByValue (ParamID id, ParamValue valueNormalized, String128 string)
{
  const int precision = 2;
  TextFragment number = textUtils::floatNumberToText(normalizedParamToPlain(id, valueNormalized), precision);
  TextFragment suffix = _parameterDescriptions[id]->getProperty("units").getTextValue();
  TextFragment numberWithSuffix = suffix ? (TextFragment (number, " ", suffix)) : (number);
  Steinberg::UString(string, 128).fromAscii(numberWithSuffix.getText());
  return kResultTrue;
}

tresult PLUGIN_API PluginController::getParamValueByString (ParamID id, TChar* string, ParamValue& valueNormalized)
{
  String wrapper((TChar*)string); // don't know buffer size here!

  UString128 tmp(string);
  char ascii[128];
  tmp.toAscii(ascii, 128);

  Text inputText(ascii);
  // to finish in madronalib //  valueNormalized = plainParamToNormalized(id, textUtils::textToFloatNumber(inputText));

  float val {NAN};
  sscanf(inputText.getText(), "%f", &val);
  
  return kResultTrue;
}

ParamValue PLUGIN_API PluginController::normalizedParamToPlain (ParamID id, ParamValue valueNormalized)
{
  ParameterDescription& param = *_parameterDescriptions[id];
    
  // MLTEST
    if(isnan(valueNormalized))
    {
      std::cout << "huh?\n";
    }
  
  // std::cout << "normalizedParamToPlain: " << valueNormalized << " -> " << param.normalizedToReal(clamp(valueNormalized, 0., 1.)) << "\n";
  
  return param.normalizedToReal(valueNormalized);
}

ParamValue PLUGIN_API PluginController::plainParamToNormalized (ParamID id, ParamValue plainValue)
{
  ParameterDescription& param = *_parameterDescriptions[id];
  return param.realToNormalized(plainValue);
}

ParamValue PLUGIN_API PluginController::getParamNormalized (ParamID id)
{
  return _paramValues[id].getFloatValue();
}

tresult PLUGIN_API PluginController::setParamNormalized (ParamID id, ParamValue value)
{
  Path paramName = getParamNameByID(id);
  if(paramName)
  {
    _changesToReport.push(ValueChange{concat("param", paramName), value});
  }
  _paramValues[id] = value;
  return EditController::setParamNormalized(id, value);
}

Path PluginController::getParamNameByID(int id)
{
  ParameterDescription& param = *_parameterDescriptions[id];
  return Path(param.getProperty("name").getTextValue());
}

int PluginController::getParamIDByName(Path paramPath)
{
  if(!paramPath.getSize()) return -1;
  if(auto node = _paramIDsByName.getConstNode(paramPath))
  {
    return _paramIDsByName[paramPath];
  }
  else
  {
    return -1;
  }
}

void PluginController::reportAllParameterValues()
{
  for(int i=0; i<_parameterDescriptions.size(); ++i)
  {
    _changesToReport.push(ValueChange{concat("param", getParamNameByID(i)), getParamNormalized(i)});
  }
}

IPlugView* PLUGIN_API PluginController::createView (const char* name)
{
  // someone wants my editor
  if (name && strcmp (name, "editor") == 0)
  {
    auto defaultSize = getMatrixPropertyWithDefault("view_size", { kGridUnitsX*kDefaultGridSize, kGridUnitsY*kDefaultGridSize });
    int w = defaultSize[0];
    int h = defaultSize[1];
    
    // TODO view should not get Controller ref. rather, a message stream.
    auto* view = new PluginEditorView (*this, w, h, gHINSTANCE);
    
    // TODO send all parameter descriptions to Editor via message stream
    
    reportAllParameterValues();
    return view;
  }
  return nullptr;
}

ParameterDescription* PluginController::getParamDescriptionByPath(Path paramName)
{
  ParameterDescription* returnVal {nullptr};
  if (_paramIDsByName.getConstNode(paramName))
  {
    int idx = _paramIDsByName[paramName];
    returnVal = _parameterDescriptions[idx].get();
  }
  return returnVal;
}

ParameterDescription* PluginController::getParamDescriptionByIndex(int i)
{
  ParameterDescription* returnVal {nullptr};
  if (within(i, 0, static_cast<int>(_parameterDescriptions.size())))
  {
    returnVal = _parameterDescriptions[i].get();
  }
  return returnVal;
}

void PluginController::handleChangeFromEditor(ValueChange v)
{
  if(!v.name) return;

  switch(hash(head(v.name)))
  {
    case(hash("param")):
      {
        // TODO undo
        // controller->addToUndoHistory(CoalesceValueChange(v));

        int paramID = getParamIDByName(tail(v.name));
        
        // if ID is valid, call VST3 API to do edit
        if(within(paramID, 0, (int)_paramValues.size()))
        {
          if(v.startGesture)
          {
            std::cout << "BEGIN " << paramID << "\n";
            beginEdit(paramID);
          }

          std::cout << "handleChangeFromEditor: " << paramID << " -> " << v.newValue.getFloatValue() << "\n";

          // test
          setParamNormalized(paramID, v.newValue.getFloatValue());
          
          performEdit(paramID, v.newValue.getFloatValue());
          
          
          // DEBUG
// 0.5           std::cout << "handleChangeFromEditor: " << paramID << " -> " << _paramValues[paramID].getFloatValue();
          std::cout << "                      : " << paramID << " -> " << getParamNormalized(paramID) << "\n";
          
          if(v.endGesture)
          {
            std::cout << "END " << paramID << "\n";
            endEdit(paramID);
          }
        }
        
        
      }
      break;
    case(hash("view_size")):
      setProperty("view_size", v.newValue);
      break;
    default:
      break;
  }
}


}}} // namespaces
