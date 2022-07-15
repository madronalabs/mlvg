// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include "mldsp.h"
#include "madronalib.h"
#include "MLPlatform.h"
#include "pluginParameters.h"

#include "MLDebug.h"

extern void* gHINSTANCE;

using namespace ml;


//-----------------------------------------------------------------------------

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

constexpr int kMaxProcessBlockFrames = 4096;
constexpr int kInputChannels = 2;
constexpr int kOutputChannels = 2;


//-----------------------------------------------------------------------------
class PluginProcessor : public AudioEffect, public PropertyTree
{
public:
  static FUnknown* createInstance(void*) { return(IAudioProcessor*)new PluginProcessor; }
  static FUID uid;

  PluginProcessor();
  ~PluginProcessor();

  // AudioEffect interface
  tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate() SMTG_OVERRIDE;
  tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
  tresult PLUGIN_API process(ProcessData& data) SMTG_OVERRIDE;
  tresult PLUGIN_API setState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState(IBStream* state) SMTG_OVERRIDE;
  tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup) SMTG_OVERRIDE;
  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) SMTG_OVERRIDE;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) SMTG_OVERRIDE;
  tresult PLUGIN_API notify(IMessage* message) SMTG_OVERRIDE;
  
private:
  bool processParameterChanges(IParameterChanges* changes);
  void processSignals(ProcessData& data);
  
  // this vector contains a description of each parameter in order of ID.
  // it is filled in by the createPluginParameters() defined for this plugin in parameters.h.
  std::vector< std::unique_ptr< ParameterDescription > > _parameterDescriptions;
  
  // buffer object to call processVectors from process() calls of arbitrary frame sizes
  VectorProcessBuffer<kInputChannels, kOutputChannels, kMaxProcessBlockFrames> processBuffer;

  // declare the processVectors function that will run our DSP in vectors of size kFloatsPerDSPVector
  DSPVectorArray<kOutputChannels> processVectors(const DSPVectorArray<kInputChannels>& inputVectors);
  
  // set up the function parameter for processBuffer.process()
  using processFnType = std::function<DSPVectorArray<kOutputChannels>(const DSPVectorArray<kInputChannels>&)>;
  processFnType processFn{ [&](const DSPVectorArray<kInputChannels> inputVectors) { return processVectors(inputVectors); } };
  
  float _sampleRate{0.f};
  
  // sine generators.
  SineGen s1, s2;
  
  // debug
  SineGen st1, st2;
  
  // TODO LFOGen
  SineGen lfoL1;
  

  
  // meter machinery
  ml::RMS mRMSInL, mRMSOutL, mRMSInR, mRMSOutR;
  
  // private methods
  void setParameterDefaults();
  
  class PublishedSignal
  {
    Downsampler _downsampler;
    DSPBuffer _buffer;
    size_t _channels{0};
    
  public:
    PublishedSignal(int channels, int octavesDown) :
      _downsampler(channels, octavesDown),
      _channels(channels)
    {
      _buffer.resize(kPublishedSignalBufferSize*_channels);
    }
    
    ~PublishedSignal() = default;
    
    size_t getNumChannels() { return _channels; }
    int getReadAvailable() { return _buffer.getReadAvailable(); }
    
    // write a single vector of data.
    template< size_t CHANNELS >
    void write(DSPVectorArray< CHANNELS > v)
    {
      if(_downsampler.write(v))
      {
        _buffer.write(_downsampler.read< CHANNELS >());
      }
    }
    
    int read(float* pDest, size_t framesRequested)
    {
      return _buffer.read(pDest, framesRequested);
    }
  };
  
  Tree< std::unique_ptr < PublishedSignal > > _publishedSignals;
  
  void publishSignal(Symbol signalName, int channels, int octavesDown = 0);
  
  template< size_t CHANNELS >
  void storePublishedSignal(Symbol signalName, DSPVectorArray< CHANNELS > v);

  void sendPublishedSignalsToController();
  
  
  ml::Timer _ioTimer;
  
  
};

}}} // namespaces

