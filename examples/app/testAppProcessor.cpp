// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include "testAppProcessor.h"
#include "testAppController.h"
#include "testAppParameters.h"

#include <cmath>
#include <cstdlib>
#include <math.h>
#include <iostream>

using namespace ml;

// --------------------------------------------------------------------------------
// testAppProcessor
  
TestAppProcessor::TestAppProcessor()
  : SignalProcessor(kInputChannels, kOutputChannels)
{
  // register self
  _uniqueID = _registry->getUniqueID();
  
  // the processor can use a temporary ParameterDescriptionList here.
  ParameterDescriptionList pdl;
  
  // read parameter descriptions from processorParameters.h
  readParameterDescriptions(pdl);
  
  // build the stored parameter tree, creating projections
  buildParameterTree(pdl, _params);
  
  // store all param IDs by name and param names by ID
  // TODO can move to SignalProcessor
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    Path paramName = pd.getTextProperty("name");
    _paramIDsByName[paramName] = i;
    _paramNamesByID.push_back(paramName);
  }

  getProcessorStateParams(pdl, _processorStateParams);
}

TestAppProcessor::~TestAppProcessor()
{

}

// processVector() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
void TestAppProcessor::processVector(MainInputs inputs, MainOutputs outputs, void *stateData)
{
  
  // TEST
  //debug << "ppq: " << _currentTime._quarterNotesPhase << ", secs: " << _currentTime._seconds << "\n";

  
  const float kNoiseLevel{0.01f};
  outputs[0] = _noise0()*kNoiseLevel;
  outputs[1] = _noise1()*kNoiseLevel;
  
}

void TestAppProcessor::handleMessage(Message msg)
{
  std::cout << "TestAppProcessor: " << msg.address << " -> " << msg.value << "\n";
  
  switch(hash(head(msg.address)))
  {
    case(hash("set_param")):
    {
      _params[tail(msg.address)] = msg.value;
      break;
    }
      
    // no set_prop, do?
    default:
    {
      std::cout << " TestAppProcessor: uncaught message " << msg << "! \n";
      break;
    }
  }
}
