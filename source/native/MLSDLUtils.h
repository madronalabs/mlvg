// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#pragma once


#include "SDL.h"
#include "SDL_syswm.h"
#include "MLPlatform.h"

namespace ml {

struct ResizingEventWatcherData
{
  SDL_Window* window{nullptr};
  PlatformView* platformView{nullptr};
};

inline void SdlAppResize(ResizingEventWatcherData* watcherData)
{
  int w{ 0 };
  int h{ 0 };
  SDL_GetWindowSize(watcherData->window, &w, &h);
  if ((w > 0) && (h > 0))
  {
    watcherData->platformView->resizePlatformView(w, h);
  }
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

inline uint32_t getPlatformWindowCreateFlags()
{
#if ML_WINDOWS
  return SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#elif ML_LINUX
  return 0;
#elif ML_IOS
  return SDL_WINDOW_METAL;
#elif ML_MAC
  return SDL_WINDOW_METAL;
#endif
}


inline SDL_Window* newSDLWindow(ml::Rect b, Vec2 minDims, Vec2 maxDims, const char* windowName)
{
  // Enable standard application logging
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
  
  // Initialize SDL
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
    return nullptr;
  }
  
  int commonFlags = SDL_WINDOW_RESIZABLE;
  
  // Create window
  int x = b.left();
  int y = b.top();
  int w = b.width();
  int h = b.height();
  
  SDL_Window* newWindow = SDL_CreateWindow(windowName, x, y, w, h,
                                           commonFlags | getPlatformWindowCreateFlags());
  
  if (!newWindow)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n", SDL_GetError());
    return nullptr;
  }
  
  // set min and max sizes for window
  SDL_SetWindowMinimumSize(newWindow, minDims.x(), minDims.y());
  SDL_SetWindowMaximumSize(newWindow, maxDims.x(), maxDims.y());
  
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

}



