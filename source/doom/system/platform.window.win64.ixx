module;

#include "windows.h"

#include <GL/glew.h>
#include <GL/wglew.h>
#include <gl/gl.h>

#include <cassert>

export module platform.window.win64;

import std;
import nstd;

import log;
import input;
import platform.window;
import platform.win64;

namespace {

input::event_id TranslateKey(WORD vk_code, uint32 scan_code = 0, bool is_extended = false)
{
    if (vk_code >= 'A' && vk_code <= 'Z')
        return input::event_id("A") + (vk_code - 'A');

    if (vk_code >= '0' && vk_code <= '9')
        return input::event_id{"0"} + (vk_code - '0');

    switch (vk_code)
    {
    case VK_BACK:       return {"Backspace"};
    case VK_TAB:		return {"Tab"};
    case VK_ESCAPE:		return {"Escape"};
    case VK_SPACE:		return {"Space"};

    case VK_PRIOR:      return {"PageUp"};
    case VK_NEXT:       return {"PageDown"};
    case VK_HOME:       return {"Home"};
    case VK_END:        return {"End"};
    case VK_INSERT:     return {"Insert"};
    case VK_DELETE:     return {"Delete"};

    case VK_UP:			return {"UpArrow"};
    case VK_LEFT:		return {"LeftArrow"};
    case VK_RIGHT:		return {"RightArrow"};
    case VK_DOWN:		return {"DownArrow"};

    case VK_RETURN:		return {is_extended ? "NumpadEnter" : "Enter"};
    
    case VK_SELECT:     return {"Select"};
    case VK_PRINT:      return {"Print"};
    case VK_EXECUTE:    return {"Execute"};
    case VK_SNAPSHOT:   return {"PrintScreen"};
    case VK_PAUSE:      return {"Pause"};
    case VK_CANCEL:     return {"Break"};
    case VK_HELP:       return {"Help"};
    case VK_SLEEP:      return {"Sleep"};
    case VK_CLEAR:      return {"Clear"};

    case VK_OEM_PLUS:   return {"Plus"};
    case VK_OEM_COMMA:	return {"Comma"};
    case VK_OEM_MINUS:  return {"Minus"};
    case VK_OEM_PERIOD:	return {"Period"};
    case VK_OEM_1:		return {"Semicolon"};
    case VK_OEM_2:		return {"Slash"};
    case VK_OEM_3:		return {"Tilde"};
    case VK_OEM_4:		return {"LeftBracket"};
    case VK_OEM_5:		return {"BackSlash"};
    case VK_OEM_6:		return {"RightBracket"};
    case VK_OEM_7:		return {"Quote"};
    case VK_OEM_8:		return {"OEM8"};

    case VK_NUMPAD0:    return {"Numpad0"};
    case VK_NUMPAD1:    return {"Numpad1"};
    case VK_NUMPAD2:    return {"Numpad2"};
    case VK_NUMPAD3:    return {"Numpad3"};
    case VK_NUMPAD4:    return {"Numpad4"};
    case VK_NUMPAD5:    return {"Numpad5"};
    case VK_NUMPAD6:    return {"Numpad6"};
    case VK_NUMPAD7:    return {"Numpad7"};
    case VK_NUMPAD8:    return {"Numpad8"};
    case VK_NUMPAD9:    return {"Numpad9"};
    case VK_MULTIPLY:   return {"Multiply"};
    case VK_ADD:        return {"Add"};
    case VK_SEPARATOR:  return {"Separator"};
    case VK_SUBTRACT:   return {"Subtract"};
    case VK_DECIMAL:    return {"Decimal"};
    case VK_DIVIDE:     return {"Divide"};

    case VK_F1:			return {"F1"};
    case VK_F2:			return {"F2"};
    case VK_F3:			return {"F3"};
    case VK_F4:			return {"F4"};
    case VK_F5:			return {"F5"};
    case VK_F6:			return {"F6"};
    case VK_F7:			return {"F7"};
    case VK_F8:			return {"F8"};
    case VK_F9:			return {"F9"};
    case VK_F10:		return {"F10"};
    case VK_F11:		return {"F11"};
    case VK_F12:		return {"F12"};
    case VK_F13:		return {"F13"};
    case VK_F14:		return {"F14"};
    case VK_F15:		return {"F15"};
    case VK_F16:		return {"F16"};
    case VK_F17:		return {"F17"};
    case VK_F18:		return {"F18"};
    case VK_F19:		return {"F19"};
    case VK_F20:		return {"F20"};
    case VK_F21:		return {"F21"};
    case VK_F22:		return {"F22"};
    case VK_F23:		return {"F23"};
    case VK_F24:		return {"F24"};

    case VK_CAPITAL:    return {"CapsLock"};
    case VK_NUMLOCK:    return {"NumLock"};
    case VK_SCROLL:     return {"ScrollLock"};

    case VK_SHIFT:
        switch (MapVirtualKey(scan_code, MAPVK_VSC_TO_VK_EX))
        {
        case 0:         return {"Shift"};
        case VK_LSHIFT: return {"LeftShift"};
        case VK_RSHIFT: return {"RightShift"};
        default:        return input::event_id::Invalid;
        }
    case VK_LSHIFT:		return {"LeftShift"};
    case VK_RSHIFT:		return {"RightShift"};

    case VK_CONTROL:
        switch (MapVirtualKey(scan_code, MAPVK_VSC_TO_VK_EX))
        {
        case 0:             return {"Ctrl"};
        case VK_LCONTROL:   return {"LeftCtrl"};
        case VK_RCONTROL:   return {"RightCtrl"};
        default:            return input::event_id::Invalid;
        }
    case VK_LCONTROL:   return {"LeftCtrl"};
    case VK_RCONTROL:   return {"RightCtrl"};

    case VK_MENU:
        switch (MapVirtualKey(scan_code, MAPVK_VSC_TO_VK_EX))
        {
        case 0:         return {"Alt"};
        case VK_LMENU:  return {"LeftAlt"};
        case VK_RMENU:  return {"RightAlt"};
        default:        return input::event_id::Invalid;
        }
    case VK_LMENU:      return {"LeftAlt"};
    case VK_RMENU:      return {"RightAlt"};

    case VK_LWIN:       return {"LeftWin"};
    case VK_RWIN:       return {"RightWin"};

    case VK_APPS:                   return {"Apps"};
    case VK_LAUNCH_APP1:            return {"LaunchApp1"};
    case VK_LAUNCH_APP2:            return {"LaunchApp2"};
    case VK_LAUNCH_MAIL:            return {"LaunchMail"};
    case VK_LAUNCH_MEDIA_SELECT:    return {"LaunchMedia"};

    case VK_BROWSER_BACK:       return {"BrowserBack"};
    case VK_BROWSER_FORWARD:    return {"BrowserForward"};
    case VK_BROWSER_REFRESH:    return {"BrowserRefresh"};
    case VK_BROWSER_STOP:       return {"BrowserStop"};
    case VK_BROWSER_SEARCH:     return {"BrowserSearch"};
    case VK_BROWSER_FAVORITES:  return {"BrowserFavorites"};
    case VK_BROWSER_HOME:       return {"BrowserHome"};
    
    case VK_VOLUME_MUTE:        return {"VolumeMute"};
    case VK_VOLUME_UP:          return {"VolumeUp"};
    case VK_VOLUME_DOWN:        return {"VolumeDown"};

    case VK_MEDIA_NEXT_TRACK:   return {"MediaNext"};
    case VK_MEDIA_PREV_TRACK:   return {"MediaPrev"};
    case VK_MEDIA_STOP:         return {"MediaStop"};
    case VK_MEDIA_PLAY_PAUSE:   return {"MediaPlay"};

    default:
        return input::event_id::Invalid;
    }
}

std::map<wstring, ATOM> WindowClassDefinitions;

} // namespace

namespace platform {

export struct SystemEvent
{
    HWND handle = nullptr;
    UINT message = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;

    SystemEvent(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
        : handle{handle}
        , message{message}
        , wParam{wParam}
        , lParam{lParam}
    {}
};

export class Window
{
public:
    static bool RegisterClass(WindowClassDefinition& definition);

    Window() = default;
    Window(const WindowDefinition& definition)
        : definition{definition}
    {}

    bool IsValid() const noexcept { return is_valid; }

    void Show() { ShowWindow(handle, SW_SHOWNORMAL); }
    void SwapBuffers() { ::SwapBuffers(device_context); }

private:
    bool is_valid = false;
    bool hasFocus = true;
    WindowDefinition definition;
    HWND handle = NULL;
    HDC device_context = NULL;
    HGLRC render_context = NULL;

    LRESULT HandleSystemEvent(const SystemEvent& event);

    static LRESULT CALLBACK Proc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto* window = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));
        return window->HandleSystemEvent({handle, msg, wParam, lParam});
    }
    static LRESULT CALLBACK InitProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);

    friend Window* CreateWindow(const WindowDefinition& definition);
};

LRESULT CALLBACK Window::InitProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg != WM_NCCREATE)
        return DefWindowProc(handle, msg, wParam, lParam);

    auto create = reinterpret_cast<LPCREATESTRUCT>(lParam);

    // Assign the pointer to the Window class that owns this window to the "user data" of this window
    SetLastError(0);
    auto result = SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    if (auto error_code = GetLastError(); result == 0 && error_code != 0)
    {
        log::error(error::get_message());
        return FALSE;
    }

    // Replace this window proc with the simpler version which passes the event to the Window class
    SetLastError(0);
    result = SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Window::Proc));
    if (auto error_code = GetLastError(); result == 0 && error_code != 0)
    {
        log::error(error::get_message());
        return FALSE;
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

LRESULT Window::HandleSystemEvent(const SystemEvent& event)
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

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(event.handle, &ps);
        EndPaint(event.handle, &ps);
        return 0;
    }

    case WM_SIZING:
    {
        RECT actual = {};
        /*GetClientRect(windowHandle, &actual);
        windowWidth = actual.right - actual.left;
        windowHeight = actual.bottom - actual.top;*/
        //display->SetDimensions(width, height);
        return TRUE;
    }

    case WM_SIZE:
        {
        RECT actual = {};
        /*GetClientRect(windowHandle, &actual);
        windowWidth = actual.right - actual.left;
        windowHeight = actual.bottom - actual.top;*/
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
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        auto vk_code = LOWORD(event.wParam);
        if (vk_code == VK_SHIFT)
            return 0;

        auto key_flags = HIWORD(event.lParam);
        auto count = LOWORD(event.lParam);
        
        auto scan_code = LOBYTE(key_flags);
        auto is_extended = (key_flags & KF_EXTENDED) == KF_EXTENDED;
        auto alt_down = (key_flags & KF_ALTDOWN) == KF_ALTDOWN;
        auto is_repeat = (key_flags & KF_REPEAT) == KF_REPEAT;
        auto is_up = (key_flags & KF_UP) == KF_UP;

        auto key = TranslateKey(vk_code, scan_code, is_extended);

        input::event_flags flags{is_up ? "up" : "down"};
        flags.set<"alt">(alt_down);
        flags.set<"repeated">(!is_up && is_repeat);
        flags.set<"changed">(is_up || !is_repeat);

        /*
        auto nstd_flag_names = flags.names();
        std::vector<string> flag_names(nstd_flag_names.begin(), nstd_flag_names.end());
        std::cout << std::format("vk: {} scan code: {} count: {} extended: {} alt: {} repeat: {} down: {} name: {} [{}]\n",
            vk_code, scan_code, count, is_extended, alt_down, is_repeat, !is_up, key.name(),
            std::ranges::fold_left(flag_names | std::ranges::views::filter([](string& s){ return !s.empty(); }) | std::ranges::views::join_with(string{"|"}), string(), std::plus())
            );
        /**/

        static const auto Keyboard = input::device_id{"Keyboard"};
        for (int32 i = 0; i < count; ++i)
        {
            input::event input_event = {
                .device = Keyboard,
                .id = key,
                .flags = flags,
                .value = 1
            };

            //doom->PostEvent({ev_keydown, key});
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

export Window* CreateWindow(const WindowDefinition& definition)
{
    auto* window = new Window{definition};

    DWORD windowStyleEx = 0;
    DWORD windowStyle = 0;

    windowStyle |= definition.withBorder ? WS_OVERLAPPEDWINDOW : WS_POPUP;
    windowStyle |= WS_CAPTION;

    if (definition.isFullScreen)
        windowStyle |= WS_MAXIMIZE;

    if (definition.isResizeable)
        windowStyle |= WS_THICKFRAME;
    else
        windowStyle &= ~WS_THICKFRAME;

    auto dpi = GetDpiForSystem();
    auto screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, dpi);
    auto screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, dpi);
    //SystemParametersInfoForDpi(SPI_GETWORKAREA, 
    auto windowWidth = std::min(screenWidth, definition.width);
    auto windowHeight = std::min(screenHeight, definition.height);

    if (definition.isFullScreen)
    {
        RECT rec_non_client = {};
        AdjustWindowRectExForDpi(&rec_non_client, windowStyle, FALSE, 0, dpi);
        windowWidth = screenWidth - (rec_non_client.right - rec_non_client.left);
        windowHeight = screenHeight - (rec_non_client.bottom - rec_non_client.top);
    }

    RECT rec = {
        .left = 0,
        .top = 0,
        .right = windowWidth,
        .bottom = windowHeight,
    };
    AdjustWindowRectExForDpi(&rec, windowStyle, FALSE, 0, dpi);

    // Create the window
    window->handle = CreateWindowExW(
        windowStyleEx,
        MAKEINTATOM(definition.class_id),
        definition.title.c_str(),
        windowStyle,
        definition.isFullScreen ? 0 : CW_USEDEFAULT,
        definition.isFullScreen ? 0 : CW_USEDEFAULT,
        rec.right - rec.left,
        rec.bottom - rec.top,
        NULL,
        NULL,
        0,
        window);

    if (window->handle == NULL)
    {
        log::error(error::get_message());
        delete window;
        return nullptr;
    }

    std::cout << "Created window " << windowWidth << "x" << windowHeight << "\n";

    RECT actual = {};
    GetClientRect(window->handle, &actual);
    windowWidth = actual.right - actual.left;
    windowHeight = actual.bottom - actual.top;

    // Get a device context for the window
    window->device_context = GetDC(window->handle);

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

    auto pixel_format = ChoosePixelFormat(window->device_context, &pfd);
    SetPixelFormat(window->device_context, pixel_format, &pfd);

    auto dummyContext = wglCreateContext(window->device_context);
    wglMakeCurrent(window->device_context, dummyContext);

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

    window->render_context = wglCreateContextAttribsARB(window->device_context, nullptr, attributes);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyContext);

    wglMakeCurrent(window->device_context, window->render_context);

    window->is_valid = true;
    return window;
}

bool Window::RegisterClass(WindowClassDefinition& definition)
{
    auto signature = definition.get_signature();
    auto it = WindowClassDefinitions.find(signature);
    if (it != WindowClassDefinitions.end())
        return true;

    auto window_class = WNDCLASSEXW{
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | (definition.processDoubleClicks ? CS_DBLCLKS : 0u) | (definition.disableClose ? CS_NOCLOSE : 0u),
        .lpfnWndProc = Window::InitProc,
        .cbClsExtra = 0,
        .cbWndExtra = sizeof(Window*),
        .hInstance = GetModuleHandleW(NULL),
        .hIcon = NULL,
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .hbrBackground = NULL,
        .lpszMenuName = NULL,
        .lpszClassName = signature.c_str(),
        .hIconSm = NULL,
    };

    auto result = RegisterClassExW(&window_class);
    if (result == 0)
    {
        log::error(error::get_message());
        return false;
    }

    definition.id = result;
    WindowClassDefinitions.insert({signature, result});

    return true;
}

} // namespace platform
