
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
  enum PlatformFlags
  {
    kParentIsNSWindow = 1
  };

  enum DeviceScaleMode
  {
      kScaleModeUnknown = 0,
      kUseSystemCoords = 1,
      kUseDeviceCoords = 2
  };
  
  // window / resolution helpers
  //
  // get default point to put the center of a new window
  static Vec2 getPrimaryMonitorCenter();
  //
  // get the scale the OS considers the window's device to be at, compared to "usual" DPI
  // static float getDeviceScaleForWindow(void* parent, int PlatformFlags = 0);
  // static float getDpiScaleForWindow(void* parent, int PlatformFlags = 0);
  //
  static Rect getWindowRect(void* parent, int PlatformFlags);
  
  // make a platform view to draw the application to the given parent window.
  // parent: pointer to the parent window
  // pView: existing appView to draw
  // platformHandle: platform-specific data
  // PlatformFlags: platform-specific flags
  // fps: target refresh rate
  PlatformView(void* parent, AppView* pView, void* platformHandle, int PlatformFlags, int fps);
  
  ~PlatformView();

  // attach view to the parent context, which will make it visible and allow resizing.
  // 
  void attachViewToParent(DeviceScaleMode mode);

  // resize the PlatformView, in system or device coordinates
  void setPlatformViewSize(int w, int h);

  // when the DPI scale may have changed, call this to inspect the new DPI scale 
  // of the window and make any needed changes
  void updateDpiScale();
  
  NativeDrawContext* getNativeDrawContext();

protected:
  struct Impl;
  std::unique_ptr< Impl > _pImpl;
};

} // namespace ml

