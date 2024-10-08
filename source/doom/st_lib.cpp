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
//	The status bar widget code.
//
//-----------------------------------------------------------------------------
import std;

#include "doomdef.h"

#include "z_zone.h"
#include "v_video.h"

#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"
#include "d_main.h"
#include "i_video.h"


extern Doom* g_doom;



// in AM_map.c
extern bool		automapactive;

// Hack display negative frags.
//  Loads and store the stminus lump.
const patch_t* sttminus;

void STlib_init()
{
    sttminus = WadManager::GetLumpData<patch_t>("STTMINUS");
}

void STlib_initNum(st_number_t* n, int x, int y, const patch_t** pl, int* num, bool* on, int width)
{
    n->x = x;
    n->y = y;
    n->oldnum = 0;
    n->width = width;
    n->num = num;
    n->on = on;
    n->p = pl;
}

// A fairly efficient way to draw a number based on differences from the old number.
// Note: worth the trouble?
void STlib_drawNum(st_number_t* n, [[maybe_unused]] bool refresh)
{
    int		numdigits = n->width;
    int		num = *n->num;

    int		w = (n->p[0]->width);
    int		h = (n->p[0]->height);
    int		x = n->x;

    int		neg;

    n->oldnum = *n->num;

    neg = num < 0;

    if (neg)
    {
        if (numdigits == 2 && num < -9)
            num = -9;
        else if (numdigits == 3 && num < -99)
            num = -99;

        num = -num;
    }

    // clear the area
    x = n->x - numdigits * w;

    if (n->y - ST_Y < 0)
        I_Error("drawNum: n->y - ST_Y < 0");

    V_CopyRect(x, n->y - ST_Y, BG, w * numdigits, h, x, n->y, FG);

    // if non-number, do not draw it
    if (num == 1994)
        return;

    x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
        g_doom->GetVideo()->DrawPatch(x - w, n->y, FG, n->p[0]);

    // draw the new number
    while (num && numdigits--)
    {
        x -= w;
        g_doom->GetVideo()->DrawPatch(x, n->y, FG, n->p[num % 10]);
        num /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
        g_doom->GetVideo()->DrawPatch(x - 8, n->y, FG, sttminus);
}

void STlib_updateNum(st_number_t* n, bool refresh)
{
    if (*n->on)
        STlib_drawNum(n, refresh);
}

void STlib_initPercent(st_percent_t* p, int x, int y, const patch_t** pl, int* num, bool* on, const patch_t* percent)
{
    STlib_initNum(&p->n, x, y, pl, num, on, 3);
    p->p = percent;
}

void
STlib_updatePercent
(st_percent_t* per,
    int			refresh)
{
    if (refresh && *per->n.on)
        g_doom->GetVideo()->DrawPatch(per->n.x, per->n.y, FG, per->p);

    STlib_updateNum(&per->n, refresh);
}

void STlib_initMultIcon(st_multicon_t* i, int x, int y, const patch_t** il, int* inum, bool* on)
{
    i->x = x;
    i->y = y;
    i->oldinum = -1;
    i->inum = inum;
    i->on = on;
    i->p = il;
}

void STlib_updateMultIcon(st_multicon_t* mi, bool refresh)
{
    if (*mi->on
        && (mi->oldinum != *mi->inum || refresh)
        && (*mi->inum != -1))
    {
        if (mi->oldinum != -1)
        {
            auto x = mi->x - (mi->p[mi->oldinum]->leftoffset);
            auto y = mi->y - (mi->p[mi->oldinum]->topoffset);
            auto w = (mi->p[mi->oldinum]->width);
            auto h = (mi->p[mi->oldinum]->height);

            if (y - ST_Y < 0)
                I_Error("updateMultIcon: y - ST_Y < 0");

            V_CopyRect(x, y - ST_Y, BG, w, h, x, y, FG);
        }
        g_doom->GetVideo()->DrawPatch(mi->x, mi->y, FG, mi->p[*mi->inum]);
        mi->oldinum = *mi->inum;
    }
}

void STlib_initBinIcon(st_binicon_t* b, int x, int y, const patch_t* i, bool* val, bool* on)
{
    b->x = x;
    b->y = y;
    b->oldval = 0;
    b->val = val;
    b->on = on;
    b->p = i;
}

void STlib_updateBinIcon(st_binicon_t* bi, bool refresh)
{
    if (*bi->on
        && (bi->oldval != *bi->val || refresh))
    {
        auto x = bi->x - bi->p->leftoffset;
        auto y = bi->y - bi->p->topoffset;
        auto w = bi->p->width;
        auto h = bi->p->height;

        if (y - ST_Y < 0)
            I_Error("updateBinIcon: y - ST_Y < 0");

        if (*bi->val)
            g_doom->GetVideo()->DrawPatch(bi->x, bi->y, FG, bi->p);
        else
            V_CopyRect(x, y - ST_Y, BG, w, h, x, y, FG);

        bi->oldval = *bi->val;
    }

}

