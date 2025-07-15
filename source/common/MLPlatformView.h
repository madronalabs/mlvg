
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
  
  // static utilities
  //
  static void initPlatform();
  static Vec2 getPrimaryMonitorCenter();
  static float getDeviceScaleForWindow(void* pParent, int platformFlags = 0);
  static ml::Rect getWindowRect(void* pParent, int platformFlags = 0);
  
  // make a platform view to draw the application to the given parent window.
  // className: unique name of this app or plugin
  // parent: pointer to the parent window
  // pView: existing appView to draw
  // platformHandle: platform-specific data if needed
  // PlatformFlags: platform-specific flags
  // fps: target refresh rate
  PlatformView(const char* className, void* parent, AppView* pView, void* platformHandle, int PlatformFlags, int fps);
  
  ~PlatformView();

  // attach view to the parent context, which will make it visible and allow resizing.
  //
  void attachViewToParent();

  // resize the PlatformView, in system or device coordinates
  void setPlatformViewSize(int w, int h);
  
  void setPlatformViewScale(float f);

  // return the NativeDrawContext, typically needed outside the view for initialization
  NativeDrawContext* getNativeDrawContext();

protected:
  struct Impl;
  std::unique_ptr< Impl > _pImpl;
};

} // namespace ml

