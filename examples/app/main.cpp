// mlvg test application
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.


#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"
#include "SDL_syswm.h"

#include "testAppView.h"
#include "testAppController.h"

SDL_Window *window;

bool done{false};

std::unique_ptr< TestAppProcessor > _appProcessor;
std::unique_ptr< TestAppView > _appView;
std::unique_ptr< TestAppController > _appController;


void* _platformHandle{ nullptr };
Vec2 _defaultSystemSize{1280, 720};

void doResize()
{
  // resize mlvg app
  if(_appView)
  {
    int w{0};
    int h{0};
    SDL_GetWindowSize(window, &w, &h);
    if((w > 0) && (h > 0))
    {
      _appView->doResize(Vec2{float(w), float(h)});
    }
  }
}

int resizingEventWatcher( void* data, SDL_Event* event )
{
  SDL_Window* window = static_cast< SDL_Window* >( data );
  SDL_Event& ev = *event;
  
  if( ev.type == SDL_WINDOWEVENT &&
     ev.window.event == SDL_WINDOWEVENT_RESIZED )
  {
    if( SDL_GetWindowFromID( ev.window.windowID ) == window )
    {
      //std::cout << std::this_thread::get_id() << ": " << ev.window.data1 << " " << ev.window.data2 << std::endl;
      doResize();
    }
  }
  return 0;
}

void loop()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    int w{0};
    int h{0};

    // Re-create when window has been resized
    if ((e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_RESIZED))
    {
      SDL_GetWindowSize(window, &w, &h);
      std::cout << "SDL_WINDOWEVENT_RESIZED: " << w << " x " << h << "\n";
      //doResize();
    }
    
    if ((e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
    {
      SDL_GetWindowSize(window, &w, &h);
      std::cout << "SDL_WINDOWEVENT_SIZE_CHANGED: " << w << " x " << h << "\n";
      //doResize();
    }
    
    if (e.type == SDL_QUIT)
    {
      done = 1;
      return;
    }
    
    if ((e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_ESCAPE))
    {
      done = 1;
      return;
    }
  }
}

int main(int argc, char *argv[])
{
  float w = _defaultSystemSize.x();
  float h = _defaultSystemSize.y();
  
  // Enable standard application logging
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
  
  // Initialize SDL 
  if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
    return 1;
  }
  
  // Create window
  window = SDL_CreateWindow("mlvg test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL);
  if(!window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n",SDL_GetError());
    return 1;
  }
  
  // watch for window resize events (during drag)
  SDL_AddEventWatch( resizingEventWatcher, window );
  
  // set min and max sizes
  SDL_SetWindowMinimumSize(window, kGridUnitsX*kMinGridSize, kGridUnitsY*kMinGridSize);
  SDL_SetWindowMaximumSize(window, kGridUnitsX*kMaxGridSize, kGridUnitsY*kMaxGridSize);

  // constraining aspect ratio TODO

  // get window info and initialize info structure
  SDL_SysWMinfo info;
  SDL_VERSION(&info.version);

  if(SDL_GetWindowWMInfo(window, &info))
  {
    const char *subsystem = "an unknown system!";
    switch(info.subsystem)
    {
      default:
      case SDL_SYSWM_UNKNOWN:   break;
      case SDL_SYSWM_WINDOWS:   subsystem = "Microsoft Windows(TM)";  break;
      case SDL_SYSWM_X11:       subsystem = "X Window System";        break;
#if SDL_VERSION_ATLEAST(2, 0, 3)
      case SDL_SYSWM_WINRT:     subsystem = "WinRT";                  break;
#endif
      case SDL_SYSWM_DIRECTFB:  subsystem = "DirectFB";               break;
      case SDL_SYSWM_COCOA:     subsystem = "Apple OS X";             break;
      case SDL_SYSWM_UIKIT:     subsystem = "UIKit";                  break;
#if SDL_VERSION_ATLEAST(2, 0, 2)
      case SDL_SYSWM_WAYLAND:   subsystem = "Wayland";                break;
      case SDL_SYSWM_MIR:       subsystem = "Mir";                    break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 4)
      case SDL_SYSWM_ANDROID:   subsystem = "Android";                break;
#endif
#if SDL_VERSION_ATLEAST(2, 0, 5)
      case SDL_SYSWM_VIVANTE:   subsystem = "Vivante";                break;
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
    
  _appController = make_unique< TestAppController >();
  
  TestAppView* viewPtr = _appController->createTestAppView();
  _appView = std::unique_ptr< TestAppView >(viewPtr);
  
  NSWindow* pParent = info.info.cocoa.window;
  int flags = PlatformView::kParentIsNSWindow;
  _appView->doAttached(static_cast< void* >(pParent), flags);
  
  doResize();
  while (!done)
  {
    loop();
  }
  
  SDL_Quit();
  return 0;
}


