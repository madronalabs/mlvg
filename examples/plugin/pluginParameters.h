// VST3 example code for madronalib
// (c) 2020, Madrona Labs LLC, all rights reserved
// see LICENSE.txt for details

#pragma once

#include "mldsp.h"
#include "madronalib.h"
#include "MLPropertyTree.h" // TODO add to madronalib

constexpr int kPublishedSignalBufferSize = 1024;

namespace ml {

// todo refactor

class ParameterDescription : public PropertyTree
{
public:
  ParameterDescription(WithValues p) : PropertyTree(p){}
  virtual ~ParameterDescription() = default;
  
  Projection normalizedToReal{projections::unity};
  Projection realToNormalized{projections::unity};
};

inline void createProjections(std::vector< std::unique_ptr< ParameterDescription > >& _parameterDescriptions)
{
  // make projections
  for(auto & paramPtr : _parameterDescriptions)
  {
    ParameterDescription& param = *paramPtr;
    
    bool bLog = param.getProperty("log").getBoolValueWithDefault(false);
    Matrix range = param.getProperty("range").getMatrixValueWithDefault({0, 1});
    Interval fullRange{range[0], range[1]};
    
    if(bLog)
    {
      param.normalizedToReal = ml::projections::intervalMap({0, 1}, fullRange, ml::projections::log(fullRange));
      param.realToNormalized = ml::projections::intervalMap(fullRange, {0, 1}, ml::projections::exp(fullRange));
    }
    else
    {
      param.normalizedToReal = projections::linear({0, 1}, fullRange);
      param.realToNormalized = projections::linear(fullRange, {0, 1});
    }
  }
}

} // namespaces

