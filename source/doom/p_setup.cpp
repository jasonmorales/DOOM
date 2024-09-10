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
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------
#include "d_main.h"
#include "z_zone.h"
#include "m_swap.h"
#include "m_bbox.h"
#include "g_game.h"
#include "i_system.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "doomstat.h"
#include "r_things.h"

import std;


extern Doom* g_doom;


void	P_SpawnMapThing(mapthing_t* mthing);

const vector<string_view> spriteNames = {
    "TROO","SHTG","PUNG","PISG","PISF","SHTF","SHT2","CHGG","CHGF","MISG",
    "MISF","SAWG","PLSG","PLSF","BFGG","BFGF","BLUD","PUFF","BAL1","BAL2",
    "PLSS","PLSE","MISL","BFS1","BFE1","BFE2","TFOG","IFOG","PLAY","POSS",
    "SPOS","VILE","FIRE","FATB","FBXP","SKEL","MANF","FATT","CPOS","SARG",
    "HEAD","BAL7","BOSS","BOS2","SKUL","SPID","BSPI","APLS","APBX","CYBR",
    "PAIN","SSWV","KEEN","BBRN","BOSF","ARM1","ARM2","BAR1","BEXP","FCAN",
    "BON1","BON2","BKEY","RKEY","YKEY","BSKU","RSKU","YSKU","STIM","MEDI",
    "SOUL","PINV","PSTR","PINS","MEGA","SUIT","PMAP","PVIS","CLIP","AMMO",
    "ROCK","BROK","CELL","CELP","SHEL","SBOX","BPAK","BFUG","MGUN","CSAW",
    "LAUN","PLAS","SHOT","SGN2","COLU","SMT2","GOR1","POL2","POL5","POL4",
    "POL3","POL1","POL6","GOR2","GOR3","GOR4","GOR5","SMIT","COL1","COL2",
    "COL3","COL4","CAND","CBRA","COL6","TRE1","TRE2","ELEC","CEYE","FSKU",
    "COL5","TBLU","TGRN","TRED","SMBT","SMGT","SMRT","HDB1","HDB2","HDB3",
    "HDB4","HDB5","HDB6","POB1","POB2","BRS1","TLMP","TLP2"
};

// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int		numvertexes;
vertex_t* vertexes;

int		numsegs;
seg_t* segs;

int		numsectors;
sector_t* sectors;

int		numsubsectors;
subsector_t* subsectors;

int		numnodes;
node_t* nodes;

int		numlines;
line_t* lines;

int		numsides;
side_t* sides;


// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int		bmapwidth;
int		bmapheight;	// size in mapblocks
const short* blockmap;	// int for larger maps
// offsets in blockmap are from here
const short* blockmaplump;
// origin of block map
fixed_t		bmaporgx;
fixed_t		bmaporgy;
// for thing chains
mobj_t** blocklinks;


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
const byte* rejectmatrix;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS	10

mapthing_t	deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t* deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];

//
// P_LoadVertexes
//
void P_LoadVertexes(int lump)
{
    mapvertex_t* ml;
    vertex_t* li;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = WadManager::GetLump(lump).size / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc<vertex_t>(numvertexes * sizeof(vertex_t), PU_LEVEL, 0);

    // Load data into cache.
    auto* data = WadManager::GetLumpData<byte>(lump);

    ml = (mapvertex_t*)data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (int i = 0; i < numvertexes; i++, li++, ml++)
    {
        li->x = (ml->x) << FRACBITS;
        li->y = (ml->y) << FRACBITS;
    }
}

void P_LoadSegs(int32 lump)
{
    line_t* ldef;
    int			side;

    numsegs = WadManager::GetLump(lump).size / sizeof(mapseg_t);
    segs = Z_Malloc<seg_t>(numsegs * sizeof(seg_t), PU_LEVEL, 0);
    std::memset(segs, 0, numsegs * sizeof(seg_t));
    auto* data = WadManager::GetLumpData<mapseg_t>(lump);

    auto* ml = data;
    auto* li = segs;
    for (int i = 0; i < numsegs; i++, li++, ml++)
    {
        li->v1 = &vertexes[(ml->v1)];
        li->v2 = &vertexes[(ml->v2)];

        li->angle = ((ml->angle)) << 16;
        li->offset = ((ml->offset)) << 16;
        auto _linedef = (ml->linedef);
        ldef = &lines[_linedef];
        li->linedef = ldef;
        side = (ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;
        if (ldef->flags & ML_TWOSIDED)
            li->backsector = sides[ldef->sidenum[side ^ 1]].sector;
        else
            li->backsector = 0;
    }
}

void P_LoadSubsectors(int lump)
{
    int			i;
    mapsubsector_t* ms;
    subsector_t* ss;

    numsubsectors = WadManager::GetLump(lump).size / sizeof(mapsubsector_t);
    subsectors = Z_Malloc<subsector_t>(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
    auto* data = WadManager::GetLumpData<byte>(lump);

    ms = (mapsubsector_t*)data;
    std::memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
    ss = subsectors;

    for (i = 0; i < numsubsectors; i++, ss++, ms++)
    {
        ss->numlines = (ms->numsegs);
        ss->firstline = (ms->firstseg);
    }
}

void P_LoadSectors(int32 lumpId)
{
    auto* lump = WadManager::FindLump(lumpId);
    if (!lump)
        return;

    numsectors = lump->size / sizeof(mapsector_t);
    sectors = Z_Malloc<sector_t>(numsectors * sizeof(sector_t), PU_LEVEL, 0);
    std::memset(sectors, 0, numsectors * sizeof(sector_t));

    auto* ms = lump->as<mapsector_t>();
    auto* ss = sectors;
    for (int32 i = 0; i < numsectors; ++i, ++ss, ++ms)
    {
        ss->floorheight = (ms->floorheight) << FRACBITS;
        ss->ceilingheight = (ms->ceilingheight) << FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = (ms->lightlevel);
        ss->special = (ms->special);
        ss->tag = (ms->tag);
        ss->thinglist = nullptr;
    }
}

void P_LoadNodes(intptr_t lump)
{
    numnodes = WadManager::GetLump(lump).size / sizeof(mapnode_t);
    nodes = Z_Malloc<node_t>(numnodes * sizeof(node_t), PU_LEVEL, 0);
    auto* data = WadManager::GetLumpData<mapnode_t>(lump);
    auto* mn = data;
    auto* no = nodes;

    for (int32 i = 0; i < numnodes; i++, no++, mn++)
    {
        no->x = (mn->x) << FRACBITS;
        no->y = (mn->y) << FRACBITS;
        no->dx = (mn->dx) << FRACBITS;
        no->dy = (mn->dy) << FRACBITS;
        for (int32 j = 0; j < 2; j++)
        {
            no->children[j] = (mn->children[j]);
            for (int32 k = 0; k < 4; k++)
                no->bounds[j][k] = (mn->bounds[j][k]) << FRACBITS;
        }
    }
}

void P_LoadThings(intptr_t lump)
{
    int			i;
    mapthing_t* mt;
    int			numthings;
    bool spawn = false;

    auto* data = WadManager::GetLumpData<byte>(lump);
    numthings = WadManager::GetLump(lump).size / sizeof(mapthing_t);

    mt = (mapthing_t*)data;
    for (i = 0; i < numthings; i++, mt++)
    {
        spawn = true;

        // Do not spawn cool, new monsters if !commercial
        if (g_doom->GetGameMode() != GameMode::Doom2Commercial)
        {
            switch (mt->type)
            {
            case 68:	// Arachnotron
            case 64:	// Archvile
            case 88:	// Boss Brain
            case 89:	// Boss Shooter
            case 69:	// Hell Knight
            case 67:	// Mancubus
            case 71:	// Pain Elemental
            case 65:	// Former Human Commando
            case 66:	// Revenant
            case 84:	// Wolf SS
                spawn = false;
                break;
            }
        }
        if (!spawn)
            break;

        // Do spawn all other stuff. 
        mt->x = (mt->x);
        mt->y = (mt->y);
        mt->angle = (mt->angle);
        mt->type = (mt->type);
        mt->options = (mt->options);

        P_SpawnMapThing(mt);
    }
}

// Also counts secret lines for intermissions.
void P_LoadLineDefs(intptr_t lump)
{
    int			i;
    maplinedef_t* mld;
    line_t* ld;
    vertex_t* v1;
    vertex_t* v2;

    numlines = WadManager::GetLump(lump).size / sizeof(maplinedef_t);
    lines = Z_Malloc<line_t>(numlines * sizeof(line_t), PU_LEVEL, 0);
    std::memset(lines, 0, numlines * sizeof(line_t));
    auto* data = WadManager::GetLumpData<byte>(lump);

    mld = (maplinedef_t*)data;
    ld = lines;
    for (i = 0; i < numlines; i++, mld++, ld++)
    {
        ld->flags = (mld->flags);
        ld->special = (mld->special);
        ld->tag = (mld->tag);
        v1 = ld->v1 = &vertexes[(mld->v1)];
        v2 = ld->v2 = &vertexes[(mld->v2)];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;

        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv(ld->dy, ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }

        if (v1->x < v2->x)
        {
            ld->bounds.left = v1->x;
            ld->bounds.right = v2->x;
        }
        else
        {
            ld->bounds.left = v2->x;
            ld->bounds.right = v1->x;
        }

        if (v1->y < v2->y)
        {
            ld->bounds.bottom = v1->y;
            ld->bounds.top = v2->y;
        }
        else
        {
            ld->bounds.bottom = v2->y;
            ld->bounds.top = v1->y;
        }

        ld->sidenum[0] = (mld->sidenum[0]);
        ld->sidenum[1] = (mld->sidenum[1]);

        if (ld->sidenum[0] != -1)
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else
            ld->frontsector = 0;

        if (ld->sidenum[1] != -1)
            ld->backsector = sides[ld->sidenum[1]].sector;
        else
            ld->backsector = 0;
    }
}

void P_LoadSideDefs(intptr_t lump)
{
    int			i;
    mapsidedef_t* msd;
    side_t* sd;

    numsides = WadManager::GetLump(lump).size / sizeof(mapsidedef_t);
    sides = Z_Malloc<side_t>(numsides * sizeof(side_t), PU_LEVEL, 0);
    std::memset(sides, 0, numsides * sizeof(side_t));
    auto* data = WadManager::GetLumpData<byte>(lump);

    msd = (mapsidedef_t*)data;
    sd = sides;
    for (i = 0; i < numsides; i++, msd++, sd++)
    {
        sd->textureoffset = (msd->textureoffset) << FRACBITS;
        sd->rowoffset = (msd->rowoffset) << FRACBITS;
        sd->toptexture = R_TextureNumForName({msd->toptexture, 8});
        sd->bottomtexture = R_TextureNumForName({msd->bottomtexture, 8});
        sd->midtexture = R_TextureNumForName({msd->midtexture, 8});
        sd->sector = &sectors[(msd->sector)];
    }
}

void P_LoadBlockMap(intptr_t lump)
{
    blockmaplump = WadManager::GetLumpData<short>(lump);
    blockmap = blockmaplump + 4;
    auto count = WadManager::GetLump(lump).size / 2;

    bmaporgx = blockmaplump[0] << FRACBITS;
    bmaporgy = blockmaplump[1] << FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains
    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc<mobj_t*>(count, PU_LEVEL, 0);
    std::memset(blocklinks, 0, count);
}

// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
void P_GroupLines()
{
    line_t** linebuffer;
    int			i;
    int			j;
    int			total;
    line_t* li;
    sector_t* sector;
    subsector_t* ss;
    seg_t* seg;
    int			block;

    // look up sector number for each subsector
    ss = subsectors;
    for (i = 0; i < numsubsectors; i++, ss++)
    {
        seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    total = 0;
    for (i = 0; i < numlines; i++, li++)
    {
        total++;
        li->frontsector->linecount++;

        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            total++;
        }
    }

    // build line tables for each sector	
    linebuffer = Z_Malloc<line_t*>(total * sizeof(line_t*), PU_LEVEL, 0);
    sector = sectors;
    for (i = 0; i < numsectors; i++, sector++)
    {
        bbox bounds;
        sector->lines = linebuffer;
        li = lines;
        for (j = 0; j < numlines; j++, li++)
        {
            if (li->frontsector == sector || li->backsector == sector)
            {
                *linebuffer++ = li;
                bounds.add(li->v1->x, li->v1->y);
                bounds.add(li->v2->x, li->v2->y);
            }
        }
        if (linebuffer - sector->lines != sector->linecount)
            I_Error("P_GroupLines: miscounted");

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = bounds.midx();
        sector->soundorg.y = bounds.midy();

        // adjust bounding box to map blocks
        block = (bounds.top - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight - 1 : block;
        sector->blockbox.top = block;

        block = (bounds.bottom - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox.bottom = block;

        block = (bounds.right - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth - 1 : block;
        sector->blockbox.right = block;

        block = (bounds.left - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox.left = block;
    }
}

void P_SetupLevel(int episode, int map, int /*playermask*/, skill_t /*skill*/)
{
    int		i;

    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;
    for (i = 0; i < MAXPLAYERS; i++)
    {
        players[i].killcount = players[i].secretcount
            = players[i].itemcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1;

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start();


#if 0 // UNUSED
    if (debugfile.is_open())
    {
        Z_FreeTags(PU_LEVEL, MAXINT);
        Z_FileDumpHeap(debugfile);
    }
    else
#endif
        Z_FreeTags(PU_LEVEL, PU_PURGELEVEL - 1);


    P_InitThinkers();

    // find map name
    string lumpname;
    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
        lumpname = std::format("MAP{:02d}", map);
    else
        lumpname = std::format("E{}M{}", episode, map);

    auto lumpnum = W_GetNumForName(lumpname);

    leveltime = 0;

    // note: most of this ordering is important	
    P_LoadBlockMap(lumpnum + ML_BLOCKMAP);
    P_LoadVertexes(lumpnum + ML_VERTEXES);
    P_LoadSectors(lumpnum + ML_SECTORS);
    P_LoadSideDefs(lumpnum + ML_SIDEDEFS);

    P_LoadLineDefs(lumpnum + ML_LINEDEFS);
    P_LoadSubsectors(lumpnum + ML_SSECTORS);
    P_LoadNodes(lumpnum + ML_NODES);
    P_LoadSegs(lumpnum + ML_SEGS);

    rejectmatrix = WadManager::GetLumpData<byte>(lumpnum + ML_REJECT);
    P_GroupLines();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings(lumpnum + ML_THINGS);

    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
        for (i = 0; i < MAXPLAYERS; i++)
            if (playeringame[i])
            {
                players[i].mo = nullptr;
                G_DeathMatchSpawnPlayer(i);
            }

    }

    // clear special respawning que
    iquehead = iquetail = 0;

    // set up world state
    P_SpawnSpecials();

    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
        R_PrecacheLevel();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());

}

void P_Init(Doom* doom)
{
    P_InitSwitchList();
    P_InitPicAnims();
    R_InitSprites(doom, spriteNames);
}
