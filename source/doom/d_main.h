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

#include "d_event.h"

#include "containers/vector.h"

#include <filesystem>

//
// Called by IO functions when input is detected.
void D_PostEvent(event_t* ev);

//
// BASE LEVEL
//
void D_PageTicker();
void D_PageDrawer();
void D_AdvanceDemo();
void D_StartTitle();

class Video;

class Doom
{
public:
    Doom() = default;
    ~Doom() = default;

    void Main();
    void Loop();

    bool IsModified() const { return isModified; }
    bool IsDemoRecording() const { return isDemoRecording; }

    void SetDemoRecording(bool b) { isDemoRecording = b; }

private:
    void IdentifyVersion();
    void AddFile(const std::filesystem::path& path);

    Video* video = nullptr;

    // Set if homebrew PWAD stuff has been added.
    bool isModified = false;

    bool isDemoRecording = false;

    vector<std::filesystem::path> wadFiles;
};
