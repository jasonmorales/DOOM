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
//-----------------------------------------------------------------------------
#pragma once

#include "doomdef.h"
#include "d_event.h"

class Doom;

// GAME

void G_DeathMatchSpawnPlayer(int playernum);

void G_InitNew(skill_t skill, int episode, int map);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew(skill_t skill, int episode, int map);

void G_DeferedPlayDemo(const char* demo);

// Called by M_Responder.
void G_SaveGame(int slot, string_view description);

// Only called by startup code.
void G_RecordDemo(Doom* doom, string_view name);

void G_BeginRecording();

void G_TimeDemo(const char* name);
bool G_CheckDemoStatus(Doom* doom);

void G_ExitLevel();
void G_SecretExitLevel();

void G_WorldDone();

bool G_Responder(const event_t& ev);

void G_ScreenShot();

class Game
{
public:
    static filesys::path GetSaveFilePath(int32 n)
    {
        return SavePath / std::format("{}{:02}.dsg", SaveFileName, n);
    }

    static const int32 SaveStringSize = 24;

    Game(Doom* doom) : doom{doom} {}

    void Ticker();

    // Can be called by the startup code or M_Responder, calls P_SetupLevel or W_EnterWorld.
    void LoadGame(const filesys::path& fileName);

private:
    static const filesys::path SavePath;
    static constexpr const char* SaveFileName = "doomsav";

    static const int32 VersionSize = 16;

    void DoSaveGame();
    void DoLoadGame();

    Doom* doom = nullptr;

    filesys::path loadFileName;
};
