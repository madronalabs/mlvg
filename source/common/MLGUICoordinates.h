
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include "MLDrawContext.h"

namespace ml
{
// GUICoordinates objects manage the three coordinate systems we need:
// 1. system coordinates, what the OS uses for making windows and sending events,
// 2. pixel (native) coordinates, the actual pixels on the screen
// 3. grid coordinates, the positioning system for Widgets

// TODO consistent naming!!! 

struct GUICoordinates
{
  // number of pixels per single grid unit.
  int gridSize{};

  // width and height of entire view in pixels.
  Vec2 pixelSize{};

  // origin of grid system, relative to view top left,
  // in pixel coordinate system.
  Vec2 origin{};
  
  // number of native pixels per system coordinate unit. often 1. or 2.
  float displayScale{1.f};
  
  template< class T > // TODO for what, why not all the below
  T systemToNative(T vc) const
  {
    return vc*displayScale;
  }

  Vec4 nativeToSystem(Vec4 vc) const
  {
    return vc/displayScale;
  }

  Vec4 nativeToGrid(Vec4 vc) const
  {
    return (vc - origin)/gridSize;
  }

  Vec4 gridToNative(Vec4 gc) const
  {
    return ((gc*gridSize) + origin);
  }

  Vec4 systemToGrid(Vec4 vc) const
  {
    return nativeToGrid(systemToNative(vc));
  }

  Vec4 gridToSystem(Vec4 gc) const
  {
    return nativeToSystem(gridToNative(gc));
  }
};

}; // namespace ml
 
