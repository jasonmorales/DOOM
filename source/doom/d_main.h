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
void D_AdvanceDemo();

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo. 
enum class GameState : int32
{
    Level,
    Intermission,
    Finale,
    Demo,

    ForceWipe
};

class Video;
class Render;

class Doom
{
public:
    Doom() = default;
    ~Doom() = default;

    void Main();
    void Loop();
    void ProcessEvents();
    void DoAdvanceDemo();

    void StartTitle();

    bool IsModified() const { return isModified; }
    bool IsDemoRecording() const { return isDemoRecording; }
    bool UseSingleTicks() const { return useSingleTicks; }
    GameState GetGameState() const { return gameState; }

    void SetDemoRecording(bool b) { isDemoRecording = b; }
    void SetUseSingleTicks(bool b) { useSingleTicks = b; }
    void SetGameState(GameState state) { gameState = state; }

    Video* GetVideo() const { return video; }
    Render* GetRender() const { return render; }

private:
    void IdentifyVersion();
    void AddFile(const std::filesystem::path& path) { wadFiles.push_back(path); }

    void Display();
    void PageDraw();

    Video* video = nullptr;
    bool viewActiveState = false;
    bool menuActiveState = false;
    bool inHelpScreensState = false;
    bool fullScreen = false;
    int32 borderDrawCount = -1;

    Render* render = nullptr;

    GameState gameState = GameState::Demo;
    GameState oldGameState = GameState::ForceWipe;

    int32 demoSequence = 0;

    // Set if homebrew PWAD stuff has been added.
    bool isModified = false;

    // for comparative timing purposes 
    bool noDrawers = false;

    bool isDemoRecording = false;

    // debug flag to cancel adaptiveness
    bool useSingleTicks = false;

    vector<std::filesystem::path> wadFiles;
};
