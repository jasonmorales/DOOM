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

import std;
import nstd;

#include "d_event.h"

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

// Game mode handling - identify IWAD version
//  to handle IWAD dependent animations etc.
enum class GameMode : int32
{
    Doom1Shareware,	    // DOOM 1 shareware, E1, M9
    Doom1Registered,	// DOOM 1 registered, E3, M27
    Doom2Commercial,	// DOOM 2 retail, E1 M34
    Doom1Retail,	    // DOOM 1 retail, E4, M36
    Unknown,            // Well, no IWAD found.
};

class Video;
class Render;
class Game;

class Doom
{
public:
    static const int32 Version = 109;

    Doom() = default;
    ~Doom() = default;

    void Main();
    void Loop();
    void ProcessEvents();
    void DoAdvanceDemo();

    void StartTitle();

    bool IsModified() const { return isModified; }
    bool IsDevMode() const { return isDevMode; }
    bool IsDemoRecording() const { return isDemoRecording; }
    bool UseSingleTicks() const { return useSingleTicks; }
    GameState GetGameState() const { return gameState; }
    GameMode GetGameMode() const { return gameMode; }

    void SetDemoRecording(bool b) { isDemoRecording = b; }
    void SetUseSingleTicks(bool b) { useSingleTicks = b; }
    void SetGameState(GameState state) { gameState = state; }

    Video* GetVideo() const { return video; }
    Render* GetRender() const { return render; }
    Game* GetGame() const { return game; }

    bool HasEscEventInQueue();

private:
    void IdentifyVersion();

    void Display();
    void PageDraw();

    Video* video = nullptr;
    bool viewActiveState = false;
    bool menuActiveState = false;
    bool inHelpScreensState = false;
    bool fullScreen = false;
    int32 borderDrawCount = -1;

    Render* render = nullptr;

    Game* game = nullptr;
    GameMode gameMode = GameMode::Unknown;
    GameState gameState = GameState::Demo;
    GameState oldGameState = GameState::ForceWipe;

    int32 demoSequence = 0;

    bool isModified = false; // Set if homebrew PWAD stuff has been added.
    bool isDevMode = false;	// DEBUG: launched with -devparm

    // for comparative timing purposes 
    bool noDrawers = false;

    bool isDemoRecording = false;

    // debug flag to cancel adaptiveness
    bool useSingleTicks = false;
};
