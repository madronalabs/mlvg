// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once


#include "SDL.h"
#include "SDL_syswm.h"
#include "MLPlatform.h"

namespace ml {

inline uint32_t getPlatformWindowCreateFlags()
{
#if ML_WINDOWS
  return SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#elif ML_LINUX
  return 0;
#elif ML_IOS
  return SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI;
#elif ML_MAC
  return SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI;
#endif
}

// create a new window. dims in system (screen) coodinates.
inline SDL_Window* newSDLWindow(ml::Rect b, const char* windowName, int flags)
{
  // Enable standard application logging
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
  
  // Initialize SDL
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
    return nullptr;
  }
  
  // Create window
  int x = b.left;
  int y = b.top;
  int w = b.width;
  int h = b.height;
  
  // combine user flags parameters with platform flags and make window
  SDL_Window* newWindow = SDL_CreateWindow(windowName, x, y, w, h,
                                           flags | getPlatformWindowCreateFlags());
  
  if (!newWindow)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n", SDL_GetError());
    return nullptr;
  }
  
  const int kWindowMinDim = 200;
  SDL_SetWindowMinimumSize(newWindow, kWindowMinDim, kWindowMinDim);
  SDL_SetWindowMaximumSize(newWindow, 10000, 10000);

  return newWindow;
}

inline void SDLAppLoop(SDL_Window* window, bool* done)
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    int w{ 0 };
    int h{ 0 };
    
    if (e.type == SDL_QUIT)
    {
      *done = true;
      return;
    }
    
    if ((e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_ESCAPE))
    {
      *done = true;
      return;
    }
  }
}

struct ParentWindowInfo
{
  void* windowPtr{ nullptr };
  unsigned int flags{ 0 };
};

inline ParentWindowInfo getParentWindowInfo(SDL_Window* window)
{
  ParentWindowInfo p{};
  
  // get window info and initialize info structure
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);
  
  if (SDL_GetWindowWMInfo(window, &info))
  {
    const char* subsystem = "an unknown system!";
    switch (info.subsystem)
    {
      default:
      case SDL_SYSWM_UNKNOWN:
        break;
        
#if ML_WINDOWS
        
      case SDL_SYSWM_WINDOWS:
        subsystem = "Microsoft Windows(TM)";
        p.windowPtr = static_cast<void*>(info.info.win.window);
        break;
#endif
      case SDL_SYSWM_X11:
        subsystem = "X Window System";
        
        break;
#if SDL_VERSION_ATLEAST(2, 0, 3)
      case SDL_SYSWM_WINRT:
        subsystem = "WinRT";
        break;
#endif
      case SDL_SYSWM_DIRECTFB:
        subsystem = "DirectFB";
        break;
#if defined(SDL_VIDEO_DRIVER_COCOA)
      case SDL_SYSWM_COCOA:
        subsystem = "Apple OS X";
        p.windowPtr = static_cast<void*>(info.info.cocoa.window);
        p.flags = PlatformView::kParentIsNSWindow;
        break;
#endif
      case SDL_SYSWM_UIKIT:
        subsystem = "UIKit";
        break;
#if SDL_VERSION_ATLEAST(2, 0, 2)
      case SDL_SYSWM_WAYLAND:
        subsystem = "Wayland";
        break;
      case SDL_SYSWM_MIR:
        subsystem = "Mir";
        break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
      case SDL_SYSWM_ANDROID:
        subsystem = "Android";
        break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
      case SDL_SYSWM_VIVANTE:
        subsystem = "Vivante";
        break;
#endif
    }
    

    SDL_Log("This program is running SDL version %d.%d.%d on %s",
            (int)info.version.major,
            (int)info.version.minor,
            (int)info.version.patch,
            subsystem);
     
  }
  else
  {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get window information: %s", SDL_GetError());
  }
  
  return p;
}


inline Vec2 getWindowSizeInPixels(SDL_Window* window)
{
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  
  auto p = getParentWindowInfo(window);
  
  float scale = PlatformView::getDeviceScaleForWindow(p.windowPtr, p.flags);
  return Vec2(w*scale, h*scale);
}


inline void setWindowSizeInPixels(SDL_Window* window, Vec2 pixelSize)
{
  auto p = getParentWindowInfo(window);
  
  float scale = PlatformView::getDeviceScaleForWindow(p.windowPtr, p.flags);
  Vec2 systemSize = pixelSize/scale;

  SDL_SetWindowSize(window, systemSize.x, systemSize.y);
}


struct ResizingEventWatcherData
{
  SDL_Window* window{nullptr};
  PlatformView* platformView{nullptr};
};

inline void SdlAppResize(ResizingEventWatcherData* watcherData)
{
  int w, h;
  SDL_GetWindowSize(watcherData->window, &w, &h);
  if(w <= 0 || h <= 0) return;
  
  
 // std::cout << "SdlAppResize: " << w << " x " << h << "\n";
  
  watcherData->platformView->resizePlatformView(w, h);
  
}

inline int resizingEventWatcher(void* data, SDL_Event* event)
{
  ResizingEventWatcherData* watcherData = static_cast<ResizingEventWatcherData*>(data);
  SDL_Event& ev = *event;
  
  if(ev.type != SDL_WINDOWEVENT) return 0;
  if (SDL_GetWindowFromID(ev.window.windowID) != watcherData->window) return 0;
  
  switch(ev.window.event)
  {
    case SDL_WINDOWEVENT_RESIZED:// || (ev.window.event == SDL_WINDOWEVENT_MOVED ))
    {
      //std::cout << std::this_thread::get_id() << ": " << ev.window.data1 << " " << ev.window.data2 << std::endl;
      SdlAppResize(watcherData);
      break;
    }
  }
  return 0;
}

} // namespace ml



