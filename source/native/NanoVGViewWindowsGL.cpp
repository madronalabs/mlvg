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


constexpr bool kDoubleBufferView{ true };


// static utilities

static Vec2 PointToVec2(POINT p) { return Vec2{ float(p.x), float(p.y) }; }

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

static GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
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
    HWND _windowHandle{ nullptr };
    HDC _deviceContext{ nullptr };
    HGLRC _openGLContext{ nullptr };
    NVGcontext* _nvg{ nullptr };
    ml::AppView* _appView{ nullptr };
    std::unique_ptr< DrawableImage > _nvgBackingLayer;
    UINT_PTR _timerID{ 0 };
    CRITICAL_SECTION _drawLock{ nullptr };

    // inputs to scale logic
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

    TextFragment windowClassName_;
    GLuint shaderTestProgram_;

    Impl(const char* windowClassName, void* pParentWindow, AppView* pView, void* platformHandle, int flags, int fps);
    ~Impl() noexcept;

    static void createWindowClass(const char* className)
    {
        instanceCount++;
        if (instanceCount == 1)
        {
            WNDCLASS windowClass = {};
            windowClass.style = CS_OWNDC;//CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

            windowClass.lpfnWndProc = appWindowProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = HINST_THISCOMPONENT;
            windowClass.hIcon = nullptr;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.lpszMenuName = nullptr;
            windowClass.lpszClassName = className;
            windowClass.hbrBackground = NULL;

            RegisterClass(&windowClass);
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
        PlatformView::Impl* pImpl = (PlatformView::Impl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

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

    void convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    void convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    Vec2 eventPositionOnScreen(LPARAM lParam);
    void convertEventFlags(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    void setMousePosition(Vec2 newPos);

    bool makeContextCurrent() const;
    bool lockContext();
    bool unlockContext();
    void swapBuffers();
    void updateDpiScale();
    void resizeIfNeeded();

    void paintTestPattern(HWND hWnd);

    LRESULT handleMessage(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
};


// PlatformView::Impl implementation 

PlatformView::Impl::Impl(const char* windowClassName, void* pParentWindow, AppView* pView, void* platformHandle, int flags, int fps)
{
    // store window class name for CreateWindowEx()
    windowClassName_ = TextFragment(windowClassName);

    PlatformView::Impl::createWindowClass(windowClassName);
    InitializeCriticalSection(&_drawLock);

    parentPtr = (HWND)pParentWindow;
    Rect bounds = getWindowRect(pParentWindow);

    createWindow(parentPtr, platformHandle, bounds);
    _appView = pView;
    targetFPS_ = fps;
}

PlatformView::Impl::~Impl() noexcept
{
    destroyOpenGLContext();
    destroyWindow();

    DeleteCriticalSection(&_drawLock);
    PlatformView::Impl::destroyWindowClass(windowClassName_.getText());
}

bool PlatformView::Impl::createWindow(HWND parentWindow, void* platformHandle, ml::Rect bounds)
{
    int w = bounds.width();
    int h = bounds.height();

    auto hInst = static_cast<HINSTANCE>(platformHandle);

    // create child window of the parent we are passed.
    // calls windowProc with WM_CREATE msg
    _windowHandle = CreateWindowEx(0, windowClassName_.getText(), TEXT("MLVG"),
        WS_CHILD | WS_VISIBLE,
        0, 0, w, h,
        parentWindow, nullptr, hInst, nullptr); 
  
    if (_windowHandle)
    {
        SetWindowLongPtr(_windowHandle, GWLP_USERDATA, (__int3264)(LONG_PTR)this);
        newSystemSize_ = Vec2(w, h);
        newDpiScale_ = getDpiScaleForWindow(_windowHandle);
    }

    // Get device context
    _deviceContext = GetDC(_windowHandle);
    if (!_deviceContext) {
        printf("GetDC failed\n");
        return false;
    }

    // Create OpenGL context
    if (!createOpenGLContext(_windowHandle)) {
        printf("Failed to create OpenGL context\n");
        return -1;
    }

    // setting these sizes will cause resize in resizeIfNeeded()
    newSystemSize_ = Vec2(w, h);
    newDpiScale_ = getDpiScaleForWindow(parentWindow);

    return true;
}

// destroy our child window.
void PlatformView::Impl::destroyWindow()
{
    destroyOpenGLContext();

    if (_deviceContext)
    {
        ReleaseDC(_windowHandle, _deviceContext);
        _deviceContext = nullptr;
    }
    if (_windowHandle)
    {
        SetWindowLongPtr(_windowHandle, GWLP_USERDATA, (LONG_PTR)NULL);
        DestroyWindow(_windowHandle);
        _windowHandle = nullptr;
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
        auto format = ChoosePixelFormat(_deviceContext, &pfd);

        // make that match the device context's current pixel format  
        if(!SetPixelFormat(_deviceContext, format, &pfd)) return false;
    }

    // Create OpenGL context
    _openGLContext = wglCreateContext(_deviceContext);
    if (!_openGLContext) 
    {
        printf("wglCreateContext failed: %d\n", GetLastError());
        return false;
    }

    // Make context current
    if (!wglMakeCurrent(_deviceContext, _openGLContext)) 
    {
        printf("wglMakeCurrent failed: %d\n", GetLastError());
        return false;
    }

   // printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
   // printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));

    gladLoadGL();

    _nvg = nvgCreateGL3(NVG_ANTIALIAS);
    if (!_nvg) return false;

    return true;
}

void PlatformView::Impl::destroyOpenGLContext()
{
    if (_nvg)
    {
        _nvgBackingLayer = nullptr;

        // delete nanovg
        lockContext();
        makeContextCurrent();
        nvgDeleteGL3(_nvg);
        _nvg = NULL;
        unlockContext();
    }

    if (_openGLContext)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(_openGLContext);
        _openGLContext = NULL;
    }
}


void PlatformView::Impl::updatePlatformScaleMode()
{
    HMONITOR hMonitor = MonitorFromWindow(_windowHandle, MONITOR_DEFAULTTONEAREST);

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
    }
}

void PlatformView::Impl::resizeIfNeeded()
{
    bool needsResize{ false };

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
        case kUseSystemCoords: // non-dpi-aware plugins
            backingScale = 1.0f;
            eventScale_ = 1.0f;
            break;
        case kUseDeviceCoords: // dpi-aware plugin, app
            backingScale = 1.0f;
            eventScale_ = 1.0f;
            break;
        default:
            backingScale = 1.0f;
            eventScale_ = 1.0f;
            break;
        }

        backingLayerSize_ = systemSize_ * backingScale; 

        // resize window, GL, nanovg  
        if (_windowHandle)
        {
            long flags = SWP_NOZORDER | SWP_NOMOVE;
            lockContext();
            makeContextCurrent();
            SetWindowPos(_windowHandle, NULL, 0, 0, backingLayerSize_.x(), backingLayerSize_.y(), flags);

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

   //std::cout << "CLICK viewPos: [" << x << ", " << y << "] -> pos: " << vgEvent->position << " screen: " << vgEvent->screenPos << " \n";
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

void PlatformView::Impl::paintTestPattern(HWND hWnd)
{
    size_t w = backingLayerSize_.x();
    size_t h = backingLayerSize_.y();

    glClearColor(0.f, 0.125f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    float vertices[] = {
        // Positions (x, y, z)
        0.0f,  0.5f, 0.0f,  // Top vertex
       -0.5f, -0.5f, 0.0f,  // Bottom-left vertex
        0.5f, -0.5f, 0.0f   // Bottom-right vertex
    };

    // 2. Create and bind a Vertex Array Object (VAO)
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // 3. Create and bind a Vertex Buffer Object (VBO)
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 4. Define the vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 5. Unbind the VAO and VBO (optional for safety)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 6. Render the triangle in your render loop
    glUseProgram(shaderTestProgram_); // Use your shader program
    glBindVertexArray(VAO);      // Bind the VAO
    glDrawArrays(GL_TRIANGLES, 0, 3); // Draw the triangle
    glBindVertexArray(0);        // Unbind the VAO
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
        
        return 0;
    }
    case WM_DESTROY:
    {
        break;
    }
    case WM_TIMER:
    {
        if (wParam == kTimerID)
        {
            // we invalidate the entire window and do our own update region handling.
            InvalidateRect(hWnd, NULL, false);
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
        std::cout << "DPI CHANGED: " << dpi << "\n";
        updateDpiScale();

        // Force repaint
        InvalidateRect(_windowHandle, NULL, false);
        return 0;
    }
                                             
    case WM_PAINT:
    {
        if (!makeContextCurrent()) return 0;
        _appView->animate(_nvg);
        resizeIfNeeded();

        size_t w = backingLayerSize_.x();
        size_t h = backingLayerSize_.y();

        if (kDoubleBufferView)
        {
            if (!_nvgBackingLayer) return 0;
            auto pBackingLayer = _nvgBackingLayer.get();
            NVGpaint img = nvgImagePattern(_nvg, 0, 0, w, h, 0, pBackingLayer->_buf->image, 1.0f);

            drawToImage(pBackingLayer);
            nvgBeginFrame(_nvg, w, h, 1.0f);
            _appView->render(_nvg);
            nvgEndFrame(_nvg);

            drawToImage(nullptr);
            glViewport(0, 0, w, h);
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
            nvgBeginFrame(_nvg, w, h, 1.0f);
            nvgSave(_nvg);
            nvgResetTransform(_nvg);
            nvgBeginPath(_nvg);
            nvgRect(_nvg, 0, 0, w, h);
            nvgFillPaint(_nvg, img);
            nvgFill(_nvg);
            nvgRestore(_nvg);
            nvgEndFrame(_nvg);
        }
        else
        {
            drawToImage(nullptr);
            glViewport(0, 0, w, h);
            nvgBeginFrame(_nvg, w, h, 1.0f);

            // set view dirty to redraw entire frame
            _appView->setDirty(true);
            _appView->render(_nvg);
            nvgEndFrame(_nvg);
        }
        swapBuffers();

        // Validate since we didn't use BeginPaint
        ValidateRect(hWnd, NULL);

        return 0;
    }

    case WM_ERASEBKGND:
    {
        return 1; // we handle background erasing ourselves
    }
    case WM_LBUTTONDOWN:
    {
        SetFocus(_windowHandle);
        SetCapture(_windowHandle);

        GUIEvent e{ "down" };
        convertEventPositions(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);
        _totalDrag = Vec2{ 0, 0 };

        if (_appView)
        {
            _appView->pushEvent(e);
        }

        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        SetFocus(_windowHandle);
        SetCapture(_windowHandle);

        GUIEvent e{ "down" };
        convertEventPositions(wParam, lParam, &e);
        convertEventFlags(wParam, lParam, &e);
        e.keyFlags |= controlModifier;

        _totalDrag = Vec2{ 0, 0 };

        if (_appView)
        {
            _appView->pushEvent(e);
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
              _totalDrag -= moveDelta;
            }
            else
             */
            {
                //e.position += _totalDrag;
                if (_appView)
                {
                    _appView->pushEvent(e);
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
        if (_appView)
        {
            _appView->pushEvent(e);
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
        if (_appView)
        {
            _appView->pushEvent(e);
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

        if (_appView)
        {
            _appView->pushEvent(e);
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
        ScreenToClient(_windowHandle, &p);

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
            HWND rootHWnd = GetAncestor(_windowHandle, GA_ROOT);
            SendMessage(rootHWnd, msg, wParam, lParam);
            return DefWindowProc(_windowHandle, msg, wParam, lParam);
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



// PlatformView

Vec2 PlatformView::getPrimaryMonitorCenter()
{
    float x = GetSystemMetrics(SM_CXSCREEN);
    float y = GetSystemMetrics(SM_CYSCREEN);
    return Vec2{ x / 2, y / 2 };
}


PlatformView::PlatformView(const char* className, void* pParent, AppView* pView, void* platformHandle, int PlatformFlags, int fps)
{
    if (!pParent) return;

    _pImpl = std::make_unique< Impl >(className, pParent, pView, platformHandle, PlatformFlags, fps);
}

PlatformView::~PlatformView()
{

}

void PlatformView::attachViewToParent(DeviceScaleMode mode)
{
    if (!_pImpl) return;

    void* pParent = _pImpl->parentPtr;
    Rect parentFrame = getWindowRect(pParent);
    updateDpiScale();

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
