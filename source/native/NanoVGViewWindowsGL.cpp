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

#include "shtypes.h"
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

    // inputs to scale logic
    double systemScale_{ 1. };
    double newSystemScale_{ 1. };
    double dpiScale_{ 1. };
    double newDpiScale_{ 1. };
    Vec2 systemSize_{ 0, 0 };
    Vec2 newSystemSize_{ 0, 0 };

    DeviceScaleMode platformScaleMode_{ kScaleModeUnknown };

    // outputs from scale logic, used for event handling, drawing and backing store creation
    float displayScale_{ 1.0f };
    float eventScale_{ 1.0f };

    Vec2 backingLayerSize_;


    int targetFPS_{ 60 };

    HWND parentPtr{ nullptr };
    Vec2 _totalDrag;

    Impl();
    ~Impl() noexcept;

    void setPlatformScaleMode(DeviceScaleMode mode);
    void setPlatformViewScale(float scale);


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
    void updateDpiScale();
    void resizeIfNeeded();
};

// static utilities

// TEMP

float GetDisplayScalePercentage(HWND hwnd = NULL)
{
    // Initialize COM if needed (call once in your app)
    // CoInitialize(NULL);

    HMONITOR hMonitor;

    if (hwnd != NULL)
    {
        // Get the monitor where the specified window is displayed
        hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    }
    else
    {
        // Get the primary monitor if no window is specified
        hMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    }

    // Get the scale factor for this monitor
    DEVICE_SCALE_FACTOR scaleFactor;
    if (SUCCEEDED(GetScaleFactorForMonitor(hMonitor, &scaleFactor)))
    {
        switch (scaleFactor)
        {
        case SCALE_100_PERCENT: return 1.00f; // 100%
        case SCALE_125_PERCENT: return 1.25f; // 125%
        case SCALE_150_PERCENT: return 1.50f; // 150%
        case SCALE_175_PERCENT: return 1.75f; // 175%
        case SCALE_200_PERCENT: return 2.00f; // 200%
        case SCALE_225_PERCENT: return 2.25f; // 225%
        case SCALE_250_PERCENT: return 2.50f; // 250%
        case SCALE_300_PERCENT: return 3.00f; // 300%
        case SCALE_350_PERCENT: return 3.50f; // 350%
        case SCALE_400_PERCENT: return 4.00f; // 400%
        case SCALE_450_PERCENT: return 4.50f; // 450%
        case SCALE_500_PERCENT: return 5.00f; // 500%
        default: return 1.00f; // Default to 100% if unknown
        }
    }

    // Fallback method using DPI if the above fails
    UINT dpiX, dpiY;
    if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
    {
        return (float)dpiX / 96.0f; // 96 DPI is the default (100%)
    }

    // Second fallback using the system DPI
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        return (float)dpi / 96.0f;
    }

    return 1.0f; // Default to 100% if all methods fail
}

// TEMP

// Get the client area size in physical pixels
SIZE GetClientAreaPhysicalPixelSize(HWND hwnd)
{
    SIZE size = { 0, 0 };

    if (!hwnd || !IsWindow(hwnd))
        return size;

    // Get the client rect
    RECT clientRect;
    if (!GetClientRect(hwnd, &clientRect))
        return size;

    // Convert to screen coordinates (physical pixels)
    POINT topLeft = { clientRect.left, clientRect.top };
    POINT bottomRight = { clientRect.right, clientRect.bottom };

    ClientToScreen(hwnd, &topLeft);
    ClientToScreen(hwnd, &bottomRight);

    // Calculate width and height
    size.cx = bottomRight.x - topLeft.x;
    size.cy = bottomRight.y - topLeft.y;

    return size;
}



static BOOL findMainMonitor(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM out)
{
    MONITORINFO info = {};

    info.cbSize = sizeof(info);
    GetMonitorInfo(monitor, &info);

    if (info.dwFlags & MONITORINFOF_PRIMARY) {
        *((HMONITOR*)out) = monitor;
        return FALSE;
    }

    return TRUE;
}


static float getMonitorScaleFromWindow(HWND hwnd)
{
    std::cout << "----------------------\n";
    DPI_AWARENESS_CONTEXT dpiAwarenessContext = GetThreadDpiAwarenessContext();
    DPI_AWARENESS dpiAwareness = GetAwarenessFromDpiAwarenessContext(dpiAwarenessContext);

    bool tryToChangeContext = (dpiAwareness != DPI_AWARENESS_PER_MONITOR_AWARE);

    bool changedContext{ false };
    if (tryToChangeContext)
    {

        if (SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            changedContext = true;
            std::cout << "Thread DPI awareness context set to Per-Monitor Aware V2." << std::endl;
        }
        else
        {
            std::cout << "couldn't set DPI awareness! \n";
        }
    }

    UINT dpi_x, dpi_y;
    /*
    HMONITOR main_monitor = NULL;


    EnumDisplayMonitors(NULL, NULL, &find_main_monitor,
        (LPARAM)&main_monitor);

    if (main_monitor)
    {
        std::cout << "found main monitor. \n";
    }
    else
    {
        std::cout << "main monitor not found. \n";
    }
    */

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    float monitorDpi{ 96.f };

    if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y) == S_OK)
    {
        std::cout << "monitor DPI: " << dpi_x << "\n";   
        monitorDpi = dpi_x;
    }



    if (changedContext)
    {
        SetThreadDpiAwarenessContext(dpiAwarenessContext);
    }

    return monitorDpi / 96.f;
}


static double getDeviceScaleForWindow(void* parent)
{
    HWND parentWindow = static_cast<HWND>(parent);
    HMONITOR hMonitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);

    DEVICE_SCALE_FACTOR sf;
    GetScaleFactorForMonitor(hMonitor, &sf);
    float scaleRatio = sf / 100.f;
    return scaleRatio;
}

static double getDpiScaleForWindow(void* parent)
{
    HWND parentWindow = static_cast<HWND>(parent);
    HMONITOR hMonitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);

    DPI_AWARENESS_CONTEXT dpiAwarenessContext = GetThreadDpiAwarenessContext();
    DPI_AWARENESS dpiAwareness = GetAwarenessFromDpiAwarenessContext(dpiAwarenessContext);

    std::cout << "--------------\n awareness: " << dpiAwareness << "\n";

    bool tryToChangeContext = (dpiAwareness != DPI_AWARENESS_PER_MONITOR_AWARE);
    bool changedContext{ false };
    if (tryToChangeContext)
    {
        if (SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            changedContext = true;
            std::cout << "Thread DPI awareness context set to Per-Monitor Aware V2." << std::endl;
        }
        else
        {
            std::cout << "couldn't set DPI awareness! \n";
        }
    }

    UINT dpi_x, dpi_y;
    HMONITOR monitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);

    float monitorDpi{ 96.f };

    if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y) == S_OK)
    {
        std::cout << "monitor DPI: " << dpi_x << "\n";
        std::cout << "DPI scale: " << dpi_x / 96.f << "\n";
        monitorDpi = dpi_x;
    }

    if (changedContext)
    {
        SetThreadDpiAwarenessContext(dpiAwarenessContext);
    }
    return monitorDpi / 96.f;
}


// PlatformView::Impl implementation 

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

    int x = bounds.left();
    int y = bounds.top();
    int w = bounds.width();
    int h = bounds.height();

    auto hInst = static_cast<HINSTANCE>(platformHandle);

    // create child window of the parent we are passed
    _windowHandle = CreateWindowEx(0, gWindowClassName, TEXT("MLVG"),
        WS_CHILD | WS_VISIBLE,
        0, 0, w, h,
        parentWindow, nullptr, hInst, nullptr); 

    if (_windowHandle)
    {
        // set mode and device scale
        setPlatformScaleMode(kUseDeviceCoords);
        setPlatformViewScale(getDeviceScaleForWindow(_windowHandle));

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

void PlatformView::Impl::updateDpiScale()
{
    if (_windowHandle)
    {
        newDpiScale_ = getDpiScaleForWindow(_windowHandle);

        std::cout << "updateDpiScale dpi scale: " << newDpiScale_ << "\n";
    }
}

void PlatformView::Impl::resizeIfNeeded()
{
    bool needsResize{ false };
    if (newSystemScale_ != systemScale_)
    {
        systemScale_ = newSystemScale_;
        needsResize = true;
    }
    if (newSystemSize_ != systemSize_)
    {
        systemSize_ = newSystemSize_;
        needsResize = true;
    }
    if (newDpiScale_ != dpiScale_)
    {
        dpiScale_ = newDpiScale_;
        needsResize = true;
    }

    if(needsResize)
    {
        float backingScale{ 1.0f };
        switch (platformScaleMode_)
        {
        case kUseSystemCoords: // plugins
            backingScale = dpiScale_;
            eventScale_ = dpiScale_;
            break;
        case kUseDeviceCoords: // test app
            backingScale = 1.0f;
            eventScale_ = 1.0f;
            break;
        default:
            break;
        }

        backingLayerSize_ = systemSize_ * backingScale; // TEMP

        // NOTE: events change scale w/ backing scale, due to AppView stuff?!

        std::cout << "resizeIfNeeded: system size: " << systemSize_ << ", backing: " << backingScale << ", events: " << eventScale_ << "\n";
        std::cout << "                backing size: " << backingLayerSize_ << "\n";

        // resize window, GL, nanovg
        if (_windowHandle)
        {
            long flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE;
            flags |= (SWP_NOCOPYBITS | SWP_DEFERERASE);
            lockContext();
            makeContextCurrent();
            SetWindowPos(_windowHandle, NULL, 0, 0, systemSize_.x(), systemSize_.y(), flags);

            // resize main backing layer
            if (_nvg)
            {
                _nvgBackingLayer = std::make_unique< DrawableImage >(_nvg, backingLayerSize_.x(), backingLayerSize_.y());
            }
            unlockContext();
        }

        // notify the renderer
        if (_appView)
        {
            _appView->viewResized(_nvg, backingLayerSize_, backingScale);
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


void PlatformView::Impl::setPlatformScaleMode(DeviceScaleMode mode)
{
    platformScaleMode_ = mode;
}

void PlatformView::Impl::setPlatformViewScale(float scale)
{
    std::cout << "setPlatformViewScale: " << scale << "\n";
    newSystemScale_ = scale;
}

static Vec2 pointToVec2(POINT p)
{
    return Vec2{ float(p.x), float(p.y) };
}

void PlatformView::Impl::convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
    long x = GET_X_LPARAM(lParam);
    long y = GET_Y_LPARAM(lParam);
    POINT viewPos{ x, y };

    POINT screenPos = viewPos;
    ClientToScreen(_windowHandle, &screenPos);

    vgEvent->screenPos = pointToVec2(screenPos);
    vgEvent->position = pointToVec2(viewPos) * eventScale_;

   std::cout << "CLICK viewPos: [" << x << ", " << y << "] -> pos: " << vgEvent->position << " screen: " << vgEvent->screenPos << " \n";
}

void PlatformView::Impl::convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
    long x = GET_X_LPARAM(lParam);
    long y = GET_Y_LPARAM(lParam);
    POINT screenPos{ x, y };

    POINT viewPos = screenPos;

    ScreenToClient(_windowHandle, &viewPos);

    vgEvent->screenPos = pointToVec2(screenPos);
    vgEvent->position = pointToVec2(viewPos) * eventScale_;


    std::cout << "WHEEL screenPos: [" << x << ", " << y << "] -> viewPos: " << vgEvent->position << " screen: " << vgEvent->screenPos << " \n";
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
    SetCursorPos(newPos.x(), newPos.y());
}

// static
LRESULT CALLBACK PlatformView::Impl::appWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PlatformView* pGraphics = (PlatformView*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (msg == WM_CREATE)
    {
        // targetFPS_needs to be a little higher than actual preferred rate
        // because of window sync
        float fFps = 60; // NOT WORKING NULLPTR pGraphics->_pImpl->targetFPS_;
        int mSec = static_cast<int>(std::round(1000.0 / (fFps * 1.1f)));
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
        std::cout << "POS CHANGED: \n";
        return 0;
    }
    case WM_DPICHANGED:
    {
        UINT dpi = HIWORD(wParam);
        std::cout << "DPI CHANGED: " << dpi << "\n";


        // NOT BEING CALLED! REVISIT.
        /*
       
        UINT dpi = HIWORD(wParam);

   

        int iDpi = GetDpiForWindow(hWnd);
        std::cout << "WINDOW DPI CHANGED: " << iDpi << "\n";


        // store changed dpi
        //float dpiRatio = dpi / 96.f;
        //pGraphics->setPlatformViewScale(dpiRatio);

        // Get suggested rect for the window
        RECT* pRect = reinterpret_cast<RECT*>(lParam);

        // Resize window according to OS suggestion
        SetWindowPos(
            hWnd, NULL,
            pRect->left, pRect->top,
            pRect->right - pRect->left, pRect->bottom - pRect->top,
            SWP_NOZORDER | SWP_NOACTIVATE
        );

        */

        // Force repaint
        InvalidateRect(hWnd, NULL, TRUE);
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

        pGraphics->_pImpl->resizeIfNeeded();

        // draw

        if (!pGraphics->_pImpl->_nvgBackingLayer) return 0;
        auto pBackingLayer = pGraphics->_pImpl->_nvgBackingLayer.get();
        if (!pBackingLayer) return 0;

        size_t w = pBackingLayer->width;
        size_t h = pBackingLayer->height;



        // draw the AppView to the backing Layer. The backing layer is our persistent
        // buffer, so don't clear it.
        {
            // TEMP
            //drawToImage(pBackingLayer);

            // TEMP - draw direct to window no backing
            BeginPaint(hWnd, &ps);
            drawToImage(nullptr);
            glViewport(0, 0, w, h);
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            pView->setDirty(true);


            pView->render(nvg);


            // TEMP
            EndPaint(hWnd, &ps);
        }

        //BeginPaint(hWnd, &ps);
        if(0)
        {
            // blit backing layer to main layer
            drawToImage(nullptr);

            // clear
            glViewport(0, 0, w, h);

            // TEMP
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            // TEMP
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



            // TEMP
            nvgStrokeWidth(nvg, 4);
            nvgStrokeColor(nvg, colors::black);
            nvgBeginPath(nvg);
            for (int i = 0; i < w; i += 100)
            {
                nvgMoveTo(nvg, i, h);
                nvgLineTo(nvg, i, h - 100);
            }
            nvgStroke(nvg);

            // draw X

            nvgStrokeWidth(nvg, 2);
            nvgStrokeColor(nvg, colors::red);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, 0, 0);
            nvgLineTo(nvg, w, h);
            nvgMoveTo(nvg, w, 0);
            nvgLineTo(nvg, 0, h);
            nvgStroke(nvg);

            // end main update
            nvgEndFrame(nvg);
        }

        // finish platform GL drawing
        pGraphics->_pImpl->swapBuffers();

        // TEMP EndPaint(hWnd, &ps);

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

            if (pv.y() < 1)
            {
              moveDelta = { 0, kDragMargin };
              repositioned = true;
            }
            else if (pv.y() > screenMaxY - 2)
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
                //e.position += pGraphics->_pImpl->_totalDrag;
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

        float d = float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        e.delta = Vec2{ 0, d * kScrollSensitivity };

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
    case WM_ERASEBKGND:
    {
        return 0;
    }
    case WM_SETCURSOR:
    {
        return 0;
    }

    default:
    {
        std::cout << "unhandled window msg: " << std::hex << msg << " ( " << 0x1f << ") " << std::dec << "\n";
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}



// PlatformView

Vec2 PlatformView::getPrimaryMonitorCenter()
{
    float x = GetSystemMetrics(SM_CXSCREEN);
    float y = GetSystemMetrics(SM_CYSCREEN);
    return Vec2{ x / 2, y / 2 };
}

Rect PlatformView::getWindowRect(void* parent, int)
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

PlatformView::PlatformView(void* pParent, AppView* pView, void* platformHandle, int PlatformFlags, int fps)
{
    if (!pParent) return;

    _pImpl = std::make_unique< Impl >();
    //_pImpl->systemScale_ = getDeviceScaleForWindow(pParent);
    //_pImpl->_dpiScale = getDpiScaleForWindow(pParent);

   // std::cout << "PlatformView::PlatformView: device% " << _pImpl->systemScale_ << " dpi% " << _pImpl->_dpiScale << " flags: " << PlatformFlags << "\n";

    _pImpl->parentPtr = (HWND)pParent;
    Rect bounds = getWindowRect(pParent, 0);

    std::cout << "    parent bounds: " << bounds.width() << " x " << bounds.height() << "\n";

    // attach and create GL
    if (_pImpl->createWindow(_pImpl->parentPtr, this, platformHandle, bounds))
    {
        _pImpl->targetFPS_ = fps;

        // create nanovg
        _pImpl->_nvg = nvgCreateGL3(NVG_ANTIALIAS);
    }

    // set app view
    _pImpl->_appView = pView;
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

void PlatformView::attachViewToParent(DeviceScaleMode mode)
{
    if (!_pImpl) return;

    void* pParent = _pImpl->parentPtr;
    Rect parentFrame = getWindowRect(pParent, 0);
    _pImpl->dpiScale_ = getDpiScaleForWindow(pParent);

    _pImpl->setPlatformScaleMode(mode);


    setPlatformViewSize(parentFrame.width(), parentFrame.height());
}

void PlatformView::setPlatformViewSize(int w, int h)
{
    // std::cout << "setPlatformViewSize: " << Vec2(w, h) << "\n";
    if (!_pImpl) return;
    if (_pImpl->platformScaleMode_ == kScaleModeUnknown)
    {
        assert(false, "PlatformView: view size set with scale mode unknown!\n");
    }
    Vec2 newSize(w, h);
    _pImpl->newSystemSize_ = newSize;
}

void PlatformView::updateDpiScale()
{
    // std::cout << "setPlatformViewSize: " << Vec2(w, h) << "\n";
    if (!_pImpl) return;
    _pImpl->updateDpiScale();
}


NativeDrawContext* PlatformView::getNativeDrawContext()
{
    if (!_pImpl) return nullptr;
    return _pImpl->_nvg;
}
