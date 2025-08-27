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

using namespace ml;

//-----------------------------------------------------------------------------
// AppController implementation

#ifdef DEBUG
#ifdef ML_WINDOWS
#include <Windows.h>
#endif
#endif


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
  buildParameterTree(pdl, params);
  setDefaults(params);
  
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
  auto numText = ml::textUtils::naturalNumberToText(_instanceNum);
  _instanceName = TextFragment(appName, "controller", numText);
  registerActor(_instanceName, this);
  Actor::start();
  
  // get other Actor names
  _processorName = TextFragment(appName, "processor", numText);
  _viewName = TextFragment(appName, "view", numText);

  _timers->start(true);
}

AppController::~AppController()
{
}

#pragma mark mlvg

void AppController::broadcastParam(Path pname, uint32_t flags)
{
  auto pval = params.getNormalizedValue(pname);
  sendMessageToActor(_processorName, {Path("set_param", pname), pval, flags});
  sendMessageToActor(_viewName, {Path("set_param", pname), pval, kMsgFromController});
}

void AppController::broadcastParams()
{
  for(auto& pname : _paramNamesByID)
  {
    broadcastParam(pname, kMsgSequenceStart | kMsgSequenceEnd);
  }
}

void AppController::sendAllCollectionsToView()
{
  for(auto it = _fileTreeIndex.begin(); it != _fileTreeIndex.end(); ++it)
  {
    const Path p = it.getCurrentPath();
    sendMessageToActor(_viewName, {"editor/do/update_collection", pathToText(p)});
  }
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
  
  Path addr = m.address;
  switch(hash(head(addr)))
  {
    case(hash("set_param")):
    {
      // set a parameter from the normalized value in the message.
      Path whatParam = tail(addr);
      switch(hash(head(whatParam)))
      {
        default:
        {
          // usual set_param messages
          
          // TODO
          // make undo-able value change here
          // then coalesce and add to history
          // undo will be another message
          
          // save in our ParameterTree
          params.setFromNormalizedValue(whatParam, m.value);
          broadcastParam(whatParam, m.flags);
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
          
      }
      break;
    }
      
    default:
    {
      std::cout << "AppController: unhandled message: " << m << " \n ";
      break;
    }
  }
}


