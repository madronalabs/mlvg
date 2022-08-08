// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"
#include "SDL_syswm.h"
#include "testAppView.h"
#include "testAppController.h"
#include "mldsp.h"
#include "MLRtAudioProcessor.h"

SDL_Window *window;

bool done{false};

void* _platformHandle{ nullptr };

constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;

constexpr int kSampleRate = 48000;

constexpr float kOutputGain = 0.1f;
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
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "gain" },
    { "range", {0, kOutputGain} }
  } ) );
  
  // Controller parameters
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "view_size" },
    { "save_in_controller", true },
    { "vst_compatible", false }
  } ) );
}

void testAppResize(TestAppView& view)
{
  int w{0};
  int h{0};
  SDL_GetWindowSize(window, &w, &h);
  if((w > 0) && (h > 0))
  {
    view.doResize(Vec2{float(w), float(h)});
  }
}

struct ResizingEventWatcherData
{
  SDL_Window* window;
  TestAppView* view;
};

int resizingEventWatcher( void* data, SDL_Event* event )
{
  ResizingEventWatcherData* watcherData = static_cast< ResizingEventWatcherData* >( data );
  SDL_Event& ev = *event;
  
  if( ev.type == SDL_WINDOWEVENT &&
     ev.window.event == SDL_WINDOWEVENT_RESIZED )
  {
    if( SDL_GetWindowFromID( ev.window.windowID ) == watcherData->window )
    {
      //std::cout << std::this_thread::get_id() << ": " << ev.window.data1 << " " << ev.window.data2 << std::endl;
      testAppResize(*watcherData->view);
    }
  }
  return 0;
}

void loop()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    int w{0};
    int h{0};

    // Re-create when window has been resized
    if ((e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_RESIZED))
    {
      SDL_GetWindowSize(window, &w, &h);
      std::cout << "SDL_WINDOWEVENT_RESIZED: " << w << " x " << h << "\n";
      //testAppResize();
    }
    
    if ((e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
    {
      SDL_GetWindowSize(window, &w, &h);
      std::cout << "SDL_WINDOWEVENT_SIZE_CHANGED: " << w << " x " << h << "\n";
      //testAppResize();
    }
    
    if (e.type == SDL_QUIT)
    {
      done = 1;
      return;
    }
    
    if ((e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_ESCAPE))
    {
      done = 1;
      return;
    }
  }
}

struct ParentWindowInfo
{
  void* windowPtr{nullptr};
  unsigned int flags{0};
};

ParentWindowInfo getParentWindowInfo()
{
  ParentWindowInfo p{};
  
  // get window info and initialize info structure
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  
  if(SDL_GetWindowWMInfo(window, &info))
  {
    const char *subsystem = "an unknown system!";
    switch(info.subsystem)
    {
      default:
      case SDL_SYSWM_UNKNOWN:   break;
      case SDL_SYSWM_WINDOWS:   subsystem = "Microsoft Windows(TM)";  break;
      case SDL_SYSWM_X11:       subsystem = "X Window System";        break;
#if SDL_VERSION_ATLEAST(2, 0, 3)
      case SDL_SYSWM_WINRT:     subsystem = "WinRT";                  break;
#endif
      case SDL_SYSWM_DIRECTFB:  subsystem = "DirectFB";               break;
      case SDL_SYSWM_COCOA:     subsystem = "Apple OS X";             break;
      case SDL_SYSWM_UIKIT:     subsystem = "UIKit";                  break;
#if SDL_VERSION_ATLEAST(2, 0, 2)
      case SDL_SYSWM_WAYLAND:   subsystem = "Wayland";                break;
      case SDL_SYSWM_MIR:       subsystem = "Mir";                    break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
      case SDL_SYSWM_ANDROID:   subsystem = "Android";                break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
      case SDL_SYSWM_VIVANTE:   subsystem = "Vivante";                break;
#endif
    }
    
    SDL_Log("This program is running SDL version %d.%d.%d on %s",
            (int)info.version.major,
            (int)info.version.minor,
            (int)info.version.patch,
            subsystem);
  }
  else
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get window information: %s", SDL_GetError());
  }
  
  p.windowPtr = static_cast< void* >(info.info.cocoa.window);
  p.flags = PlatformView::kParentIsNSWindow;
  
  return p;
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
  // read parameter descriptions into a list
  ParameterDescriptionList pdl;
  readParameterDescriptions(pdl);

  // make controller and get instance number
  TestAppController appController(pdl);
  auto instanceNum = appController.getInstanceNum();

  // make view
  TestAppView appView(getAppName(), instanceNum);
  
  // set initial size. This test app does not have a fixed aspect ratio.
  appView.setSizeInGridUnits({16, 9});

  // make widgets and setup parameters
  appView.startup(pdl);

  // could set fixed grid size for view here
  Vec2 defaultDims = appView.getDefaultDims();

  
  // Enable standard application logging
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
  
  // Initialize SDL
  if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
    return 1;
  }
  
  // Create window
  window = SDL_CreateWindow("mlvg test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            defaultDims.x(), defaultDims.y(), SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL);
  if(!window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n",SDL_GetError());
    return 1;
  }
  
  // set min and max sizes for window
  Vec2 minDims = appView.getMinDims();
  Vec2 maxDims = appView.getMaxDims();
  SDL_SetWindowMinimumSize(window, minDims.x(), minDims.y());
  SDL_SetWindowMaximumSize(window, maxDims.x(), maxDims.y());
  
  
  // make Processor and register Actor
  TestAppProcessor appProcessor(kInputChannels, kOutputChannels, kSampleRate, pdl);
  TextFragment processorName(getAppName(), "processor", ml::textUtils::naturalNumberToText(instanceNum));
  registerActor(Path(processorName), &appProcessor);
  appProcessor.start();

  
  // watch for window resize events during drag
  ResizingEventWatcherData watcherData{window, &appView};
  SDL_AddEventWatch( resizingEventWatcher, &watcherData );
    
  // attach app view to window and resize
  ParentWindowInfo windowInfo = getParentWindowInfo();
  appView.doAttached(windowInfo.windowPtr, windowInfo.flags);
  testAppResize(appView);

  // start audio processing
  appProcessor.startAudio();
  
  // just vibe
  while(!done)
  {
    loop();
  }
  
  // stop audio and quit
  appProcessor.stopAudio();
  SDL_Quit();
  return 0;
}


