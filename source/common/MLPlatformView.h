
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
// forward declaration of a class that has a render() method.
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
  // get the scale at the point on the desktop, compared to "usual" DPI
  static float getDeviceScaleAtPoint(Vec2 p);
  //
  // get the scale the OS considers the window's device to be at, compared to "usual" DPI
  static float getDeviceScaleForWindow(void* parent, int platformFlags = 0);
  //
  static Rect getWindowRect(void* parent, int platformFlags);
  
  // make a platform view for the given parent window.
  // parent: pointer to the parent window
  // platformHandle: platform-specific data
  // platformFlags: platform-specific flags
  PlatformView(void* parent, void* platformHandle, int platformFlags);
  
  ~PlatformView();
  
  // set a non-owning pointer to the AppView. We will call the AppView to animate and render,
  // and notify it when window size and display scale change.
  void setAppView(AppView* pView);
  
  // TODO only used for Mac, make internal only
  void setPlatformViewDisplayScale(float scale);

  // resize the PlatformView, in pixel coordinates
  void resizePlatformView(int w, int h);

protected:
  struct Impl;
  std::unique_ptr< Impl > _pImpl;
  float displayScale_{1.0f};
  Vec2 displaySize{0, 0};
};

} // namespace ml

