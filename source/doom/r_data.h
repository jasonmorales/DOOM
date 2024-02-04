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
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//
//-----------------------------------------------------------------------------
#pragma once

#include "r_defs.h"
#include "r_state.h"

import nstd;


// Retrieve column data for span blitting.
const byte* R_GetColumn(int32 tex, int32 col);

// I/O, setting up the stuff.
void R_InitData();
void R_PrecacheLevel();

// Retrieval.
// Floor/ceiling opaque texture tiles, lookup by name. For animation?
int32 R_FlatNumForName(string_view name);

// Called by P_Ticker for switches and animations, returns the texture number for the texture name.
int32 R_TextureNumForName(string_view name);
int32 R_CheckTextureNumForName(string_view name);
