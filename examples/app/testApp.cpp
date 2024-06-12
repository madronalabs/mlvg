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
#include "mlvg.h"
#include "MLRtAudioProcessor.h"

#include "testApp.h"
#include "testAppView.h"

#define ML_INCLUDE_SDL 1
#include "native/MLSDLUtils.h"

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;
constexpr int kSampleRate = 48000;
constexpr float kDefaultGain = 0.1f;
constexpr float kMaxGain = 0.5f;
constexpr float kFreqLo = 40, kFreqHi = 4000;
const ml::Vec2 kDefaultGridUnits{ 16, 9 };
const int kDefaultGridUnitSize{ 60 };

void readParameterDescriptions(ParameterDescriptionList& params)
{
  // Processor parameters
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "freq1" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" },
    { "default", 0.75 }
  } ) );
  
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "freq2" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" }
    // no default here means the normalized default will be 0.5 (400 Hz)
  } ) );
  
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
    { "name", "gain" },
    { "range", {0, kMaxGain} },
    { "plaindefault", kDefaultGain }
  } ) );
  
  // Controller parameters
  params.push_back( std::make_unique< ParameterDescription >(WithValues{
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
    float f1 = _params.getRealFloatValue("freq1");
    float f2 = _params.getRealFloatValue("freq2");
    float gain = _params.getRealFloatValue("gain");

    // Running the sine generators makes DSPVectors as output.
    // The input parameter is omega: the frequency in Hz divided by the sample rate.
    // The output sines are multiplied by the gain.
    outputs[0] = s1(f1/kSampleRate)*gain;
    outputs[1] = s2(f2/kSampleRate)*gain;
  }
};

class TestAppController :
public AppController
{
public:
  TestAppController(TextFragment appName, const ParameterDescriptionList& pdl) : AppController(appName, pdl) {}
  ~TestAppController() = default;
  
  
  int _loadSampleFromDialog()
  {
    int OK{ false };
    std::string sourcePath;
    
    // just a test, show the dialog but don't do anything yet
    std::cout << FileDialog::getFolderForLoad("/Users", "");
    
    return OK;
  }

  
  
  
  void onMessage(Message m)
  {
    if(!m.address) return;
    
    bool messageHandled{false};
    
    Path addr = m.address;
    switch(hash(head(addr)))
    {
      case(hash("set_param")):
      {
        Path whatParam = tail(addr);
        switch(hash(head(whatParam)))
        {
        }
        break;
      }
      case(hash("set_prop")):
      {
        Path whatProp = tail(addr);
        switch(hash(head(whatProp)))
        {
          case(hash("playback_progress")):
          {
            sendMessageToActor(_viewName, {"widget/sample/set_prop/progress", m.value});
            break;
          }
        }
        break;
      }
      case(hash("do")):
      {
        Path whatAction = tail(addr);
        switch(hash(head(whatAction)))
        {
          case(hash("open")):
          {
            if(_loadSampleFromDialog())
            {

            }

            messageHandled = true;
            break;
          }
          default:
          {
            break;
          }
            
        }
        break;
      }
      default:
      {
        break;
      }
    }
    
    if(!messageHandled)
    {
      AppController::onMessage(m);
    }
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
  TestAppController appController(getAppName(), pdl);
  auto instanceNum = appController.getInstanceNum();

  // make view
  TestAppView appView(getAppName(), instanceNum);


  // TODO get persistent window rect if available


  // if there is no persistent rect, use default
  // we have a few utilities in PlatformView that apps can use to make their own default strategies.
  Vec2 c = PlatformView::getPrimaryMonitorCenter();
  float devScale = PlatformView::getDeviceScaleAtPoint(c);


  // set initial size. This is not a fixed-ratio app, meaning the window sizes
  // freely and the grid unit size remains constant. 
  appView.setSizeInGridUnits(kDefaultGridUnits);
  appView.setGridSizeDefault(kDefaultGridUnitSize* devScale);

  // get default rect 
  Vec2 defaultSize = kDefaultGridUnits * kDefaultGridUnitSize * devScale;
  Rect boundsRect(0, 0, defaultSize.x(), defaultSize.y());
  Rect defaultRect = alignCenterToPoint(boundsRect, c);

  SDL_Window *window = ml::initSDLWindow(appView, defaultRect, "mlvg test");
  if(window)
  {
    // make Processor and register Actor
    TestAppProcessor appProcessor(kInputChannels, kOutputChannels, kSampleRate, pdl);
    TextFragment processorName(getAppName(), "processor", ml::textUtils::naturalNumberToText(instanceNum));
    registerActor(Path(processorName), &appProcessor);
      
    // attach app view to window, make UI and resize
    ParentWindowInfo windowInfo = ml::getParentWindowInfo(window);
    appView.makeWidgets(pdl);
    appView.createPlatformView(windowInfo.windowPtr, windowInfo.flags);
    appView.startTimersAndActor();

    // watch for window resize events during drag
    ResizingEventWatcherData watcherData{ window, &appView };
    SDL_AddEventWatch(resizingEventWatcher, &watcherData);
    SdlAppResize(&watcherData);
    
    appController.broadcastParams();
    
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


