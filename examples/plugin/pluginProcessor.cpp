// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "pluginProcessor.h"
#include "pluginController.h"
#include "pluginParameters.h"
#include "parameters.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstpresetkeys.h"  // for use of IStreamAttributes
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include <cmath>
#include <cstdlib>
#include <math.h>
#include <iostream>

// TODO cleanup
/*
#ifdef WIN32
#undef UNICODE
#include "windows.h"
#undef min
#undef max
#endif
*/
// implementation: a handle to this instance of the windows module
void* gHINSTANCE{ nullptr };


namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

FUID PluginProcessor::uid(0xBBBBBBBB, 0xBBBBBBBB, 0xBBBBBBBB, 0xBBBBBBBB);

PluginProcessor::PluginProcessor()
{
  // register its editor class(the same than used in againentry.cpp)
	setControllerClass(PluginController::uid);
  
  // read parameter descriptions from parameters.h
  createPluginParameters(_parameterDescriptions);
  
  // make normalized <-> real projections
  createProjections(_parameterDescriptions);
  
  setParameterDefaults();
}

PluginProcessor::~PluginProcessor()
{
  _ioTimer.stop();
}

tresult PLUGIN_API PluginProcessor::initialize(FUnknown* context)
{
  //---always initialize the parent-------
  tresult result = AudioEffect::initialize(context);

  if(result != kResultOk)
  {
    return result;
  }
  
  //---create Audio In/Out buses------
  // we want a stereo Input and a Stereo Output
  addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  
  // modulated frequency L
  publishSignal("freq_l_mod", 1, 4);
  
  publishSignal("scope", 2, 2);
  publishSignal("input_meter", 2, 4);
  publishSignal("output_meter", 2, 4);
  
  _ioTimer.start([=](){sendPublishedSignalsToController();}, milliseconds(1000/60));

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::terminate()
{
  return AudioEffect::terminate();
}

tresult PLUGIN_API PluginProcessor::setActive(TBool state)
{
  return AudioEffect::setActive(state);
}

tresult PLUGIN_API PluginProcessor::process(ProcessData& data)
{
  processParameterChanges(data.inputParameterChanges);
  
  // TODO for instruments
  // processEvents(data.inputEvents);
  
  processSignals(data);
  
  return kResultTrue;
}

tresult PLUGIN_API PluginProcessor::setState(IBStream* state)
{
  // called when we load a preset, the model has to be reloaded
  
  // let's use the Steinberg streaming class
  IBStreamer streamer(state, kLittleEndian);

  for(auto& p : _parameterDescriptions)
  {
    Path paramName = p->getTextProperty("name");
    float fTemp;
    streamer.readFloat(fTemp);
    setProperty(paramName, fTemp);
  }
  
std::cout << "PluginProcessor SET STATE: \n";
dump();

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::getState(IBStream* state)
{
  // here we need to save the model
  IBStreamer streamer(state, kLittleEndian);
 
  for(auto& p : _parameterDescriptions)
  {
    Path paramName = p->getTextProperty("name");
    streamer.writeFloat(getFloatProperty(paramName));
  }
  
  std::cout << "PluginProcessor GET STATE: \n";
  dump();
  
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::setupProcessing(ProcessSetup& newSetup)
{
  // called before the process call, always in a disable state(not active)
  // here we could keep a trace of the processing mode(offline,...) for example.
  // currentProcessMode = newSetup.processMode;
  
  _sampleRate = newSetup.sampleRate;
  return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API PluginProcessor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
  if(numIns)
  {
    if(SpeakerArr::getChannelCount(inputs[0]) != 2)
      return kResultFalse;
  }
  if(numOuts)
  {
    if(SpeakerArr::getChannelCount(outputs[0]) != 2)
      return kResultFalse;
  }
  return kResultTrue;
}

tresult PLUGIN_API PluginProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
  if(symbolicSampleSize == kSample32)
    return kResultTrue;
  
  // we support double processing
  if(symbolicSampleSize == kSample64)
    return kResultTrue;
  
  return kResultFalse;
}

tresult PLUGIN_API PluginProcessor::notify(IMessage* message)
{
  // we could respond to messages here
  return AudioEffect::notify(message);
}

// --------------------------------------------------------------------------------
// private implementation


bool PluginProcessor::processParameterChanges(IParameterChanges* changes)
{
  if(changes)
  {
    int32 numParamsChanged = changes->getParameterCount();
    
    // for each parameter which are some changes in this audio block:
    for(int32 i = 0; i < numParamsChanged; i++)
    {
      IParamValueQueue* paramQueue = changes->getParameterData(i);
      if(paramQueue)
      {
        ParamValue value;
        int32 sampleOffset;
        int32 numPoints = paramQueue->getPointCount();
        int32 id = paramQueue->getParameterId();
        
        // TODO sample-accurate, smoothing
        
//        if (ml::within(id, 0, (int)_parameterDescriptions.size()))
        if ((id >= 0) && (id < _parameterDescriptions.size()))
          {
          ParameterDescription* pDesc = _parameterDescriptions[id].get();
          
          Path paramName = pDesc->getTextProperty("name");
          if(paramName)
          {
            if(paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
            {
              // convert the normalized value to the real value and set the property.
              // DEBUG
              std::cout << "PluginProcessor: setting " << paramName << " to " << pDesc->normalizedToReal(value) << "\n";
              
              setProperty(paramName, pDesc->normalizedToReal(value));
            }
          }
        }
      }
    }
  }
  return false;
}

void PluginProcessor::setParameterDefaults()
{
  int nParams = _parameterDescriptions.size();
  for(int id=0; id < nParams; ++id)
  {
    ParameterDescription* pDesc = _parameterDescriptions[id].get();
    Path paramName = pDesc->getTextProperty("name");
    if(paramName)
    {
      float defaultVal = pDesc->getFloatProperty("default");
      setProperty(paramName, pDesc->normalizedToReal(defaultVal));
    }
  }
}

void PluginProcessor::publishSignal(Symbol signalName, int channels, int octavesDown)
{
  _publishedSignals[signalName] = ml::make_unique< PublishedSignal >(channels, octavesDown);
  
}

// store a DSPVectorArray to the named signal buffer.
// we need buffers here in the Processor because we can't communicate with
// the Controller in the audio thread.
template<size_t CHANNELS>
void PluginProcessor::storePublishedSignal(Symbol signalName, DSPVectorArray< CHANNELS > v)
{
  PublishedSignal* publishedSignal = _publishedSignals[signalName].get();
  if(publishedSignal)
  {
    publishedSignal->write(v);
  }
}

void PluginProcessor::sendPublishedSignalsToController()
{
  for(auto it = _publishedSignals.begin(); it != _publishedSignals.end(); ++it)
  {
    Symbol signalName = it.getCurrentNodeName();
    const std::unique_ptr < PublishedSignal >& publishedSignal = *it;
    size_t samplesAvailable = publishedSignal->getReadAvailable();
    
    if(samplesAvailable)
    {
      if (IPtr<IMessage> message = owned(allocateMessage()))
      {
        message->setMessageID (signalName.getUTF8Ptr());
        
        // add space for signal width
        size_t dataSize = samplesAvailable + 1;

        // MLTEST
        // alloc temp space for read
        std::vector<float> tempVec(dataSize);
        
        // write # of channels for controller
        tempVec[0] = publishedSignal->getNumChannels();
        
        // read floats from downsampled signal to temp space
        publishedSignal->read(tempVec.data() + 1, samplesAvailable);

        // make binary message - this copies the data to the message
        message->getAttributes()->setBinary(vstBinaryAttrID, tempVec.data(), dataSize*sizeof(float));

        sendMessage(message);
      }
    }
  }
}

void PluginProcessor::processSignals(ProcessData& data)
{
  if(data.numInputs == 0 || data.numOutputs == 0)
  {
    // nothing to do
    return;
  }
  
  // mark our outputs has not silent
  data.outputs[0].silenceFlags = 0;
  
  // cast I/O pointers: necessary ugliness due to VST's use of void*
  void** in = getChannelBuffersPointer(processSetup, data.inputs[0]);
  void** out = getChannelBuffersPointer(processSetup, data.outputs[0]);
  const float** inputs = const_cast<const float **>(reinterpret_cast<float**>(in));
  float** outputs = reinterpret_cast<float**>(out);

  // run buffered processing
  processBuffer.process(inputs, outputs, data.numSamples, processFn);

  // test
  // static periodicAction()
  //doEverySecond(data.numSamples, [](){ std::cout << "tick \n";} );
  static int c{0};
  c += data.numSamples;
  if(c > _sampleRate)
  {
    c -= _sampleRate;

  //  DBGMSG("PluginProcessor tick : freqL = %f, freqR = %f\n", getFloatProperty("freq_l"), getFloatProperty("freq_r"));
    //std::cout << "PluginProcessor tick : freqL = " << getFloatProperty("freq_l") << ", freqR = " << getFloatProperty("freq_r") << "\n";
  }
}

// processVectors() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
DSPVectorArray<kOutputChannels> PluginProcessor::processVectors(const DSPVectorArray<kInputChannels>& inputVectors)
{
  float fGain = getFloatProperty("gain");
  float fFreqL = getFloatProperty("freq_l");
  float fFreqR = getFloatProperty("freq_r");
  int bBypass = getFloatProperty("bypass");

  float fFreqLLfoRate = getFloatProperty("freq_l/lfo/rate");
  float fFreqLLfoAmount = getFloatProperty("freq_l/lfo/amount");

  // testing LFO just sampled once per vector here
  auto lfoOscL = lfoL1(fFreqLLfoRate/_sampleRate);
  auto lfoOscUnipolar = 0.5f*(1.0f + lfoOscL); // [0 - 1]
  float fLfoL = lfoOscUnipolar[0] * fFreqLLfoAmount;
  float m = powf(2.0f, fLfoL);
   
  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  auto sineL = s1(fFreqL*m/_sampleRate)*fGain;
  auto sineR = s2(fFreqR/_sampleRate)*fGain;
  
  // store published output signals to they can be read later by subscribers.
  // currently DSP graph nodes don't have names, except in the context of this function, so
  // we just write them here explicitly one by one. When we are reading a DSP graph from
  // a text description, nodes will have names and we can do smth like "publishSignalOutput(graphNodeName, externalName)"
  // to make a list
  auto test1 = abs(st1(2.0f/_sampleRate));
  auto test2 = abs(st2(3.0f/_sampleRate));

  if(bBypass)
  {
    // default constuctor for DSPVectorArray returns zero-initialized buffers
    return DSPVectorArray<kOutputChannels>();
  }
  else
  {
    //  storePublishedSignal("input_meter", append(mRMSInL(inputL), mRMSInR(inputR)));
    //  storePublishedSignal("output_meter", append(mRMSOutL(outputL), mRMSOutR(outputR)));

    storePublishedSignal("freq_l_mod", DSPVector(fFreqL*m));

    storePublishedSignal("input_meter", concatRows(test1, test2));
    storePublishedSignal("output_meter", concatRows(test2, test1));
    storePublishedSignal("scope", concatRows(sineL, sineR));
    
    // appending the two DSPVectors makes a DSPVectorArray<2>: our stereo output.
    return concatRows(sineL, sineR);
  }
}

}}} // namespaces

