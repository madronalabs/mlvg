// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

// use clipboard code from VST3 SDK
// #include "public.sdk/source/common/systemclipboard.h"

#include "testAppController.h"
#include "testAppView.h"

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


// get list of persistent params stored in Processor
inline void getProcessorStateParams(const ParameterDescriptionList& pdl, std::vector< Path >& _paramList)
{
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    Path paramName = pd.getTextProperty("name");
    bool isPrivate = pd.getBoolPropertyWithDefault("private", false);
    bool controllerParam = pd.getBoolPropertyWithDefault("save_in_controller", false);
    bool isPersistent = pd.getBoolPropertyWithDefault("persistent", true);
    
    if((!isPrivate) && (!controllerParam) && (isPersistent))
    {
      _paramList.push_back(paramName);
    }
  }
}


// get list of persistent params stored in Controller
inline void getControllerStateParams(const ParameterDescriptionList& pdl, std::vector< Path >& _paramList)
{
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    Path paramName = pd.getTextProperty("name");
    bool controllerParam = pd.getBoolPropertyWithDefault("save_in_controller", false);
    bool isPersistent = pd.getBoolPropertyWithDefault("persistent", true);
    
    if((controllerParam) && (isPersistent))
    {
      _paramList.push_back(paramName);
    }
  }
}



//-----------------------------------------------------------------------------
// TestAppController implementation

TestAppController::TestAppController(const ParameterDescriptionList& pdl)
{
#ifdef DEBUG
#ifdef ML_WINDOWS
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
#endif
#endif
  
  // make parameter descriptions and projections
  buildParameterTree(pdl, _params);
  
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
  std::cout << "controller Actor: " << _instanceName << "\n";
  
  _processorName = TextFragment(getAppName(), "processor", ml::textUtils::naturalNumberToText(_instanceNum));
  _viewName = TextFragment(getAppName(), "view", ml::textUtils::naturalNumberToText(_instanceNum));

  Actor::start();
}

TestAppController::~TestAppController()
{
}

#pragma mark mlvg

void TestAppController::setAllParamsToDefaults()
{
  //setDefaults(_params);
  

}

void TestAppController::dumpParams()
{
  _params.dump();
}

void TestAppController::sendMessageToView(Message msg)
{
  sendMessageToActor(_viewName, msg);
}

void TestAppController::sendParamToView(Path pname)
{
  sendMessageToActor(_viewName, {Path("set_param", pname), getNormalizedValue(_params, pname), kMsgFromController});
}

void TestAppController::sendAllParamsToView()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToView(pname);
  }
}

void TestAppController::sendAllCollectionsToView()
{
  for(auto it = _fileTreeIndex.begin(); it != _fileTreeIndex.end(); ++it)
  {
    const Path p = it.getCurrentNodePath();
    sendMessageToActor(_viewName, {"editor/do/update_collection", pathToText(p)});
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
  auto pval = getNormalizedValue(_params, pname);
  
  std::cout << "sendParamToProcessor: " << pname << " -> " << pval << "\n";
  
  //Value pval = _params.getNormalizedValue(pname);
  sendMessageToActor(_processorName, {Path("set_param", pname), pval, flags});
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


