
// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#pragma once

#include <memory>
#include <array>

#include "MLDrawContext.h"
#include "MLGUIEvent.h"
#include "MLGUICoordinates.h"

#include "madronalib.h"

namespace ml
{
// forward declaration of a class that can animate(), render(), and initializeResources().
class AppView;

// declare PlatformView, which draws our application into an AppView.
// PlatformView is implemented in the platform-specific files in mlvg/source/native.
class PlatformView
{
  
public:
  enum platformFlags
  {
    kParentIsNSWindow = 1
  };
  
  // window / resolution helpers
  //
  // get default point to put the center of a new window
  static Vec2 getPrimaryMonitorCenter();
  //
  // get the scale the OS considers the window's device to be at, compared to "usual" DPI
  static float getDeviceScaleForWindow(void* parent, int platformFlags = 0);
  //
  static Rect getWindowRect(void* parent, int platformFlags);
  
  // make a platform view to draw the application to the given parent window.
  // parent: pointer to the parent window
  // pView: existing appView to draw
  // platformHandle: platform-specific data
  // platformFlags: platform-specific flags
  // fps: target refresh rate
  PlatformView(void* parent, AppView* pView, void* platformHandle, int platformFlags, int fps);
  
  ~PlatformView();

  // attach view to the parent context, which will make it visible and allow resizing.
  void attachViewToParent();

  // resize the PlatformView, in system coordinates
  void setPlatformViewSize(int w, int h);
  void setPlatformViewScale(float scale);
  
  NativeDrawContext* getNativeDrawContext();

protected:
  struct Impl;
  std::unique_ptr< Impl > _pImpl;
  float displayScale_{1.0f};
};

} // namespace ml

