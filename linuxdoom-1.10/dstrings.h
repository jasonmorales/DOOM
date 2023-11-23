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
//
// DESCRIPTION:
//	DOOM strings, by language.
//
//-----------------------------------------------------------------------------
#pragma once

// All important printed strings.
// Language selection (message strings).
// Use -DFRENCH etc.

#include "d_englsh.h"

// Misc. other strings.
#define SAVEGAMENAME	"doomsav"

//
// File locations,
//  relative to current position.
// Path names are OS-sensitive.
//
#define DEVMAPS "devmaps"
#define DEVDATA "devdata"

// QuitDOOM messages
#define NUM_QUITMESSAGES   22

extern const char* endmsg[];
