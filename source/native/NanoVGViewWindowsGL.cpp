// mlvg: GUI library for madronalib apps and plugins
// Copyright (C) 2019-2022 Madrona Labs LLC
// This software is provided 'as-is', without any express or implied warranty.
// See LICENSE.txt for details.

#define NANOVG_GL3 1
#define NANOVG_GL3_IMPLEMENTATION

#undef UNICODE

#include "windows.h"
#include "windowsx.h"
#undef min
#undef max
#include "glad.h"
#include "nanovg.h"
#include "nanovg_gl.h"
#include "stdio.h"
#include "shellscalingapi.h"

#include "MLAppView.h"
#include "MLPlatformView.h"

constexpr int kTimerID{ 2 };

// flip scroll and optional scale- could be a runtime setting.
constexpr float kScrollSensitivity{ -1.0f };

// utils
Vec2 PointToVec2(POINT p) { return Vec2{ float(p.x), float(p.y) }; }


// PlatformView

static size_t instanceCount = 0;
const char gWindowClassName[] = "MLVG Window";

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

struct PlatformView::Impl
{
  static LRESULT CALLBACK appWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

  NVGcontext* _nvg{ nullptr };
  ml::AppView* _appView{ nullptr };
  std::unique_ptr< DrawableImage > _nvgBackingLayer;

  HWND _windowHandle{ nullptr };
  HDC _deviceContext{ nullptr };
  HGLRC _openGLContext{ nullptr };
  UINT_PTR _timerID{ 0 };

  CRITICAL_SECTION _drawLock{ nullptr };

  float _deviceScale{ 0 };
  int targetFPS_{ 30 };

  Vec2 _totalDrag;
  Vec2 systemSize_;
  Vec2 newViewSize_;
  bool viewNeedsResize_{ false };

  Impl();
  ~Impl() noexcept;

  void convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* e);
  void convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* e);
  Vec2 eventPositionOnScreen(LPARAM lParam);
  void convertEventFlags(WPARAM wParam, LPARAM lParam, GUIEvent* e);
  void setMousePosition(Vec2 newPos);

  static void initWindowClass();
  static void destroyWindowClass();

  bool setPixelFormat(HDC __deviceContext);
  bool createWindow(HWND parentWindow, void* userData, void* platformHandle, ml::Rect bounds);
  void destroyWindow();
  bool makeContextCurrent();
  bool lockContext();
  bool unlockContext();
  void swapBuffers();
  void doResize();
};

// static utilities

Vec2 PlatformView::getPrimaryMonitorCenter()
{
    float x = GetSystemMetrics(SM_CXSCREEN);
    float y = GetSystemMetrics(SM_CYSCREEN);
    return Vec2{ x/2, y/2 };
}

float PlatformView::getDeviceScaleAtPoint(Vec2 p)
{
    POINT pt{ (long)p.x, (long)p.y };
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    DEVICE_SCALE_FACTOR sf;
    GetScaleFactorForMonitor(hMonitor, &sf);

    return (float)sf / 100.f;
}

float PlatformView::getDeviceScaleForWindow(void* parent, int /*platformFlags*/)
{
    HWND parentWindow = static_cast<HWND>(parent);
    HMONITOR hMonitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);
    DEVICE_SCALE_FACTOR sf;
    GetScaleFactorForMonitor(hMonitor, &sf);

    return (float)sf / 100.f;
}

Rect PlatformView::getWindowRect(void* parent, int )
{
    RECT winRect;
    HWND parentWindow = static_cast<HWND>(parent);
    GetWindowRect(parentWindow, &winRect);
    int x = winRect.left;
    int y = winRect.top;
    int x2 = winRect.right;
    int y2 = winRect.bottom;
    return ml::Rect(x, y, x2 - x, y2 - y);
}

// PlatformView implementation 

PlatformView::Impl::Impl()
{
  initWindowClass();
  InitializeCriticalSection(&_drawLock);
}

PlatformView::Impl::~Impl() noexcept
{
  DeleteCriticalSection(&_drawLock);
  destroyWindowClass();
}

void PlatformView::Impl::initWindowClass()
{
  instanceCount++;
  if (instanceCount == 1)
  {
    WNDCLASS windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_GLOBALCLASS;//|CS_OWNDC; // add Private-DC constant 
    windowClass.lpfnWndProc = appWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = HINST_THISCOMPONENT;
    windowClass.hIcon = nullptr;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszMenuName = nullptr;
    windowClass.lpszClassName = gWindowClassName;
    RegisterClass(&windowClass);
  }
}

void PlatformView::Impl::destroyWindowClass()
{
  instanceCount--;
  if (instanceCount == 0)
  {
    UnregisterClass(gWindowClassName, HINST_THISCOMPONENT);
  }
}

// https://building.enlyze.com/posts/writing-win32-apps-like-its-2020-part-3/ // TEMP

bool PlatformView::Impl::setPixelFormat(HDC dc)
{
    bool status = false;
    if (dc)
    {
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd  
            1,                                // version number  
            PFD_DRAW_TO_WINDOW |              // support window  
            PFD_SUPPORT_OPENGL |              // support OpenGL  
            PFD_DOUBLEBUFFER,                 // double buffered  
            PFD_TYPE_RGBA,                    // RGBA type  
            24,                               // 24-bit color depth  
            0, 0, 0, 0, 0, 0,                 // color bits ignored  
            0,                                // no alpha buffer  
            0,                                // shift bit ignored  
            0,                                // no accumulation buffer  
            0, 0, 0, 0,                       // accum bits ignored  
            32,                               // 32-bit z-buffer      
            8,                                // no stencil buffer  
            0,                                // no auxiliary buffer  
            PFD_MAIN_PLANE,                   // main layer  
            0,                                // reserved  
            0, 0, 0                           // layer masks ignored  
        };

        // get the device context's best, available pixel format match  
        auto format = ChoosePixelFormat(dc, &pfd);

        // make that match the device context's current pixel format  
        status = SetPixelFormat(dc, format, &pfd);
    }
  return status;
}


bool PlatformView::Impl::createWindow(HWND parentWindow, void* userData, void* platformHandle, ml::Rect bounds)
{
  if (_windowHandle)
    return false;

  int x = bounds.left;
  int y = bounds.top;
  int w = bounds.width;
  int h = bounds.height;

  // get monitor device scale
  _deviceScale = getDeviceScaleForWindow(parentWindow);

  auto hInst = static_cast<HINSTANCE>(platformHandle);

  // create child window of the parent we are passed
  _windowHandle = CreateWindowEx(0, gWindowClassName, TEXT("MLVG"),
    WS_CHILD | WS_VISIBLE,
    0, 0, w, h,
    parentWindow, nullptr, hInst, nullptr);

  if (_windowHandle)
  {
    SetWindowLongPtr(_windowHandle, GWLP_USERDATA, (__int3264)(LONG_PTR)userData);

    _deviceContext = GetDC(_windowHandle);

    if (_deviceContext)
    {
      if (setPixelFormat(_deviceContext))
      {
        _openGLContext = wglCreateContext(_deviceContext);
        if (_openGLContext)
        {
          if (makeContextCurrent())
          {
            // TODO error
            if (!gladLoadGL())
            {
              std::cout << "Error initializing glad";
            }
            else
            {
              return true;
            }
          }          
        }
      }
    }

    SetWindowLongPtr(_windowHandle, GWLP_USERDATA, (LONG_PTR)NULL);
    DestroyWindow(_windowHandle);
    _windowHandle = nullptr;
  }
  return false;
}

// destroy our child window.
void PlatformView::Impl::destroyWindow()
{
  if (_windowHandle)
  {
    if (_openGLContext)
    {
      if (wglGetCurrentContext() == _openGLContext)
      {
        wglMakeCurrent(nullptr, nullptr);
      }

      ReleaseDC(_windowHandle, _deviceContext);
      wglDeleteContext(_openGLContext);
      _openGLContext = nullptr;
    }

    DestroyWindow(_windowHandle);
    _windowHandle = nullptr;
  }
}

bool PlatformView::Impl::makeContextCurrent()
{
  if (_openGLContext && _deviceContext)
  {
    return wglMakeCurrent(_deviceContext, _openGLContext) ? true : false;
  }
  return false;
}

bool PlatformView::Impl::lockContext()
{
  EnterCriticalSection(&_drawLock);
  return true;
}

bool PlatformView::Impl::unlockContext()
{
  LeaveCriticalSection(&_drawLock);
  return true;
}

void PlatformView::Impl::doResize()
{
    if (newViewSize_ != systemSize_)
    {
        systemSize_ = newViewSize_;
        float d = _deviceScale;

        // resize window, GL, nanovg
        if (_windowHandle)
        {
            long flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;
            flags |= (SWP_NOCOPYBITS | SWP_DEFERERASE);
            lockContext();
            makeContextCurrent();
            SetWindowPos(_windowHandle, NULL, 0, 0, newViewSize_.x, newViewSize_.y, flags);

            // resize main backing layer
            if (_nvg)
            {
                _nvgBackingLayer = std::make_unique< DrawableImage >(_nvg, newViewSize_.x, newViewSize_.y);
            }
            unlockContext();
        }

        // notify the renderer
        if (_appView)
        {
            _appView->viewResized(_nvg, newViewSize_, _deviceScale);
        }
    }
}

void PlatformView::Impl::swapBuffers()
{
  if (_deviceContext)
  {
    wglMakeCurrent(_deviceContext, nullptr);
    SwapBuffers(_deviceContext);
  }
}


// PlatformView

PlatformView::PlatformView(void* pParent, void* platformHandle, int platformFlags, int fps)
{
  if(!pParent) return;

  _pImpl = std::make_unique< Impl >();
  _pImpl->_deviceScale = getDeviceScaleForWindow(pParent);

  HWND parentHandle = (HWND)pParent;
  Rect bounds = getWindowRect(pParent, 0);


  // create child window and GL
  if (_pImpl->createWindow(parentHandle, this, platformHandle, bounds))
  {
    _pImpl->targetFPS_ = fps;
    
    // create nanovg
    _pImpl->_nvg = nvgCreateGL3(NVG_ANTIALIAS);

  }
}

PlatformView::~PlatformView()
{
    if (!_pImpl) return;

    if (_pImpl->_windowHandle)
    {
        if (_pImpl->_nvg)
        {
            // delete nanovg
            _pImpl->lockContext();
            _pImpl->makeContextCurrent();
            _pImpl->_nvgBackingLayer = nullptr;
            nvgDeleteGL3(_pImpl->_nvg);
            _pImpl->_nvg = 0;
            _pImpl->unlockContext();
        }
        // delete GL, window
        _pImpl->destroyWindow();
    }
}

void PlatformView::setAppView(AppView* pView)
{
    _pImpl->_appView = pView;
    _pImpl->_appView->initializeResources(_pImpl->_nvg);
}

void PlatformView::resizePlatformView(int w, int h)
{
    if (_pImpl)
    {
        _pImpl->viewNeedsResize_ = true;
        _pImpl->newViewSize_ = Vec2(w, h);
    }
}

void PlatformView::Impl::convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
    // get point in view/backing coordinates
    long x = GET_X_LPARAM(lParam);
    long y = GET_Y_LPARAM(lParam);
    vgEvent->screenPos = eventPositionOnScreen(lParam)*_deviceScale;
    vgEvent->position = Vec2(x, y);
}
void PlatformView::Impl::convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
  // get point in view/backing coordinates
  long x = GET_X_LPARAM(lParam);
  long y = GET_Y_LPARAM(lParam);
  POINT p{ x, y };
  ScreenToClient(_windowHandle, &p);
  vgEvent->screenPos = Vec2(x, y) * _deviceScale;
  vgEvent->position = Vec2(p.x, p.y);
}

Vec2 PlatformView::Impl::eventPositionOnScreen(LPARAM lParam)
{
  long x = GET_X_LPARAM(lParam);
  long y = GET_Y_LPARAM(lParam);
  POINT p{ x, y };
  ClientToScreen(_windowHandle, &p);
  return Vec2{ float(p.x), float(p.y) };
}

void PlatformView::Impl::convertEventFlags(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
  uint32_t myFlags{ 0 };
  bool shiftDown = (wParam & MK_SHIFT);

  if (shiftDown)
  {
    myFlags |= shiftModifier;
  }
  vgEvent->keyFlags = myFlags;

  // TODO remaining flags
}

void PlatformView::Impl::setMousePosition(Vec2 newPos)
{
  SetCursorPos(newPos.x, newPos.y);
}

// static
LRESULT CALLBACK PlatformView::Impl::appWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  PlatformView* pGraphics = (PlatformView*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

  if (msg == WM_CREATE)
  {
    // targetFPS_needs to be a little higher than actual preferred rate
    // because of window sync
      float fFps = 30;// pGraphics->_pImpl->targetFPS_;
    int mSec = static_cast<int>(std::round(1000.0 / (fFps * 1.1f )));
    UINT_PTR  err = SetTimer(hWnd, kTimerID, mSec, NULL);
    SetFocus(hWnd);
    DragAcceptFiles(hWnd, true);
    return 0;
  }
  if (msg == WM_DESTROY)
  {
   // SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)NULL);
    return 0;
  }

  if ((!pGraphics) || (hWnd != pGraphics->_pImpl->_windowHandle))
  {
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }

  NVGcontext* nvg = pGraphics->_pImpl->_nvg;
  ml::AppView* pView = pGraphics->_pImpl->_appView;

  switch (msg)
  {
    case WM_TIMER:
    {
      if (wParam == kTimerID)
      {
        // we invalidate the entire window and do our own update region handling.
        InvalidateRect(hWnd, NULL, true);
      }
      return 0;
    }
    case WM_WINDOWPOSCHANGED:
    {
        return 0;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      if ((!nvg) || (!pView)) return 0;
      if (!pGraphics->_pImpl->makeContextCurrent()) return 0;

      // allow Widgets to animate. 
      // NOTE: this might change the backing layer!
      pView->animate(nvg);

      // resize if needed.
      if (pGraphics->_pImpl->viewNeedsResize_)
      {
          pGraphics->_pImpl->doResize();
          pGraphics->_pImpl->viewNeedsResize_ = false;
      }

      // draw

      if (!pGraphics->_pImpl->_nvgBackingLayer) return 0;
      auto pBackingLayer = pGraphics->_pImpl->_nvgBackingLayer.get();
      if (!pBackingLayer) return 0;

      size_t w = pBackingLayer->width;
      size_t h = pBackingLayer->height;

      // draw the AppView to the backing Layer. The backing layer is our persistent
      // buffer, so don't clear it.
      {
          drawToImage(pBackingLayer);
          pView->render(nvg); 
      }

      BeginPaint(hWnd, &ps);
      {
          // blit backing layer to main layer
          drawToImage(nullptr);

          // clear
          glViewport(0, 0, w, h);
          glClearColor(1.f, 1.f, 0.f, 1.f);
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

          nvgBeginFrame(nvg, w, h, 1.0f);

          // get image pattern for 1:1 blit
          NVGpaint img = nvgImagePattern(nvg, 0, 0, w, h, 0, pBackingLayer->_buf->image, 1.0f);
          
          // blit the image
          nvgSave(nvg);
          nvgResetTransform(nvg);
          nvgBeginPath(nvg);
          nvgRect(nvg, 0, 0, w, h);
          nvgFillPaint(nvg, img);
          nvgFill(nvg);
          nvgRestore(nvg);

          // end main update
          nvgEndFrame(nvg);
      }

      // finish platform GL drawing
      pGraphics->_pImpl->swapBuffers();
      EndPaint(hWnd, &ps);

      return 0;
    }

    case WM_LBUTTONDOWN:
    {
      SetFocus(hWnd);
      SetCapture(hWnd);

      GUIEvent e{ "down" };
      pGraphics->_pImpl->convertEventPositions(wParam, lParam, &e);
      pGraphics->_pImpl->convertEventFlags(wParam, lParam, &e);
      pGraphics->_pImpl->_totalDrag = Vec2{ 0, 0 };

      if (pGraphics->_pImpl->_appView)
      { 
        pGraphics->_pImpl->_appView->pushEvent(e);
      }
  
      return 0;
    }

    case WM_RBUTTONDOWN:
    {
      SetFocus(hWnd);
      SetCapture(hWnd);

      GUIEvent e{ "down" };
      pGraphics->_pImpl->convertEventPositions(wParam, lParam, &e);
      pGraphics->_pImpl->convertEventFlags(wParam, lParam, &e);
      e.keyFlags |= controlModifier;

      pGraphics->_pImpl->_totalDrag = Vec2{ 0, 0 };

      if (pGraphics->_pImpl->_appView)
      {
        pGraphics->_pImpl->_appView->pushEvent(e);
      }

      return 0;
    }

    case WM_MOUSEMOVE:
    {
      if (GetCapture())
      {
        GUIEvent e{ "drag" };
        pGraphics->_pImpl->convertEventPositions(wParam, lParam, &e);
        pGraphics->_pImpl->convertEventFlags(wParam, lParam, &e);

        bool repositioned{ false };

        // TEMP screen edge fix commented out for now. TODO fix
        /*
        // get screen height
        HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        GetMonitorInfo(monitor, &info);
        int screenMaxY = info.rcMonitor.bottom - info.rcMonitor.top;

        Vec2 pv = pGraphics->_pImpl->eventPositionOnScreen(lParam);
        const float kDragMargin{ 50.f };
        Vec2 moveDelta;
        
        if (pv.y < 1)
        {
          moveDelta = { 0, kDragMargin };
          repositioned = true;
        }
        else if (pv.y > screenMaxY - 2)
        {
          moveDelta = { 0, -kDragMargin };
          repositioned = true;
        }
       

        if (repositioned)
        {
          // don't send event when repositioning
          Vec2 moveToPos = pv + moveDelta;
          pGraphics->_pImpl->setMousePosition(moveToPos);
          pGraphics->_pImpl->_totalDrag -= moveDelta;
        }
        else
         */
        {
          e.position += pGraphics->_pImpl->_totalDrag;
          if (pGraphics->_pImpl->_appView)
          {
            pGraphics->_pImpl->_appView->pushEvent(e);
          }
        }
      }
      return 0;
    }

    case WM_LBUTTONUP:
    {
      ReleaseCapture();
      GUIEvent e{ "up" };

      pGraphics->_pImpl->convertEventPositions(wParam, lParam, &e);
      pGraphics->_pImpl->convertEventFlags(wParam, lParam, &e);
      if (pGraphics->_pImpl->_appView)
      {
        pGraphics->_pImpl->_appView->pushEvent(e);
      }

      return 0;
    }

    case WM_RBUTTONUP:
    {
      ReleaseCapture();
      GUIEvent e{ "up" };

      pGraphics->_pImpl->convertEventPositions(wParam, lParam, &e);
      pGraphics->_pImpl->convertEventFlags(wParam, lParam, &e);
      e.keyFlags |= controlModifier;
      if (pGraphics->_pImpl->_appView)
      {
        pGraphics->_pImpl->_appView->pushEvent(e);
      }

      return 0;
    }

    case WM_LBUTTONDBLCLK:
    {
      // ? SetCapture(hWnd);
      return 0;
    }
    case WM_MOUSEWHEEL:
    {
      GUIEvent e{ "scroll" };

      // mousewheel messages are sent in screen coordinates!
      pGraphics->_pImpl->convertEventPositionsFromScreen(wParam, lParam, &e);
      pGraphics->_pImpl->convertEventFlags(wParam, lParam, &e);

      float d = float(GET_WHEEL_DELTA_WPARAM(wParam))/WHEEL_DELTA;
      e.delta = Vec2{ 0, d*kScrollSensitivity };

      if (pGraphics->_pImpl->_appView)
      {
        pGraphics->_pImpl->_appView->pushEvent(e);
      }

      return 0;
    }
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(hWnd, &p);

      BYTE keyboardState[256] = {};
      GetKeyboardState(keyboardState);
      const int keyboardScanCode = (lParam >> 16) & 0x00ff;
      WORD character = 0;
      const int len = ToAscii(wParam, keyboardScanCode, keyboardState, &character, 0);
      // TODO: should get unicode?
      bool handle = false;

      // send when len is 0 because wParam might be something like VK_LEFT or VK_HOME, etc.
      if (len == 0 || len == 1)
      {
        char str[2];
        str[0] = static_cast<char>(character);
        str[1] = '\0';

        /*
        IKeyPress keyPress{ str, static_cast<int>(wParam),
          static_cast<bool>(GetKeyState(VK_SHIFT) & 0x8000),
          static_cast<bool>(GetKeyState(VK_CONTROL) & 0x8000),
          static_cast<bool>(GetKeyState(VK_MENU) & 0x8000) };

        if (msg == WM_KEYDOWN)
          handle = pGraphics->OnKeyDown(p.x, p.y, keyPress);
        else
          handle = pGraphics->OnKeyUp(p.x, p.y, keyPress);
          */
      }

      if (!handle)
      {
        HWND rootHWnd = GetAncestor(hWnd, GA_ROOT);
        SendMessage(rootHWnd, msg, wParam, lParam);
        return DefWindowProc(hWnd, msg, wParam, lParam);
      }
      else
        return 0;
    }

    case WM_DROPFILES:
    {
      HDROP hdrop = (HDROP)wParam;

      char pathToFile[1025];
      DragQueryFile(hdrop, 0, pathToFile, 1024);

      POINT point;  
      DragQueryPoint(hdrop, &point);

      // TODO

      return 0;
    }
    case WM_SETFOCUS:
    {
      return 0;
    }
    case WM_KILLFOCUS:
    {
      return 0;
    }
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}
