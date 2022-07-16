// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "mldsp.h"
#include "madronalib.h"
#include "MLPlatform.h"
#include "signalProcessor.h"
#include "testAppParameters.h"

#include "MLActor.h"
#include "MLMath2D.h"

using namespace ml;

constexpr int kMaxProcessBlockFrames = 4096;
constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;

//-----------------------------------------------------------------------------
class testAppProcessor :
  public SignalProcessor< kInputChannels, kOutputChannels >,
  public Actor
{
public:
  testAppProcessor();
  ~testAppProcessor();
    
  // Actor interface
  void handleMessage(Message m) override;
  
private:
  
  // registry stuff
  
  // class used for assigning each instance of our Controller a unique ID
  // TODO refactor w/ ProcessorRegistry etc.
  class ProcessorRegistry
  {
    std::mutex _IDMutex;
    size_t _IDCounter{0};
  public:
    size_t getUniqueID()
    {
      std::unique_lock<std::mutex> lock(_IDMutex);
      return ++_IDCounter;
    }
  };
  
  SharedResourcePointer< ProcessorRegistry > _registry ;
  size_t _uniqueID;
  bool _connectedToController{false};

  // buffer object to call processVectors from process() calls of arbitrary frame sizes
  VectorProcessBuffer<kInputChannels, kOutputChannels, kMaxProcessBlockFrames> processBuffer;
  
  // declare the processVectors function that will run our DSP in vectors of size kFloatsPerDSPVector
  DSPVectorArray<kOutputChannels> processVector(const DSPVectorArray<kInputChannels>& inputVectors) override;
  
  // set up the function parameter for processBuffer.process()
  using processFnType = std::function<DSPVectorArray<kOutputChannels>(const DSPVectorArray<kInputChannels>&, void*)>;
  processFnType processFn{ [&](const DSPVectorArray<kInputChannels> inputVectors, void*) { return processVector(inputVectors); } };
    std::vector< std::set< int > > _inputParamIDsByMIDISource;
  
  // list of all the params that we save as part of our state
  std::vector< Path > _processorStateParams;
    
  // private methods
  bool sendMessageToController(Message msg);
  void sendPublishedSignalsToController();
  void sendInstanceNumberToController();

};
