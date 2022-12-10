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
#include "nfd.h"
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

class TestAppController :
public AppController
{
public:
  TestAppController(TextFragment appName, const ParameterDescriptionList& pdl) : AppController(appName, pdl) {}
  ~TestAppController() = default;
  
  
  int _loadSampleFromDialog()
  {
    int OK{ false };
    nfdchar_t *outPath;
    std::string sourcePath;
    nfdfilteritem_t filterItem[2] = { { "WAV audio", "wav" }, { "AIFF audio", "aiff,aif,aifc" } };
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, NULL);
    if (result == NFD_OKAY)
    {
      puts("Success!");
      puts(outPath);
      
      sourcePath = outPath;
      NFD_FreePath(outPath);
    }
    else if (result == NFD_CANCEL)
    {
      puts("User pressed cancel.");
    }
    else
    {
      printf("Error: %s\n", NFD_GetError());
    }
    
    Path filePath(sourcePath.c_str());
    File f(filePath);
    /*
    if(f)
    {
      std::cout << "file to load: " << filePath << "\n";
      std::cout << "init sr: " << _sample.sampleRate << "\n";
      
      // load the file
      
      SF_INFO fileInfo;
      auto file = sf_open(sourcePath.c_str(), SFM_READ, &fileInfo);
      
      std::cout << "        format: " << fileInfo.format << "\n";
      std::cout << "        frames: " << fileInfo.frames << "\n";
      std::cout << "        samplerate: " << fileInfo.samplerate << "\n";
      std::cout << "        channels: " << fileInfo.channels << "\n";
      
      constexpr size_t kMaxSeconds = 32;
      size_t fileSizeInFrames = fileInfo.frames;
      size_t kMaxFrames = kMaxSeconds*fileInfo.samplerate;
      
      size_t framesToRead = std::min(fileSizeInFrames, kMaxFrames);
      size_t samplesToRead = framesToRead*fileInfo.channels;
      
      _printToConsole(TextFragment("loading ", pathToText(filePath), "..."));
      
      _sample.data.resize(samplesToRead);
      float* pData = _sample.data.data();
      _sample.sampleRate = fileInfo.samplerate;
      
      sf_count_t framesRead = sf_readf_float(file, pData, static_cast<sf_count_t>(framesToRead));
      
      TextFragment readStatus;
      if(framesRead != framesToRead)
      {
        readStatus = "file read failed!";
      }
      else
      {
        TextFragment truncatedMsg = (framesToRead == kMaxFrames) ? "(truncated)" : "";
        readStatus = (TextFragment(textUtils::naturalNumberToText(framesRead), " frames read ", truncatedMsg ));
        OK = true;
      }
      
      _printToConsole(readStatus);
      sf_close(file);
      
      // deinterleave to extract first channel if needed
      if(fileInfo.channels > 1)
      {
        for(int i=0; i < framesRead; ++i)
        {
          pData[i] = pData[i*fileInfo.channels];
        }
        _sample.data.resize(framesRead);
      }
      
    }*/
    
    return OK;
  }

  
  
  
  void onMessage(Message m)
  {
    if(!m.address) return;
    
    std::cout << "TestAppController::onMessage:" << m.address << " " << m.value << " \n ";
    
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
  
  // set initial size.
  appView.setSizeInGridUnits({16, 9});
  appView.setGridSizeDefault(60);
  
  //appView.setFixedRatioSize(true);

  SDL_Window *window = initSDLWindow(appView);
  if(window)
  {
    // init NFD after SDL.
    NFD_Init();
    
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
    NFD_Quit();
    SDL_Quit();
  }
    
  return 0;
}


