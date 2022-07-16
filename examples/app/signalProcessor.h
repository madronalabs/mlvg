// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "mldsp.h"
#include "madronalib.h"
#include "MLPlatform.h"
#include "MLParameters.h"

constexpr size_t kPublishedSignalMaxChannels = 2;
constexpr size_t kPublishedSignalReadFrames = 128;

namespace ml {

// TODO not a template!

template<int IN_CHANS, int OUT_CHANS>
class SignalProcessor
{
public:
  
  class PublishedSignal
  {
    Downsampler _downsampler;
    DSPBuffer _buffer;
    size_t _channels{0};
    
  public:
    
    bool test{0};

    PublishedSignal(int channels, int octavesDown) :
    _downsampler(channels, octavesDown),
    _channels(channels)
    {
      _buffer.resize(kPublishedSignalReadFrames*_channels);
    }
    
    ~PublishedSignal() = default;
    
    inline size_t getNumChannels() const { return _channels; }
    inline int getAvailableFrames() const { return _buffer.getReadAvailable()/_channels; }
    
    // write a single DSPVectorArray< CHANNELS > of data.
    template< size_t CHANNELS >
    inline void write(DSPVectorArray< CHANNELS > v)
    {
      // write to downsampler, which returns true if there is a new output vector
      if(_downsampler.write(v))
      {
        // write output vector array to the buffer
        _buffer.write(_downsampler.read< CHANNELS >());
        
        // MLTEST
        //if(test) std::cout << "        PublishedSignal write: writing " << kFloatsPerDSPVector << " frames\n";
      }
    }
    
    inline size_t readLatest(float* pDest, size_t framesRequested)
    {
      size_t available = getAvailableFrames();
      //if(test) std::cout << "        PublishedSignal readLatest: available " << available << " \n";

      if(available > framesRequested)
      {
        _buffer.discard((available - framesRequested)*_channels);
        
        // MLTEST
        //if(test) std::cout << "        PublishedSignal readLatest: discard " << available - framesRequested << " frames\n";

        
      }
      auto readResult = _buffer.read(pDest, framesRequested*_channels);

      //if(test) std::cout << "        PublishedSignal readLatest: read " << readResult/_channels << " frames\n";

      return readResult;

    }
    
    inline size_t read(float* pDest, size_t framesRequested)
    {
      return _buffer.read(pDest, framesRequested*_channels);
    }
  };
  
  SignalProcessor() {};
  ~SignalProcessor() {};
  
  virtual DSPVectorArray< OUT_CHANS > processVector(const DSPVectorArray< IN_CHANS >& inputs) = 0;
  
protected:
  
  // the plain parameter values are stored here.
  ParameterTreePlain _params;
  std::vector< ml::Path > _paramNamesByID; // needed?
  Tree< size_t > _paramIDsByName;
  
  // single buffer for reading from signals
  std::vector< float > _readBuffer;
  
  // brief way to access params:
  inline float getParamNormalized(Path pname) { return getNormalizedValue(_params, pname); }
  inline float getParam(Path pname) { return getPlainValue(_params, pname); }
  
  Tree< std::unique_ptr< PublishedSignal > > _publishedSignals;
  
  inline void publishSignal(Path signalName, int channels, int octavesDown)
  {
    _publishedSignals[signalName] = ml::make_unique< PublishedSignal >(channels, octavesDown);
  }
  
  // store a DSPVectorArray to the named signal buffer.
  // we need buffers here in the Processor because we can't communicate with
  // the Controller in the audio thread.
  template<size_t CHANNELS>
  inline void storePublishedSignal(Path signalName, DSPVectorArray< CHANNELS > v)
  {
    PublishedSignal* publishedSignal = _publishedSignals[signalName].get();
    if(publishedSignal)
    {
      publishedSignal->write(v);
    }
  }
};

} // namespaces


