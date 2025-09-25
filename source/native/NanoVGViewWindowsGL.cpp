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

enum DeviceScaleMode
{
    kScaleModeUnknown = 0,
    kUseSystemCoords = 1,
    kUseDeviceCoords = 2
};

constexpr int kTimerID{ 2 };

// flip scroll and optional scale- could be a runtime setting.
constexpr float kScrollSensitivity{ -1.0f };

// when true, apps keep track of dirty regions and composite to an offscreen buffer.
// when false, apps redraw the entire view each frame.
constexpr bool kDoubleBufferView{ true }; 

// static utilities

static Vec2 pointToVec2(POINT p) { return Vec2{ float(p.x), float(p.y) }; }

void PlatformView::initPlatform()
{
    // set DPI awareness.
    // NOTE: some docs state this must be done before making any windows.
    // however it seems to be working for us here after making the SDL window.
    if (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        std::cout << "main: Process marked as Per Monitor DPI Aware v2 successfully.\n";
    }
    else {
        std::cerr << "Failed to set DPI awareness. Error: " << GetLastError() << std::endl;
    }
}

Vec2 PlatformView::getPrimaryMonitorCenter()
{
    float x = GetSystemMetrics(SM_CXSCREEN);
    float y = GetSystemMetrics(SM_CYSCREEN);
    return Vec2{ x / 2, y / 2 };
}

float PlatformView::getDeviceScaleForWindow(void* parent, int /*platformFlags*/)
{
    HWND parentWindow = static_cast<HWND>(parent);
    HMONITOR hMonitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);
    DEVICE_SCALE_FACTOR sf;
    GetScaleFactorForMonitor(hMonitor, &sf);

    return (float)sf / 100.f;
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

static double getDpiScaleForWindow(void* parent)
{
    DPI_AWARENESS_CONTEXT dpiAwarenessContext = GetThreadDpiAwarenessContext();
    DPI_AWARENESS dpiAwareness = GetAwarenessFromDpiAwarenessContext(dpiAwarenessContext);
    bool tryToChangeContext = (dpiAwareness != DPI_AWARENESS_PER_MONITOR_AWARE);
    bool changedContext{ false };
    if (tryToChangeContext)
    {
        if (SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            changedContext = true;
        }
        else
        {
            std::cout << "couldn't set DPI awareness! \n";
        }
    }

    UINT dpi_x, dpi_y;
    HWND parentWindow = static_cast<HWND>(parent);
    HMONITOR monitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);

    float monitorDpi{ 96.f };
    if (GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y) == S_OK)
    {
        monitorDpi = dpi_x;
    }

    if (changedContext)
    {
        SetThreadDpiAwarenessContext(dpiAwarenessContext);
    }
    return monitorDpi / 96.f;
}

static Rect getWindowRect(void* parent)
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


// PlatformView

static size_t instanceCount = 0;
const char gWindowClassName[] = "MLVG Window";

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

struct PlatformView::Impl
{
    HWND windowHandle_{ nullptr };
    HDC deviceContext_{ nullptr };
    HGLRC openGLContext_{ nullptr };
    NVGcontext* nvg_{ nullptr };
    ml::AppView* appView_{ nullptr };
    std::unique_ptr< DrawableImage > nvgBackingLayer_;
    CRITICAL_SECTION drawLock_{ nullptr };
    int targetFPS_{ 30 };
    HWND parentPtr_{ nullptr };
    Vec2 totalDrag_;
    TextFragment windowClassName_;

    // inputs to scale logic
    double dpiScale_{ 1. };
    double newDpiScale_{ 1. };
    Vec2 systemSize_{ 0, 0 };
    Vec2 newSystemSize_{ 0, 0 };
    DeviceScaleMode platformScaleMode_{ kScaleModeUnknown };

    // outputs from scale logic, used for event handling, drawing and backing store creation
    float backingScale_{ 1.0f };
    float eventScale_{ 1.0f };
    Vec2 backingLayerSize_;

    Impl(const char* windowClassName, void* pParentWindow, AppView* pView, void* platformHandle, int flags, int fps);
    ~Impl() noexcept;

    static void createWindowClass(const char* className)
    {
        instanceCount++;
        if (instanceCount == 1)
        {
            WNDCLASSEX windowClass = {};
            windowClass.cbSize = sizeof(WNDCLASSEX);
            windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
            windowClass.lpfnWndProc = appWindowProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = HINST_THISCOMPONENT;
            windowClass.hIcon = nullptr;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszMenuName = nullptr;
            windowClass.lpszClassName = className;
            windowClass.hbrBackground = NULL;
            RegisterClassEx(&windowClass);
        }
    }

    static void destroyWindowClass(const char* className)
    {
        instanceCount--;
        if (instanceCount == 0)
        {
            UnregisterClass(className, HINST_THISCOMPONENT);
        }
    }

    static LRESULT CALLBACK appWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result{ 0 };
        PlatformView::Impl* pImpl = (PlatformView::Impl*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        switch (msg) {
        case WM_CREATE:
            PostMessage(hWnd, WM_SHOWWINDOW, 0, 0);
        default:
            if (pImpl)
            {
                result = pImpl->handleMessage(hWnd, msg, wParam, lParam);
            }
            else
            {
                result = DefWindowProcW(hWnd, msg, wParam, lParam);
            }
        }
        return result;
    }

    bool createWindow(HWND parentWindow, void* platformHandle, ml::Rect bounds);
    void destroyWindow();

    bool createOpenGLContext(HWND hwnd);
    void destroyOpenGLContext();

    void updatePlatformScaleMode();
    void updateDpiScale();
    void convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    void convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    Vec2 eventPositionOnScreen(LPARAM lParam);
    void convertEventFlags(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    void setMousePosition(Vec2 newPos);
    bool makeContextCurrent() const;
    bool lockContext();
    bool unlockContext();
    void swapBuffers();
    void resizeIfNeeded();

    void handlePaint();
    void cleanup();

    LRESULT handleMessage(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
};


// PlatformView::Impl implementation 

PlatformView::Impl::Impl(const char* windowClassNameCStr, void* pParentWindow, AppView* pView, void* platformHandle, int flags, int fps)
{
    // store window class name for CreateWindowEx()
    windowClassName_ = TextFragment(windowClassNameCStr);

    createWindowClass(windowClassNameCStr);
    InitializeCriticalSection(&drawLock_);

    parentPtr_ = (HWND)pParentWindow;
    Rect bounds = getWindowRect(pParentWindow);

    createWindow(parentPtr_, platformHandle, bounds);
    appView_ = pView;
    targetFPS_ = fps;
}

PlatformView::Impl::~Impl() noexcept
{
    cleanup();
}


LRESULT PlatformView::Impl::handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
    case WM_CREATE:
    case WM_SHOWWINDOW:
    case WM_SIZE:
    {
        // targetFPS_needs to be a little higher than actual preferred rate
        // because of window sync
        float fFps = targetFPS_;
        int mSec = static_cast<int>(std::round(1000.0 / (fFps * 1.1f)));
        UINT_PTR  err = SetTimer(hWnd, kTimerID, mSec, NULL);
        SetFocus(hWnd);
        DragAcceptFiles(hWnd, true);
        ShowWindow(hWnd, SW_SHOW);
        return 0;
    }
    case WM_DESTROY:
    {
        cleanup();
        break;
    }
    case WM_TIMER:
    {
        if (wParam == kTimerID)
        {
            // we invalidate the entire window and do our own update region handling.
            InvalidateRect(hWnd, NULL, false);
            handlePaint();
        }
        return 0;
    }
    case WM_WINDOWPOSCHANGED:
    {
        return 0;
    }

    case WM_DPICHANGED:
    {
        UINT dpi = HIWORD(wParam);
        updateDpiScale();

        // Force repaint
        InvalidateRect(windowHandle_, NULL, false);
        return 0;
    }

    case WM_PAINT:
    {
        handlePaint();
        return 0;
    }

    case WM_ERASEBKGND:
    {
        return 1; // we handle background erasing ourselves
    }
    case WM_LBUTTONDOWN:
    {
        SetFocus(windowHandle_);
        SetCapture(windowHandle_);

        GUIEvent e{ "down" };
        convertEventPositions(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);
        totalDrag_ = Vec2{ 0, 0 };

        if (appView_)
        {
            appView_->pushEvent(e);
        }

        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        SetFocus(windowHandle_);
        SetCapture(windowHandle_);

        GUIEvent e{ "down" };
        convertEventPositions(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);
        e.keyFlags |= controlModifier;

        totalDrag_ = Vec2{ 0, 0 };

        if (appView_)
        {
            appView_->pushEvent(e);
        }

        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (GetCapture())
        {
            GUIEvent e{ "drag" };
            convertEventPositions(wParam, lParam, &e);
            convertEventFlags(wParam, lParam, &e);

            bool repositioned{ false };

            // TEMP screen edge fix commented out for now. TODO fix
            /*
            // get screen height
            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &info);
            int screenMaxY = info.rcMonitor.bottom - info.rcMonitor.top;

            Vec2 pv = eventPositionOnScreen(lParam);
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
              setMousePosition(moveToPos);
              totalDrag_ -= moveDelta;
            }
            else
             */
            {
                //e.position += totalDrag_;
                if (appView_)
                {
                    appView_->pushEvent(e);
                }
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        ReleaseCapture();
        GUIEvent e{ "up" };

        convertEventPositions(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);
        if (appView_)
        {
            appView_->pushEvent(e);
        }

        return 0;
    }

    case WM_RBUTTONUP:
    {
        ReleaseCapture();
        GUIEvent e{ "up" };

        convertEventPositions(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);
        e.keyFlags |= controlModifier;
        if (appView_)
        {
            appView_->pushEvent(e);
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
        convertEventPositionsFromScreen(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);

        float d = float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        e.delta = Vec2{ 0, d * kScrollSensitivity };

        if (appView_)
        {
            appView_->pushEvent(e);
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
        ScreenToClient(windowHandle_, &p);

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
            HWND rootHWnd = GetAncestor(windowHandle_, GA_ROOT);
            SendMessage(rootHWnd, msg, wParam, lParam);
            return DefWindowProc(windowHandle_, msg, wParam, lParam);
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


    default:
    {
        //std::cout << "unhandled window msg: " << std::hex << msg << std::dec << "\n";
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}



bool PlatformView::Impl::createWindow(HWND parentWindow, void* platformHandle, ml::Rect bounds)
{
    int w = bounds.width();
    int h = bounds.height();

    auto hInst = static_cast<HINSTANCE>(platformHandle);

    // create child window of the parent we are passed.
    // calls windowProc with WM_CREATE msg
    windowHandle_ = CreateWindowEx(0, windowClassName_.getText(), TEXT("MLVG"),
        WS_CHILD | WS_VISIBLE,
        0, 0, w, h,
        parentWindow, nullptr, hInst, nullptr);

    if (windowHandle_)
    {
        SetWindowLongPtr(windowHandle_, GWLP_USERDATA, (__int3264)(LONG_PTR)this);
        newSystemSize_ = Vec2(w, h);
        newDpiScale_ = getDpiScaleForWindow(windowHandle_);
        // ? newDpiScale_ = getDpiScaleForWindow(parentWindow);
    }
    else
    {
        return false;
    }

    // Get device context
    deviceContext_ = GetDC(windowHandle_);
    if (!deviceContext_) {
        printf("GetDC failed\n");
        return false;
    }

    // Create OpenGL context
    if (!createOpenGLContext(windowHandle_)) {
        printf("Failed to create OpenGL context\n");
        return -1;
    }

    return true;
}

// destroy our child window.
void PlatformView::Impl::destroyWindow()
{
    destroyOpenGLContext();

    if (deviceContext_)
    {
        ReleaseDC(windowHandle_, deviceContext_);
        deviceContext_ = nullptr;
    }
    if (windowHandle_)
    {
        SetWindowLongPtr(windowHandle_, GWLP_USERDATA, (LONG_PTR)NULL);
        DestroyWindow(windowHandle_);
        windowHandle_ = nullptr;
    }
}

bool PlatformView::Impl::createOpenGLContext(HWND hwnd)
{
    // Setup pixel format
    {
        PIXELFORMATDESCRIPTOR pfd = {};

        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;

        // get the device context's best, available pixel format match  
        auto format = ChoosePixelFormat(deviceContext_, &pfd);

        // make that match the device context's current pixel format  
        if (!SetPixelFormat(deviceContext_, format, &pfd)) return false;
    }

    // Create OpenGL context
    openGLContext_ = wglCreateContext(deviceContext_);
    if (!openGLContext_)
    {
        printf("wglCreateContext failed: %d\n", GetLastError());
        return false;
    }

    // Make context current
    if (!wglMakeCurrent(deviceContext_, openGLContext_))
    {
        printf("wglMakeCurrent failed: %d\n", GetLastError());
        return false;
    }

    gladLoadGL();

    nvg_ = nvgCreateGL3(NVG_ANTIALIAS);
    if (!nvg_) return false;

    return true;
}

void PlatformView::Impl::destroyOpenGLContext()
{
    if (nvg_)
    {
        nvgBackingLayer_ = nullptr;

        // delete nanovg
        lockContext();
        makeContextCurrent();
        nvgDeleteGL3(nvg_);
        nvg_ = NULL;
        unlockContext();
    }

    if (openGLContext_)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(openGLContext_);
        openGLContext_ = NULL;
    }
}

void PlatformView::Impl::updatePlatformScaleMode()
{
    HMONITOR hMonitor = MonitorFromWindow(windowHandle_, MONITOR_DEFAULTTONEAREST);

    DPI_AWARENESS_CONTEXT dpiAwarenessContext = GetThreadDpiAwarenessContext();
    DPI_AWARENESS dpiAwareness = GetAwarenessFromDpiAwarenessContext(dpiAwarenessContext);

    if (dpiAwareness == DPI_AWARENESS_PER_MONITOR_AWARE)
    {
        platformScaleMode_ = kUseDeviceCoords;
    }
    else
    {
        platformScaleMode_ = kUseSystemCoords;
    }
}

bool PlatformView::Impl::makeContextCurrent() const
{
    if (openGLContext_ && deviceContext_)
    {
        return wglMakeCurrent(deviceContext_, openGLContext_) ? true : false;
    }
    return false;
}

bool PlatformView::Impl::lockContext()
{
    EnterCriticalSection(&drawLock_);
    return true;
}

bool PlatformView::Impl::unlockContext()
{
    LeaveCriticalSection(&drawLock_);
    return true;
}

void PlatformView::Impl::updateDpiScale()
{
    if (windowHandle_)
    {
        newDpiScale_ = getDpiScaleForWindow(windowHandle_);
    }
}

// NOTE: This implementation has a lot of extra logic and always ends up
// setting a scale of 1.0. I'm leaving the extra logic here because
// it's likely to be needed in the future. 
void PlatformView::Impl::resizeIfNeeded()
{
    bool needsResize{ false };

    if (newSystemSize_ != systemSize_)
    {
        needsResize = true;
    }
    if (newDpiScale_ != dpiScale_)
    {
        needsResize = true;
    }

    if (needsResize)
    {
        // use newSystemSize_ and newDpiScale_ values to resize view and backing store
        switch (platformScaleMode_)
        {
        case kUseSystemCoords: // non-dpi-aware plugins
            backingScale_ = 1.0f;
            eventScale_ = 1.0f;
            break;
        case kUseDeviceCoords: // dpi-aware plugin, app
            backingScale_ = 1.0f;
            eventScale_ = 1.0f;
            break;
        default:
            backingScale_ = 1.0f;
            eventScale_ = 1.0f;
            break;
        }

        backingLayerSize_ = newSystemSize_ * backingScale_;

        // resize window, GL, nanovg  
        if (windowHandle_)
        {
            long flags = SWP_NOZORDER | SWP_NOMOVE | SWP_NOCOPYBITS | SWP_NOACTIVATE;
            lockContext();
            makeContextCurrent();
            SetWindowPos(windowHandle_, NULL, 0, 0, backingLayerSize_.x(), backingLayerSize_.y(), flags);

            // resize main backing layer
            if (nvg_)
            {
                nvgBackingLayer_ = std::make_unique< DrawableImage >(nvg_, backingLayerSize_.x(), backingLayerSize_.y());
            }
            unlockContext();
        }

        // notify the renderer
        if (appView_)
        {
            appView_->viewResized(nvg_, backingLayerSize_, backingScale_);
        }

        // change current size and scale 
        systemSize_ = newSystemSize_;
        dpiScale_ = newDpiScale_;
    }
}

void PlatformView::Impl::swapBuffers()
{
    if (deviceContext_)
    {
        wglMakeCurrent(deviceContext_, nullptr);
        SwapBuffers(deviceContext_);
    }
}

void PlatformView::Impl::convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
    long x = GET_X_LPARAM(lParam);
    long y = GET_Y_LPARAM(lParam);
    POINT viewPos{ x, y };

    POINT screenPos = viewPos;
    ClientToScreen(windowHandle_, &screenPos);

    vgEvent->screenPos = pointToVec2(screenPos);
    vgEvent->position = pointToVec2(viewPos) * eventScale_;

    //std::cout << "CLICK viewPos: [" << x << ", " << y << "] -> pos: " << vgEvent->position << " screen: " << vgEvent->screenPos << " \n";
}

void PlatformView::Impl::convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* vgEvent)
{
    long x = GET_X_LPARAM(lParam);
    long y = GET_Y_LPARAM(lParam);
    POINT screenPos{ x, y };

    POINT viewPos = screenPos;

    ScreenToClient(windowHandle_, &viewPos);

    vgEvent->screenPos = pointToVec2(screenPos);
    vgEvent->position = pointToVec2(viewPos) * eventScale_;

    //std::cout << "WHEEL screenPos: [" << x << ", " << y << "] -> viewPos: " << vgEvent->position << " screen: " << vgEvent->screenPos << " \n";
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

void PlatformView::Impl::handlePaint()
{
    if (!windowHandle_) return;
    if (!makeContextCurrent()) return;
    appView_->animate(nvg_);
    resizeIfNeeded();

    size_t w = backingLayerSize_.x();
    size_t h = backingLayerSize_.y();

    if (kDoubleBufferView)
    {
        if (!nvgBackingLayer_) return;
        auto pBackingLayer = nvgBackingLayer_.get();
        NVGpaint img = nvgImagePattern(nvg_, 0, 0, w, h, 0, pBackingLayer->_buf->image, 1.0f);

        drawToImage(pBackingLayer);
        nvgBeginFrame(nvg_, w, h, 1.0f);
        appView_->render(nvg_);
        nvgEndFrame(nvg_);

        drawToImage(nullptr);
        glViewport(0, 0, w, h);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgBeginFrame(nvg_, w, h, 1.0f);
        nvgSave(nvg_);
        nvgResetTransform(nvg_);
        nvgBeginPath(nvg_);
        nvgRect(nvg_, 0, 0, w, h);
        nvgFillPaint(nvg_, img);
        nvgFill(nvg_);
        nvgRestore(nvg_);
        nvgEndFrame(nvg_);
    }
    else
    {
        drawToImage(nullptr);
        glViewport(0, 0, w, h);
        nvgBeginFrame(nvg_, w, h, 1.0f);

        // set view dirty to redraw entire frame
        appView_->setDirty(true);
        appView_->render(nvg_);
        nvgEndFrame(nvg_);
    }

    // Validate since we didn't use BeginPaint
    ValidateRect(windowHandle_, NULL);

    swapBuffers();

    return;
}


void PlatformView::Impl::cleanup() 
{
    destroyOpenGLContext();
    destroyWindow();

    DeleteCriticalSection(&drawLock_);
    destroyWindowClass(windowClassName_.getText());
}



// PlatformView

PlatformView::PlatformView(const char* className, void* pParent, AppView* pView, void* platformHandle, int PlatformFlags, int fps)
{
    if (!pParent) return;
    _pImpl = std::make_unique< Impl >(className, pParent, pView, platformHandle, PlatformFlags, fps);
}

PlatformView::~PlatformView()
{

}

void PlatformView::attachViewToParent()
{
    if (!_pImpl) return;

    void* pParent = _pImpl->parentPtr_;
    Rect parentFrame = getWindowRect(pParent);
    _pImpl->updateDpiScale();
    _pImpl->updatePlatformScaleMode();
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

NativeDrawContext* PlatformView::getNativeDrawContext()
{
    if (!_pImpl) return nullptr;
    return _pImpl->nvg_;
}
