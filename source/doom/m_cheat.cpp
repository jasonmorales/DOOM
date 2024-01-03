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
//	Cheat sequence checking.
//
//-----------------------------------------------------------------------------
import std;

#include "m_cheat.h"

// CHEAT SEQUENCE PACKAGE

static int		firsttime = 1;
static unsigned char	cheat_xlate_table[256];

// Called in st_stuff module, which handles the input.
// Returns a 1 if the cheat was successful, 0 if failed.
int32 cht_CheckCheat(cheatseq_t* cht, input::event_id id)
{
    int i;
    int rc = 0;

    if (firsttime)
    {
        firsttime = 0;
        for (i = 0;i < 256;i++) cheat_xlate_table[i] = SCRAMBLE(i);
    }

    byte key = 0;
    if (id >= input::event_id("A") && id <= input::event_id("Z"))
        key = static_cast<byte>('A' + id - input::event_id("A"));
    else if (id >= input::event_id("0") && id <= input::event_id("9"))
        key = static_cast<byte>('0' + id - input::event_id("0"));

    if (!cht->p)
        cht->p = cht->sequence; // initialize if first time

    if (*cht->p == 0)
        *(cht->p++) = key;
    else if
        (cheat_xlate_table[(unsigned char)key] == *cht->p) cht->p++;
    else
        cht->p = cht->sequence;

    if (*cht->p == 1)
        cht->p++;
    else if (*cht->p == 0xff) // end of sequence character
    {
        cht->p = cht->sequence;
        rc = 1;
    }

    return rc;
}

void cht_GetParam(cheatseq_t* cht, char* buffer)
{
    unsigned char* p, c;

    p = cht->sequence;
    while (*(p++) != 1);

    do
    {
        c = *p;
        *(buffer++) = c;
        *(p++) = 0;
    } while (c && *p != 0xff);

    if (*p == 0xff)
        *buffer = 0;
}
