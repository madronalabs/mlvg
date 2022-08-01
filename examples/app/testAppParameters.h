// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once


#include "mldsp.h"
#include "madronalib.h"
#include "MLParameters.h"

using namespace ml;

// set ML parameter properties
// these affect how a parameter is displayed and edited
// note defaults are in normalized values

// TODO load from plugin description file


constexpr float kGridUnitsX{ 9 };
constexpr float kGridUnitsY{ 5 };
constexpr int kDefaultGridSize{ 120 };
constexpr int kMinGridSize{ 40 };
constexpr int kMaxGridSize{ 240 };


constexpr float kSizeLo = 0, kSizeHi = 40;
constexpr float kToneLo = 250, kToneHi = 4000;
constexpr float kDecayLo = 0.8, kDecayHi = 20;
constexpr float kLevelLo = 0.5f, kLevelHi = 2.f;

inline void readParameterDescriptions(ParameterDescriptionList& params)//std::vector< std::unique_ptr< ParameterDescription > >& params)
{
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "size" },
    { "range", { kSizeLo, kSizeHi } },
    { "units", "m" },
    { "lfoable", true },
    { "learnable", true }
  } ) );

  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "decay" },
    { "range", {kDecayLo, kDecayHi} },
    { "log", true },
    { "units", "s" },
    { "maxinfinity", true },
    { "lfoable", true },
    { "learnable", true }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "tone" },
    { "range", {kToneLo, kToneHi} },
    { "log", true },
    { "units", "Hz" },
    { "lfoable", true },
    { "learnable", true }
  } ) );
  
  
  // Controller parameters
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "view_size" },
    { "save_in_controller", true },
    { "vst_compatible", false },
    { "default", { kGridUnitsX*kDefaultGridSize, kGridUnitsY*kDefaultGridSize } }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "current_patch" },
    { "save_in_controller", true },
    { "vst_compatible", false },
    { "default", "default" },
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "status" },
    { "private", true },
    { "persistent", false }, // don't save with Controller or Processor.
    { "default", "demo" }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "licensor" },
    { "private", true },
    { "persistent", false }, // don't save with Controller or Processor.
    { "default", "none" }
  } ) );
}


// get list of non-hidden, VST-compatible params. If a param is not VST-compatible
// (a matrix or text param, say) it should have its "vst_compatible" property set to false.
inline void getVST3CompatibleParams(ParameterDescriptionList& pdl, std::vector< Path >& _paramList)
{
  for(size_t i=0; i < pdl.size(); ++i)
  {
    ParameterDescription& pd = *pdl[i];
    Path paramName = pd.getTextProperty("name");
    bool isPrivate = pd.getBoolPropertyWithDefault("private", false);
    bool controllerParam = pd.getBoolPropertyWithDefault("save_in_controller", false);
    bool isPersistent = pd.getBoolPropertyWithDefault("persistent", true);
    bool isVST3Compatible = pd.getBoolPropertyWithDefault("vst_compatible", true);

    if((!isPrivate) && (!controllerParam) && (isPersistent) && (isVST3Compatible))
    {
      _paramList.push_back(paramName);
    }
  }
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


// get list of persistent params stored in Processor
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

