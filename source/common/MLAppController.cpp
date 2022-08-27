// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

// use clipboard code from VST3 SDK
// #include "public.sdk/source/common/systemclipboard.h"

#include "MLAppController.h"

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
// AppController implementation

AppController::AppController(TextFragment appName, const ParameterDescriptionList& pdl)
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
  
  // register and start Actor
  _instanceNum = _controllerRegistry->getUniqueID();
  _instanceName = TextFragment(appName, "controller", ml::textUtils::naturalNumberToText(_instanceNum));
  registerActor(_instanceName, this);
  Actor::start();
  
  // get other Actor names
  _processorName = TextFragment(appName, "processor", ml::textUtils::naturalNumberToText(_instanceNum));
  _viewName = TextFragment(appName, "view", ml::textUtils::naturalNumberToText(_instanceNum));

  _timers->start(true);
}

AppController::~AppController()
{
}

#pragma mark mlvg

void AppController::sendParamToView(Path pname)
{
  sendMessageToActor(_viewName, {Path("set_param", pname), getNormalizedValue(_params, pname), kMsgFromController});
}

void AppController::sendAllParamsToView()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToView(pname);
  }
}

void AppController::sendAllCollectionsToView()
{
  for(auto it = _fileTreeIndex.begin(); it != _fileTreeIndex.end(); ++it)
  {
    const Path p = it.getCurrentNodePath();
    sendMessageToActor(_viewName, {"editor/do/update_collection", pathToText(p)});
  }
}

void AppController::sendAllParamsToProcessor()
{
  for(auto& pname : _paramNamesByID)
  {
    sendParamToProcessor(pname, kMsgSequenceStart | kMsgSequenceEnd);
  }
}

void AppController::sendParamToProcessor(Path pname, uint32_t flags)
{
  auto pval = getNormalizedValue(_params, pname);
  sendMessageToActor(_processorName, {Path("set_param", pname), pval, flags});
}

void AppController::onFullQueue()
{
  std::cout << "Controller: full queue! \n";
}

FileTree* AppController::updateCollection(Path which)
{
  FileTree* pTree {_fileTreeIndex[which].get()};
  if(pTree)
  {
    pTree->scan();
  }
  
  return pTree;
}

void AppController::onMessage(Message m)
{
  if(!m.address) return;
  
  //std::cout << "AppController::onMessage:" << m.address << " " << m.value << " \n ";
  
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
          
          /*
        case(hash("view_size")):
        {
          //std::cout << "AppController::onMessage: view_size = " << m.value << " \n ";
          _params["view_size"] = m.value;
          break;
        }
          */
          
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
      break;
    }
      
    default:
    {
      std::cout << "AppController: unhandled message: " << m << " \n ";
      break;
    }
  }
}


