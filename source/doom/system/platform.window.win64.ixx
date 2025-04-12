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
#if 0
#endif


namespace {

std::map<wstring, ATOM> WindowClassDefinitions;

const vector<UINT> InputDownEvents = { WM_KEYDOWN, WM_SYSKEYDOWN, WM_LBUTTONDOWN, WM_RBUTTONDOWN, WM_MBUTTONDOWN, WM_XBUTTONDOWN };
const vector<UINT> InputUpEvents = { WM_KEYUP, WM_SYSKEYUP, WM_LBUTTONUP, WM_RBUTTONUP, WM_MBUTTONUP, WM_XBUTTONUP };
const vector<UINT> InputCharEvents = { WM_CHAR, WM_SYSCHAR, WM_DEADCHAR, WM_SYSDEADCHAR, WM_UNICHAR };
const vector<UINT> InputDoubleClickEvents = { WM_LBUTTONDBLCLK, WM_RBUTTONDBLCLK, WM_MBUTTONDBLCLK, WM_XBUTTONDBLCLK };

std::map<UINT, input::event_id> InputMouseEvents = {
    { WM_LBUTTONDOWN, input::event_id_e::MouseLeft },
    { WM_LBUTTONUP, input::event_id_e::MouseLeft },
    { WM_LBUTTONDBLCLK, input::event_id_e::MouseLeft },
    { WM_MBUTTONDOWN, input::event_id_e::MouseMiddle },
    { WM_MBUTTONUP, input::event_id_e::MouseMiddle },
    { WM_MBUTTONDBLCLK, input::event_id_e::MouseMiddle },
    { WM_RBUTTONDOWN, input::event_id_e::MouseRight },
    { WM_RBUTTONUP, input::event_id_e::MouseRight },
    { WM_RBUTTONDBLCLK, input::event_id_e::MouseRight },
    { WM_XBUTTONDOWN, input::event_id_e::MouseX },
    { WM_XBUTTONUP, input::event_id_e::MouseX },
    { WM_XBUTTONDBLCLK, input::event_id_e::MouseX },
    { WM_MOUSEWHEEL, input::event_id_e::WheelVert },
    { WM_MOUSEHWHEEL, input::event_id_e::WheelHorz },
    { WM_MOUSEMOVE, input::event_id_e::MouseAbsolute },
};

std::map<UINT, input::event_id> InputKeyboardEvents = {
    { VK_BACK, input::event_id_e::Backspace }, { VK_TAB, input::event_id_e::Tab }, { VK_ESCAPE, input::event_id_e::Escape }, { VK_SPACE, input::event_id_e::Space },
    { VK_PRIOR, input::event_id_e::PageUp }, { VK_NEXT, input::event_id_e::PageDown }, { VK_HOME, input::event_id_e::Home }, { VK_END, input::event_id_e::End }, { VK_INSERT, input::event_id_e::Insert }, { VK_DELETE, input::event_id_e::Delete },
    { VK_UP, input::event_id_e::UpArrow }, { VK_LEFT, input::event_id_e::LeftArrow }, { VK_RIGHT, input::event_id_e::RightArrow }, { VK_DOWN, input::event_id_e::DownArrow },
    { VK_RETURN, input::event_id_e::Enter},
    { VK_SELECT, input::event_id_e::Select }, { VK_PRINT, input::event_id_e::Print }, { VK_EXECUTE, input::event_id_e::Execute }, { VK_SNAPSHOT, input::event_id_e::PrintScreen },
    { VK_PAUSE, input::event_id_e::Pause }, { VK_CANCEL, input::event_id_e::Break }, { VK_HELP, input::event_id_e::Help }, { VK_SLEEP, input::event_id_e::Sleep }, { VK_CLEAR, input::event_id_e::Clear },

    { VK_OEM_PLUS, input::event_id_e::Plus }, { VK_OEM_COMMA, input::event_id_e::Comma }, { VK_OEM_MINUS, input::event_id_e::Minus }, { VK_OEM_PERIOD, input::event_id_e::Period }, { VK_OEM_1, input::event_id_e::Semicolon }, { VK_OEM_2, input::event_id_e::Slash },
    { VK_OEM_3, input::event_id_e::Tilde }, { VK_OEM_4, input::event_id_e::LeftBracket }, { VK_OEM_5, input::event_id_e::BackSlash }, { VK_OEM_6, input::event_id_e::RightBracket }, { VK_OEM_7, input::event_id_e::Quote }, { VK_OEM_8, input::event_id_e::OEM8 },

    { VK_NUMPAD0, input::event_id_e::Numpad0 }, { VK_NUMPAD1, input::event_id_e::Numpad1 }, { VK_NUMPAD2, input::event_id_e::Numpad2 }, { VK_NUMPAD3, input::event_id_e::Numpad3 }, { VK_NUMPAD4, input::event_id_e::Numpad4 },
    { VK_NUMPAD5, input::event_id_e::Numpad5 }, { VK_NUMPAD6, input::event_id_e::Numpad6 }, { VK_NUMPAD7, input::event_id_e::Numpad7 }, { VK_NUMPAD8, input::event_id_e::Numpad8 }, { VK_NUMPAD9, input::event_id_e::Numpad9 },
    { VK_MULTIPLY, input::event_id_e::Multiply }, { VK_ADD, input::event_id_e::Add }, { VK_SEPARATOR, input::event_id_e::Separator }, { VK_SUBTRACT, input::event_id_e::Subtract }, { VK_DECIMAL, input::event_id_e::Decimal }, { VK_DIVIDE, input::event_id_e::Divide },

    { VK_F1, input::event_id_e::F1 }, { VK_F2, input::event_id_e::F2 }, { VK_F3, input::event_id_e::F3 }, { VK_F4, input::event_id_e::F4 }, { VK_F5, input::event_id_e::F5 }, { VK_F6, input::event_id_e::F6 }, { VK_F7, input::event_id_e::F7 }, { VK_F8, input::event_id_e::F8 }, { VK_F9, input::event_id_e::F9 }, { VK_F10, input::event_id_e::F10 }, { VK_F11, input::event_id_e::F11 }, { VK_F12, input::event_id_e::F12 },
    { VK_F13, input::event_id_e::F13 }, { VK_F14, input::event_id_e::F14 }, { VK_F15, input::event_id_e::F15 }, { VK_F16, input::event_id_e::F16 }, { VK_F17, input::event_id_e::F17 }, { VK_F18, input::event_id_e::F18 }, { VK_F19, input::event_id_e::F19 }, { VK_F20, input::event_id_e::F20 }, { VK_F21, input::event_id_e::F21 }, { VK_F22, input::event_id_e::F22 }, { VK_F23, input::event_id_e::F23 }, { VK_F24, input::event_id_e::F24 },

    { VK_CAPITAL, input::event_id_e::CapsLock }, { VK_NUMLOCK, input::event_id_e::NumLock }, { VK_SCROLL, input::event_id_e::ScrollLock },

    { VK_SHIFT, input::event_id_e::Shift }, { VK_LSHIFT, input::event_id_e::LeftShift }, { VK_RSHIFT, input::event_id_e::RightShift },
    { VK_CONTROL, input::event_id_e::Ctrl }, { VK_LCONTROL, input::event_id_e::LeftCtrl }, { VK_RCONTROL, input::event_id_e::RightCtrl },
    { VK_MENU, input::event_id_e::Alt }, { VK_LMENU, input::event_id_e::LeftAlt }, { VK_RMENU, input::event_id_e::RightAlt },
    { VK_LWIN, input::event_id_e::LeftWin }, { VK_RWIN, input::event_id_e::RightWin },

    { VK_APPS, input::event_id_e::Apps }, { VK_LAUNCH_APP1, input::event_id_e::LaunchApp1 }, { VK_LAUNCH_APP2, input::event_id_e::LaunchApp2 }, { VK_LAUNCH_MAIL, input::event_id_e::LaunchMail }, { VK_LAUNCH_MEDIA_SELECT, input::event_id_e::LaunchMedia },
    { VK_BROWSER_BACK, input::event_id_e::BrowserBack }, { VK_BROWSER_FORWARD, input::event_id_e::BrowserForward }, { VK_BROWSER_REFRESH, input::event_id_e::BrowserRefresh }, { VK_BROWSER_STOP, input::event_id_e::BrowserStop },
    { VK_BROWSER_SEARCH, input::event_id_e::BrowserSearch }, { VK_BROWSER_FAVORITES, input::event_id_e::BrowserFavorites }, { VK_BROWSER_HOME, input::event_id_e::BrowserHome },

    { VK_VOLUME_MUTE, input::event_id_e::VolumeMute }, { VK_VOLUME_UP, input::event_id_e::VolumeUp }, { VK_VOLUME_DOWN, input::event_id_e::VolumeDown },
    { VK_MEDIA_NEXT_TRACK, input::event_id_e::MediaNext }, { VK_MEDIA_PREV_TRACK, input::event_id_e::MediaPrev }, { VK_MEDIA_STOP, input::event_id_e::MediaStop }, { VK_MEDIA_PLAY_PAUSE, input::event_id_e::MediaPlay },
};

} // namespace

namespace platform {

export struct SystemEvent;

export class Window
{
public:
    static bool RegisterClass(WindowClassDefinition& definition);

    Window() = default;
    Window(const WindowDefinition& definition)
        : definition{definition}
    {}

    bool IsValid() const noexcept { return is_valid; }

    void Update();

    void Show() { ShowWindow(handle, SW_SHOWNORMAL); }
    void SwapBuffers() { ::SwapBuffers(device_context); }
    RECT GetClientArea() const;
    nstd::v2i GetCursorPos() const;

private:
    bool is_valid = false;
    bool hasFocus = true;
    WindowDefinition definition;
    BOOL has_menu = FALSE;
    DWORD style = 0;
    DWORD style_ex = 0;
    HWND handle = NULL;
    HDC device_context = NULL;
    HGLRC render_context = NULL;
    nstd::v2i old_cursor_pos;

    LRESULT HandleSystemEvent(SystemEvent event);
    void CreateInputEvent(const SystemEvent& event);

    static LRESULT CALLBACK Proc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK InitProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam);

    friend Window* CreateWindow(const WindowDefinition& definition);
};

export struct SystemEvent
{
    HWND handle = NULL;
    UINT message = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;

    bool is_keyboard_event() const noexcept { return message >= WM_KEYFIRST && message <= WM_KEYLAST; }
    bool is_char_event() const noexcept { return InputCharEvents.find(message) != InputCharEvents.end(); }
    bool is_mouse_event() const noexcept { return message >= WM_MOUSEFIRST && message <= WM_MOUSELAST; }
    bool is_down_event() const noexcept { return InputDownEvents.find(message) != InputDownEvents.end(); }
    bool is_up_event() const noexcept { return InputUpEvents.find(message) != InputUpEvents.end(); }
    bool is_double_click_event() const noexcept { return InputDoubleClickEvents.find(message) != InputDoubleClickEvents.end(); }
    bool is_wheel_event() const noexcept { return message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL; }

    input::event_flag get_input_flags() const noexcept
    {
        input::event_flag flags;
        flags.set<input::event_flag_f::down>(is_down_event());
        flags.set<input::event_flag_f::up>(is_up_event());
        flags.set<input::event_flag_f::caps_lock>(nstd::is_bit_set<0>(GetKeyState(VK_CAPITAL)));
        flags.set<input::event_flag_f::scroll_lock>(nstd::is_bit_set<0>(GetKeyState(VK_SCROLL)));
        flags.set<input::event_flag_f::num_lock>(nstd::is_bit_set<0>(GetKeyState(VK_NUMLOCK)));

        if (is_keyboard_event())
        {
            auto key_flags = HIWORD(lParam);
            auto is_repeat = nstd::bit_test_all(key_flags, KF_REPEAT);
            auto is_up = nstd::bit_test_all(key_flags, KF_UP);

            flags.set<input::event_flag_f::ctrl>(nstd::is_bit_set<7>(GetKeyState(VK_CONTROL)));
            flags.set<input::event_flag_f::shift>(nstd::is_bit_set<7>(GetKeyState(VK_SHIFT)));
            flags.set<input::event_flag_f::alt>(nstd::bit_test_all(key_flags, KF_ALTDOWN));
            flags.set<input::event_flag_f::repeated>(!is_up && is_repeat);
            flags.set<input::event_flag_f::changed>(is_up || !is_repeat);
        }
        else if (is_mouse_event())
        {
            flags.set<input::event_flag_f::ctrl>(nstd::bit_test_all(wParam, MK_CONTROL));
            flags.set<input::event_flag_f::shift>(nstd::bit_test_all(wParam, MK_SHIFT));
            flags.set<input::event_flag_f::alt>(nstd::is_bit_set<7>(GetKeyState(VK_MENU)));
            flags.set<input::event_flag_f::changed>();
            flags.set<input::event_flag_f::mouse_left>(nstd::bit_test_all(wParam, MK_LBUTTON));
            flags.set<input::event_flag_f::mouse_middle>(nstd::bit_test_all(wParam, MK_MBUTTON));
            flags.set<input::event_flag_f::mouse_right>(nstd::bit_test_all(wParam, MK_RBUTTON));
            flags.set<input::event_flag_f::mouse_x1>(nstd::bit_test_all(wParam, MK_XBUTTON1));
            flags.set<input::event_flag_f::mouse_x2>(nstd::bit_test_all(wParam, MK_XBUTTON2));
            flags.set<input::event_flag_f::double_click>(is_double_click_event());
        }

        return flags;
    }

    input::event_id get_keyboard_event_id() const noexcept
    {
        auto vk_code = LOWORD(wParam);

        auto key_flags = HIWORD(lParam);
        auto scan_code = LOBYTE(key_flags);
        auto is_extended = (key_flags & KF_EXTENDED) == KF_EXTENDED;

        if (vk_code >= 'A' && vk_code <= 'Z')
            return input::event_id_e::A + (vk_code - 'A');

        if (vk_code >= '0' && vk_code <= '9')
            return input::event_id_e::Zero + (vk_code - '0');

        if (vk_code == VK_RETURN && is_extended)
            return input::event_id_e::NumpadEnter;

        if (vk_code == VK_CONTROL)
            return is_extended ? input::event_id_e::RightCtrl : input::event_id_e::LeftCtrl;

        if (vk_code == VK_MENU)
            return is_extended ? input::event_id_e::RightAlt : input::event_id_e::LeftAlt;

        if (vk_code == VK_SHIFT)
        {
            auto mapped_key = nstd::size_cast<WORD>(MapVirtualKey(scan_code, MAPVK_VSC_TO_VK_EX));
            if (mapped_key != 0)
                vk_code = mapped_key;
        }

        auto it = InputKeyboardEvents.find(vk_code);
        if (it == InputKeyboardEvents.end())
            return input::event_id_e::Invalid;

        return it->second;
    }

    input::event_id get_mouse_event_id() const noexcept
    {
        auto it = InputMouseEvents.find(message);
        if (it == InputMouseEvents.end())
            return input::event_id_e::Invalid;

        if (it->second == input::event_id_e::MouseX)
        {
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return input::event_id_e::MouseX1;
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2) return input::event_id_e::MouseX2;
        }

        if (it->second == input::event_id_e::WheelVert)
            return GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? input::event_id_e::WheelDown : input::event_id_e::WheelUp;
        if (it->second == input::event_id_e::WheelHorz)
            return GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? input::event_id_e::WheelLeft : input::event_id_e::WheelRight;

        return it->second;
    }

    nstd::v2i get_cursor_pos(Window* window) const noexcept
    {
        if (is_wheel_event()) return { GET_X_LPARAM(lParam) - window->GetClientArea().left, GET_Y_LPARAM(lParam) - window->GetClientArea().top };
        if (is_mouse_event()) return { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        
        auto pos = window->GetCursorPos();
        return { pos.x, pos.y };
    }

    input::event_id get_event_id() const noexcept
    {
        if (is_keyboard_event()) return get_keyboard_event_id();
        if (is_mouse_event()) return get_mouse_event_id();
        return input::event_id_e::Invalid;
    }

    input::device_id get_source_device() const noexcept
    {
        if (is_keyboard_event()) return input::device_id_e::Keyboard;
        if (is_mouse_event()) return input::device_id_e::Mouse;
        return input::device_id_e::Invalid;
    }

    WORD get_count() const noexcept
    {
        if (is_keyboard_event()) return LOWORD(lParam);
        if (is_wheel_event()) return nstd::size_cast<WORD>(std::abs(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA));
        return 1;
    }
};

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
        logger::error(error::get_message());
        return false;
    }

    definition.id = result;
    WindowClassDefinitions.insert({signature, result});

    return true;
}

void Window::Update()
{
    auto new_cursor_pos = GetCursorPos();
    auto delta = new_cursor_pos - old_cursor_pos;

    if (!delta.is_zero())
    {
        static const auto MouseAbsolute = input::event_id_e::MouseAbsolute;
        static const auto MouseDelta = input::event_id_e::MouseDelta;
        static const auto MouseDeltaX = input::event_id_e::MouseDeltaX;
        static const auto MouseDeltaY = input::event_id_e::MouseDeltaY;

        input::event_flag flags = { input::event_flag_f::changed };
        flags.set<input::event_flag_f::caps_lock>(nstd::is_bit_set<0>(GetKeyState(VK_CAPITAL)));
        flags.set<input::event_flag_f::scroll_lock>(nstd::is_bit_set<0>(GetKeyState(VK_SCROLL)));
        flags.set<input::event_flag_f::num_lock>(nstd::is_bit_set<0>(GetKeyState(VK_NUMLOCK)));
        flags.set<input::event_flag_f::ctrl>(nstd::is_bit_set<7>(GetKeyState(VK_CONTROL)));
        flags.set<input::event_flag_f::shift>(nstd::is_bit_set<7>(GetKeyState(VK_SHIFT)));
        flags.set<input::event_flag_f::alt>(nstd::is_bit_set<7>(GetKeyState(VK_MENU)));

        input::event event = {
            .device = input::enhanced_enum_base_type_device_id::Mouse,
            .id = MouseAbsolute,
            .count = 1,
            .flags = flags,
            .cursor_pos = new_cursor_pos,
            };

        input::manager::add_event(event);

        event.id = input::event_id_e::MouseDelta;
        event.cursor_pos = delta;
        event.f_value = delta.length<float>();
        input::manager::add_event(event);

        if (delta.x != 0)
        {
            event.id = input::event_id_e::MouseDeltaX;
            event.i_value = delta.x;
            input::manager::add_event(event);
        }

        if (delta.y != 0)
        {
            event.id = input::event_id_e::MouseDeltaY;
            event.i_value = delta.y;
            input::manager::add_event(event);
        }
    }

    old_cursor_pos = new_cursor_pos;
}

RECT Window::GetClientArea() const
{
    RECT client_area;
    GetClientRect(handle, &client_area);
    auto client_width = client_area.right;
    auto client_height = client_area.bottom;
    AdjustWindowRectEx(&client_area, style, has_menu, style_ex);

    RECT window_area;
    GetWindowRect(handle, &window_area);

    return {
        .left =  window_area.left - client_area.left,
        .top = window_area.top - client_area.top,
        .right = client_width,
        .bottom = client_height,
    };
}

nstd::v2i Window::GetCursorPos() const
{
    RECT client_area = GetClientArea();
    POINT point;
    ::GetCursorPos(&point);
    return {
        .x = point.x - client_area.left,
        .y = point.y - client_area.top
    };
}


LRESULT Window::HandleSystemEvent(SystemEvent event)
{
    switch (event.message)
    {
    case WM_ACTIVATE:
        hasFocus = (LOWORD(event.wParam) != WA_INACTIVE);
        //input->OnFocusChanged(has_focus);
        old_cursor_pos = GetCursorPos();
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

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_UNICHAR:
    case WM_CHAR:
    case WM_SYSCHAR:

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        //case WM_MOUSEMOVE:
        CreateInputEvent(event);
        return 0;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
        CreateInputEvent(event);
        return TRUE;
    }

    return DefWindowProc(event.handle, event.message, event.wParam, event.lParam);
}

void Window::CreateInputEvent(const SystemEvent& event)
{
    if (event.is_char_event())
    {
        //input::character_stream += wstring(event.get_count(), static_cast<wchar_t>(event.wParam));
        return;
    }

    input::manager::add_event({
        .device = event.get_source_device(),
        .id = event.get_event_id(),
        .count = event.get_count(),
        .flags = event.get_input_flags(),
        .cursor_pos = event.get_cursor_pos(this),
        });
}

LRESULT CALLBACK Window::Proc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* window = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));
    return window->HandleSystemEvent({ handle, msg, wParam, lParam });
}

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
        logger::error(error::get_message());
        return FALSE;
    }

    // Replace this window proc with the simpler version which passes the event to the Window class
    SetLastError(0);
    result = SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Window::Proc));
    if (auto error_code = GetLastError(); result == 0 && error_code != 0)
    {
        logger::error(error::get_message());
        return FALSE;
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

export Window* CreateWindow(const WindowDefinition& definition)
{
    auto* window = new Window{definition};

    window->style = 0;
    window->style_ex = 0;

    window->style |= definition.withBorder ? WS_OVERLAPPEDWINDOW : WS_POPUP;
    window->style |= WS_CAPTION;

    if (definition.isFullScreen)
        window->style |= WS_MAXIMIZE;

    if (definition.isResizeable)
        window->style |= WS_THICKFRAME;
    else
        window->style &= ~WS_THICKFRAME;

    auto dpi = GetDpiForSystem();
    auto screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, dpi);
    auto screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, dpi);
    //SystemParametersInfoForDpi(SPI_GETWORKAREA, 
    auto windowWidth = std::min(screenWidth, definition.width);
    auto windowHeight = std::min(screenHeight, definition.height);

    if (definition.isFullScreen)
    {
        RECT rec_non_client = {};
        AdjustWindowRectExForDpi(&rec_non_client, window->style, FALSE, 0, dpi);
        windowWidth = screenWidth - (rec_non_client.right - rec_non_client.left);
        windowHeight = screenHeight - (rec_non_client.bottom - rec_non_client.top);
    }

    RECT rec = {
        .left = 0,
        .top = 0,
        .right = windowWidth,
        .bottom = windowHeight,
    };
    AdjustWindowRectExForDpi(&rec, window->style, FALSE, 0, dpi);

    // Create the window
    window->handle = CreateWindowExW(
        window->style_ex,
        MAKEINTATOM(definition.class_id),
        definition.title.c_str(),
        window->style,
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
        logger::error(error::get_message());
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

} // namespace platform
