
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

        // window / resolution helpers that are here for now 
        // 
        // get default point to put the center of a new window
        static Vec2 getPrimaryMonitorCenter();
        // 
        // get the scale at the device covering the given point
        static float getDeviceScaleAtPoint(Vec2 p);
        //      
        // get the scale the OS considers the window's device to be at, compared to "usual" DPI
        static float getDeviceScaleForWindow(void* parent, int platformFlags = 0);
        //
        //
      static Rect getWindowRect(void* parent, int platformFlags);

        PlatformView(void* parent, ml::Rect boundsRect, AppView* pR, void* platformHandle, int platformFlags);
        ~PlatformView();

        void resizePlatformView(int w, int h);

    protected:
        struct Impl;
        std::unique_ptr< Impl > _pImpl;
    };

} // namespace ml
