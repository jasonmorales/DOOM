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
//	Main program, simply calls Doom::Main high level loop.
//
//-----------------------------------------------------------------------------
import std;
#define __STD_MODULE__

#include "m_argv.h"
#include "d_main.h"

#include "system/windows.h"

Doom* g_doom = nullptr;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR args, int)
{
    CommandLine::Initialize(args);

    g_doom = new Doom;
    g_doom->Main();

    return 0;
}
