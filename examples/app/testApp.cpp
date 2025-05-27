// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#include <stdlib.h>
#include <stdio.h>

#include "MLAppController.h"
#include "mldsp.h"
#include "mlvg.h"

#include "testApp.h"
#include "testAppView.h"
#include "testAppParameters.h"

// TODO migrate to SDL 3 and investigate whether we can dump nanovg in favor of an SDL3-based solution
#include "SDL.h"
#include "SDL_syswm.h"
#define ML_INCLUDE_SDL 1
#include "native/MLSDLUtils.h"

// TEMP
#if(ML_WINDOWS)
#include <windows.h>
#include <ShellScalingApi.h>
#endif

struct TestAppProcessor : public SignalProcessor, public Actor
{
  // sine generators.
  SineGen s1, s2;
  
  void onMessage(Message msg)
  {
    switch(hash(head(msg.address)))
    {
      case(hash("set_param")):
      {
        auto paramName = tail(msg.address);
        auto newParamValue = msg.value;
        _params.setFromNormalizedValue(paramName, newParamValue);
      }
      default:
      {
        break;
      }
    }
  }
};
 

void processTestApp(AudioContext* ctx, void *untypedState)
{
  auto state = static_cast< TestAppProcessor* >(untypedState);
  
  // get params from the SignalProcessor.
  float f1 = state->getRealFloatParam("freq1");
  float f2 = state->getRealFloatParam("freq2");
  float gain = state->getRealFloatParam("gain");
  
  // Running the sine generators makes DSPVectors as output.
  // The input parameter is omega: the frequency in Hz divided by the sample rate.
  // The output sines are multiplied by the gain.
  ctx->outputs[0] = state->s1(f1/kSampleRate)*gain;
  ctx->outputs[1] = state->s2(f2/kSampleRate)*gain;
}

class TestAppController :
public AppController
{
public:

    SDL_Window* window{ nullptr };
    std::unique_ptr< PlatformView > platformView;
    std::unique_ptr< TestAppView > appView;
    ResizingEventWatcherData watcherData;

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
          switch (hash(second(addr)))
          {
              case(hash("view_size")):
              {
                  // set our new view size from system coordinates
                  // TODO better API for all this, no matrixes
                  Value v = m.value;
                  Matrix m = v.getMatrixValue();
                  Vec2 c (m[0], m[1]);
                  if (window)
                  {
                    SDL_SetWindowSize(window, c[0], c[1]);
                  }
                  messageHandled = true;
                  break;
              }
              default:
              {
                break;
              }
          }
      }
      case(hash("set_prop")):
      {
        // no properties in test app
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
                std::cout << "loaded sample.\n";
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
  
  int createView(Rect defaultSize)
  {
    int r{1};
    
    // make SDL window
    int windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
#if !TEST_RESIZER
    windowFlags |= SDL_WINDOW_RESIZABLE;
#endif

    window = ml::newSDLWindow(defaultSize, "mlvg test", windowFlags);
    if(!window)
    {
      std::cout << "newSDLWindow failed!\n";
      r = 0;
    }
    
    // make AppView (holds widgets, runs UI)
    if(r)
    {
      appView = std::make_unique< TestAppView > (getAppName(), _instanceNum);
      
      // set initial size. When this is not a fixed-ratio app, the window sizes
      // freely and the grid unit size remains constant.
      appView->setGridSizeDefault(kDefaultGridUnitSize);
      
#if TEST_FIXED_RATIO
      appView->setFixedAspectRatio(kDefaultGridUnits);
#endif
    }
    
    // make PlatformView (drawing apparatus connecting AppView to Window)
    if(r)
    {
      ParentWindowInfo windowInfo = ml::getParentWindowInfo(window);
      platformView = std::make_unique< PlatformView >(windowInfo.windowPtr, appView.get(), nullptr, windowInfo.flags, 60);
      appView->initializeResources(platformView->getNativeDrawContext());
    }

    return r;
  }
  
  void watchForResize()
  {
    // connect window to the PlatformView: watch for window resize events during drag
    // this does not trigger immediate resize
    watcherData = ResizingEventWatcherData{ window, platformView.get() };
    SDL_AddEventWatch(resizingEventWatcher, &watcherData);
  }
};

int main(int argc, char *argv[])
{
    // set DPI awareness before making any windows.
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        std::cout << "main: Process marked as Per Monitor DPI Aware v2 successfully.\n";
    }
    else {
        std::cerr << "Failed to set DPI awareness. Error: " << GetLastError() << std::endl;
    }

  bool doneFlag{false};
  
  ParameterDescriptionList pdl;
  readParameterDescriptions(pdl);

  // get window size in system coords
  Vec2 defaultSize = kDefaultGridUnits * kDefaultGridUnitSize;
  Rect systemBoundsRect(0, 0, defaultSize.x(), defaultSize.y());
  Vec2 screenCenter = PlatformView::getPrimaryMonitorCenter();
  Rect systemDefaultRect = alignCenterToPoint(systemBoundsRect, screenCenter);

  // make controller and get instance number. The Controller
  // creates the ActorRegistry, allowing us to register other Actors.
  TestAppController appController(getAppName(), pdl);
  auto appInstanceNum = appController.getInstanceNum();

  // make view and initialize drawing resources
  if (!appController.createView(systemDefaultRect)) return 1;
  appController.appView->makeWidgets(pdl);
  
  // attach view to parent window, allowing resizing
  appController.platformView->attachViewToParent(PlatformView::kUseDeviceCoords);
  appController.watchForResize();
  appController.appView->startTimersAndActor();

  // make Processor and register Actor
  TestAppProcessor appProcessor;
  AudioContext ctx(kInputChannels, kOutputChannels, kSampleRate);
  AudioTask testAppTask(&ctx, processTestApp, &appProcessor);
  
  appProcessor.buildParams(pdl);
  appProcessor.setDefaultParams();
  appProcessor.start();
  
  TextFragment processorName(getAppName(), "processor", ml::textUtils::naturalNumberToText(appInstanceNum));
  registerActor(Path(processorName), &appProcessor);

  // broadcast all params to update display
  appController.broadcastParams();
  
  testAppTask.startAudio();
  while(!doneFlag)
  {
    SDLAppLoop(appController.window, &doneFlag);
  }
  testAppTask.stopAudio();
  
  appProcessor.stop();
  appController.appView->stop();
  SDL_DestroyWindow(appController.window);
  SDL_Quit();
  return 0;
}




