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
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#include "i_system.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <time.h>
#include <chrono>
#include <thread>


#include "d_main.h"
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



//
// I_GetTime
// returns time in 1/70th second tics
//
int  I_GetTime()
{
    timespec ts;
    time_t newtics;
    static time_t basetime = 0;

    timespec_get(&ts, TIME_UTC);
    if (!basetime)
        basetime = ts.tv_sec;
    newtics = (ts.tv_sec - basetime) * TICRATE + ts.tv_nsec * TICRATE / 1'000'000'000;
    return newtics;
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
    M_SaveDefaults();
    I_ShutdownGraphics();
    exit(0);
}

void I_WaitVBL(int count)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1'000 / 70));
}

void I_BeginRead()
{}

void I_EndRead()
{}

byte* I_AllocLow(int length)
{
    byte* mem;

    mem = (byte*)malloc(length);
    memset(mem, 0, length);
    return mem;
}


//
// I_Error
//
void I_Error(const char* error, ...)
{
    va_list	argptr;

    // Message first.
    va_start(argptr, error);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, error, argptr);
    fprintf(stderr, "\n");
    va_end(argptr);

    fflush(stderr);

    // Shutdown. Here might be other errors.
    if (g_doom->IsDemoRecording())
        G_CheckDemoStatus(g_doom);

    D_QuitNetGame();
    I_ShutdownGraphics();

    exit(-1);
}
