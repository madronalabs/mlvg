
/* Sample program:  Draw a Chess Board  by using SDL_CreateSoftwareRenderer API */

#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"
#include "SDL_syswm.h"

#include "testAppView.h"

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Surface *surface;
int done;

std::unique_ptr< TestAppView > _appView;

std::unique_ptr< PlatformView > _platformView;
void* _platformHandle{ nullptr };
Vec2 _defaultSystemSize{1280, 720};

void DrawChessBoard(SDL_Renderer * renderer)
{
  int row = 0,column = 0,x = 0;
  SDL_Rect rect, darea;
  
  /* Get the Size of drawing surface */
  SDL_RenderGetViewport(renderer, &darea);
  
  for( ; row < 8; row++)
  {
    column = row%2;
    x = column;
    for( ; column < 4+(row%2); column++)
    {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
      
      rect.w = darea.w/8;
      rect.h = darea.h/8;
      rect.x = x * rect.w;
      rect.y = row * rect.h;
      x = x + 2;
      SDL_RenderFillRect(renderer, &rect);
    }
  }
}

void loop()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    /* Re-create when window has been resized */
    if ((e.type == SDL_WINDOWEVENT) && (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
      
      SDL_DestroyRenderer(renderer);
      
      surface = SDL_GetWindowSurface(window);
      renderer = SDL_CreateSoftwareRenderer(surface);
      /* Clear the rendering surface with the specified color */
      SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
      SDL_RenderClear(renderer);
    }
    
    if (e.type == SDL_QUIT) {
      done = 1;
      return;
    }
    
    if ((e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_ESCAPE)) {
      done = 1;
      return;
    }
  }
  
  DrawChessBoard(renderer);
  
  /* Got everything on rendering surface,
   now Update the drawing image on window screen */
  SDL_UpdateWindowSurface(window);
}

int main(int argc, char *argv[])
{
  
  float w = _defaultSystemSize.x();
  float h = _defaultSystemSize.y();
  
  // Enable standard application logging
  SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
  
  /* Initialize SDL */
  if(SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
    return 1;
  }
  
  // Create window and renderer for given surface
  window = SDL_CreateWindow("Chess Board", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL);
  if(!window)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n",SDL_GetError());
    return 1;
  }
  surface = SDL_GetWindowSurface(window);
  renderer = SDL_CreateSoftwareRenderer(surface);
  if(!renderer)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Render creation for surface fail : %s\n",SDL_GetError());
    return 1;
  }
  
  // Clear the rendering surface with the specified color
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer);
    
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
    
  NSWindow* pParent = info.info.cocoa.window;
  
//  void* pParent = static_cast< NSView* > ();
  

  int flags = PlatformView::kParentIsNSWindow;
  
  ml::Rect boundsRect(0, 0, w, h);
  _appView = make_unique< TestAppView >(boundsRect, controllerName);
  
  _platformView = make_unique< PlatformView >(pParent, boundsRect, *_appView, _platformHandle, flags);

  
 // _resizeEditor(_constrainSize(Vec2(w, h)));
  
  
  
  /* Draw the Image on rendering surface */
  done = 0;
  
  while (!done)
  {
    loop();
  }
  
  SDL_Quit();
  return 0;
}


