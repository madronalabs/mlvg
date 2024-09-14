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
#include "testAppParameters.h"

#define ML_INCLUDE_SDL 1
#include "native/MLSDLUtils.h"

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
          switch (hash(second(addr)))
          {
              case(hash("view_size")):
              {
                  // set our new view size from system coordinates
                  // TODO better API for all this, no matrixes
                  Value v = m.value;
                  Matrix m = v.getMatrixValue();
                  Vec2 c (m[0], m[1]);

                  if (window_)
                  {
                      SDL_SetWindowSize(window_, c[0], c[1]);
                  }
                  break;
              }

                default:
              // do nothing, param will be set in AppController::onMessage();
              break;
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


   SDL_Window* window_{ nullptr };
   std::unique_ptr<TestAppView> appView_;
};

int main(int argc, char *argv[])
{
  bool doneFlag{false};
  
  // read parameter descriptions into a list
  ParameterDescriptionList pdl;
  readParameterDescriptions(pdl);

  // TODO get persistent window rect from AppData if available

  // get monitor and scale for making window. if there is no persistent rect, use defaults
  // we have a few utilities in PlatformView that apps can use to make their own default strategies.
  Vec2 c = PlatformView::getPrimaryMonitorCenter(); // TODO -> platformViewUtils

  // get window size in system coords
  Vec2 defaultSize = kDefaultGridUnits * kDefaultGridUnitSize;
  Rect boundsRectInPixels(0, 0, defaultSize.x(), defaultSize.y());
  Rect defaultRectInPixels = alignCenterToPoint(boundsRectInPixels, c);

  // make controller and get instance number. The Controller
  // creates the ActorRegistry, allowing us to register other Actors.
  TestAppController appController(getAppName(), pdl);
  auto appInstanceNum = appController.getInstanceNum();

  // make SDL window
  int windowFlags = SDL_WINDOW_RESIZABLE;
#if TEST_RESIZER
  windowFlags = 0;
#endif
  
  appController.window_ = ml::newSDLWindow(defaultRectInPixels, "mlvg test", windowFlags);
  if(!appController.window_)
  {
    std::cout << "newSDLWindow failed!\n";
    return -1;
  }
  
  // make app view for this instance of our app
  appController.appView_ = std::make_unique<TestAppView> (getAppName(), appInstanceNum);
  appController.appView_->attachToWindow(appController.window_);

  // make Processor and register Actor
  TestAppProcessor appProcessor(kInputChannels, kOutputChannels, kSampleRate, pdl);
  TextFragment processorName(getAppName(), "processor", ml::textUtils::naturalNumberToText(appInstanceNum));
  registerActor(Path(processorName), &appProcessor);
  
  // broadcast all params to update display
  appController.broadcastParams();
  
  // start audio processing
  appProcessor.start();
  appProcessor.startAudio();
  
  // just vibe
  while(!doneFlag)
  {
    SDLAppLoop(appController.window_, &doneFlag);
  }
  
  // stop doing things, clean up and return
  appProcessor.stopAudio();
  appProcessor.stop();
  appController.appView_->stop();
  SDL_DestroyWindow(appController.window_);
  SDL_Quit();
  return 0;
}
