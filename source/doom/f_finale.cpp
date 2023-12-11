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
//	Game completion, final screen animation.
//
//-----------------------------------------------------------------------------
import std;
#define __STD_MODULE__

#include "i_system.h"
#include "m_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"
#include "i_video.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"
#include "d_main.h"


extern Doom* g_doom;
extern GameState wipegamestate;


// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int		finalestage;

int		finalecount;

#define	TEXTSPEED	3
#define	TEXTWAIT	250

const char* e1text = E1TEXT;
const char* e2text = E2TEXT;
const char* e3text = E3TEXT;
const char* e4text = E4TEXT;

const char* c1text = C1TEXT;
const char* c2text = C2TEXT;
const char* c3text = C3TEXT;
const char* c4text = C4TEXT;
const char* c5text = C5TEXT;
const char* c6text = C6TEXT;

const char* p1text = P1TEXT;
const char* p2text = P2TEXT;
const char* p3text = P3TEXT;
const char* p4text = P4TEXT;
const char* p5text = P5TEXT;
const char* p6text = P6TEXT;

const char* t1text = T1TEXT;
const char* t2text = T2TEXT;
const char* t3text = T3TEXT;
const char* t4text = T4TEXT;
const char* t5text = T5TEXT;
const char* t6text = T6TEXT;

string_view finaletext;
const char* finaleflat;

int		castnum;
int		casttics;
state_t* caststate;
bool castdeath;
int		castframes;
int		castonmelee;
bool castattacking;

// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
struct castinfo_t
{
    const char* name;
    mobjtype_t type;
};

castinfo_t	castorder[] = {
    {CC_ZOMBIE, MT_POSSESSED},
    {CC_SHOTGUN, MT_SHOTGUY},
    {CC_HEAVY, MT_CHAINGUY},
    {CC_IMP, MT_TROOP},
    {CC_DEMON, MT_SERGEANT},
    {CC_LOST, MT_SKULL},
    {CC_CACO, MT_HEAD},
    {CC_HELL, MT_KNIGHT},
    {CC_BARON, MT_BRUISER},
    {CC_ARACH, MT_BABY},
    {CC_PAIN, MT_PAIN},
    {CC_REVEN, MT_UNDEAD},
    {CC_MANCU, MT_FATSO},
    {CC_ARCH, MT_VILE},
    {CC_SPIDER, MT_SPIDER},
    {CC_CYBER, MT_CYBORG},
    {CC_HERO, MT_PLAYER},

    {nullptr, MT_INVALID}
};

void	F_StartCast();
void	F_CastTicker();
void	F_CastDrawer();

void F_StartFinale()
{
    gameaction = ga_nothing;
    g_doom->SetGameState(GameState::Finale);
    viewactive = false;
    automapactive = false;

    // Okay - IWAD dependent stuff.
    // This has been changed severely, and
    //  some stuff might have changed in the process.
    switch (gamemode)
    {

        // DOOM 1 - E1, E3 or E4, but each nine missions
    case GameMode::Doom1Shareware:
    case GameMode::Doom1Registered:
    case GameMode::Doom1Retail:
    {
        S_ChangeMusic(mus_victor, true);

        switch (gameepisode)
        {
        case 1:
            finaleflat = "FLOOR4_8";
            finaletext = e1text;
            break;
        case 2:
            finaleflat = "SFLR6_1";
            finaletext = e2text;
            break;
        case 3:
            finaleflat = "MFLR8_4";
            finaletext = e3text;
            break;
        case 4:
            finaleflat = "MFLR8_3";
            finaletext = e4text;
            break;
        default:
            // Ouch.
            break;
        }
        break;
    }

    // DOOM II and missions packs with E1, M34
    case GameMode::Doom2Commercial:
    {
        S_ChangeMusic(mus_read_m, true);

        switch (gamemap)
        {
        case 6:
            finaleflat = "SLIME16";
            finaletext = c1text;
            break;
        case 11:
            finaleflat = "RROCK14";
            finaletext = c2text;
            break;
        case 20:
            finaleflat = "RROCK07";
            finaletext = c3text;
            break;
        case 30:
            finaleflat = "RROCK17";
            finaletext = c4text;
            break;
        case 15:
            finaleflat = "RROCK13";
            finaletext = c5text;
            break;
        case 31:
            finaleflat = "RROCK19";
            finaletext = c6text;
            break;
        default:
            // Ouch.
            break;
        }
        break;
    }
    // Indeterminate.
    default:
        S_ChangeMusic(mus_read_m, true);
        finaleflat = "F_SKY1"; // Not used anywhere else.
        finaletext = c1text;  // FIXME - other text, music?
        break;
    }

    finalestage = 0;
    finalecount = 0;

}

bool F_CastResponder(const event_t& event)
{
    if (event.type != ev_keydown)
        return false;

    if (castdeath)
        return true;			// already in dying frames

    // go into death frame
    castdeath = true;
    caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
    casttics = caststate->tics;
    castframes = 0;
    castattacking = false;
    if (mobjinfo[castorder[castnum].type].deathsound)
        S_StartSound(nullptr, mobjinfo[castorder[castnum].type].deathsound);

    return true;
}

bool F_Responder(const event_t& event)
{
    if (finalestage == 2)
        return F_CastResponder(event);

    return false;
}

void F_Ticker()
{
    int		i;

    // check for skipping
    if ((gamemode == GameMode::Doom2Commercial)
        && (finalecount > 50))
    {
        // go on to the next level
        for (i = 0; i < MAXPLAYERS; i++)
            if (players[i].cmd.buttons)
                break;

        if (i < MAXPLAYERS)
        {
            if (gamemap == 30)
                F_StartCast();
            else
                gameaction = ga_worlddone;
        }
    }

    // advance animation
    finalecount++;

    if (finalestage == 2)
    {
        F_CastTicker();
        return;
    }

    if (gamemode == GameMode::Doom2Commercial)
        return;

    if (!finalestage && finalecount > finaletext.length() * TEXTSPEED + TEXTWAIT)
    {
        finalecount = 0;
        finalestage = 1;
        wipegamestate = GameState::ForceWipe;		// force a wipe
        if (gameepisode == 3)
            S_StartMusic(mus_bunny);
    }
}

//
// F_TextWrite
//

#include "hu_stuff.h"
extern	const patch_t* hu_font[HU_FONTSIZE];


void F_TextWrite()
{
    int		x, y, w;
    int		count;
    int		cx;
    int		cy;

    // erase the entire screen to a tiled background
    auto* src = WadManager::GetLumpData<byte>(finaleflat);
    byte* dest = g_doom->GetVideo()->GetScreen(0);

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        for (x = 0; x < SCREENWIDTH / 64; x++)
        {
            memcpy(dest, src + ((y & 63) << 6), 64);
            dest += 64;
        }
        if constexpr (SCREENWIDTH & 63)
        {
            memcpy(dest, src + ((y & 63) << 6), SCREENWIDTH & 63);
            dest += (SCREENWIDTH & 63);
        }
    }

    // draw some of the text onto the screen
    cx = 10;
    cy = 10;
    int32 ch = 0;

    count = (finalecount - 10) / TEXTSPEED;
    if (count < 0)
        count = 0;
    for (; count; count--)
    {
        auto c = finaletext[ch++];
        if (ch >= finaletext.length())
            break;

        if (c == '\n')
        {
            cx = 10;
            cy += 11;
            continue;
        }

        c = static_cast<char>(toupper(c)) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = (hu_font[c]->width);
        if (cx + w > SCREENWIDTH)
            break;
        g_doom->GetVideo()->DrawPatch(cx, cy, 0, hu_font[c]);
        cx += w;
    }

}

void F_StartCast()
{
    wipegamestate = GameState::ForceWipe;		// force a screen wipe
    castnum = 0;
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    casttics = caststate->tics;
    castdeath = false;
    finalestage = 2;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
    S_ChangeMusic(mus_evil, true);
}

void F_CastTicker()
{
    int		st;
    int		sfx;

    if (--casttics > 0)
        return;			// not time to change state yet

    if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
        // switch from deathstate to next monster
        castnum++;
        castdeath = false;
        if (castorder[castnum].name == nullptr)
            castnum = 0;
        if (mobjinfo[castorder[castnum].type].seesound)
            S_StartSound(nullptr, mobjinfo[castorder[castnum].type].seesound);
        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        castframes = 0;
    }
    else
    {
        // just advance to next state in animation
        if (caststate == &states[S_PLAY_ATK1])
            goto stopattack;	// Oh, gross hack!
        st = caststate->nextstate;
        caststate = &states[st];
        castframes++;

        // sound hacks....
        switch (st)
        {
        case S_PLAY_ATK1:	sfx = sfx_dshtgn; break;
        case S_POSS_ATK2:	sfx = sfx_pistol; break;
        case S_SPOS_ATK2:	sfx = sfx_shotgn; break;
        case S_VILE_ATK2:	sfx = sfx_vilatk; break;
        case S_SKEL_FIST2:	sfx = sfx_skeswg; break;
        case S_SKEL_FIST4:	sfx = sfx_skepch; break;
        case S_SKEL_MISS2:	sfx = sfx_skeatk; break;
        case S_FATT_ATK8:
        case S_FATT_ATK5:
        case S_FATT_ATK2:	sfx = sfx_firsht; break;
        case S_CPOS_ATK2:
        case S_CPOS_ATK3:
        case S_CPOS_ATK4:	sfx = sfx_shotgn; break;
        case S_TROO_ATK3:	sfx = sfx_claw; break;
        case S_SARG_ATK2:	sfx = sfx_sgtatk; break;
        case S_BOSS_ATK2:
        case S_BOS2_ATK2:
        case S_HEAD_ATK2:	sfx = sfx_firsht; break;
        case S_SKULL_ATK2:	sfx = sfx_sklatk; break;
        case S_SPID_ATK2:
        case S_SPID_ATK3:	sfx = sfx_shotgn; break;
        case S_BSPI_ATK2:	sfx = sfx_plasma; break;
        case S_CYBER_ATK2:
        case S_CYBER_ATK4:
        case S_CYBER_ATK6:	sfx = sfx_rlaunc; break;
        case S_PAIN_ATK3:	sfx = sfx_sklatk; break;
        default: sfx = 0; break;
        }

        if (sfx)
            S_StartSound(nullptr, sfx);
    }

    if (castframes == 12)
    {
        // go into attack frame
        castattacking = true;
        if (castonmelee)
            caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
        else
            caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
        castonmelee ^= 1;
        if (caststate == &states[S_NULL])
        {
            if (castonmelee)
                caststate =
                &states[mobjinfo[castorder[castnum].type].meleestate];
            else
                caststate =
                &states[mobjinfo[castorder[castnum].type].missilestate];
        }
    }

    if (castattacking)
    {
        if (castframes == 24
            || caststate == &states[mobjinfo[castorder[castnum].type].seestate])
        {
        stopattack:
            castattacking = false;
            castframes = 0;
            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
    }

    casttics = caststate->tics;
    if (casttics == -1)
        casttics = 15;
}

void F_CastPrint(const char* text)
{
    // find width
    const char* ch = text;
    int width = 0;

    while (ch)
    {
        int c = *ch++;
        if (!c)
            break;

        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            width += 4;
            continue;
        }

        int w = (hu_font[c]->width);
        width += w;
    }

    // draw it
    int cx = 160 - width / 2;
    ch = text;
    while (ch)
    {
        int c = *ch++;
        if (!c)
            break;

        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        int w = (hu_font[c]->width);
        g_doom->GetVideo()->DrawPatch(cx, 180, 0, hu_font[c]);
        cx += w;
    }
}

//
// F_CastDrawer
//
void V_DrawPatchFlipped(int x, int y, int scrn, const patch_t* patch);

void F_CastDrawer()
{
    // erase the entire screen to a background
    g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("BOSSBACK"));

    F_CastPrint(castorder[castnum].name);

    // draw the current frame in the middle of the screen
    spritedef_t* sprdef = &sprites[caststate->sprite];
    spriteframe_t* sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];
    int32 id = sprframe->lump[0];
    bool flip = (sprframe->flip[0] != 0);

    auto* patch = WadManager::GetLumpData<patch_t>(id + firstspritelump);
    if (flip)
        V_DrawPatchFlipped(160, 170, 0, patch);
    else
        g_doom->GetVideo()->DrawPatch(160, 170, 0, patch);
}

void F_DrawPatchCol(int x, const patch_t* patch, int col)
{
    auto* column = (column_t*)((byte*)patch + (patch->columnofs[col]));
    auto* desttop = g_doom->GetVideo()->GetScreen(0) + x;

    // step through the posts in a column
    while (column->topdelta != 0xff)
    {
        auto* source = (byte*)column + 3;
        auto* dest = desttop + column->topdelta * SCREENWIDTH;
        auto count = column->length;

        while (count--)
        {
            *dest = *source++;
            dest += SCREENWIDTH;
        }
        column = (column_t*)((byte*)column + column->length + 4);
    }
}

// F_BunnyScroll
void F_BunnyScroll()
{
    int		scrolled;
    int		x;
    int		stage;
    static int	laststage;

    auto* p1 = WadManager::GetLumpData<patch_t>("PFUB2");
    auto* p2 = WadManager::GetLumpData<patch_t>("PFUB1");

    scrolled = 320 - (finalecount - 230) / 2;
    if (scrolled > 320)
        scrolled = 320;
    if (scrolled < 0)
        scrolled = 0;

    for (x = 0; x < SCREENWIDTH; x++)
    {
        if (x + scrolled < 320)
            F_DrawPatchCol(x, p1, x + scrolled);
        else
            F_DrawPatchCol(x, p2, x + scrolled - 320);
    }

    if (finalecount < 1130)
        return;
    if (finalecount < 1180)
    {
        g_doom->GetVideo()->DrawPatch((SCREENWIDTH - 13 * 8) / 2, (SCREENHEIGHT - 8 * 8) / 2, 0, WadManager::GetLumpData<patch_t>("END0"));
        laststage = 0;
        return;
    }

    stage = (finalecount - 1180) / 5;
    if (stage > 6)
        stage = 6;
    if (stage > laststage)
    {
        S_StartSound(nullptr, sfx_pistol);
        laststage = stage;
    }

    auto* patch = WadManager::GetLumpData<patch_t>(std::format("END{}", stage));
    g_doom->GetVideo()->DrawPatch((SCREENWIDTH - 13 * 8) / 2, (SCREENHEIGHT - 8 * 8) / 2, 0, patch);
}

void F_Drawer()
{
    if (finalestage == 2)
    {
        F_CastDrawer();
        return;
    }

    if (!finalestage)
        F_TextWrite();
    else
    {
        switch (gameepisode)
        {
        case 1:
            if (gamemode == GameMode::Doom1Retail)
                g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("CREDIT"));
            else
                g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("HELP2"));
            break;
        case 2:
            g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("VICTORY2"));
            break;
        case 3:
            F_BunnyScroll();
            break;
        case 4:
            g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("ENDPIC"));
            break;
        }
    }
}
