// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLFiles.h"
#include "MLPropertyTree.h"
#include "MLActor.h"
#include "MLParameters.h"

using namespace ml;

//-----------------------------------------------------------------------------
class AppController :
  public Actor
{
public:
  
  AppController(TextFragment appName, const ParameterDescriptionList& pdl);
	~AppController();
  
  // AppController interface
  void sendMessageToView(Message);
  void sendParamToView(Path pname);
  void sendAllCollectionsToView();
  void sendAllParamsToView();
  void sendParamToProcessor(Path pname, uint32_t flags);
  void sendAllParamsToProcessor();

  // Actor interface
  void onMessage(Message m) override;
  void onFullQueue() override;
  
  // update the named collection of files and return a pointer to it.
  FileTree* updateCollection(Path which);
  
  size_t getInstanceNum() { return _instanceNum; }
  Path getInstanceName() { return _instanceName; }

protected:
  // parameters of our plugin or application.
  ParameterTree params;

  int _instanceNum;
  Path _instanceName;
  Path _viewName;
  Path _processorName;
  
private:
  std::vector< ml::Path > _paramNamesByID;
  Tree< size_t > _paramIDsByName;
  
  // timers for everyone.
  SharedResourcePointer< ml::Timers > _timers ;
  
  // class used for assigning each instance of our Controller a unique ID
  // TODO refactor w/ ProcessorRegistry etc.
  // until Actors are communicating over the network this is not needed.
  class ControllerRegistry
  {
    std::mutex _IDMutex;
    size_t _IDCounter{0};
  public:
    size_t getUniqueID()
    {
      std::unique_lock<std::mutex> lock(_IDMutex);
      return ++_IDCounter;
    }
  };
  SharedResourcePointer< ControllerRegistry > _controllerRegistry;
  
  // when AppController is made, we have an ActorRegistry.
  // without this, we can't register Actors!
  SharedResourcePointer< ActorRegistry > _actorRegistry ;

  
  // an index to file trees
  Tree< std::unique_ptr< FileTree > > _fileTreeIndex;
  
};
