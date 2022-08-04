// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

// use clipboard code from VST3 SDK
// #include "public.sdk/source/common/systemclipboard.h"

#include "testAppController.h"
#include "testAppView.h"

// param definitions for this plugin
#include "testAppParameters.h"

#include <cmath>
#include <iostream>
#include <chrono>

#include "MLSerialization.h"

// include miniz - in mlvg/external directory
#include "external/miniz/miniz.h"
#include "external/miniz/miniz.c"

// include nfd
#include "external/nativefiledialog-extended/src/include/nfd.h"

#define JUCE_APP_CONFIG_HEADER "external/juce_core/JuceCoreConfig.h"
#include "external/juce_core/juce_core.h"

using namespace ml;

//-----------------------------------------------------------------------------
// TestAppController implementation

TestAppController::TestAppController()
{
#ifdef DEBUG
#ifdef ML_WINDOWS
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
#endif
#endif
  
  // make parameters and projections
  ParameterDescriptionList pdl;
  readParameterDescriptions(pdl);
  buildParameterTree(pdl, _params);
  setDefaults(_params);
  
  // store IDs by name and param names by ID
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    Path paramName = pd.getProperty("name").getTextValue();
    _paramIDsByName[paramName] = i;
    _paramNamesByID.push_back(paramName);
  }
  
  getProcessorStateParams(pdl, _processorStateParams);
  getControllerStateParams(pdl, _controllerStateParams);
  
  // start timers in main thread.
  _timers->start(true);
  
  // register and start Actor
  _instanceNum = _controllerRegistry->getUniqueID();
  _instanceName = TextFragment(getAppName(), "controller", ml::textUtils::naturalNumberToText(_instanceNum));
  registerActor(_instanceName, this);
  Actor::start();
}

TestAppController::~TestAppController()
{
}

#pragma mark mlvg

TestAppView* TestAppController::createTestAppView ()
{
  auto defaultSize = _params["view_size"].getMatrixValue();
  float w = defaultSize[0];
  float h = defaultSize[1];
  
  Rect size{0, 0, w, h};
  
  auto newView = new TestAppView(size, _instanceName);
  
  _viewName = TextFragment(getAppName(), "view", ml::textUtils::naturalNumberToText(_instanceNum));
  registerActor(_viewName, newView);
  
  // send all collections to view
  for(auto it = _fileTreeIndex.begin(); it != _fileTreeIndex.end(); ++it)
  {
    const Path p = it.getCurrentNodePath();
    sendMessageToActor(_viewName, {"editor/do/update_collection", pathToText(p)});
  }
  
  sendAllParamsToView();
  
  return newView;
}

void TestAppController::sendMessageToView(Message msg)
{
  sendMessageToActor(_viewName, msg);
}

void TestAppController::sendParamToView(Path pname)
{
  sendMessageToActor(_viewName, {Path("set_param", pname), _params[pname], kMsgFromController});
}

void TestAppController::sendAllParamsToView()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToView(pname);
  }
}

void TestAppController::sendAllParamsToProcessor()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToProcessor(pname, kMsgSequenceStart | kMsgSequenceEnd);
  }
}

void TestAppController::sendParamToProcessor(Path pname, uint32_t flags)
{
  
  Value pval = _params.getNormalizedValue(pname);
  sendMessageToActor(_processorName, {Path("set_param", pname), pval});
}

void TestAppController::sendMessageToProcessor(Message msg)
{
  sendMessageToActor(_processorName, msg);
}

void TestAppController::onFullQueue()
{
  std::cout << "Controller: full queue! \n";
}

FileTree* TestAppController::updateCollection(Path which)
{
  FileTree* pTree {_fileTreeIndex[which].get()};
  if(pTree)
  {
    pTree->scan();
  }
  
  return pTree;
}

void TestAppController::onMessage(Message m)
{
  if(!m.address) return;
  
  std::cout << "TestAppController::onMessage:" << m.address << " " << m.value << " \n ";
  
  Path addr = m.address;
  switch(hash(head(addr)))
  {
    case(hash("set_param")):
    {
      // set a parameter from the normalized value in the message.
      Path whatParam = tail(addr);
      switch(hash(head(whatParam)))
      {
        // TODO refactor, set params here and send to proc if needed
          
        case(hash("view_size")):
        {
          std::cout << "TestAppController::onMessage: view_size = " << m.value << " \n ";
          _params["view_size"] = m.value;
          break;
        }
          
        default:
        {
          // usual set_param messages
          
          // TODO
          // make undo-able value change here
          // then coalesce and add to history
          // undo will be another message
          
          // save in our ParameterTree
          _params.setParamFromNormalizedValue(whatParam, m.value);
          
          // send to processor
          sendParamToProcessor(whatParam, m.flags);
          
          break;
        }
      }
      break;
    }
      
    case(hash("set_prop")):
    {
      Path whatProp = tail(addr);
      switch(hash(head(whatProp)))
      {
          
      }
      break;
    }
      
    case(hash("do")):
    {
      Path whatAction = tail(addr);
      switch(hash(head(whatAction)))
      {
        case(hash("subscribe_to_signal")):
        {
          Path sigPath = textToPath(m.value.getTextValue());
          _signalsSubscribedByView[sigPath] = 1;
          break;
        }
      }
    }
      
    default:
    {
      std::cout << "TestAppController: unhandled message: " << m << " \n ";
      break;
    }
  }
}


