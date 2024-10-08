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
// DESCRIPTION:  Head up display
//
//-----------------------------------------------------------------------------
#pragma once

import input;

#include "d_event.h"


//
// Globally visible constants.
//
#define HU_FONTSTART	'!'	// the first font characters
#define HU_FONTEND	'_'	// the last font characters

// Calculate # of glyphs in font.
#define HU_FONTSIZE	(HU_FONTEND - HU_FONTSTART + 1)	

#define HU_BROADCAST	5

#define HU_MSGREFRESH	KEY_ENTER
#define HU_MSGX		0
#define HU_MSGY		0
#define HU_MSGWIDTH	64	// in characters
#define HU_MSGHEIGHT	1	// in lines

#define HU_MSGTIMEOUT	(4*TICRATE)

//
// HEADS UP TEXT
//

void HU_Init();
void HU_Start();

bool HU_Responder(const input::event& event);

void HU_Ticker();
void HU_Drawer();
char HU_dequeueChatChar();
void HU_Erase();
