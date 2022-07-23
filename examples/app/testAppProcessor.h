// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "mldsp.h"
#include "madronalib.h"
#include "MLPlatform.h"
#include "MLSignalProcessor.h"
#include "MLDSPUtils.h"
#include "testAppParameters.h"

#include "MLActor.h"
#include "MLMath2D.h"

using namespace ml;

constexpr int kMaxProcessBlockFrames = 4096;
constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;

//-----------------------------------------------------------------------------
class testAppProcessor :
  public SignalProcessor,
  public Actor
{
public:
  testAppProcessor();
  ~testAppProcessor();
    
  // Actor interface
  void handleMessage(Message m) override;
  
private:

  // buffer object to call processVectors from process() calls of arbitrary frame sizes
  std::unique_ptr< VectorProcessBuffer > processBuffer;
  
  // declare the processVectors function that will run our DSP in vectors of size kFloatsPerDSPVector
  void processVector(MainInputs inputs, MainOutputs outputs, void *stateData) override;
  
  
  // list of all the params that we save as part of our state
  std::vector< Path > _processorStateParams;
    
  // private methods
  bool sendMessageToController(Message msg);
  void sendPublishedSignalsToController();
  void sendInstanceNumberToController();
  
  NoiseGen _noise1, _noise0;


};
