// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once


#include "mldsp.h"
#include "madronalib.h"
#include "MLParameters.h"

using namespace ml;

constexpr float kGridUnitsX{ 9 };
constexpr float kGridUnitsY{ 5 };
constexpr int kMinGridSize{ 40 };
constexpr int kDefaultGridSize{ 120 };
constexpr int kMaxGridSize{ 240 };


constexpr int kInputChannels = 0;
constexpr int kOutputChannels = 2;

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

