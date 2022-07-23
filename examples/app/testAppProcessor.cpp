// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#include "testAppProcessor.h"
#include "TestAppController.h"
#include "testAppParameters.h"


#include <cmath>
#include <cstdlib>
#include <math.h>
#include <iostream>


using namespace ml;

// --------------------------------------------------------------------------------
// testAppProcessor
  
testAppProcessor::testAppProcessor()
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

testAppProcessor::~testAppProcessor()
{

}



// processVector() does all of the audio processing, in DSPVector-sized chunks.
// It is called every time a new buffer of audio is needed.
void testAppProcessor::processVector(MainInputs inputs, MainOutputs outputs, void *stateData)
{
  
  // TEST
  //debug << "ppq: " << _currentTime._quarterNotesPhase << ", secs: " << _currentTime._seconds << "\n";

  
  const float kNoiseLevel{0.01f};
  outputs[0] = _noise0()*kNoiseLevel;
  outputs[1] = _noise1()*kNoiseLevel;
  
}

void testAppProcessor::handleMessage(Message msg)
{
  std::cout << "testAppProcessor: " << msg.address << " -> " << msg.value << "\n";
  
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
      std::cout << " testAppProcessor: uncaught message " << msg << "! \n";
      break;
    }
  }
}
