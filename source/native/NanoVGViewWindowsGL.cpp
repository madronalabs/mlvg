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

// added a GL test pattern mode for diagnosing window/DPI issues
constexpr bool kGLTestPatternOnly{ false };


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

    Impl(const char* windowClassName, void* pParentWindow, AppView* pView, void* platformHandle, int flags, int fps);
    ~Impl() noexcept;

    void updatePlatformScaleMode();


    void convertEventPositions(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    void convertEventPositionsFromScreen(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    Vec2 eventPositionOnScreen(LPARAM lParam);
    void convertEventFlags(WPARAM wParam, LPARAM lParam, GUIEvent* e);
    void setMousePosition(Vec2 newPos);

    static void initWindowClass(const char* className);
    static void destroyWindowClass(const char* className);

    void createGLResources();
    void destroyGLResources();

    bool setPixelFormat(HDC __deviceContext);

    bool createWindow(HWND parentWindow, void* platformHandle, ml::Rect bounds);
    void destroyWindow();

    bool makeContextCurrent();
    bool lockContext();
    bool unlockContext();
    void swapBuffers();
    void updateDpiScale();
    void resizeIfNeeded();

    void paintTestPattern(HWND hWnd);
    void paintView(HWND hWnd);

    GLuint shaderTestProgram_;
};

// static utilities


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


static double getDpiScaleForWindow(void* parent)
{
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
    HWND parentWindow = static_cast<HWND>(parent);
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

static GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

void PlatformView::Impl::updatePlatformScaleMode()
{
    HWND parentWindow = static_cast<HWND>(_windowHandle);
    HMONITOR hMonitor = MonitorFromWindow(parentWindow, MONITOR_DEFAULTTONEAREST);

    DPI_AWARENESS_CONTEXT dpiAwarenessContext = GetThreadDpiAwarenessContext();
    DPI_AWARENESS dpiAwareness = GetAwarenessFromDpiAwarenessContext(dpiAwarenessContext);

    std::cout << "--------------\n awareness: " << dpiAwareness << "\n";


    if (dpiAwareness == DPI_AWARENESS_PER_MONITOR_AWARE)
    {
        platformScaleMode_ = kUseDeviceCoords;
    }
    else
    {
        platformScaleMode_ = kUseSystemCoords;
    }
}


// PlatformView::Impl implementation 

PlatformView::Impl::Impl(const char* windowClassName, void* pParentWindow, AppView* pView, void* platformHandle, int flags, int fps)
{
    windowClassName_ = TextFragment(windowClassName);
    initWindowClass(windowClassName_.getText());
    InitializeCriticalSection(&_drawLock);

    parentPtr = (HWND)pParentWindow;
    Rect bounds = getWindowRect(pParentWindow);

    createWindow(parentPtr, platformHandle, bounds);
    _appView = pView;
    targetFPS_ = fps;

}

PlatformView::Impl::~Impl() noexcept
{

    if (_windowHandle)
    {
        destroyWindow();
    }

    DeleteCriticalSection(&_drawLock);
    destroyWindowClass(windowClassName_.getText());
}

// static
void PlatformView::Impl::initWindowClass(const char* className)
{
    instanceCount++;
    if (instanceCount == 1)
    {
        WNDCLASS windowClass = {};
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

        windowClass.lpfnWndProc = appWindowProc;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = HINST_THISCOMPONENT;
        windowClass.hIcon = nullptr;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszMenuName = nullptr;
        windowClass.lpszClassName = className;
     //   windowClass.hbrBackground = NULL;

        RegisterClass(&windowClass);
    }
}

// static
void PlatformView::Impl::destroyWindowClass(const char* className)
{
    instanceCount--;
    if (instanceCount == 0)
    {
        UnregisterClass(className, HINST_THISCOMPONENT);
    }
}

bool PlatformView::Impl::createWindow(HWND parentWindow, void* platformHandle, ml::Rect bounds)
{
    if (_windowHandle)
        return false;

    float dpiScale = 1.00f;

    int w = bounds.width() * dpiScale;
    int h = bounds.height() * dpiScale;

    auto hInst = static_cast<HINSTANCE>(platformHandle);

    // create child window of the parent we are passed
    _windowHandle = CreateWindowEx(0, windowClassName_.getText(), TEXT("MLVG"),
        WS_CHILD | WS_VISIBLE,
        0, 0, w, h,
        parentWindow, nullptr, hInst, nullptr); 

    if (_windowHandle)
    {
        SetWindowLongPtr(_windowHandle, GWLP_USERDATA, (__int3264)(LONG_PTR)this);

        _deviceContext = GetDC(_windowHandle);

        // setting these sizes will cause resize in resizeIfNeeded()
        newSystemSize_ = Vec2(w, h);
        newDpiScale_ = getDpiScaleForWindow(parentWindow);
    }
    return false;
}

// destroy our child window.
void PlatformView::Impl::destroyWindow()
{
    if (_openGLContext)
    {
        // needed?             if (wglGetCurrentContext() == _openGLContext)
        wglMakeCurrent(NULL, NULL);

   

        wglDeleteContext(_openGLContext);
        _openGLContext = NULL;
    }

    if (_windowHandle)
    {

        ReleaseDC(_windowHandle, _deviceContext);

        SetWindowLongPtr(_windowHandle, GWLP_USERDATA, (LONG_PTR)NULL);
        DestroyWindow(_windowHandle);
        _windowHandle = nullptr;
    }

}

bool PlatformView::Impl::setPixelFormat(HDC dc)
{
    bool status = false;
    if (dc)
    {
        PIXELFORMATDESCRIPTOR pfd = {};

        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;

        // get the device context's best, available pixel format match  
        auto format = ChoosePixelFormat(dc, &pfd);

        // make that match the device context's current pixel format  
        status = SetPixelFormat(dc, format, &pfd);
    }
    return status;
}

void PlatformView::Impl::createGLResources()
{
    if (kGLTestPatternOnly)
    {
        const char* vertexShader = R"(
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
)";

        const char* fragmentShader = R"(
out vec4 FragColor;
void main() {
    FragColor = vec4(0.0, 0.875, 0.0, 1.0);
}
)";

        // Create and compile shaders
        GLuint vs = compileShader(vertexShader, GL_VERTEX_SHADER);
        GLuint fs = compileShader(fragmentShader, GL_FRAGMENT_SHADER);

        // Create shader program
        shaderTestProgram_ = glCreateProgram();
        glAttachShader(shaderTestProgram_, vs);
        glAttachShader(shaderTestProgram_, fs);
        glLinkProgram(shaderTestProgram_);
    }
    else
    {
        _nvg = nvgCreateGL3(NVG_ANTIALIAS);
    }
}

void PlatformView::Impl::destroyGLResources()
{
    if (kGLTestPatternOnly)
    {
    }
    else
    {
        if (_nvg)
        {
            // delete nanovg
            lockContext();
            makeContextCurrent();
            nvgDeleteGL3(_nvg);
            _nvg = NULL;
            unlockContext();
        }
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
        getDpiScaleForWindow(_windowHandle);
        std::cout << "updateDpiScale dpi scale: " << newDpiScale_ << "\n";
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

        std::cout << "resizeIfNeeded: system size: " << systemSize_ << ", backing: " << backingScale << ", events: " << eventScale_ << "\n";
        std::cout << "                backing size: " << backingLayerSize_ << "\n";

        // resize window, GL, nanovg  
        if (_windowHandle)
        {
            long flags = SWP_NOZORDER | SWP_NOMOVE;
            // flags |= (SWP_NOCOPYBITS | SWP_DEFERERASE);
            lockContext();
            makeContextCurrent();
            SetWindowPos(_windowHandle, NULL, 0, 0, backingLayerSize_.x(), backingLayerSize_.y(), flags);

            ReleaseDC(_windowHandle, _deviceContext);
            _deviceContext = GetDC(_windowHandle);

            // TEST
            if (_openGLContext)
            {
                wglMakeCurrent(NULL, NULL);
                destroyGLResources();

                wglDeleteContext(_openGLContext);
                _openGLContext = NULL;
            }

            if (setPixelFormat(_deviceContext))
            {
                _openGLContext = wglCreateContext(_deviceContext);
                makeContextCurrent();

                gladLoadGL();
                createGLResources();
            }

            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);

            printf("Viewport: %dx%d at (%d,%d)\n", viewport[2], viewport[3], viewport[0], viewport[1]);

            // resize main backing layer
            if (_nvg)
            {
                //_nvgBackingLayer = std::make_unique< DrawableImage >(_nvg, backingLayerSize_.x(), backingLayerSize_.y());
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

    glViewport(0, 0, w, h);

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

void PlatformView::Impl::paintView(HWND hWnd)
{
    NVGcontext* nvg = _nvg;
    size_t w = backingLayerSize_.x();
    size_t h = backingLayerSize_.y();

    // blit backing layer to main layer
    drawToImage(nullptr);

    // clear
    glViewport(0, 0, w, h);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(nvg, w, h, 1.0f);

    if (0)
    {
        // get image pattern for 1:1 blit
        NVGpaint img = nvgImagePattern(nvg, 0, 0, w, h, 0, _nvgBackingLayer->_buf->image, 1.0f);

        // blit the image
        nvgSave(nvg);
        nvgResetTransform(nvg);
        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, w, h);
        nvgFillPaint(nvg, img);
        nvgFill(nvg);
        nvgRestore(nvg);
    }

    // TEMP
    nvgStrokeWidth(nvg, 4);
    nvgStrokeColor(nvg, colors::black);
    nvgBeginPath(nvg);
    for (int i = 0; i < w; i += 100)
    {
        nvgMoveTo(nvg, i, 0);
        nvgLineTo(nvg, i, h);
    }
    for (int j = 0; j < h; j += 100)
    {
        nvgMoveTo(nvg, 0, j);
        nvgLineTo(nvg, w, j);
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

// static
LRESULT CALLBACK PlatformView::Impl::appWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PlatformView::Impl* pImpl = (PlatformView::Impl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (msg == WM_CREATE)
    {
        // targetFPS_needs to be a little higher than actual preferred rate
        // because of window sync
        float fFps = 60; // NOT WORKING NULLPTR pImpl->targetFPS_;
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

    if ((!pImpl) || (hWnd != pImpl->_windowHandle))
    {
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    NVGcontext* nvg = pImpl->_nvg;
    ml::AppView* pView = pImpl->_appView;

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
    /*
    case WM_DPICHANGED:
    case WM_DPICHANGED_AFTERPARENT:

    case WM_GETDPISCALEDSIZE:
    {
        UINT dpi = HIWORD(wParam);
        std::cout << "DPI CHANGED: " << dpi << "\n";
        pImpl->updateDpiScale();

        // Force repaint
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;
    }*/
    case WM_ERASEBKGND:
    {
        return 0;
    }
    case WM_PAINT:
    {
        pImpl->resizeIfNeeded();
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        if (!pImpl->makeContextCurrent()) return 0;

        if (kGLTestPatternOnly)
        {
            pImpl->paintTestPattern(hWnd);
        }
        else
        {
            pImpl->paintView(hWnd);
        }

        pImpl->swapBuffers();
        EndPaint(hWnd, &ps);

        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        SetFocus(hWnd);
        SetCapture(hWnd);

        GUIEvent e{ "down" };
        pImpl->convertEventPositions(wParam, lParam, &e);
        pImpl->convertEventFlags(wParam, lParam, &e);
        pImpl->_totalDrag = Vec2{ 0, 0 };

        if (pImpl->_appView)
        {
            pImpl->_appView->pushEvent(e);
        }

        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        SetFocus(hWnd);
        SetCapture(hWnd);

        GUIEvent e{ "down" };
        pImpl->convertEventPositions(wParam, lParam, &e);
        pImpl->convertEventFlags(wParam, lParam, &e);
        e.keyFlags |= controlModifier;

        pImpl->_totalDrag = Vec2{ 0, 0 };

        if (pImpl->_appView)
        {
            pImpl->_appView->pushEvent(e);
        }

        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (GetCapture())
        {
            GUIEvent e{ "drag" };
            pImpl->convertEventPositions(wParam, lParam, &e);
            pImpl->convertEventFlags(wParam, lParam, &e);

            bool repositioned{ false };

            // TEMP screen edge fix commented out for now. TODO fix
            /*
            // get screen height 
            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &info);
            int screenMaxY = info.rcMonitor.bottom - info.rcMonitor.top;

            Vec2 pv = pImpl->eventPositionOnScreen(lParam);
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
              pImpl->setMousePosition(moveToPos);
              pImpl->_totalDrag -= moveDelta;
            }
            else
             */
            {
                //e.position += pImpl->_totalDrag;
                if (pImpl->_appView)
                {
                    pImpl->_appView->pushEvent(e);
                }
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        ReleaseCapture();
        GUIEvent e{ "up" };

        pImpl->convertEventPositions(wParam, lParam, &e);
        pImpl->convertEventFlags(wParam, lParam, &e);
        if (pImpl->_appView)
        {
            pImpl->_appView->pushEvent(e);
        }

        return 0;
    }

    case WM_RBUTTONUP:
    {
        ReleaseCapture();
        GUIEvent e{ "up" };

        pImpl->convertEventPositions(wParam, lParam, &e);
        pImpl->convertEventFlags(wParam, lParam, &e);
        e.keyFlags |= controlModifier;
        if (pImpl->_appView)
        {
            pImpl->_appView->pushEvent(e);
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
        pImpl->convertEventPositionsFromScreen(wParam, lParam, &e);
        pImpl->convertEventFlags(wParam, lParam, &e);

        float d = float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        e.delta = Vec2{ 0, d * kScrollSensitivity };

        if (pImpl->_appView)
        {
            pImpl->_appView->pushEvent(e);
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
