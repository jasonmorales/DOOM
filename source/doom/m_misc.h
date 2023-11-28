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
//    
//-----------------------------------------------------------------------------
#pragma once

#include "doomtype.h"

boolean M_WriteFile(char const* name, void* source, uint32 length);

int M_ReadFile(char const* name, byte** buffer);

void M_ScreenShot();

void M_LoadDefaults();

void M_SaveDefaults();

int M_DrawText(int x, int y, boolean direct, char* string);
