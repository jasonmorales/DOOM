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
import std;
#define __STD_MODULE__

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#include "i_system.h"
#include "d_main.h"

#include <cstdlib>
#include <ctime>

extern Doom* g_doom;

int	mb_used = 6;


void
I_Tactile
(int	on,
    int	off,
    int	total)
{
    // UNUSED.
    on = off = total = 0;
}

ticcmd_t	emptycmd;
ticcmd_t* I_BaseTiccmd()
{
    return &emptycmd;
}


int  I_GetHeapSize()
{
    return mb_used * 1024 * 1024;
}

byte* I_ZoneBase(intptr_t* size)
{
    *size = mb_used * 1024 * 1024;
    return (byte*)malloc(*size);
}

// returns time in 1/70th second tics
time_t I_GetTime()
{
    static time_t basetime = 0;

    timespec ts;
    timespec_get(&ts, TIME_UTC);
    if (!basetime)
        basetime = ts.tv_sec;

    static time_t last = 0;
    time_t now = (ts.tv_sec - basetime) * TICRATE + static_cast<int64>(ts.tv_nsec) * TICRATE / 1'000'000'000;
    assert(now >= last);
    last = now;
    return now;
}

//
// I_Init
//
void I_Init()
{
    I_InitSound();
    //  I_InitGraphics();
}

//
// I_Quit
//
void I_Quit()
{
    D_QuitNetGame();
    I_ShutdownSound();
    I_ShutdownMusic();
    Settings::Save();
    I_ShutdownGraphics();
    exit(0);
}

void I_WaitVBL([[maybe_unused]] int count)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1'000 / 70));
}

void I_BeginRead()
{}

void I_EndRead()
{}

void I_Error(const string& error)
{
    std::cerr << "Error: " << error << "\n";
    std::cerr.flush();

    // Shutdown. Here might be other errors.
    if (g_doom->IsDemoRecording())
        G_CheckDemoStatus(g_doom);

    D_QuitNetGame();
    I_ShutdownGraphics();

    exit(-1);
}
