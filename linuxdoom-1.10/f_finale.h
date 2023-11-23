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
#include "d_event.h"
//
// FINALE
//

// Called by main loop.
boolean F_Responder(event_t* ev);

// Called by main loop.
void F_Ticker();

// Called by main loop.
void F_Drawer();


void F_StartFinale();
