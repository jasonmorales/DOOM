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

#include "doomtype.h"

#include "types/numbers.h"


// Event handling.

// Input event types.
enum evtype_t
{
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_joystick,

    ev_invalid
};

// Event structure.
struct event_t
{
    bool operator==(const event_t& other) const = default;

    evtype_t type = ev_invalid;
    int32 data1 = 0; // keys / mouse/joystick buttons
    int32 data2 = 0; // mouse/joystick x move
    int32 data3 = 0; // mouse/joystick y move
};

enum gameaction_t
{
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_playdemo,
    ga_completed,
    ga_victory,
    ga_worlddone,
    ga_screenshot
};

// Button/action code definitions.
enum buttoncode_t
{
    // Press "Fire".
    BT_ATTACK = 1,
    // Use button, to open doors, activate switches.
    BT_USE = 2,

    // Flag: game events, not really buttons.
    BT_SPECIAL = 128,
    BT_SPECIALMASK = 3,

    // Flag, weapon change pending.
    // If true, the next 3 bits hold weapon num.
    BT_CHANGE = 4,
    // The 3bit weapon mask and shift, convenience.
    BT_WEAPONMASK = (8 + 16 + 32),
    BT_WEAPONSHIFT = 3,

    // Pause the game.
    BTS_PAUSE = 1,
    // Save the game at each console.
    BTS_SAVEGAME = 2,

    // Savegame slot numbers
    //  occupy the second byte of buttons.    
    BTS_SAVEMASK = (4 + 8 + 16),
    BTS_SAVESHIFT = 2,

};

// GLOBAL VARIABLES

extern  gameaction_t    gameaction;
