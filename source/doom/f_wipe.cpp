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
//	Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------
#include "z_zone.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"
#include "d_main.h"

#include "doomdef.h"

#include "f_wipe.h"


extern Doom* g_doom;


//                       SCREEN WIPE PACKAGE

// when zero, stop the wipe
static boolean	go = 0;

static byte* wipe_scr_start;
static byte* wipe_scr_end;
static byte* wipe_scr;


void
wipe_shittyColMajorXform
(short* array,
    int		width,
    int		height)
{
    int		x;
    int		y;
    short* dest;

    dest = (short*)Z_Malloc(width * height * 2, PU_STATIC, 0);

    for (y = 0;y < height;y++)
        for (x = 0;x < width;x++)
            dest[x * height + y] = array[y * width + x];

    memcpy(array, dest, width * height * 2);

    Z_Free(dest);

}

int wipe_initColorXForm(int width, int height, [[maybe_unused]] time_t ticks)
{
    memcpy(wipe_scr, wipe_scr_start, width * height);
    return 0;
}

int32 wipe_doColorXForm(int32	width, int32 height, time_t ticks)
{
    auto bticks = static_cast<byte>(ticks);

    bool changed = false;
    byte* w = wipe_scr;
    byte* e = wipe_scr_end;

    while (w != wipe_scr + width * height)
    {
        if (*w != *e)
        {
            if (*w > *e)
            {
                byte newval = *w - bticks;
                if (newval < *e)
                    *w = *e;
                else
                    *w = newval;
                changed = true;
            }
            else if (*w < *e)
            {
                byte newval = *w + bticks;
                if (newval > *e)
                    *w = *e;
                else
                    *w = newval;
                changed = true;
            }
        }
        w++;
        e++;
    }

    return !changed;
}

int
wipe_exitColorXForm
([[maybe_unused]] int	width,
    [[maybe_unused]] int	height,
    [[maybe_unused]] time_t	ticks)
{
    return 0;
}

static int* wipe_y;

int wipe_initMelt(int width, int height, [[maybe_unused]] time_t ticks)
{
    int i, r;

    // copy start screen to main screen
    memcpy(wipe_scr, wipe_scr_start, width * height);

    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform((short*)wipe_scr_start, width / 2, height);
    wipe_shittyColMajorXform((short*)wipe_scr_end, width / 2, height);

    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    wipe_y = (int*)Z_Malloc(width * sizeof(int), PU_STATIC, 0);
    wipe_y[0] = -(M_Random() % 16);
    for (i = 1;i < width;i++)
    {
        r = (M_Random() % 3) - 1;
        wipe_y[i] = wipe_y[i - 1] + r;
        if (wipe_y[i] > 0)
            wipe_y[i] = 0;
        else if (wipe_y[i] == -16)
            wipe_y[i] = -15;
    }

    return 0;
}

int
wipe_doMelt
(int	width,
    int	height,
    time_t	ticks)
{
    int		i;
    int		j;
    int		dy;
    int		idx;

    short* s;
    short* d;
    boolean	done = true;

    width /= 2;

    while (ticks--)
    {
        for (i = 0;i < width;i++)
        {
            if (wipe_y[i] < 0)
            {
                wipe_y[i]++; done = false;
            }
            else if (wipe_y[i] < height)
            {
                dy = (wipe_y[i] < 16) ? wipe_y[i] + 1 : 8;
                if (wipe_y[i] + dy >= height) dy = height - wipe_y[i];
                s = &((short*)wipe_scr_end)[i * height + wipe_y[i]];
                d = &((short*)wipe_scr)[wipe_y[i] * width + i];
                idx = 0;
                for (j = dy;j;j--)
                {
                    d[idx] = *(s++);
                    idx += width;
                }
                wipe_y[i] += dy;
                s = &((short*)wipe_scr_start)[i * height];
                d = &((short*)wipe_scr)[wipe_y[i] * width + i];
                idx = 0;
                for (j = height - wipe_y[i];j;j--)
                {
                    d[idx] = *(s++);
                    idx += width;
                }
                done = false;
            }
        }
    }

    return done;

}

int
wipe_exitMelt
([[maybe_unused]] int	width,
    [[maybe_unused]] int	height,
    [[maybe_unused]] time_t	ticks)
{
    Z_Free(wipe_y);
    return 0;
}

int wipe_StartScreen([[maybe_unused]] int	x,
    [[maybe_unused]] int	y,
    [[maybe_unused]] int	width,
    [[maybe_unused]]  int	height)
{
    wipe_scr_start = g_doom->GetVideo()->CopyScreen(2);
    return 0;
}

int wipe_EndScreen(int	x,
    int	y,
    int	width,
    int	height)
{
    wipe_scr_end = g_doom->GetVideo()->CopyScreen(3);
    V_DrawBlock(x, y, 0, width, height, wipe_scr_start); // restore start scr.
    return 0;
}

int wipe_ScreenWipe(int	wipeno, [[maybe_unused]] int x, [[maybe_unused]] int y, int width, int height, time_t ticks)
{
    static int (*wipes[])(int, int, time_t) =
    {
        wipe_initColorXForm,
        wipe_doColorXForm,
        wipe_exitColorXForm,
        wipe_initMelt,
        wipe_doMelt,
        wipe_exitMelt
    };

    // initial stuff
    if (!go)
    {
        go = 1;
        // wipe_scr = (byte *) Z_Malloc(width*height, PU_STATIC, 0); // DEBUG
        wipe_scr = g_doom->GetVideo()->GetScreen(0);
        (*wipes[wipeno * 3])(width, height, ticks);
    }

    // do a piece of wipe-in
    g_doom->GetVideo()->MarkRect(0, 0, width, height);
    auto rc = (*wipes[wipeno * 3 + 1])(width, height, ticks);
    //  V_DrawBlock(x, y, 0, width, height, wipe_scr); // DEBUG

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes[wipeno * 3 + 2])(width, height, ticks);
    }

    return !go;

}
