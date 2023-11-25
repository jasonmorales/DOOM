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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------
#pragma once

#include "doomtype.h"

#include "system/windows.h"
#include "types/numbers.h"

void I_ShutdownGraphics();

// Takes full 8 bit values.
void I_SetPalette(byte* palette);

void I_UpdateNoBlit();
void I_FinishUpdate();

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

void I_ReadScreen(byte* scr);

void I_BeginRead();
void I_EndRead();

struct SystemEvent
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

class Video
{
public:
    void Init();

    // Called by Doom::Loop,
    // called before processing any tics in a frame
    // (just after displaying a frame).
    // Time consuming synchronous operations
    // are performed here (joystick reading).
    // Can call D_PostEvent.
    void StartFrame();

    // Called by Doom::Loop,
    // called before processing each tic in a frame.
    // Quick synchronous operations are performed here.
    // Can call D_PostEvent.
    void StartTick();

    LRESULT HandleSystemEvent(const SystemEvent& event);
    void DeliverSystemMessages();

private:
    bool RegisterWindowClass();

    const wchar_t* WindowClassName = L"DoomWindow";

    bool isInitialized = false;

    bool isFullScreen = false;
    bool isExclusive = false;
    bool isBorderless = false;
    bool isResizeable = false;

    HWND windowHandle = nullptr;
    HDC deviceContext = nullptr;
    HGLRC renderContext = nullptr;

    bool hasFocus = true;

    // Blocky mode,
    // replace each 320x200 pixel with multiply*multiply pixels.
    // According to Dave Taylor, it still is a bonehead thing to use ....
    int32 screenMultiply = 1;

    int32 windowWidth = 320;
    int32 windowHeight = 200;

    int32 screenWidth = 0;
    int32 screenHeight = 0;

    DWORD windowStyle = 0;
    DWORD windowStyleEx = 0;
};
