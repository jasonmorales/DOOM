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
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR args, int showCmd)
{
    myargc = 1;
    myargv = *args;
 
    D_DoomMain (); 

    return 0;
} 
