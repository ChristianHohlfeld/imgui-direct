#include "app.hpp"
#include "ui/theme.hpp"
// Dear ImGui: standalone example application for Windows API + OpenGL

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// This is provided for completeness, however it is strongly recommended you use OpenGL with SDL or GLFW.

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>
#include <tchar.h>
#include <stdio.h>
#include <windowsx.h>

// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;          // Rendering context
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;
static bool             g_Fullscreen = false;
static LONG_PTR         g_WindowedStyle = 0;
static WINDOWPLACEMENT  g_WindowedPlacement = { sizeof(WINDOWPLACEMENT) };

// Native borderless fullscreen: reuse the existing HWND/WGL context without
// changing display modes. Preserve placement (including maximized state) and
// target the monitor containing the current window.
static void ToggleFullscreen(HWND hwnd)
{
    if (!g_Fullscreen)
    {
        WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
        if (!::GetWindowPlacement(hwnd, &placement))
            return;
        MONITORINFO monitor = { sizeof(MONITORINFO) };
        if (!::GetMonitorInfoW(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
            return;

        g_WindowedPlacement = placement;
        g_WindowedStyle = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
        ::SetWindowLongPtrW(hwnd, GWL_STYLE,
            (g_WindowedStyle & ~static_cast<LONG_PTR>(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
        const RECT& r = monitor.rcMonitor;
        ::SetWindowPos(hwnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top,
                       SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_Fullscreen = true;
    }
    else
    {
        ::SetWindowLongPtrW(hwnd, GWL_STYLE, g_WindowedStyle);
        ::SetWindowPlacement(hwnd, &g_WindowedPlacement);
        ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                       SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_Fullscreen = false;
    }
}

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int nCmdShow)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Direct", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ImGui Direct", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize OpenGL
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // The renderer receives a correct client size from the first frame onward.
    RECT client = {};
    ::GetClientRect(hwnd, &client);
    g_Width = client.right - client.left;
    g_Height = client.bottom - client.top;

    // Show the window
    ::ShowWindow(hwnd, nCmdShow);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    app::setup();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

    // UI styling and scale are owned exclusively by src/ui/theme.cpp.
    // Physical desktop viewport coordinates need DPI-aware touch targets/fonts.
    ui::set_scale(main_scale);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;
        if (::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        app::draw();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, g_Width, g_Height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present
        ::SwapBuffers(g_MainWindow.hDC);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceWGL(hwnd, &g_MainWindow);
    wglDeleteContext(g_hRC);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    HDC hDc = ::GetDC(hWnd);
    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    const int pf = ::ChoosePixelFormat(hDc, &pfd);
    if (pf == 0)
        return false;
    if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
        return false;
    ::ReleaseDC(hWnd, hDc);

    data->hDC = ::GetDC(hWnd);
    if (g_hRC)
        return true;

    // Create legacy context
    HGLRC tempRC = wglCreateContext(data->hDC);
    if (!tempRC) { ::ReleaseDC(hWnd, data->hDC); return false; }
    if (!wglMakeCurrent(data->hDC, tempRC))
    {
        wglDeleteContext(tempRC);
        ::ReleaseDC(hWnd, data->hDC);
        return false;
    }

    // Get context version
    GLint major = 0, minor = 0;
    glGetIntegerv(0x821B, &major); // GL_MAJOR_VERSION
    glGetIntegerv(0x821C, &minor); // GL_MINOR_VERSION
    const char* gl_version_str = (const char*)glGetString(GL_VERSION);
    if (major == 0 && minor == 0)
        sscanf_s(gl_version_str, "%d.%d", &major, &minor); // Query GL_VERSION in desktop GL 2.x, the string will start with "<major>.<minor>"
    const GLuint gl_version = (GLuint)(major * 100 + minor * 10);

    // Keep temporary context: already OpenGL 3.0+.
    g_hRC = tempRC;
    if (gl_version >= 300)
        return true;

    typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
    const PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

    // GL 3.0
    const int attribs[] =
    {
        0x2091, 3,      // WGL_CONTEXT_MAJOR_VERSION_ARB
        0x2092, 0,      // WGL_CONTEXT_MINOR_VERSION_ARB
        0x9126, 0x0001, // WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB
        0
    };
    HGLRC newRC = nullptr;
    if (wglCreateContextAttribsARB)
        newRC = wglCreateContextAttribsARB(data->hDC, 0, attribs);

    // If we managed to create 3.0+ context: use that one and destroy the temporary OpenGL 2.x compatibility context.
    if (newRC)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempRC);
        g_hRC = newRC;
        wglMakeCurrent(data->hDC, g_hRC);
    }

    return true;
}

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
{
    wglMakeCurrent(nullptr, nullptr);
    ::ReleaseDC(hWnd, data->hDC);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Alt+Enter is a window-manager shortcut, never a widget action.
    // Bit 29: Alt is held; bit 30: key auto-repeat. Ignore repeats.
    if (msg == WM_SYSKEYDOWN && wParam == VK_RETURN && (lParam & (1L << 29)))
    {
        if (!(lParam & (1L << 30)))
            ToggleFullscreen(hWnd);
        return 0;
    }
    if (msg == WM_SYSKEYUP && wParam == VK_RETURN && (lParam & (1L << 29)))
        return 0;

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_Width = LOWORD(lParam);
            g_Height = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
