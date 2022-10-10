// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"
#include "SDL_syswm.h"
#include "MLAppController.h"
#include "mldsp.h"
#include "MLRtAudioProcessor.h"

#include "testApp.h"
#include "testAppView.h"

#include "MLSDLUtils.h"

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kDefaultGain = 0.1f;
constexpr float kMaxGain = 0.5f;
constexpr float kFreqLo = 40, kFreqHi = 4000;

void readParameterDescriptions(ParameterDescriptionList& params)
{
  // Processor parameters
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq1" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" },
    { "default", 0.75 }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq2" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" }
    // no default here means the normalized default will be 0.5 (400 Hz)
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "gain" },
    { "range", {0, kMaxGain} },
    { "plaindefault", kDefaultGain }
  } ) );
  
  // Controller parameters
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "view_size" },
    { "save_in_controller", true }
  } ) );
}

class TestAppProcessor :
public RtAudioProcessor
{
  // sine generators.
  SineGen s1, s2;
  
public:
  TestAppProcessor(size_t nInputs, size_t nOutputs, int sampleRate, const ParameterDescriptionList& pdl) :
    RtAudioProcessor(nInputs, nOutputs, sampleRate)
  {
    buildParameterTree(pdl, _params);
  }
  
  // declare the processVector function that will run our DSP in vectors of size kFloatsPerDSPVector
  // with the nullptr constructor argument above, RtAudioProcessor
  void processVector(MainInputs inputs, MainOutputs outputs, void *stateDataUnused) override
  {
    // get params from the SignalProcessor.
    float f1 = getParam("freq1");
    float f2 = getParam("freq2");
    float gain = getParam("gain");

    // Running the sine generators makes DSPVectors as output.
    // The input parameter is omega: the frequency in Hz divided by the sample rate.
    // The output sines are multiplied by the gain.
    outputs[0] = s1(f1/kSampleRate)*gain;
    outputs[1] = s2(f2/kSampleRate)*gain;
  }
};

int main(int argc, char *argv[])
{
  bool doneFlag{false};
  
  // read parameter descriptions into a list
  ParameterDescriptionList pdl;
  readParameterDescriptions(pdl);

  // make controller and get instance number. The Controller
  // creates the ActorRegistry, allowing us to register other Actors.
  AppController appController(getAppName(), pdl);
  auto instanceNum = appController.getInstanceNum();

  // make view
  TestAppView appView(getAppName(), instanceNum);
  
  // set initial size.
  appView.setSizeInGridUnits({16, 9});
  appView.setGridSizeDefault(60);
  
  //appView.setFixedRatioSize(true);

  SDL_Window *window = initSDLWindow(appView);
  if(window)
  {
    // watch for window resize events during drag
    ResizingEventWatcherData watcherData{window, &appView};
    SDL_AddEventWatch( resizingEventWatcher, &watcherData );

    // make Processor and register Actor
    TestAppProcessor appProcessor(kInputChannels, kOutputChannels, kSampleRate, pdl);
    TextFragment processorName(getAppName(), "processor", ml::textUtils::naturalNumberToText(instanceNum));
    registerActor(Path(processorName), &appProcessor);
      
    // attach app view to window, make UI and resize
    ParentWindowInfo windowInfo = getParentWindowInfo(window);
    appView.createPlatformView(windowInfo.windowPtr, windowInfo.flags);
    appView.makeWidgets(pdl);
    appView.startTimersAndActor();
    SdlAppResize(&watcherData);
    
    appController.sendAllParamsToView();
    appController.sendAllParamsToProcessor();

    // start audio processing
    appProcessor.start();
    appProcessor.startAudio();
    
    // just vibe
    while(!doneFlag)
    {
      SDLAppLoop(window, &doneFlag);
    }
    
    // stop doing things and quit
    appView.stopTimersAndActor();
    appProcessor.stopAudio();
    appProcessor.stop();
    SDL_Quit();
  }
    
  return 0;
}


