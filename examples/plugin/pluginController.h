// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/base/ustring.h"

#include "pluginProcessor.h"

#include "pluginParameters.h"

using namespace ml;

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

const int kChangeQueueSize = 128;

extern const char* vstBinaryAttrID;


//-----------------------------------------------------------------------------
class PluginController : public EditController, public PropertyTree
{
public:
  
  // create function required for Plug-in factory,
  // it will be called to create new instances of this controller
  static FUnknown* createInstance(void*)
  {
    return (IEditController*)new PluginController;
  }
  static FUID uid;
  
	PluginController();
	~PluginController();
  
  // from ComponentBase
  tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;

  // from IPluginBase
  tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;
  
  // from EditController
  tresult PLUGIN_API setComponentState(IBStream* state) SMTG_OVERRIDE;  
  tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE;
  int32 PLUGIN_API getParameterCount () SMTG_OVERRIDE;
  // tresult PLUGIN_API getParameterInfo (int32 paramIndex, ParameterInfo& info) SMTG_OVERRIDE;
  tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string) SMTG_OVERRIDE;
  tresult PLUGIN_API getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized) SMTG_OVERRIDE;
  ParamValue PLUGIN_API normalizedParamToPlain (ParamID tag, ParamValue valueNormalized) SMTG_OVERRIDE;
  ParamValue PLUGIN_API plainParamToNormalized (ParamID tag, ParamValue plainValue) SMTG_OVERRIDE;
  ParamValue PLUGIN_API getParamNormalized (ParamID tag) SMTG_OVERRIDE;
  tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE;
  // tresult PLUGIN_API setComponentHandler (IComponentHandler* handler) SMTG_OVERRIDE;
  IPlugView* PLUGIN_API createView (const char* name) SMTG_OVERRIDE;
  DELEGATE_REFCOUNT(EditController)
  
  // PluginController interface
  
  // called by the view to get changes. getting changes from the queue only works correctly if there is a single consumer,
  // in this case, the view. this is not currently enforced.
  ValueChange getChange() { return _changesToReport.pop(); }
  
  // make changes synchronously from PluginEditor. TODO message, not sync
  void handleChangeFromEditor(ValueChange v);

  void reportAllParameterValues();
  Path getParamNameByID(int tag);
  int getParamIDByName(Path p);
  ParameterDescription* getParamDescriptionByPath(Path paramName);
  ParameterDescription* getParamDescriptionByIndex(int i);
  
  DSPBuffer* getSignalFromProcessor(Symbol signalName) { return _signalsFromProcessor[signalName].get(); }


// TEMP public
  // TODO message pipe, serialized
  // the template type would need to be serializable for RemoteQueue
  Queue< ValueChange > _changesToReport{ kChangeQueueSize };
  
private:

  // this vector contains a description of each parameter in order of ID.
  // it is filled in by the createPluginParameters() defined for this plugin in parameters.h.
  std::vector< std::unique_ptr< ParameterDescription > > _parameterDescriptions;
  
  // this vector of values is the plugin's current state.
  std::vector< ml::Value > _paramValues;
  
  // TODO a param should be a thing with a value and a description and a bijection.
  // since the bijection itself cannot be serialized, it is created from the description
  // as a post processing step after param descriptions are communicated
  
  Tree< int > _paramIDsByName;
  
  // signals received from processor for signal viewers, transmitters, etc
  Tree< std::unique_ptr < DSPBuffer > > _signalsFromProcessor;

  SharedResourcePointer< ml::Timers > _timers ;
  
};

}}} // namespaces
