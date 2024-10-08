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
//   Menu widget stuff, episode selection and such.
//    
//-----------------------------------------------------------------------------
#pragma once

import nstd;
import input;

#include "d_event.h"

// MENUS
//
// Called by main loop, saves config file and calls I_Quit when user exits.
// Even when the menu is not displayed, this can resize the view and change game parameters.
// Does all the real work of the menu interaction.
bool M_Responder(const input::event& event);


// Called by main loop, only used for menu (skull cursor) animation.
void M_Ticker();

// Called by main loop, draws the menus directly into the screen buffer.
void M_Drawer();

// Called by intro code to force menu up upon a keypress, does nothing if menu is already up.
void M_StartControlPanel();

class Menu
{
public:
    static void Init();

    static void StartMessage(string_view message, void(*routine)(bool), bool input);

    static string messageString;

private:
};
