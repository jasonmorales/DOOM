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
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------
#include "i_video.h"

#include "doomstat.h"
#include "st_stuff.h"
#include "m_misc.h"
#include "d_englsh.h"
#include "w_wad.h"
#include "m_cheat.h"
#include "p_local.h"
#include "d_main.h"

import std;


extern Doom* g_doom;


// For use if I do walls with outsides/insides
#define REDS		byte(256-5*16)
#define REDRANGE	byte(16)
#define BLUES		byte(256-4*16+8)
#define BLUERANGE	byte(8)
#define GREENS		byte(7*16)
#define GREENRANGE	byte(16)
#define GRAYS		byte(6*16)
#define GRAYSRANGE	byte(16)
#define BROWNS		byte(4*16)
#define BROWNRANGE	byte(16)
#define YELLOWS		byte(256-32+7)
#define YELLOWRANGE	byte(1)
#define BLACK		byte(0)
#define WHITE		byte(256-47)
#define NO_COLOR	byte(-1)

// Automap colors
#define BACKGROUND	BLACK
#define YOURCOLORS	WHITE
#define YOURRANGE	0
#define WALLCOLORS	REDS
#define WALLRANGE	REDRANGE
#define TSWALLCOLORS	GRAYS
#define TSWALLRANGE	GRAYSRANGE
#define FDWALLCOLORS	BROWNS
#define FDWALLRANGE	BROWNRANGE
#define CDWALLCOLORS	YELLOWS
#define CDWALLRANGE	YELLOWRANGE
#define THINGCOLORS	GREENS
#define THINGRANGE	GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS	(GRAYS + GRAYSRANGE/2)
#define GRIDRANGE	0
#define XHAIRCOLORS	GRAYS

// drawing stuff
#define	FB		0

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN        ((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (f_x + MTOF((x)-m_x))
#define CYMTOF(y)  (f_y + (f_h - MTOF((y)-m_y)))

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

struct fpoint_t
{
    int x = 0;
    int y = 0;
};

struct fline_t
{
    fpoint_t a = {};
    fpoint_t b = {};
};

struct mpoint_t
{
    fixed_t x = {};
    fixed_t y = {};
};

struct mline_t
{
    mpoint_t a = {};
    mpoint_t b = {};
};

struct islope_t
{
    fixed_t slp = {};
    fixed_t islp = {};
};

//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = {
    { { -R + R / 8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R - R / 2, R / 4 } },  // ----->
    { { R, 0 }, { R - R / 2, -R / 4 } },
    { { -R + R / 8, 0 }, { -R - R / 8, R / 4 } }, // >---->
    { { -R + R / 8, 0 }, { -R - R / 8, -R / 4 } },
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, R / 4 } }, // >>--->
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, -R / 4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = {
    { { -R + R / 8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R - R / 2, R / 6 } },  // ----->
    { { R, 0 }, { R - R / 2, -R / 6 } },
    { { -R + R / 8, 0 }, { -R - R / 8, R / 6 } }, // >----->
    { { -R + R / 8, 0 }, { -R - R / 8, -R / 6 } },
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, R / 6 } }, // >>----->
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, -R / 6 } },
    { { -R / 2, 0 }, { -R / 2, -R / 6 } }, // >>-d--->
    { { -R / 2, -R / 6 }, { -R / 2 + R / 6, -R / 6 } },
    { { -R / 2 + R / 6, -R / 6 }, { -R / 2 + R / 6, R / 4 } },
    { { -R / 6, 0 }, { -R / 6, -R / 6 } }, // >>-dd-->
    { { -R / 6, -R / 6 }, { 0, -R / 6 } },
    { { 0, -R / 6 }, { 0, R / 4 } },
    { { R / 6, R / 4 }, { R / 6, -R / 7 } }, // >>-ddt->
    { { R / 6, -R / 7 }, { R / 6 + R / 32, -R / 7 - R / 32 } },
    { { R / 6 + R / 32, -R / 7 - R / 32 }, { R / 6 + R / 10, -R / 7 } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))

auto _mult = [](const auto in) consteval
    {
        constexpr auto R = FRACUNIT;
        return static_cast<fixed_t>(in * R);
    };

mline_t triangle_guy[] = {
    { { _mult(-.867), _mult(-.5) }, { _mult(.867), _mult(-.5) } },
    { { _mult(.867), _mult(-.5) } , { 0, _mult(1) } },
    { { 0, _mult(1) }, { _mult(-.867), _mult(-.5) } }
};

#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))

mline_t thintriangle_guy[] = {
    { { _mult(-.5), _mult(-.7)}, { _mult(1), 0 } },
    { { _mult(1), 0 }, { _mult(-.5), _mult(.7) } },
    { { _mult(-.5), _mult(.7) }, { _mult(-.5), _mult(-.7) } }
};
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))




static int 	cheating = 0;
static int 	grid = 0;

static int 	leveljuststarted = 1; 	// kluge until AM_LevelInit() is called

bool    	automapactive = false;
static int 	finit_width = SCREENWIDTH;
static int 	finit_height = SCREENHEIGHT - 32;

// location of window on screen
static int 	f_x;
static int	f_y;

// size of window on screen
static int 	f_w;
static int	f_h;

static byte lightlev; 		// used for funky strobing effect
static byte* fb; 			// pseudo-frame buffer
static int 	amclock;

static mpoint_t m_paninc; // how far the window pans each tic (map coords)
static fixed_t 	mtof_zoommul; // how far the window zooms in each tic (map coords)
static fixed_t 	ftom_zoommul; // how far the window zooms in each tic (fb coords)

static fixed_t 	m_x, m_y;   // LL x,y where the window is on the map (map coords)
static fixed_t 	m_x2, m_y2; // UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t 	m_w;
static fixed_t	m_h;

// based on level size
static fixed_t 	min_x;
static fixed_t	min_y;
static fixed_t 	max_x;
static fixed_t  max_y;

static fixed_t 	max_w; // max_x-min_x,
static fixed_t  max_h; // max_y-min_y

// based on player size
static fixed_t 	min_w;
static fixed_t  min_h;


static fixed_t 	min_scale_mtof; // used to tell when to stop zooming out
static fixed_t 	max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = static_cast<fixed_t>(INITSCALEMTOF);
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static player_t* plr; // the player represented by an arrow

static const patch_t* marknums[10]; // numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0; // next point to be assigned

static int followplayer = 1; // specifies whether to follow the player around

static unsigned char cheat_amap_seq[] = { 0xb2, 0x26, 0x26, 0x2e, 0xff };
static cheatseq_t cheat_amap = { cheat_amap_seq, 0 };

static bool stopped = true;

extern bool viewactive;

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.
void AM_getIslope(mline_t* ml, islope_t* is)
{
    int dy = ml->a.y - ml->b.y;
    int dx = ml->b.x - ml->a.x;
    if (!dy)
        is->islp = (dx < 0 ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max());
    else
        is->islp = FixedDiv(dx, dy);
    if (!dx)
        is->slp = (dy < 0 ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max());
    else
        is->slp = FixedDiv(dy, dx);
}

//
//
//
void AM_activateNewScale()
{
    m_x += m_w / 2;
    m_y += m_h / 2;
    m_w = FTOM(f_w);
    m_h = FTOM(f_h);
    m_x -= m_w / 2;
    m_y -= m_h / 2;
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

//
//
//
void AM_saveScaleAndLoc()
{
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;
}

//
//
//
void AM_restoreScaleAndLoc()
{

    m_w = old_m_w;
    m_h = old_m_h;
    if (!followplayer)
    {
        m_x = old_m_x;
        m_y = old_m_y;
    }
    else
    {
        m_x = plr->mo->x - m_w / 2;
        m_y = plr->mo->y - m_h / 2;
    }
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;

    // Change the scaling multipliers
    scale_mtof = FixedDiv(f_w << FRACBITS, m_w);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark()
{
    markpoints[markpointnum].x = m_x + m_w / 2;
    markpoints[markpointnum].y = m_y + m_h / 2;
    markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;

}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries()
{
    int i;
    fixed_t a;
    fixed_t b;

    min_x = min_y = std::numeric_limits<decltype(min_x)>::max();
    max_x = max_y = std::numeric_limits<decltype(min_x)>::min();

    for (i = 0; i < numvertexes; i++)
    {
        if (vertexes[i].x < min_x)
            min_x = vertexes[i].x;
        else if (vertexes[i].x > max_x)
            max_x = vertexes[i].x;

        if (vertexes[i].y < min_y)
            min_y = vertexes[i].y;
        else if (vertexes[i].y > max_y)
            max_y = vertexes[i].y;
    }

    max_w = max_x - min_x;
    max_h = max_y - min_y;

    min_w = 2 * PLAYERRADIUS; // const? never changed?
    min_h = 2 * PLAYERRADIUS;

    a = FixedDiv(f_w << FRACBITS, max_w);
    b = FixedDiv(f_h << FRACBITS, max_h);

    min_scale_mtof = a < b ? a : b;
    max_scale_mtof = FixedDiv(f_h << FRACBITS, 2 * PLAYERRADIUS);

}

//
//
//
void AM_changeWindowLoc()
{
    if (m_paninc.x || m_paninc.y)
    {
        followplayer = 0;
        f_oldloc.x = std::numeric_limits<decltype(f_oldloc.x)>::max();;
    }

    m_x += m_paninc.x;
    m_y += m_paninc.y;

    if (m_x + m_w / 2 > max_x)
        m_x = max_x - m_w / 2;
    else if (m_x + m_w / 2 < min_x)
        m_x = min_x - m_w / 2;

    if (m_y + m_h / 2 > max_y)
        m_y = max_y - m_h / 2;
    else if (m_y + m_h / 2 < min_y)
        m_y = min_y - m_h / 2;

    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

void AM_initVariables()
{
    static input::event st_notify { .flags = {"down", "automap"} };

    automapactive = true;
    fb = g_doom->GetVideo()->GetScreen(0);

    f_oldloc.x = std::numeric_limits<decltype(f_oldloc.x)>::max();
    amclock = 0;
    lightlev = 0;

    m_paninc.x = m_paninc.y = 0;
    ftom_zoommul = FRACUNIT;
    mtof_zoommul = FRACUNIT;

    m_w = FTOM(f_w);
    m_h = FTOM(f_h);

    int32 pnum = 0;
    // find player to center on initially
    if (!playeringame[pnum = consoleplayer])
        for (pnum = 0; pnum < MAXPLAYERS; pnum++)
            if (playeringame[pnum])
                break;

    plr = &players[pnum];
    m_x = plr->mo->x - m_w / 2;
    m_y = plr->mo->y - m_h / 2;
    AM_changeWindowLoc();

    // for saving & restoring
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;

    // inform the status bar of the change
    ST_Responder(st_notify);
}

void AM_loadPics()
{
    for (int i = 0; i < 10; i++)
    {
        marknums[i] = WadManager::GetLumpData<patch_t>(std::format("AMMNUM{}", i));
    }
}

void AM_unloadPics()
{
#if 0
    for (int32 i = 0; i < 10; ++i)
        Z_ChangeTag(marknums[i], PU_CACHE);
#endif 
}

void AM_clearMarks()
{
    int i;

    for (i = 0; i < AM_NUMMARKPOINTS; i++)
        markpoints[i].x = -1; // means empty
    markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit()
{
    leveljuststarted = 0;

    f_x = f_y = 0;
    f_w = finit_width;
    f_h = finit_height;

    AM_clearMarks();

    AM_findMinMaxBoundaries();
    scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7 * FRACUNIT));
    if (scale_mtof > max_scale_mtof)
        scale_mtof = min_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

void AM_Stop()
{
    static input::event st_notify = { .flags = {"up", "automap"} };

    AM_unloadPics();
    automapactive = false;
    ST_Responder(st_notify);
    stopped = true;
}

void AM_Start()
{
    static int lastlevel = -1, lastepisode = -1;

    if (!stopped) AM_Stop();
    stopped = false;
    if (lastlevel != gamemap || lastepisode != gameepisode)
    {
        AM_LevelInit();
        lastlevel = gamemap;
        lastepisode = gameepisode;
    }
    AM_initVariables();
    AM_loadPics();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale()
{
    scale_mtof = min_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale()
{
    scale_mtof = max_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    AM_activateNewScale();
}

// Handle events (user inputs) in automap mode
bool AM_Responder(const input::event& event)
{
    static int cheatstate = 0;
    static int bigstate = 0;

    bool rc = false;

    if (!automapactive)
    {
        if (event.down() && Settings::CheckBind("MapOpen", event.id))
        {
            AM_Start();
            viewactive = false;
            rc = true;
        }
    }
    else if (event.down())
    {
        rc = true;
        if (Settings::CheckBind("MapRight", event.id))
        {
            if (!followplayer) m_paninc.x = FTOM(F_PANINC);
            else rc = false;
        }
        else if (Settings::CheckBind("MapLeft", event.id))
        {
            if (!followplayer) m_paninc.x = -FTOM(F_PANINC);
            else rc = false;
        }
        else if (Settings::CheckBind("MapUp", event.id))
        {
            if (!followplayer) m_paninc.y = FTOM(F_PANINC);
            else rc = false;
        }
        else if (Settings::CheckBind("MapDown", event.id))
        {
            if (!followplayer) m_paninc.y = -FTOM(F_PANINC);
            else rc = false;
        }
        else if (Settings::CheckBind("MapZoomOut", event.id))
        {
            mtof_zoommul = M_ZOOMOUT;
            ftom_zoommul = M_ZOOMIN;
        }
        else if (Settings::CheckBind("MapZoomIn", event.id))
        {
            mtof_zoommul = M_ZOOMIN;
            ftom_zoommul = M_ZOOMOUT;
        }
        else if (Settings::CheckBind("MapClose", event.id))
        {
            bigstate = 0;
            viewactive = true;
            AM_Stop();
        }
        else if (Settings::CheckBind("MapGoBig", event.id))
        {
            bigstate = !bigstate;
            if (bigstate)
            {
                AM_saveScaleAndLoc();
                AM_minOutWindowScale();
            }
            else AM_restoreScaleAndLoc();
        }
        else if (Settings::CheckBind("MapToggleFollow", event.id))
        {
            followplayer = !followplayer;
            f_oldloc.x = std::numeric_limits<decltype(f_oldloc.x)>::max();;
            plr->message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
        }
        else if (Settings::CheckBind("MapToggleGrid", event.id))
        {
            grid = !grid;
            plr->message = grid ? AMSTR_GRIDON : AMSTR_GRIDOFF;
        }
        else if (Settings::CheckBind("MapSetMark", event.id))
        {
            plr->message = std::format("{} {}", AMSTR_MARKEDSPOT, markpointnum);
            AM_addMark();
        }
        else if (Settings::CheckBind("MapClearMark", event.id))
        {
            AM_clearMarks();
            plr->message = AMSTR_MARKSCLEARED;
        }
        else
        {
            cheatstate = 0;
            rc = false;
        }

        if (!deathmatch && cht_CheckCheat(&cheat_amap, event.id.value))
        {
            rc = false;
            cheating = (cheating + 1) % 3;
        }
    }
    else if (event.is_keyboard() && event.up())
    {
        rc = false;
        if (Settings::CheckBind("MapRight", event.id))
        {
            if (!followplayer) m_paninc.x = 0;
        }
        else if (Settings::CheckBind("MapLeft", event.id))
        {
            if (!followplayer) m_paninc.x = 0;
        }
        else if (Settings::CheckBind("MapUp", event.id))
        {
            if (!followplayer) m_paninc.y = 0;
        }
        else if (Settings::CheckBind("MapDown", event.id))
        {
            if (!followplayer) m_paninc.y = 0;
        }
        else if (Settings::CheckBind("MapZoomIn", event.id) || Settings::CheckBind("MapZoomOut", event.id))
        {
            mtof_zoommul = FRACUNIT;
            ftom_zoommul = FRACUNIT;
        }
    }

    return rc;
}

// Zooming
void AM_changeWindowScale()
{
    // Change the scaling multipliers
    scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

    if (scale_mtof < min_scale_mtof)
        AM_minOutWindowScale();
    else if (scale_mtof > max_scale_mtof)
        AM_maxOutWindowScale();
    else
        AM_activateNewScale();
}


//
//
//
void AM_doFollowPlayer()
{

    if (f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
    {
        m_x = FTOM(MTOF(plr->mo->x)) - m_w / 2;
        m_y = FTOM(MTOF(plr->mo->y)) - m_h / 2;
        m_x2 = m_x + m_w;
        m_y2 = m_y + m_h;
        f_oldloc.x = plr->mo->x;
        f_oldloc.y = plr->mo->y;

        //  m_x = FTOM(MTOF(plr->mo->x - m_w/2));
        //  m_y = FTOM(MTOF(plr->mo->y - m_h/2));
        //  m_x = plr->mo->x - m_w/2;
        //  m_y = plr->mo->y - m_h/2;

    }

}

//
//
//
void AM_updateLightLev()
{
    static int nexttic = 0;
    //static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
    static byte litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };
    static int litelevelscnt = 0;

    // Change light level
    if (amclock > nexttic)
    {
        lightlev = litelevels[litelevelscnt++];
        if (litelevelscnt == sizeof(litelevels) / sizeof(int)) litelevelscnt = 0;
        nexttic = amclock + 6 - (amclock % 6);
    }

}


//
// Updates on Game Tick
//
void AM_Ticker()
{

    if (!automapactive)
        return;

    amclock++;

    if (followplayer)
        AM_doFollowPlayer();

    // Change the zoom if necessary
    if (ftom_zoommul != FRACUNIT)
        AM_changeWindowScale();

    // Change x,y location
    if (m_paninc.x || m_paninc.y)
        AM_changeWindowLoc();

    // Update light level
    // AM_updateLightLev();

}


//
// Clear automap frame buffer.
//
void AM_clearFB(int color)
{
    memset(fb, color, f_w * f_h);
}


//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
bool AM_clipMline(mline_t* ml, fline_t* fl)
{
    enum
    {
        LEFT = 1,
        RIGHT = 2,
        BOTTOM = 4,
        TOP = 8
    };

    int outcode1 = 0;
    int outcode2 = 0;
    int outside;

    int		dx;
    int		dy;

#define DOOUTCODE(oc, mx, my) \
    (oc) = 0; \
    if ((my) < 0) (oc) |= TOP; \
    else if ((my) >= f_h) (oc) |= BOTTOM; \
    if ((mx) < 0) (oc) |= LEFT; \
    else if ((mx) >= f_w) (oc) |= RIGHT;

    // do trivial rejects and outcodes
    if (ml->a.y > m_y2)
        outcode1 = TOP;
    else if (ml->a.y < m_y)
        outcode1 = BOTTOM;

    if (ml->b.y > m_y2)
        outcode2 = TOP;
    else if (ml->b.y < m_y)
        outcode2 = BOTTOM;

    if (outcode1 & outcode2)
        return false; // trivially outside

    if (ml->a.x < m_x)
        outcode1 |= LEFT;
    else if (ml->a.x > m_x2)
        outcode1 |= RIGHT;

    if (ml->b.x < m_x)
        outcode2 |= LEFT;
    else if (ml->b.x > m_x2)
        outcode2 |= RIGHT;

    if (outcode1 & outcode2)
        return false; // trivially outside

    // transform to frame-buffer coordinates.
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
        return false;

    while (outcode1 | outcode2)
    {
        // may be partially inside box
        // find an outside point
        if (outcode1)
            outside = outcode1;
        else
            outside = outcode2;

        fpoint_t tmp = {};

        // clip to each side
        if (outside & TOP)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
            tmp.y = 0;
        }
        else if (outside & BOTTOM)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y - f_h)) / dy;
            tmp.y = f_h - 1;
        }
        else if (outside & RIGHT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (f_w - 1 - fl->a.x)) / dx;
            tmp.x = f_w - 1;
        }
        else if (outside & LEFT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
            tmp.x = 0;
        }

        if (outside == outcode1)
        {
            fl->a = tmp;
            DOOUTCODE(outcode1, fl->a.x, fl->a.y);
        }
        else
        {
            fl->b = tmp;
            DOOUTCODE(outcode2, fl->b.x, fl->b.y);
        }

        if (outcode1 & outcode2)
            return false; // trivially outside
    }

    return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void AM_drawFline(fline_t* fl, byte color)
{
    static int fuck = 0;

    // For debugging only
    if (fl->a.x < 0 || fl->a.x >= f_w
        || fl->a.y < 0 || fl->a.y >= f_h
        || fl->b.x < 0 || fl->b.x >= f_w
        || fl->b.y < 0 || fl->b.y >= f_h)
    {
        std::cerr << std::format("fuck {} \n", fuck++);
        return;
    }

#define PUTDOT(xx, yy, cc) fb[(yy) * f_w + (xx)] = (cc)
    auto put_dot = [](auto x, auto y, auto c) {
        fb[y * f_w + x] = c;
        };

    int dx = fl->b.x - fl->a.x;
    int ax = 2 * (dx < 0 ? -dx : dx);
    int sx = dx < 0 ? -1 : 1;

    int dy = fl->b.y - fl->a.y;
    int ay = 2 * (dy < 0 ? -dy : dy);
    int sy = dy < 0 ? -1 : 1;

    int x = fl->a.x;
    int y = fl->a.y;

    if (ax > ay)
    {
        int d = ay - ax / 2;
        while (1)
        {
            put_dot(x, y, color);
            if (x == fl->b.x)
                return;

            if (d >= 0)
            {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    }
    else
    {
        int d = ax - ay / 2;
        while (1)
        {
            PUTDOT(x, y, color);

            if (y == fl->b.y)
                return;

            if (d >= 0)
            {
                x += sx;
                d -= ay;
            }

            y += sy;
            d += ax;
        }
    }
}


//
// Clip lines, draw visible part sof lines.
//
void AM_drawMline(mline_t* ml, byte color)
{
    static fline_t fl;
    if (AM_clipMline(ml, &fl))
        AM_drawFline(&fl, color); // draws it on frame buffer using fb coords
}



//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
void AM_drawGrid(byte color)
{
    fixed_t x, y;
    fixed_t start, end;
    mline_t ml;

    // Figure out start of vertical gridlines
    start = m_x;
    if ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS))
        start += (MAPBLOCKUNITS << FRACBITS)
        - ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS));
    end = m_x + m_w;

    // draw vertical gridlines
    ml.a.y = m_y;
    ml.b.y = m_y + m_h;
    for (x = start; x < end; x += (MAPBLOCKUNITS << FRACBITS))
    {
        ml.a.x = x;
        ml.b.x = x;
        AM_drawMline(&ml, color);
    }

    // Figure out start of horizontal gridlines
    start = m_y;
    if ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS))
        start += (MAPBLOCKUNITS << FRACBITS)
        - ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS));
    end = m_y + m_h;

    // draw horizontal gridlines
    ml.a.x = m_x;
    ml.b.x = m_x + m_w;
    for (y = start; y < end; y += (MAPBLOCKUNITS << FRACBITS))
    {
        ml.a.y = y;
        ml.b.y = y;
        AM_drawMline(&ml, color);
    }

}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls()
{
    int i;
    static mline_t l;

    for (i = 0; i < numlines; i++)
    {
        l.a.x = lines[i].v1->x;
        l.a.y = lines[i].v1->y;
        l.b.x = lines[i].v2->x;
        l.b.y = lines[i].v2->y;
        if (cheating || (lines[i].flags & ML_MAPPED))
        {
            if ((lines[i].flags & LINE_NEVERSEE) && !cheating)
                continue;
            if (!lines[i].backsector)
            {
                AM_drawMline(&l, WALLCOLORS + lightlev);
            }
            else
            {
                if (lines[i].special == 39)
                { // teleporters
                    AM_drawMline(&l, WALLCOLORS + WALLRANGE / 2);
                }
                else if (lines[i].flags & ML_SECRET) // secret door
                {
                    if (cheating) AM_drawMline(&l, SECRETWALLCOLORS + lightlev);
                    else AM_drawMline(&l, WALLCOLORS + lightlev);
                }
                else if (lines[i].backsector->floorheight
                    != lines[i].frontsector->floorheight)
                {
                    AM_drawMline(&l, FDWALLCOLORS + lightlev); // floor level change
                }
                else if (lines[i].backsector->ceilingheight
                    != lines[i].frontsector->ceilingheight)
                {
                    AM_drawMline(&l, CDWALLCOLORS + lightlev); // ceiling level change
                }
                else if (cheating)
                {
                    AM_drawMline(&l, TSWALLCOLORS + lightlev);
                }
            }
        }
        else if (plr->powers[pw_allmap])
        {
            if (!(lines[i].flags & LINE_NEVERSEE)) AM_drawMline(&l, GRAYS + 3);
        }
    }
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void
AM_rotate
(fixed_t* x,
    fixed_t* y,
    angle_t	a)
{
    fixed_t tmpx;

    tmpx =
        FixedMul(*x, finecosine[a >> ANGLETOFINESHIFT])
        - FixedMul(*y, finesine[a >> ANGLETOFINESHIFT]);

    *y =
        FixedMul(*x, finesine[a >> ANGLETOFINESHIFT])
        + FixedMul(*y, finecosine[a >> ANGLETOFINESHIFT]);

    *x = tmpx;
}

void
AM_drawLineCharacter(mline_t* lineguy, int lineguylines, fixed_t scale, angle_t angle, byte color, fixed_t x, fixed_t y)
{
    for (int i = 0; i < lineguylines; ++i)
    {
        mline_t l = {};

        l.a.x = lineguy[i].a.x;
        l.a.y = lineguy[i].a.y;

        if (scale)
        {
            l.a.x = FixedMul(scale, l.a.x);
            l.a.y = FixedMul(scale, l.a.y);
        }

        if (angle)
            AM_rotate(&l.a.x, &l.a.y, angle);

        l.a.x += x;
        l.a.y += y;

        l.b.x = lineguy[i].b.x;
        l.b.y = lineguy[i].b.y;

        if (scale)
        {
            l.b.x = FixedMul(scale, l.b.x);
            l.b.y = FixedMul(scale, l.b.y);
        }

        if (angle)
            AM_rotate(&l.b.x, &l.b.y, angle);

        l.b.x += x;
        l.b.y += y;

        AM_drawMline(&l, color);
    }
}

void AM_drawPlayers()
{
    static byte their_colors[] = { GREENS, GRAYS, BROWNS, REDS };
    byte their_color = NO_COLOR;

    if (!netgame)
    {
        if (cheating)
            AM_drawLineCharacter
            (cheat_player_arrow, NUMCHEATPLYRLINES, 0,
                plr->mo->angle, WHITE, plr->mo->x, plr->mo->y);
        else
            AM_drawLineCharacter
            (player_arrow, NUMPLYRLINES, 0, plr->mo->angle,
                WHITE, plr->mo->x, plr->mo->y);
        return;
    }

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        their_color++;
        player_t* p = &players[i];

        if ((deathmatch && !singledemo) && p != plr)
            continue;

        if (!playeringame[i])
            continue;

        byte color = p->powers[pw_invisibility] ? 246 /* *close* to black */ : their_colors[their_color];
        AM_drawLineCharacter(player_arrow, NUMPLYRLINES, 0, p->mo->angle, color, p->mo->x, p->mo->y);
    }
}

void AM_drawThings(byte colors, [[maybe_unused]] byte colorrange)
{
    for (int i = 0; i < numsectors; ++i)
    {
        mobj_t* t = sectors[i].thinglist;
        while (t)
        {
            AM_drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES, 16 << FRACBITS, t->angle, colors + lightlev, t->x, t->y);
            t = t->snext;
        }
    }
}

void AM_drawMarks()
{
    int i, fx, fy, w, h;

    for (i = 0; i < AM_NUMMARKPOINTS; i++)
    {
        if (markpoints[i].x != -1)
        {
            //      w = SHORT(marknums[i]->width);
            //      h = SHORT(marknums[i]->height);
            w = 5; // because something's wrong with the wad, i guess
            h = 6; // because something's wrong with the wad, i guess
            fx = CXMTOF(markpoints[i].x);
            fy = CYMTOF(markpoints[i].y);
            if (fx >= f_x && fx <= f_w - w && fy >= f_y && fy <= f_h - h)
                g_doom->GetVideo()->DrawPatch(fx, fy, FB, marknums[i]);
        }
    }

}

void AM_drawCrosshair(byte color)
{
    fb[(f_w * (f_h + 1)) / 2] = color; // single point for now

}

void AM_Drawer()
{
    if (!automapactive) return;

    AM_clearFB(BACKGROUND);
    if (grid)
        AM_drawGrid(GRIDCOLORS);
    AM_drawWalls();
    AM_drawPlayers();
    if (cheating == 2)
        AM_drawThings(THINGCOLORS, THINGRANGE);
    AM_drawCrosshair(XHAIRCOLORS);

    AM_drawMarks();
}
