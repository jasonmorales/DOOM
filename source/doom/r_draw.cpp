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
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------
#include "i_video.h"

#include "doomdef.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "r_local.h"
#include "v_video.h"
#include "doomstat.h"
#include "d_main.h"
#include "r_main.h"

import std;


extern Doom* g_doom;


#define MAXWIDTH			1120
#define MAXHEIGHT			832

// status bar height at bottom of screen
#define SBARHEIGHT		32

// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about coordinates, not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one, and we need only the base address, and the
// total size == width*height*depth/8.,

byte* viewimage;
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;
byte* ylookup[MAXHEIGHT];
int		columnofs[MAXWIDTH];

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte		translations[3][256];


// Source is the top of the column to scale.
lighttable_t* dc_colormap;
int			dc_x;
int			dc_yl;
int			dc_yh;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;

// first pixel in a column (possibly virtual) 
const byte* dc_source;

// just for profiling 
int			dccount;

// A column is a vertical slice/span from a wall texture that, given the DOOM style restrictions
// on the view orientation, will always have constant z depth.
// Thus a special case loop for very fast rendering can be used. It has also been used with
// Wolfenstein 3D.
void R_DrawColumn()
{
    auto count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if (dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
        I_Error("R_DrawColumn: {} to {} at {}", dc_yl, dc_yh, dc_x);
#endif 

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows? 
    auto* dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling, which is the only mapping to be done.
    auto fracstep = dc_iscale;
    auto frac = dc_texturemid + (dc_yl - centery) * fracstep;

    // Inner loop that does the actual texture mapping, e.g. a DDA-lile scaling.
    // This is as fast as it gets.
    do
    {
        // Re-map color indices from wall texture column using a lighting/special effects LUT.
        *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]];

        dest += SCREENWIDTH;
        frac += fracstep;

    }
    while (count--);
}

void R_DrawColumnLow()
{
    auto count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if (dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
        I_Error("R_DrawColumn: {} to {} at {}", dc_yl, dc_yh, dc_x);
#endif

    // Blocky mode, need to multiply by 2.
    auto x = dc_x << 1;

    auto* dest = ylookup[dc_yl] + columnofs[x];
    auto* dest2 = ylookup[dc_yl] + columnofs[x + 1];

    auto fracstep = dc_iscale;
    auto frac = dc_texturemid + (dc_yl - centery) * fracstep;

    do
    {
        *dest2 = *dest = dc_colormap[dc_source[(frac >> FRACBITS) & 127]];

        dest += SCREENWIDTH;
        dest2 += SCREENWIDTH;
        frac += fracstep;
    }
    while (count--);
}

// Spectre/Invisibility.
static const int32 FUZZOFF = SCREENWIDTH;

int32 fuzzoffset[] =
{
    FUZZOFF,-FUZZOFF, FUZZOFF,-FUZZOFF, FUZZOFF, FUZZOFF,-FUZZOFF,
    FUZZOFF, FUZZOFF,-FUZZOFF, FUZZOFF, FUZZOFF, FUZZOFF,-FUZZOFF,
    FUZZOFF, FUZZOFF, FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF, FUZZOFF, FUZZOFF, FUZZOFF, FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF, FUZZOFF, FUZZOFF,-FUZZOFF,-FUZZOFF, FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF, FUZZOFF, FUZZOFF,
    FUZZOFF, FUZZOFF,-FUZZOFF, FUZZOFF, FUZZOFF,-FUZZOFF, FUZZOFF
};

int	fuzzpos = 0;

// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels from adjacent ones to left and right.
// Used with an all black colormap, this could create the SHADOW effect, i.e. spectres and
// invisible players.
void R_DrawFuzzColumn()
{
    // Adjust borders. Low... 
    if (!dc_yl)
        dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight - 1)
        dc_yh = viewheight - 2;

    auto count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if (dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
        I_Error("R_DrawFuzzColumn: {} to {} at {}", dc_yl, dc_yh, dc_x);
#endif

    auto x = dc_x * (detailshift ? 2 : 1);

    auto* dest = ylookup[dc_yl] + columnofs[x];
    auto* dest2 = ylookup[dc_yl] + columnofs[x + 1];

    // Looks like an attempt at dithering, using the colormap #6 (of 0-31, a bit brighter than
    // average).
    do
    {
        // Lookup framebuffer, and retrieve a pixel that is either one column left or right of
        // the current one.
        // Add index from colormap to index.
        *dest = colormaps[6 * 256 + dest[fuzzoffset[fuzzpos]]];
        if (detailshift)
            *dest2 = colormaps[6 * 256 + dest2[fuzzoffset[fuzzpos]]];

        // Clamp table lookup index.
        fuzzpos = (fuzzpos + 1) % std::size(fuzzoffset);

        dest += SCREENWIDTH;
        dest2 += SCREENWIDTH;
    }
    while (count--);
}

// Used to draw player sprites with the green colorramp mapped to others.
// Could be used with different translation tables, e.g. the lighter colored version of the
// BaronOfHell, the HellKnight, uses identical sprites, kinda brightened up.
byte* dc_translation;
byte* translationtables;

void R_DrawTranslatedColumn()
{
    auto count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK 
    if (dc_x >= SCREENWIDTH || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
        I_Error("R_DrawTranslatedColumn: {} to {} at {}", dc_yl, dc_yh, dc_x);
#endif 

    auto x = dc_x;
    if (detailshift)
        x <<= 1;

    auto* dest = ylookup[dc_yl] + columnofs[x];
    auto* dest2 = ylookup[dc_yl] + columnofs[x + 1];

    // Looks familiar.
    auto fracstep = dc_iscale;
    auto frac = dc_texturemid + (dc_yl - centery) * fracstep;

    // Here we do an additional index re-mapping.
    do
    {
        // Translation tables are used to map certain colorramps to other ones, used with PLAY
        // sprites.
        // Thus the "green" ramp of the player 0 sprite is mapped to gray, red, black/indigo. 
        *dest = dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]];
        if (detailshift)
            *dest2 = dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]];

        dest += SCREENWIDTH;
        dest2 += SCREENWIDTH;
        frac += fracstep;
    }
    while (count--);
}

// Creates the translation tables to map the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
void R_InitTranslationTables()
{
    translationtables = Z_Malloc(256 * 3 + 255, PU_STATIC, 0);
    translationtables = (byte*)(((intptr_t)translationtables + 255) & ~255);

    // translate just the 16 green colors
    for (int i = 0; i < 256; ++i)
    {
        if (i >= 0x70 && i <= 0x7f)
        {
            // map green ramp to gray, brown, red
            translationtables[i] = 0x60 + (i & 0xf);
            translationtables[i + 256] = 0x40 + (i & 0xf);
            translationtables[i + 512] = 0x20 + (i & 0xf);
        }
        else
        {
            // Keep all other colors as is.
            translationtables[i] = translationtables[i + 256] = translationtables[i + 512] = (byte)i;
        }
    }
}

// With DOOM style restrictions on view orientation, the floors and ceilings consist of horizontal
// slices or spans with constant z depth.
// However, rotation around the world z axis is possible, thus this mapping, while simpler and
// faster than perspective correct texture mapping, has to traverse the texture at an angle in all
// but a few cases.
// In consequence, flats are not stored by column (like walls), and the inner loop has to step in
// texture space u and v.
int			ds_y;
int			ds_x1;
int			ds_x2;

lighttable_t* ds_colormap;

fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;

// start of a 64*64 tile image 
const byte* ds_source;

// just for profiling
int			dscount;

// Draws the actual span.
void R_DrawSpan()
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    byte* dest;
    int			count;
    int			spot;

#ifdef RANGECHECK 
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2 >= SCREENWIDTH
        || (unsigned)ds_y>SCREENHEIGHT)
    {
        I_Error("R_DrawSpan: {} to {} at {}",
            ds_x1, ds_x2, ds_y);
    }
    //	dscount++; 
#endif 


    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    dest = ylookup[ds_y] + columnofs[ds_x1];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
        // Current texture index in u,v.
        spot = ((yfrac >> (16 - 6)) & (63 * 64)) + ((xfrac >> 16) & 63);

        // Lookup pixel from flat texture tile,
        //  re-index using light/colormap.
        *dest++ = ds_colormap[ds_source[spot]];

        // Next step in u,v.
        xfrac += ds_xstep;
        yfrac += ds_ystep;

    } while (count--);
}

void R_DrawSpanLow()
{
#ifdef RANGECHECK 
    if (ds_x2 < ds_x1 || ds_x1 < 0 || ds_x2 >= SCREENWIDTH || ds_y > SCREENHEIGHT)
        I_Error("R_DrawSpanLow: {} to {} at {}", ds_x1, ds_x2, ds_y);
#endif 

    auto xfrac = ds_xfrac;
    auto yfrac = ds_yfrac;

    // Blocky mode, need to multiply by 2.
    ds_x1 <<= 1;
    ds_x2 <<= 1;

    auto dest = ylookup[ds_y] + columnofs[ds_x1];

    auto count = ds_x2 - ds_x1;
    do
    {
        auto spot = ((yfrac >> (16 - 6)) & (63 * 64)) + ((xfrac >> 16) & 63);
        // Lowres/blocky mode does it twice, while scale is adjusted appropriately.
        *dest++ = ds_colormap[ds_source[spot]];
        *dest++ = ds_colormap[ds_source[spot]];

        xfrac += ds_xstep;
        yfrac += ds_ystep;

    }
    while (count--);
}

// Creats lookup tables that avoid multiplies and other hazzles for getting the framebuffer
// address of a pixel to draw.
void R_InitBuffer(int32 width, int32 height)
{
    // Handle resize, e.g. smaller view windows with border and/or status bar.
    viewwindowx = (SCREENWIDTH - width) >> 1;

    // Column offset. For windows.
    for (int32 i = 0; i < width; ++i)
        columnofs[i] = viewwindowx + i;

    // Samw with base row offset.
    if (width == SCREENWIDTH)
        viewwindowy = 0;
    else
        viewwindowy = (SCREENHEIGHT - SBARHEIGHT - height) >> 1;

    // Preclaculate all row offsets.
    for (int32 i = 0; i < height; ++i)
        ylookup[i] = g_doom->GetVideo()->GetScreen(0) + (i + viewwindowy) * SCREENWIDTH;
}

// Fills the back screen with a pattern for variable screen sizes
// Also draws a beveled edge.
void R_FillBackScreen()
{
    int		x;
    int		y;

    // DOOM border patch.
    char	name1[] = "FLOOR7_2";

    // DOOM II border patch.
    char	name2[] = "GRNROCK";

    char* name;

    if (scaledviewwidth == 320)
        return;

    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
        name = name2;
    else
        name = name1;

    auto* src = WadManager::GetLumpData<byte>(name);
    auto* dest = g_doom->GetVideo()->GetScreen(1);

    for (y = 0; y < SCREENHEIGHT - SBARHEIGHT; y++)
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

    auto* patch = WadManager::GetLumpData<patch_t>("BRDR_T");

    for (x = 0; x < scaledviewwidth; x += 8)
        g_doom->GetVideo()->DrawPatch(viewwindowx + x, viewwindowy - 8, 1, patch);
    patch = WadManager::GetLumpData<patch_t>("BRDR_B");

    for (x = 0; x < scaledviewwidth; x += 8)
        g_doom->GetVideo()->DrawPatch(viewwindowx + x, viewwindowy + viewheight, 1, patch);
    patch = WadManager::GetLumpData<patch_t>("BRDR_L");

    for (y = 0; y < viewheight; y += 8)
        g_doom->GetVideo()->DrawPatch(viewwindowx - 8, viewwindowy + y, 1, patch);
    patch = WadManager::GetLumpData<patch_t>("BRDR_R");

    for (y = 0; y < viewheight; y += 8)
        g_doom->GetVideo()->DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + y, 1, patch);

    // Draw beveled edge. 
    g_doom->GetVideo()->DrawPatch(viewwindowx - 8, viewwindowy - 8, 1, WadManager::GetLumpData<patch_t>("BRDR_TL"));

    g_doom->GetVideo()->DrawPatch(viewwindowx + scaledviewwidth, viewwindowy - 8, 1, WadManager::GetLumpData<patch_t>("BRDR_TR"));

    g_doom->GetVideo()->DrawPatch(viewwindowx - 8, viewwindowy + viewheight, 1, WadManager::GetLumpData<patch_t>("BRDR_BL"));

    g_doom->GetVideo()->DrawPatch(viewwindowx + scaledviewwidth, viewwindowy + viewheight, 1, WadManager::GetLumpData<patch_t>("BRDR_BR"));
}

// Copy a screen buffer.
void R_VideoErase(unsigned ofs, int count)
{
    // LFB copy.
    // This might not be a good idea if memcpy
    //  is not optiomal, e.g. byte by byte on
    //  a 32bit CPU, as GNU GCC/Linux libc did
    //  at one point.
    memcpy(g_doom->GetVideo()->GetScreen(0) + ofs, g_doom->GetVideo()->GetScreen(1) + ofs, count);
}

// Draws the border around the view
//  for different size windows?
void R_DrawViewBorder()
{
    int		top;
    int		side;
    int		ofs;
    int		i;

    if (scaledviewwidth == SCREENWIDTH)
        return;

    top = ((SCREENHEIGHT - SBARHEIGHT) - viewheight) / 2;
    side = (SCREENWIDTH - scaledviewwidth) / 2;

    // copy top and one line of left side 
    R_VideoErase(0, top * SCREENWIDTH + side);

    // copy one line of right side and bottom 
    ofs = (viewheight + top) * SCREENWIDTH - side;
    R_VideoErase(ofs, top * SCREENWIDTH + side);

    // copy sides using wraparound 
    ofs = top * SCREENWIDTH + SCREENWIDTH - side;
    side <<= 1;

    for (i = 1; i < viewheight; i++)
    {
        R_VideoErase(ofs, side);
        ofs += SCREENWIDTH;
    }
}
