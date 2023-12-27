//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	DOOM graphics stuff for Windows
//
//-----------------------------------------------------------------------------
import std;
import nstd;

import config;
import platform;

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "d_main.h"
#include "m_bbox.h"
#include "st_stuff.h"
#include "doomdef.h"
#include "z_zone.h"

#include <GL/wglew.h>
#include <gl/gl.h>


template<typename T>
constexpr const GLenum gl_type = []{ static_assert(false, "type does not have a corresponding GL type"); return 0; }();
template<> constexpr const GLenum gl_type<int8> = GL_BYTE;
template<> constexpr const GLenum gl_type<uint8> = GL_UNSIGNED_BYTE;
template<> constexpr const GLenum gl_type<int16> = GL_SHORT;
template<> constexpr const GLenum gl_type<uint16> = GL_UNSIGNED_SHORT;
template<> constexpr const GLenum gl_type<int32> = GL_INT;
template<> constexpr const GLenum gl_type<uint32> = GL_UNSIGNED_INT;
template<> constexpr const GLenum gl_type<float> = GL_FLOAT;
template<> constexpr const GLenum gl_type<double> = GL_DOUBLE;

template<typename ...Ts>
class gl_vertex_attributes
{
public:
    static constexpr const GLsizei size = []{
        if constexpr (sizeof...(Ts) == 0) return 0;
        else return static_cast<GLsizei>((sizeof(Ts)+...));
    }();

    static void add()
    {
        GLuint index = 0;
        intptr_t offset = 0;

        _add<Ts...>(index, offset);
    }

private:
    template<typename V, size_t R = 0>
    static constexpr GLint count()
    {
        if constexpr (!std::is_array_v<V>)
            return 1;
        else if constexpr (R < std::rank_v<V> - 1)
            return std::extent_v<V, R> * count<V, R + 1>();
        else
            return std::extent_v<V, R>;
    }

    template<typename T, typename ...Ts>
    static void _add(GLuint& index, intptr_t& offset)
    {
        glEnableVertexAttribArray(index);
        if constexpr (nstd::is_integral<T>)
            glVertexAttribIPointer(index, count<T>(), gl_type<std::remove_all_extents_t<T>>, size, reinterpret_cast<GLvoid*>(offset));
        else
            glVertexAttribPointer(index, count<T>(), gl_type<std::remove_all_extents_t<T>>, GL_FALSE, size, reinterpret_cast<GLvoid*>(offset));

        offset += sizeof(T);
        index += 1;
        if constexpr (sizeof...(Ts) != 0)
            _add<Ts...>(index, offset);
    }
};

//  Translates the key currently in X_event
int32 TranslateKey(WPARAM id, uint32 scanCode = 0, bool isExtended = false)
{
    if (id >= 'A' && id <= 'Z')
        return static_cast<int32>(id) - 'A' + 'a';

    if (id >= '0' && id <= '9')
        return static_cast<int32>(id);

    switch (id)
    {
    case VK_LEFT:		return KEY_LEFTARROW;
    case VK_UP:			return KEY_UPARROW;
    case VK_RIGHT:		return KEY_RIGHTARROW;
    case VK_DOWN:		return KEY_DOWNARROW;
    case VK_ESCAPE:		return KEY_ESCAPE;
    case VK_RETURN:		return isExtended ? KEY_ENTER /*(numpad)*/ : KEY_ENTER;
    case VK_TAB:		return KEY_TAB;
    case VK_F1:			return KEY_F1;
    case VK_F2:			return KEY_F2;
    case VK_F3:			return KEY_F3;
    case VK_F4:			return KEY_F4;
    case VK_F5:			return KEY_F5;
    case VK_F6:			return KEY_F6;
    case VK_F7:			return KEY_F7;
    case VK_F8:			return KEY_F8;
    case VK_F9:			return KEY_F9;
    case VK_F10:		return KEY_F10;
    case VK_F11:		return KEY_F11;
    case VK_F12:		return KEY_F12;

    case VK_BACK:       return KEY_BACKSPACE;
    case VK_PAUSE:      return KEY_PAUSE;

    case VK_OEM_PLUS:   return KEY_EQUALS;
    case VK_OEM_MINUS:  return KEY_MINUS;

    //case VK_LSHIFT:   return KEY_RSHIFT;
    case VK_RSHIFT:     return KEY_RSHIFT;
    case VK_SHIFT:      return (scanCode == 0) ? KEY_RSHIFT /*(either)*/ : (scanCode == 0x2a) ? KEY_RSHIFT /*(left)*/ : KEY_RSHIFT /*(right)*/;

    //case VK_LCONTROL: return KEY_RCTRL;
    case VK_RCONTROL:   return KEY_RCTRL;
    case VK_CONTROL:    return (scanCode == 0) ? KEY_RCTRL /*(either)*/ : isExtended ? KEY_RCTRL /*(right)*/ : KEY_RCTRL /*(left)*/;

    case VK_LMENU:      return KEY_LALT;
    case VK_RMENU:      return KEY_RALT;
    case VK_MENU:       return (scanCode == 0) ? KEY_RALT /*(either)*/ : isExtended ? KEY_RALT /*(right)*/ : KEY_LALT /*(left)*/;

    case VK_SPACE:		return ' ';
    case VK_OEM_PERIOD:	return '.';
    case VK_OEM_COMMA:	return ',';
    case VK_OEM_1:		return ';';
    case VK_OEM_2:		return '/';
    case VK_OEM_3:		return '~';
    case VK_OEM_4:		return '[';
    case VK_OEM_5:		return '\\';
    case VK_OEM_6:		return ']';
    case VK_OEM_7:		return '\'';
    //case VK_OEM_8:		return '';

    default:
        return 0;
    }
}

void I_ShutdownGraphics()
{
}

static int	lastmousex = 0;
static int	lastmousey = 0;
bool mousemoved = false;

void Video::FinishUpdate()
{
    static time_t lasttic = 0;

    // draws little dots on the bottom of the screen
    if (doom->IsDevMode())
    {
        auto i = I_GetTime();
        auto tics = i - lasttic;
        lasttic = i;
        if (tics > 20) tics = 20;

        for (i = 0; i < tics * 2; i += 2)
            screenBuffer[((windowHeight/screenMultiply- 1)*screenTextureSize)  + i] = 0xff'ff'ff'ff;
        for (; i < 20 * 2; i += 2)
            screenBuffer[((windowHeight/screenMultiply- 1)*screenTextureSize)  + i] = 0xff'00'00'00;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);

    glUseProgram(screenShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    for (uint32 y = 0; y < windowHeight / screenMultiply; ++y)
    for (uint32 x = 0; x < windowWidth / screenMultiply; ++x)
    {
        screenBuffer[y * screenTextureSize + x] = palette[screens[0][y * SCREENWIDTH + x]];
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenTextureSize, screenTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);

    //glDrawBuffer(GL_COLOR_ATTACHMENT2);
    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    SwapBuffers(deviceContext);
}

void Video::SetPalette(const byte* inPalette)
{
    auto* p = inPalette;
    for (int32 n = 0; n < 256; ++n, p += 3)
    {
        palette[n] = 0xff'00'00'00 | (*(p + 2) << 16) | (*(p + 1) << 8) | (*(p + 0) << 0);
    }
}

// This is the default window proc that all windows use after being initialized. It
// simply looks up the associated Window class and then passes the event parameters
// along to the custom function.
LRESULT CALLBACK GlobalWindowProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* window = reinterpret_cast<Video*>(GetWindowLongPtr(handle, GWLP_USERDATA));
    return window->HandleSystemEvent({handle, msg, wParam, lParam});
}

// This is the default window proc that all windows use during initialization. It is
// meant to handle the WM_NCCREATE message, which comes prior to even WM_CREATE. This
// message carries a pointer to the Window class that this window belongs to, so it
// can be associated. It then changes over to the simpler window proc, above.
LRESULT CALLBACK GlobalInitWindowProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCT create(reinterpret_cast<LPCREATESTRUCT>(lParam));

        // Assign the pointer to the Window class that owns this window to the "user data" of this window
        SetLastError(0);
        auto result(SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams)));
        auto error_core = GetLastError();
        if (result == 0 && error_core != 0)
        {
            LPWSTR buffer(NULL);
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
            MessageBoxW(nullptr, buffer, L"Error in WM_NCCREATE", MB_OK);
            LocalFree(buffer);
            return FALSE;
        }

        // Replace this window proc with the simpler version which passes the event to the Window class
        SetLastError(0);
        result = SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GlobalWindowProc));
        error_core = GetLastError();
        if (result == 0 && error_core != 0)
        {
            LPWSTR buffer(NULL);
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
            MessageBoxW(nullptr, buffer, L"Error in WM_NCCREATE", MB_OK);
            LocalFree(buffer);
            return FALSE;
        }
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

void GLAPIENTRY Video::GLErrorCallback(
    [[maybe_unused]] GLenum source,
    [[maybe_unused]] GLenum type,
    [[maybe_unused]] GLuint id,
    [[maybe_unused]] GLenum severity,
    [[maybe_unused]] GLsizei length,
    [[maybe_unused]] const GLchar* message,
    [[maybe_unused]] const void* param)
{
    std::stringstream out;
    switch (type)
    {
    case GL_DEBUG_SOURCE_API:
        out << "[API] ";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        out << "[WINDOW SYSTEM] ";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        out << "[SHADER COMPILER] ";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        out << "[THIRD PARTY] ";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        out << "[APPLICATION] ";
        break;
    case GL_DEBUG_SOURCE_OTHER:
    default:
        out << "[???] ";
        break;
    }
    out << message;

    /*
    #define GL_DEBUG_TYPE_ERROR 0x824C
    #define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
    #define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
    #define GL_DEBUG_TYPE_PORTABILITY 0x824F
    #define GL_DEBUG_TYPE_PERFORMANCE 0x8250
    #define GL_DEBUG_TYPE_OTHER 0x8251
    #define GL_DEBUG_TYPE_MARKER 0x8268

    #define GL_DEBUG_SEVERITY_HIGH 0x9146
    #define GL_DEBUG_SEVERITY_MEDIUM 0x9147
    #define GL_DEBUG_SEVERITY_LOW 0x9148
    #define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
    */

    std::cerr << message << "\n";

    if ((type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
        && (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_LOW))
        DebugBreak();
}

void Video::Init()
{
    if (isInitialized)
        return;

    isInitialized = true;

    byte* base = new byte[SCREENWIDTH * SCREENHEIGHT * 4];
    for (int32 i = 0; i < 4; ++i)
        screens[i] = base + i * SCREENWIDTH * SCREENHEIGHT;
    screens[4] = (byte*)Z_Malloc(ST_WIDTH * ST_HEIGHT, PU_STATIC, 0);

    CommandLine::TryGetValues("-multiply", screenMultiply);

    windowWidth *= screenMultiply;
    windowHeight *= screenMultiply;

    isFullScreen = CommandLine::HasArg("-fullscreen");
    isExclusive = CommandLine::HasArg("-exclusive");
    isBorderless = CommandLine::HasArg("-borderless");
    isResizeable = CommandLine::HasArg("-resizeable");

    if (isFullScreen && isExclusive)
    {
        DEVMODE mode;
        ZeroMemory(&mode, sizeof(DEVMODE));
        mode.dmSize = sizeof(DEVMODE);
        mode.dmPelsWidth = windowWidth;
        mode.dmPelsHeight = windowHeight;
        mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

        DWORD display_flags = CDS_FULLSCREEN;
        ChangeDisplaySettings(&mode, display_flags);
    }

    if (!RegisterWindowClass())
        I_Error("Failed to register window class");


    // Figure out the dimensions of the window, factoring in style elements like
    // border and caption bar
    windowStyle = isBorderless ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    windowStyle |= WS_CAPTION;

    if (isFullScreen)
        windowStyle |= WS_MAXIMIZE;

    if (isResizeable)
        windowStyle |= WS_THICKFRAME;
    else
        windowStyle &= ~WS_THICKFRAME;

    auto dpi = GetDpiForSystem();
    screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, dpi);
    screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, dpi);
    //SystemParametersInfoForDpi(SPI_GETWORKAREA, 
    windowWidth = std::min(screenWidth, windowWidth);
    windowHeight = std::min(screenHeight, windowHeight);

    if (isFullScreen)
    {
        RECT rec_non_client = {};
        AdjustWindowRectExForDpi(&rec_non_client, windowStyle, FALSE, 0, dpi);
        windowWidth = screenWidth - (rec_non_client.right - rec_non_client.left);
        windowHeight = screenHeight - (rec_non_client.bottom - rec_non_client.top);
    }

    RECT rec;
    rec.left = 0;
    rec.right = windowWidth;
    rec.top = 0;
    rec.bottom = windowHeight;
    AdjustWindowRectExForDpi(&rec, windowStyle, FALSE, 0, dpi);

    // Create the window
    windowHandle = ::CreateWindowExW(
        windowStyleEx,
        WindowClassName,
        L"Doom",
        windowStyle,
        isFullScreen ? 0 : CW_USEDEFAULT,
        isFullScreen ? 0 : CW_USEDEFAULT,
        rec.right - rec.left,
        rec.bottom - rec.top,
        nullptr,
        nullptr,
        0,
        this);

    RECT actual = {};
    GetClientRect(windowHandle, &actual);
    windowWidth = actual.right - actual.left;
    windowHeight = actual.bottom - actual.top;

    std::cout << "Created window " << windowWidth << "x" << windowHeight << "\n";

    if (windowHandle == nullptr)
    {
        auto error_core = GetLastError();
        LPWSTR buffer = nullptr;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
        MessageBoxW(nullptr, buffer, L"Error in CreateWindowW", MB_OK);
        LocalFree(buffer);
        return;
    }

    // Get a device context for the window
    deviceContext = GetDC(windowHandle);

    // Set the pixel format
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = 0
        | PFD_DRAW_TO_WINDOW
        | PFD_SUPPORT_OPENGL
        | PFD_DOUBLEBUFFER
        | PFD_DEPTH_DONTCARE
        | PFD_TYPE_RGBA
        ;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 0;
    pfd.cAccumBits = 0;
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 8;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    auto pixel_format = ChoosePixelFormat(deviceContext, &pfd);
    SetPixelFormat(deviceContext, pixel_format, &pfd);

    auto dummyContext = wglCreateContext(deviceContext);
    wglMakeCurrent(deviceContext, dummyContext);

    glewInit();

    auto* glUnknown = "????";
    auto* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    auto* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    std::cout << "OpenGL version: " << (glVersion ? glVersion : glUnknown) << "\n";
    std::cout << "OpenGL vendor: " <<  (glVendor ? glVendor : glUnknown) << "\n";
    std::cout << "OpenGL renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << "\n";
    std::cout << "OpenGL shader version: " << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) << "\n";

    // Create the render context
    int attributes[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        //WGL_CONTEXT_LAYER_PLANE_ARB, 0,
        //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
        //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB | WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        0
    };

    renderContext = wglCreateContextAttribsARB(deviceContext, nullptr, attributes);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyContext);

    wglMakeCurrent(deviceContext, renderContext);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLErrorCallback, 0);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    glFrontFace(GL_CCW);

    glDisable(GL_MULTISAMPLE);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch (status)
    {
    case GL_FRAMEBUFFER_UNDEFINED:
        I_Error("Video::Init - framebuffer undefined");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        I_Error("Video::Init - framebuffer incomplete attachment\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        I_Error("Video::Init - framebuffer incomplete missing attachment\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        I_Error("Video::Init - framebuffer incomplete draw buffer\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        I_Error("Video::Init - framebuffer incomplete read buffer\n");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        I_Error("Video::Init - framebuffer unsupported\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        I_Error("Video::Init - framebuffer incomplete multisample\n");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        I_Error("Video::Init - framebuffer incomplete layer targets\n");
        break;
    default:
        I_Error(string("Video::Init - error: ") + std::to_string(status).c_str() + "\n");
        break;
    case GL_FRAMEBUFFER_COMPLETE:
        break;
    }

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    screenTextureSize = 512u; //std::min(std::bit_ceil(std::max(windowWidth, windowHeight)), 4096u);
    screenBuffer = new uint32[screenTextureSize * screenTextureSize];
    for (uint32 n = 0; n < screenTextureSize * screenTextureSize; ++n)
    {
        uint32 x = n % screenTextureSize;
        uint32 y = n / screenTextureSize;

        screenBuffer[n] =
            (y==(windowHeight/screenMultiply)-1) ? 0xff'00'ff'ff :
            (x==(windowWidth/screenMultiply)-1) ? 0xff'ff'00'ff :
            (y%20==0) ? 0xff'00'ff'00 :
            (x%20==0) ? 0xff'ff'00'00 :
            0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenTextureSize, screenTextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenBuffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    screenShader = LoadShader("screen");

    float s = 1.f - ((screenTextureSize - windowWidth / screenMultiply) / static_cast<float>(screenTextureSize));
    float t = 1.f - ((screenTextureSize - windowHeight / screenMultiply) / static_cast<float>(screenTextureSize));
    float screenVertices[] =
    {
        -1.f,  -1.f,  0.f, t,
        -1.f, 1.f,  0.f, 0.f,
        1.f, 1.f,  s, 0.f,

        -1.f,  -1.f,  0.f, t,
        1.f, 1.f,  s, 0.f,
        1.f,  -1.f,  s, t,
    };

    glGenVertexArrays(1, &screenVAO);
    glBindVertexArray(screenVAO);

    glGenBuffers(1, &screenVBO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

    gl_vertex_attributes<float[2], float[2]>::add();

    ShowWindow(windowHandle, SW_SHOWNORMAL);
}

void Video::StartFrame()
{
    glClearColor(0.f, 1.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Video::StartTick()
{
    DeliverSystemMessages();
}

LRESULT Video::HandleSystemEvent(const SystemEvent& event)
{
#if 0
    using Input::Button;

    auto get_modifier_key_flags = [w_param = event.wParam]() -> Input::Event::Flag
        {
            Input::Event::Flag out;
            if ((w_param & MK_CONTROL) != 0) out.set(Input::Event::Flag::Ctrl);
            if ((w_param & MK_SHIFT) != 0) out.set(Input::Event::Flag::Shift);
            if ((GetKeyState(VK_MENU) & 0x8000) != 0) out.set(Input::Event::Flag::Alt);
            if ((w_param & MK_LBUTTON) != 0) out.set(Input::Event::Flag::MouseLeft);
            if ((w_param & MK_MBUTTON) != 0) out.set(Input::Event::Flag::MouseMiddle);
            if ((w_param & MK_RBUTTON) != 0) out.set(Input::Event::Flag::MouseRight);
            if ((w_param & MK_XBUTTON1) != 0) out.set(Input::Event::Flag::MouseX1);
            if ((w_param & MK_XBUTTON2) != 0) out.set(Input::Event::Flag::MouseX2);
            return out;
        };

    auto cursor_pos = [this, l_param = event.lParam](bool absolute = false) -> v2i
        {
            if (!absolute)
                return {GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param)};

            auto client = GetClientArea();
            return {GET_X_LPARAM(l_param) - client.left(), GET_Y_LPARAM(l_param) - client.top()};
        };

    auto handle_mouse_wheel = [&, w_param = event.wParam](Input::Button key)
        {
            auto count = GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA;
            key = enum_offset(key, sign(count));
            count = std::abs(count);

            for (int i(0); i < count; ++i)
            {
                input->AddEvent(Input::Device::Mouse, key, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos(true));
            }
        };
#endif
    switch (event.message)
    {
    case WM_ACTIVATE:
        hasFocus = (LOWORD(event.wParam) != WA_INACTIVE);
        //input->OnFocusChanged(has_focus);
        break;

    case WM_SETFOCUS:
        //input->OnFocusChanged(true);
        break;
    case WM_KILLFOCUS:
        //input->OnFocusChanged(false);
        break;

    case WM_MOUSEACTIVATE:
        return MA_ACTIVATEANDEAT;

    case WM_CREATE:
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(event.handle, &ps);
        EndPaint(event.handle, &ps);
    }
    return 0;

    case WM_SIZING:
    {
        RECT actual = {};
        GetClientRect(windowHandle, &actual);
        windowWidth = actual.right - actual.left;
        windowHeight = actual.bottom - actual.top;
        //display->SetDimensions(width, height);
        return TRUE;
    }
    case WM_SIZE:
    {
        RECT actual = {};
        GetClientRect(windowHandle, &actual);
        windowWidth = actual.right - actual.left;
        windowHeight = actual.bottom - actual.top;
        //display->SetDimensions(width, height);
        return 0;
    }
    case WM_DROPFILES:
        break;

    case WM_DEVICECHANGE:
        std::cout << "WM_DEVICECHANGE\n"
            << "hwnd = " << std::to_string(reinterpret_cast<intptr_t>(event.handle)) << "\n"
            << "msg = " << std::to_string(event.message) << "\n"
            << "wparam = " << std::to_string(event.wParam) << "\n"
            << "lparam = " << std::to_string(event.lParam) << "\n";
        //input->ScanForDevices();
        break;

    case WM_SYSCOMMAND:
        switch (event.wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
        }	
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

#if 0
    case WM_MOUSEWHEEL:
        handle_mouse_wheel(Button::MouseWheel_V);
        return 0;

    case WM_MOUSEHWHEEL:
        handle_mouse_wheel(Button::MouseWheel_H);
        return 0;

    case WM_LBUTTONDOWN:
        input->AddEvent(Input::Device::Mouse, Button::MouseLeft, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_LBUTTONUP:
        input->AddEvent(Input::Device::Mouse, Button::MouseLeft, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_RBUTTONDOWN:
        input->AddEvent(Input::Device::Mouse, Button::MouseRight, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_RBUTTONUP:
        input->AddEvent(Input::Device::Mouse, Button::MouseRight, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_MBUTTONDOWN:
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddle, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_MBUTTONUP:
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddle, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_XBUTTONDOWN:
        if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON1)
            input->AddEvent(Input::Device::Mouse, Button::MouseX1, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        else if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON2)
            input->AddEvent(Input::Device::Mouse, Button::MouseX2, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return TRUE;
    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON1)
            input->AddEvent(Input::Device::Mouse, Button::MouseX1, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        else if (GET_XBUTTON_WPARAM(event.wParam) == XBUTTON2)
            input->AddEvent(Input::Device::Mouse, Button::MouseX2, Input::Event::Flag::Up | get_modifier_key_flags(), cursor_pos());
        return TRUE;

    case WM_LBUTTONDBLCLK:
        input->AddEvent(Input::Device::Mouse, Button::MouseLeftDbl, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        input->AddEvent(Input::Device::Mouse, Button::MouseLeft, Input::Event::Flag::DoubleClick | Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_RBUTTONDBLCLK:
        input->AddEvent(Input::Device::Mouse, Button::MouseRightDbl, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        input->AddEvent(Input::Device::Mouse, Button::MouseRight, Input::Event::Flag::DoubleClick | Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
    case WM_MBUTTONDBLCLK:
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddleDbl, Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        input->AddEvent(Input::Device::Mouse, Button::MouseMiddle, Input::Event::Flag::DoubleClick | Input::Event::Flag::Down | get_modifier_key_flags(), cursor_pos());
        return 0;
#endif
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if (event.wParam == VK_SHIFT)
            return 0;

        uint32 scanCode = (event.lParam >> 16) & 0xff;
        auto isExtended = nstd::is_bit_set<24>(event.lParam);
        auto key = TranslateKey(event.wParam, scanCode, isExtended);

        /*
        std::stringstream msg;
        msg << "WM_KEYDOWN " << event.message << " " << event.wParam << " " << event.lParam << " " << scan_code << " " << isExtended << " " << enum_string(key) << " " << TestBit(event.lParam, 30) << "\n";
        Host::DebugOutput(msg.str());
        /**/

        auto count = event.lParam & 0xffff;
        /*Input::Event::Flag flags = Input::Event::Flag::Down;
        auto alt_down = (HIWORD(event.lParam) & KF_ALTDOWN);
        if (alt_down)
            flags.set(Input::Event::Flag::Alt);
        if (is_bit_set<30>(event.lParam))
            flags.set(Input::Event::Flag::Repeated);
        else
            flags.set(Input::Event::Flag::Changed);*/

        for (int32 i = 0; i < count; ++i)
        {
            doom->PostEvent({ev_keydown, key});
        }
    }
    return 0;

#if 0
    event_t event;
    case ButtonPress:
        event.type = ev_mouse;
        event.data1 =
            (X_event.xbutton.state & Button1Mask)
            | (X_event.xbutton.state & Button2Mask ? 2 : 0)
            | (X_event.xbutton.state & Button3Mask ? 4 : 0)
            | (X_event.xbutton.button == Button1)
            | (X_event.xbutton.button == Button2 ? 2 : 0)
            | (X_event.xbutton.button == Button3 ? 4 : 0);
        event.data2 = event.data3 = 0;
        doom->PostEvent(&event);
        // fprintf(stderr, "b");
        break;
    case ButtonRelease:
        event.type = ev_mouse;
        event.data1 =
            (X_event.xbutton.state & Button1Mask)
            | (X_event.xbutton.state & Button2Mask ? 2 : 0)
            | (X_event.xbutton.state & Button3Mask ? 4 : 0);
        // suggest parentheses around arithmetic in operand of |
        event.data1 =
            event.data1
            ^ (X_event.xbutton.button == Button1 ? 1 : 0)
            ^ (X_event.xbutton.button == Button2 ? 2 : 0)
            ^ (X_event.xbutton.button == Button3 ? 4 : 0);
        event.data2 = event.data3 = 0;
        doom->PostEvent(&event);
        // fprintf(stderr, "bu");
        break;
    case MotionNotify:
        event.type = ev_mouse;
        event.data1 =
            (X_event.xmotion.state & Button1Mask)
            | (X_event.xmotion.state & Button2Mask ? 2 : 0)
            | (X_event.xmotion.state & Button3Mask ? 4 : 0);
        event.data2 = (X_event.xmotion.x - lastmousex) << 2;
        event.data3 = (lastmousey - X_event.xmotion.y) << 2;

        if (event.data2 || event.data3)
        {
            lastmousex = X_event.xmotion.x;
            lastmousey = X_event.xmotion.y;
            if (X_event.xmotion.x != X_width / 2 &&
                X_event.xmotion.y != X_height / 2)
            {
                doom->PostEvent(&event);
                // fprintf(stderr, "m");
                mousemoved = false;
            }
            else
            {
                mousemoved = true;
            }
        }
        break;
#endif

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        if (event.wParam == VK_SHIFT)
            return 0;

        uint32 scanCode = (event.lParam >> 16) & 0xff;
        auto isExtended = nstd::is_bit_set<24>(event.lParam);
        auto key = TranslateKey(event.wParam, scanCode, isExtended);

        /*
        std::stringstream msg;
        msg << "WM_KEYUP " << event.message << " " << event.wParam << " " << event.lParam << " " << scan_code << " " << extended << " " << enum_string(key) << "\n";
        Host::DebugOutput(msg.str());
        /**/

        //input->AddEvent(Input::Device::Keyboard, key, Input::Event::Flag::Up | Input::Event::Flag::Changed);
        doom->PostEvent({ev_keyup, key});
    }
    return 0;

#if 0
    case WM_UNICHAR:
    case WM_CHAR:
    case WM_SYSCHAR:
    {
        int count(event.lParam & 0xffff);
        for (int i(0); i < count; ++i)
        {
            input->AddCharacterToStream(Input::Device::Keyboard, wchar_t(event.wParam));
        }
    }
    return 0;
#endif
    }

    return DefWindowProc(event.handle, event.message, event.wParam, event.lParam);
}

void Video::DeliverSystemMessages()
{
    // TODO: This could be in a thread
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
    {
        // Check for a WM_QUIT message
        auto result(GetMessage(&msg, nullptr, 0, 0));
        if (result == -1)
            continue;

        if (result == 0)
            I_Quit();

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Masks a column based masked pic to the screen. 
void Video::DrawPatch(int32 x, int32 y, int32 screen, const patch_t* patch)
{
    y -= patch->topoffset;
    x -= patch->leftoffset;
#ifdef RANGECHECK 
    if (x<0
        || x + (patch->width) >SCREENWIDTH
        || y<0
        || y + (patch->height)>SCREENHEIGHT
        || screen > 4)
    {
        std::cerr << std::format("Patch at {},{} exceeds LFB\n", x, y);
        // No I_Error abort - what is up with TNT.WAD?
        std::cerr << "Video::DrawPatch: bad patch (ignored)\n";
        return;
    }
#endif 

    auto* desttop = screens[screen] + y * SCREENWIDTH + x;
    auto w = patch->width;
    for (int32 col = 0; col < w; x++, col++, desttop++)
    {
        auto* column = (column_t*)((byte*)patch + (patch->columnofs[col]));

        // step through the posts in a column 
        while (column->topdelta != 0xff)
        {
            auto* source = (byte*)column + 3;
            auto* dest = desttop + column->topdelta * SCREENWIDTH;
            auto count = column->length;

            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
            column = (column_t*)((byte*)column + column->length + 4);
        }
    }
}

byte* Video::CopyScreen(int32 dest) const
{
    assert(dest > 0 && dest < std::size(screens));
    memcpy(screens[dest], screens[0], SCREENWIDTH * SCREENHEIGHT);
    return screens[dest];
}

bool Video::RegisterWindowClass()
{
    // TODO: Build this from non OS specific data passed into the function, then check
    //   to see if a class with that "signature" already exists or not, skipping
    //   registration if it is unnecessary
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = GlobalInitWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(Video*);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = WindowClassName;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (::RegisterClassEx(&wc) == 0)
    {
        auto error_core = GetLastError();
        if (error_core != ERROR_CLASS_ALREADY_EXISTS)
        {
            LPWSTR buffer(NULL);
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
            MessageBoxW(nullptr, buffer, L"Error in RegisterClassEx", MB_OK);
            LocalFree(buffer);
            return false;
        }
    }

    return true;
}

GLuint Video::LoadShader(string_view name)
{
    std::cout << "Loading shader: " << name << "\n";

    auto program = glCreateProgram();

    string shaderName{name};

    std::filesystem::path path("data/shaders/");
    path /= shaderName + ".glsl";

    auto addShader = [&path, &shaderName, program](const string& ext, GLenum type, bool required)
    {
        std::ifstream file(path.replace_filename(shaderName + ext + ".glsl"), std::ios_base::binary | std::ios_base::in);
        string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (data.empty())
        {
            if (required)
                std::cerr << "Missing required shader: " << shaderName + ext << "\n";
            return;
        }

        auto shader = glCreateShader(type);
        auto* vdata = data.c_str();
        glShaderSource(shader, 1, &vdata, nullptr);
        glCompileShader(shader);
        GLint status = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            char buffer[4096] = {0};
            int32 length = 0;
            glGetShaderInfoLog(shader, sizeof(buffer), &length, buffer);
            std::cerr << "Error compiling shader " << shaderName + ext << ": " << buffer << "\n";
        }
        glAttachShader(program, shader);
    };

    // vertex
    addShader("_v", GL_VERTEX_SHADER, true);
    addShader("_g", GL_GEOMETRY_SHADER, false);
    addShader("_f", GL_FRAGMENT_SHADER, true);

    glLinkProgram(program);

    return program;
}
