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

import nstd;
import platform;

#include "system/windows.h"

#include <GL/glew.h>

class Doom;
struct patch_t;

void I_ShutdownGraphics();

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

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
    static void GLAPIENTRY GLErrorCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* param);

    Video() = delete;
    Video(Doom* doom) : doom{doom} {}

    void Init();

    // Called by Doom::Loop, called before processing any tics in a frame (just after displaying a
    // frame).
    // Time consuming synchronous operations are performed here (joystick reading).
    void StartFrame();

    // Called by Doom::Loop, called before processing each tic in a frame.
    // Quick synchronous operations are performed here.
    void StartTick();

    void FinishUpdate();
    void UpdateNoBlit() {}

    void DeliverSystemMessages();

    void SetPalette(const byte* palette);
    void DrawPatch(int32 x, int32 y, int32 screen, const patch_t* patch);

    byte* GetScreen(int32 n) const { return screens[n]; }
    byte* CopyScreen(int32 dest) const;

private:
    GLuint LoadShader(string_view name);

    Doom* doom = nullptr;

    //const wchar_t* WindowClassName = L"DoomWindow";

    bool isInitialized = false;

    bool isFullScreen = false;
    bool isExclusive = false;
    bool isBorderless = false;
    bool isResizeable = false;

    platform::Window* window = nullptr;

    //HWND windowHandle = nullptr;
    //HDC deviceContext = nullptr;
    //HGLRC renderContext = nullptr;

    bool hasFocus = true;

    // Blocky mode,
    // replace each 320x200 pixel with multiply*multiply pixels.
    // According to Dave Taylor, it still is a bonehead thing to use ....
    int32 screenMultiply = 1;

    uint32 windowWidth = 320;
    uint32 windowHeight = 200;

    uint32 screenWidth = 0;
    uint32 screenHeight = 0;

    //DWORD windowStyle = 0;
    //DWORD windowStyleEx = 0;

    uint32 screenTextureSize = 0;
    uint32* screenBuffer = nullptr;
    GLuint screenTexture = 0;
    GLuint screenShader = 0;

    GLuint screenVBO = GL_INVALID_INDEX;
    GLuint screenVAO = GL_INVALID_INDEX;

    byte* screens[5] = {nullptr};
    uint32 palette[256] = {0};
};
