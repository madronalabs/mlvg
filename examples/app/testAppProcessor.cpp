// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

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
  
testAppProcessor::testAppProcessor()
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
DSPVectorArray< 2 > testAppProcessor::processVector()
{
  DSPVectorArray< 2 > stereoOutput;
  
  // TEST
  //debug << "ppq: " << _currentTime._quarterNotesPhase << ", secs: " << _currentTime._seconds << "\n";

  if(1)
  {
    _test += kFloatsPerDSPVector;
    if(_test > sr)
    {
      _test -= sr;

      
 //     debug << "ppq: " << _currentTime._quarterNotesPhase << ", secs: " << _currentTime._seconds << "\n";
      
      //std::cout << std::setprecision(9) << " time: " << _currentTime._seconds << "\n\n";
      //std::cout << std::setprecision(9) << " phase: " << _currentTime._secondsPhase  << "\n\n";
    }
  }
  
  return stereoOutput;
  
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
