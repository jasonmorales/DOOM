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
//	Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------
#include "doomdef.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "r_local.h"
#include "doomstat.h"
#include "d_main.h"
#include "r_defs.h"
#include "r_state.h"
#include "r_main.h"
#include "r_things.h"
#include "r_draw.h"
#include "r_segs.h"
#include "r_bsp.h"

import std;


#define MINZ				(FRACUNIT*4)
#define BASEYCENTER			100

struct maskdraw_t
{
    int		x1;
    int		x2;

    int		column;
    int		topclip;
    int		bottomclip;
};

// Sprite rotation 0 is facing the viewer, rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle, which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
int32 pspritescale;
int32 pspriteiscale;

lighttable_t** spritelights;

// constant arrays used for psprite clipping and initializing clipping
int16 negonearray[SCREENWIDTH];
int16 screenheightarray[SCREENWIDTH];

// INITIALIZATION FUNCTIONS

// variables used to look up and range check thing_t sprites patches
spritedef_t* sprites;
int32 numsprites;

spriteframe_t sprtemp[29];
int32 maxframe;

// Local function for R_InitSprites.
void R_InstallSpriteLump(string_view name, int32 lump, uint32 frame, uint32 rotation, bool flipped)
{
    if (frame >= 29 || rotation > 8)
        I_Error("R_InstallSpriteLump: Bad frame characters in lump %i", lump);

    if ((int)frame > maxframe)
        maxframe = frame;

    if (rotation == 0)
    {
        // the lump should be used for all rotations
        if (sprtemp[frame].rotate == false)
            I_Error("R_InstallSpriteLump: Sprite {} frame {} has multip rot=0 lump", std::string(name), 'A' + frame);

        if (sprtemp[frame].rotate == true)
            I_Error("R_InstallSpriteLump: Sprite {} frame {} has rotations and a rot=0 lump", std::string(name), 'A' + frame);

        sprtemp[frame].rotate = false;
        for (int32 r = 0; r < 8; r++)
        {
            sprtemp[frame].lump[r] = lump - firstspritelump;
            sprtemp[frame].flip[r] = (byte)flipped;
        }
        return;
    }

    // the lump is only used for one rotation
    if (sprtemp[frame].rotate == false)
        I_Error("R_InstallSpriteLump: Sprite {} frame {} has rotations and a rot=0 lump", std::string(name), 'A' + frame);

    sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;
    if (sprtemp[frame].lump[rotation] != -1)
        I_Error("R_InitSprites: Sprite {} : {} : {} has two lumps mapped to it", std::string(name), 'A' + frame, '1' + rotation);

    sprtemp[frame].lump[rotation] = lump - firstspritelump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}

// Pass a null terminated list of sprite names (4 chars exactly) to be used.
// Builds the sprite rotation matrices to account for horizontally flipped sprites.
// Will report an error if the lumps are inconsistent. 
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor, a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional letter/number appended.
// The rotation character can be 0 to signify no rotations.
void R_InitSpriteDefs(Doom* doom, const vector<string_view>& names)
{
    if (names.empty())
        return;

    numsprites = names.size();
    sprites = Z_Malloc<spritedef_t>(numsprites * sizeof(spritedef_t), PU_STATIC, nullptr);

    int start = firstspritelump - 1;
    int end = lastspritelump + 1;

    // scan all the lump names for each of the names, noting the highest frame letter.
    // Just compare 4 characters as ints
    for (int i = 0; i < numsprites; ++i)
    {
        auto name = names[i];
        std::memset(sprtemp, -1, sizeof(sprtemp));

        maxframe = -1;

        // scan the lumps, filling in the frames for whatever is found
        for (int l = start + 1; l < end; ++l)
        {
            auto& lump = WadManager::GetLump(l);
            if (lump.name.substr(0, 4) == name)
            {
                int frame = lump.name[4] - 'A';
                int rotation = lump.name[5] - '0';
                int patched = doom->IsModified() ? W_GetNumForName(lump.name) : l;

                R_InstallSpriteLump(name, patched, frame, rotation, false);

                if (lump.name[6])
                {
                    frame = lump.name[6] - 'A';
                    rotation = lump.name[7] - '0';
                    R_InstallSpriteLump(name, l, frame, rotation, true);
                }
            }
        }

        // check the frames that were found for completeness
        if (maxframe == -1)
        {
            sprites[i].numframes = 0;
            continue;
        }

        maxframe++;

        for (int frame = 0; frame < maxframe; frame++)
        {
            switch ((int)sprtemp[frame].rotate)
            {
            case -1:
                // no rotations were found for that frame at all
                I_Error("R_InitSpriteDefs: No patches found for %s frame %c", std::string(name), frame + 'A');
                break;

            case 0:
                // only the first rotation is needed
                break;

            case 1:
                // must have all 8 frames
                for (int rotation = 0; rotation < 8; rotation++)
                    if (sprtemp[frame].lump[rotation] == -1)
                        I_Error("R_InitSpriteDefs: Sprite {} frame {} is missing rotations", std::string(name), frame + 'A');
                break;
            }
        }

        // allocate space for the frames present and copy sprtemp to it
        sprites[i].numframes = maxframe;
        sprites[i].spriteframes = Z_Malloc<spriteframe_t>(maxframe * sizeof(spriteframe_t), PU_STATIC, nullptr);
        std::memcpy(sprites[i].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
    }
}

//
// GAME FUNCTIONS
//
vissprite_t	vissprites[MAXVISSPRITES];
vissprite_t* vissprite_p;
int		newvissprite;

// Called at program start.
void R_InitSprites(Doom* doom, const vector<string_view>& names)
{
    for (int i = 0; i < SCREENWIDTH; i++)
    {
        negonearray[i] = -1;
    }

    R_InitSpriteDefs(doom, names);
}

// Called at frame start.
void R_ClearSprites()
{
    vissprite_p = vissprites;
}


//
// R_NewVisSprite
//
vissprite_t	overflowsprite;

vissprite_t* R_NewVisSprite()
{
    if (vissprite_p == &vissprites[MAXVISSPRITES])
        return &overflowsprite;

    vissprite_p++;
    return vissprite_p - 1;
}



//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
short* mfloorclip;
short* mceilingclip;

fixed_t		spryscale;
fixed_t		sprtopscreen;

void R_DrawMaskedColumn(column_t* column)
{
    int		topscreen;
    int 	bottomscreen;
    fixed_t	basetexturemid;

    basetexturemid = dc_texturemid;

    for (; column->topdelta != 0xff; )
    {
        // calculate unclipped screen coordinates
        //  for post
        topscreen = sprtopscreen + spryscale * column->topdelta;
        bottomscreen = topscreen + spryscale * column->length;

        dc_yl = (topscreen + FRACUNIT - 1) >> FRACBITS;
        dc_yh = (bottomscreen - 1) >> FRACBITS;

        if (dc_yh >= mfloorclip[dc_x])
            dc_yh = mfloorclip[dc_x] - 1;
        if (dc_yl <= mceilingclip[dc_x])
            dc_yl = mceilingclip[dc_x] + 1;

        if (dc_yl <= dc_yh)
        {
            dc_source = (byte*)column + 3;
            dc_texturemid = basetexturemid - (column->topdelta << FRACBITS);
            // dc_source = (byte *)column + 3 - column->topdelta;

            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            colfunc();
        }
        column = (column_t*)((byte*)column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
}

//  mfloorclip and mceilingclip should also be set.
void R_DrawVisSprite(vissprite_t* vis, [[maybe_unused]] int x1, [[maybe_unused]] int x2)
{
    column_t* column;
    int			texturecolumn;
    fixed_t		frac;

    auto* patch = WadManager::GetLumpData<patch_t>(vis->patch + firstspritelump);

    dc_colormap = vis->colormap;

    if (!dc_colormap)
    {
        // nullptr colormap = shadow draw
        colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
        colfunc = R_DrawTranslatedColumn;
        dc_translation = translationtables - 256 +
            ((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT - 8));
    }

    dc_iscale = std::abs(vis->xiscale) >> detailshift;
    dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale)
    {
        texturecolumn = frac >> FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= (patch->width))
            I_Error("R_DrawSpriteRange: bad texturecolumn");
#endif
        column = (column_t*)((byte*)patch +
            (patch->columnofs[texturecolumn]));
        R_DrawMaskedColumn(column);
    }

    colfunc = basecolfunc;
}

// Generates a vissprite for a thing if it might be visible.
void R_ProjectSprite(mobj_t* thing)
{
    // transform the origin point
    auto tr_x = thing->x - viewx;
    auto tr_y = thing->y - viewy;

    auto gxt = FixedMul(tr_x, viewcos);
    auto gyt = -FixedMul(tr_y, viewsin);

    auto tz = gxt - gyt;

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    auto xscale = FixedDiv(projection, tz);

    gxt = -FixedMul(tr_x, viewsin);
    gyt = FixedMul(tr_y, viewcos);
    auto tx = -(gyt + gxt);

    // too far off the side?
    if (std::abs(tx) > (tz << 2))
        return;

    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if (thing->sprite >= numsprites)
        I_Error("R_ProjectSprite: invalid sprite number {}", static_cast<int32>(thing->sprite));
#endif
    auto sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
    if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectSprite: invalid sprite frame {} : {}", static_cast<int32>(thing->sprite), thing->frame);
#endif
    auto sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

    int32 lump = 0;
    bool flip = false;
    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        auto ang = R_PointToAngle(thing->x, thing->y);
        auto rot = (ang - thing->angle + (unsigned)(ANG45 / 2) * 9) >> 29;
        lump = sprframe->lump[rot];
        flip = sprframe->flip[rot];
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = sprframe->flip[0];
    }

    // calculate edges of the shape
    tx -= spriteoffset[lump];
    auto x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    tx += spritewidth[lump];
    auto x2 = ((centerxfrac + FixedMul(tx, xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    auto* vis = R_NewVisSprite();
    vis->mobjflags = thing->flags;
    vis->scale = xscale << detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = thing->z + spritetopoffset[lump];
    vis->texturemid = vis->gzt - viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
    auto iscale = FixedDiv(FRACUNIT, xscale);

    if (flip)
    {
        vis->startfrac = spritewidth[lump] - 1;
        vis->xiscale = -iscale;
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);
    vis->patch = lump;

    // get light level
    if (thing->flags & MF_SHADOW)
    {
        // shadow draw
        vis->colormap = nullptr;
    }
    else if (fixedcolormap)
    {
        // fixed map
        vis->colormap = fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = colormaps;
    }
    else
    {
        // diminished light
        auto index = xscale >> (LIGHTSCALESHIFT - detailshift);

        if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE - 1;

        vis->colormap = spritelights[index];
    }
}

// During BSP traversal, this adds sprites by sector.
void R_AddSprites(sector_t* sec)
{
    mobj_t* thing;
    int			lightnum;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;

    // Well, now it will be done.
    sec->validcount = validcount;

    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT) + extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS - 1];
    else
        spritelights = scalelight[lightnum];

    // Handle all things in sector.
    for (thing = sec->thinglist; thing; thing = thing->snext)
        R_ProjectSprite(thing);
}

void R_DrawPSprite(pspdef_t* psp)
{
    // decide which patch to use
#ifdef RANGECHECK
    if ((unsigned)psp->state->sprite >= numsprites)
        I_Error("R_ProjectSprite: invalid sprite number {} ", static_cast<int32>(psp->state->sprite));
#endif
    auto* sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ((psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectSprite: invalid sprite frame {} : {} ", static_cast<int32>(psp->state->sprite), psp->state->frame);
#endif
    auto sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

    auto lump = sprframe->lump[0];
    bool flip = sprframe->flip[0];

    // calculate edges of the shape
    auto tx = psp->sx - 160 * FRACUNIT;

    tx -= spriteoffset[lump];
    auto x1 = (centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS;

    // off the right side
    if (x1 > viewwidth)
        return;

    tx += spritewidth[lump];
    auto x2 = ((centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vissprite_t avis;
    auto vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER << FRACBITS) + FRACUNIT / 2 - (psp->sy - spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
    vis->scale = pspritescale << detailshift;

    if (flip)
    {
        vis->xiscale = -pspriteiscale;
        vis->startfrac = spritewidth[lump] - 1;
    }
    else
    {
        vis->xiscale = pspriteiscale;
        vis->startfrac = 0;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;

    if (viewplayer->powers[pw_invisibility] > 4 * 32
        || viewplayer->powers[pw_invisibility] & 8)
    {
        // shadow draw
        vis->colormap = nullptr;
    }
    else if (fixedcolormap)
    {
        // fixed color
        vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = colormaps;
    }
    else
    {
        // local light
        vis->colormap = spritelights[MAXLIGHTSCALE - 1];
    }

    R_DrawVisSprite(vis, vis->x1, vis->x2);
}

void R_DrawPlayerSprites()
{
    int		i;
    int		lightnum;
    pspdef_t* psp;

    // get light level
    lightnum =
        (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT)
        + extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS - 1];
    else
        spritelights = scalelight[lightnum];

    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    // add all active psprites
    for (i = 0, psp = viewplayer->psprites;
        i < NUMPSPRITES;
        i++, psp++)
    {
        if (psp->state)
            R_DrawPSprite(psp);
    }
}

//
// R_SortVisSprites
//
vissprite_t	vsprsortedhead;

void R_SortVisSprites()
{
    fixed_t		bestscale;

    int count = vissprite_p - vissprites;

    vissprite_t unsorted;
    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
        return;

    for (vissprite_t* ds = vissprites; ds < vissprite_p; ds++)
    {
        ds->next = ds + 1;
        ds->prev = ds - 1;
    }

    vissprites[0].prev = &unsorted;
    unsorted.next = &vissprites[0];
    (vissprite_p - 1)->next = &unsorted;
    unsorted.prev = vissprite_p - 1;

    // pull the vissprites out by scale
    //best = 0;		// shut up the compiler warning
    vissprite_t* best = nullptr;
    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
    for (int i = 0; i < count; i++)
    {
        bestscale = std::numeric_limits<decltype(bestscale)>::max();
        for (vissprite_t* ds = unsorted.next; ds != &unsorted; ds = ds->next)
        {
            if (ds->scale < bestscale)
            {
                bestscale = ds->scale;
                best = ds;
            }
        }
        best->next->prev = best->prev;
        best->prev->next = best->next;
        best->next = &vsprsortedhead;
        best->prev = vsprsortedhead.prev;
        vsprsortedhead.prev->next = best;
        vsprsortedhead.prev = best;
    }
}



//
// R_DrawSprite
//
void R_DrawSprite(vissprite_t* spr)
{
    drawseg_t* ds;
    short		clipbot[SCREENWIDTH];
    short		cliptop[SCREENWIDTH];
    int			x;
    int			r1;
    int			r2;
    fixed_t		scale;
    fixed_t		lowscale;
    int			silhouette;

    for (x = spr->x1; x <= spr->x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds = ds_p - 1; ds >= drawsegs; ds--)
    {
        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2
            || ds->x2 < spr->x1
            || (!ds->silhouette
                && !ds->maskedtexturecol))
        {
            // does not cover sprite
            continue;
        }

        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if (scale < spr->scale
            || (lowscale < spr->scale
                && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
        {
            // masked mid texture?
            if (ds->maskedtexturecol)
                R_RenderMaskedSegRange(ds, r1, r2);
            // seg is behind sprite
            continue;
        }


        // clip this piece of the sprite
        silhouette = ds->silhouette;

        if (spr->gz >= ds->bsilheight)
            silhouette &= ~SIL_BOTTOM;

        if (spr->gzt <= ds->tsilheight)
            silhouette &= ~SIL_TOP;

        if (silhouette == 1)
        {
            // bottom sil
            for (x = r1; x <= r2; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        }
        else if (silhouette == 2)
        {
            // top sil
            for (x = r1; x <= r2; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        }
        else if (silhouette == 3)
        {
            // both
            for (x = r1; x <= r2; x++)
            {
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }

    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1; x <= spr->x2; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite(spr, spr->x1, spr->x2);
}




//
// R_DrawMasked
//
void R_DrawMasked()
{
    vissprite_t* spr;
    drawseg_t* ds;

    R_SortVisSprites();

    if (vissprite_p > vissprites)
    {
        // draw all vissprites back to front
        for (spr = vsprsortedhead.next;
            spr != &vsprsortedhead;
            spr = spr->next)
        {

            R_DrawSprite(spr);
        }
    }

    // render any remaining masked mid textures
    for (ds = ds_p - 1; ds >= drawsegs; ds--)
        if (ds->maskedtexturecol)
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!viewangleoffset)
        R_DrawPlayerSprites();
}



