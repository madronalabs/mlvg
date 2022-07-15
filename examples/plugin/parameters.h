// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once


#include "mldsp.h"
#include "madronalib.h"

using namespace ml;

namespace Steinberg {
namespace Vst {
namespace llllpluginnamellll {

// set ML parameter properties
// these affect how a parameter is displayed and edited
// note defaults are in normalized values

// TODO load from plugin description file

inline void createPluginParameters(std::vector< std::unique_ptr< ParameterDescription > >& params)
{
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "gain" },
    { "range", { 0., 1. } },
    { "default", 0.25 }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq_l" },
    { "range", {55., 880.} },
    { "default", 0.5 },
    { "log", true },
    { "units", "Hz" }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq_r" },
    { "range", {55., 880.} },
    { "default", 0.5 },
    { "log", true },
    { "units", "Hz" }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "bypass" },
    { "stepcount", 1 },
    { "range", { 0., 1. } },
    { "default", false }
  } ) );
  
  
 
  // learnable params push back for each destination: source, amount
  // lfoable params push back for each destination: shape, sync, rate, amount
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq_l/lfo/rate" },
    { "read_only", true },
    { "range", {0.01, 5.} },
    { "default", 0.5 },
    { "log", true },
    { "units", "Hz" }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq_l/lfo/amount" },
    { "read_only", true },
    { "range", {-1., 1.} },
    { "default", 0.5 },
    { "log", false },
    { "units", "Hz" }
  } ) );
  
  params.push_back( ml::make_unique< ParameterDescription >(WithValues{
    { "name", "freq_l/learn/amount" },
    { "read_only", true },
    { "range", {-1., 1.} },
    { "default", 0.5 },
    { "log", false },
    { "units", "Hz" }
  } ) );

}

  

}}} // namespaces

