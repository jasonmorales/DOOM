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
//  MapObj data. Map Objects or mobjs are actors, entities,
//  thinker, take-your-pick... anything that moves, acts, or
//  suffers state changes of more or less violent nature.
//
//-----------------------------------------------------------------------------
#pragma once

//
// Experimental stuff.
// To compile this as "ANSI C with classes"
//  we will need to handle the various
//  action functions cleanly.
//
using actionf_v = void(*)();
using actionf_p1 = void(*)(struct mobj_t*);
using actionf_p2 = void(*)(struct player_t*, struct pspdef_t*);

union actionf_t
{
    actionf_v acv;
    actionf_p1 acp1;
    actionf_p2 acp2;

    actionf_t(std::nullptr_t) : acv{ nullptr } {}
    actionf_t(actionf_p1 in) : acp1{ in } {}
    actionf_t(actionf_p2 in) : acp2{ in } {}
};

// Historically, "think_t" is yet another
//  function pointer to a routine to handle
//  an actor.
using think_t = actionf_t;

// Doubly linked list of actors.
struct thinker_t
{
    thinker_t* prev;
    thinker_t* next;
    think_t function;

    thinker_t()
        : prev{ nullptr }
        , next{ nullptr }
        , function{ nullptr }
    {}
};
