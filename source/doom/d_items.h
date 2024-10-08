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
//	Items: key cards, artifacts, weapon, ammunition.
//
//-----------------------------------------------------------------------------
#pragma once

#include "doomdef.h"
#include "info.h"

// Weapon info: sprite frames, ammunition use.
struct weaponinfo_t
{
    ammotype_t ammo;
    statenum_t upstate;
    statenum_t downstate;
    statenum_t readystate;
    statenum_t atkstate;
    statenum_t flashstate;

};

extern weaponinfo_t weaponinfo[NUMWEAPONS];
