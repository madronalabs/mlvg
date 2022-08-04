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

constexpr float kOutputGain = 0.1f;
constexpr float kFreqLo = 40, kFreqHi = 4000;


inline void readParameterDescriptions(ParameterDescriptionList& params)
{
  // Processor parameters
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq1" },
    { "range", { kFreqLo, kFreqHi } },
    { "log", true },
    { "units", "Hz" }
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
    { "vst_compatible", false },
    { "default", { kGridUnitsX*kDefaultGridSize, kGridUnitsY*kDefaultGridSize } }
  } ) );
}


// get list of persistent params stored in Processor
inline void getProcessorStateParams(ParameterDescriptionList& pdl, std::vector< Path >& _paramList)
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
inline void getControllerStateParams(ParameterDescriptionList& pdl, std::vector< Path >& _paramList)
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

