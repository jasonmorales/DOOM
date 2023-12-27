module;

#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOMINMAX
#define NOMENUS
#include <Windows.h>

export module platform.windows;

import std;
import nstd;

import platform.window;

namespace {

LRESULT CALLBACK InitWindowProc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg != WM_NCCREATE)
        return DefWindowProc(handle, msg, wParam, lParam);
       
    auto create = reinterpret_cast<LPCREATESTRUCT>(lParam);

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
    //result = SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GlobalWindowProc));
    error_core = GetLastError();
    if (result == 0 && error_core != 0)
    {
        LPWSTR buffer(NULL);
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_core, 0, (LPWSTR)&buffer, 0, nullptr);
        MessageBoxW(nullptr, buffer, L"Error in WM_NCCREATE", MB_OK);
        LocalFree(buffer);
        return FALSE;
    }

    return DefWindowProc(handle, msg, wParam, lParam);
}

struct ExtraWindowData
{
    class Window* owner = nullptr;
};

std::map<wstring, WindowDefinition> Definitions;

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

export bool RegisterWindowClass()
{
    auto window_class = WNDCLASSEXW{
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = InitWindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = sizeof(ExtraWindowData),
        .hInstance = GetModuleHandleW(NULL),
        .hIcon = NULL,
        .hCursor = LoadCursorW(NULL, IDC_ARROW),
        .hbrBackground = NULL,
        .lpszMenuName = NULL,
        .lpszClassName = L"",
        .hIconSm = NULL,
    };

    auto result = RegisterClassExW(&window_class);
    if (result == 0)
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

} // namespace platform
