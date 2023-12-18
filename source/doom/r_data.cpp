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
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//	Preparation of data for rendering,
//	generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------
import std;

#include "i_system.h"
#include "z_zone.h"

#include "m_swap.h"

#include "w_wad.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#include "r_data.h"

#include <malloc.h>


// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.

// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
struct mappatch_t
{
    short	originx;
    short	originy;
    short	patch;
    short	stepdir;
    short	colormap;
};

// Texture definition.
// A DOOM wall texture is a list of patches which are to be combined in a predefined order.
struct maptexture_t
{
    char		name[8];
    int32		masked;
    short		width;
    short		height;
    //void		**columndirectory;	// OBSOLETE
    int32_t _dummy;
    short		patchcount;
    mappatch_t	patches[1];
};

// A single patch from a texture definition, basically a rectangular area within the texture
// rectangle.
struct texpatch_t
{
    // Block origin (always UL), which has already accounted for the internal origin of the patch.
    int		originx;
    int		originy;
    int		patch;
};

// A maptexturedef_t describes a rectangular texture, which is composed of one or more mappatch_t
// structures that arrange graphic patches.
struct texture_t
{
    // Keep name for switch changing, etc.
    char	name[8];
    short	width;
    short	height;

    // All the patches[patchcount]
    //  are drawn back to front into the cached texture.
    short	patchcount;
    texpatch_t	patches[1];
};

int		firstflat;
int		lastflat;
int		numflats;

int		firstpatch;
int		lastpatch;
int		numpatches;

int		firstspritelump;
int		lastspritelump;
int		numspritelumps;

int		numtextures;
texture_t** textures;


int* texturewidthmask;
// needed for texture pegging
fixed_t* textureheight;
int* texturecompositesize;
short** texturecolumnlump;
unsigned short** texturecolumnofs;
byte** texturecomposite;

// for global animation
int* flattranslation;
int* texturetranslation;

// needed for pre rendering
fixed_t* spritewidth;
fixed_t* spriteoffset;
fixed_t* spritetopoffset;

lighttable_t* colormaps;


//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//



//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
void
R_DrawColumnInCache
(column_t* patch,
    byte* cache,
    int		originy,
    int		cacheheight)
{
    int		count;
    int		position;
    byte* source;
    byte* dest;

    dest = (byte*)cache + 3;

    while (patch->topdelta != 0xff)
    {
        source = (byte*)patch + 3;
        count = patch->length;
        position = originy + patch->topdelta;

        if (position < 0)
        {
            count += position;
            position = 0;
        }

        if (position + count > cacheheight)
            count = cacheheight - position;

        if (count > 0)
            memcpy(cache + position, source, count);

        patch = (column_t*)((byte*)patch + patch->length + 4);
    }
}



//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
void R_GenerateComposite(int texnum)
{
    texture_t* texture;
    texpatch_t* patch;
    int			x;
    int			x1;
    int			x2;
    int			i;
    column_t* patchcol;
    short* collump;
    unsigned short* colofs;

    texture = textures[texnum];

    auto block = static_cast<byte*>(Z_Malloc(texturecompositesize[texnum], PU_STATIC, &texturecomposite[texnum]));

    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    // Composite the columns together.
    patch = texture->patches;

    for (i = 0, patch = texture->patches;
        i < texture->patchcount;
        i++, patch++)
    {
        auto* realpatch = WadManager::GetLumpData<patch_t>(patch->patch);
        x1 = patch->originx;
        x2 = x1 + (realpatch->width);

        if (x1 < 0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;

        for (; x < x2; x++)
        {
            // Column does not have multiple patches?
            if (collump[x] >= 0)
                continue;

            patchcol = (column_t*)((byte*)realpatch
                + (realpatch->columnofs[x - x1]));
            R_DrawColumnInCache(patchcol,
                block + colofs[x],
                patch->originy,
                texture->height);
        }

    }

    // Now that the texture has been built in column cache,
    //  it is purgable from zone memory.
    Z_ChangeTag(block, PU_CACHE);
}

void R_GenerateLookup(int texnum)
{
    short* collump;
    unsigned short* colofs;

    texture_t* texture = textures[texnum];

    // Composited texture not created yet.
    texturecomposite[texnum] = 0;

    texturecompositesize[texnum] = 0;
    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    byte* patchcount = (byte*)alloca(texture->width);
    memset(patchcount, 0, texture->width);
    texpatch_t* patch = texture->patches;

    for (int i = 0; i < texture->patchcount; i++, patch++)
    {
        auto* realpatch = WadManager::GetLumpData<patch_t>(patch->patch);
        int x1 = patch->originx;
        int x2 = x1 + (realpatch->width);

        int x = (x1 < 0) ? 0 : x1;

        if (x2 > texture->width)
            x2 = texture->width;

        for (; x < x2; x++)
        {
            patchcount[x]++;
            collump[x] = static_cast<int16>(patch->patch);
            colofs[x] = static_cast<uint16>(realpatch->columnofs[x - x1]) + 3;
        }
    }

    for (int x = 0; x < texture->width; x++)
    {
        if (!patchcount[x])
        {
            printf("R_GenerateLookup: column without a patch (%s)\n", texture->name);
            return;
        }
        // I_Error ("R_GenerateLookup: column without a patch");

        if (patchcount[x] > 1)
        {
            // Use the cached block.
            collump[x] = -1;
            colofs[x] = static_cast<uint16>(texturecompositesize[texnum]);

            if (texturecompositesize[texnum] > 0x10000 - texture->height)
                I_Error("R_GenerateLookup: texture {} is >64k", texnum);

            texturecompositesize[texnum] += texture->height;
        }
    }
}

const byte* R_GetColumn(int tex, int col)
{
    int		lump;
    int		ofs;

    col &= texturewidthmask[tex];
    lump = texturecolumnlump[tex][col];
    ofs = texturecolumnofs[tex][col];

    if (lump > 0)
        return WadManager::GetLumpData<byte>(lump) + ofs;

    if (!texturecomposite[tex])
        R_GenerateComposite(tex);

    return texturecomposite[tex] + ofs;
}

// Initializes the texture list with the textures from the world map.
void R_InitTextures()
{
    // Load the patch names from pnames.lmp.
    auto* names = WadManager::GetLumpData<char>("PNAMES");
    auto nummappatches = *reinterpret_cast<const int32*>(names);
    auto* name_p = names + sizeof(int32);
    vector<int32> patchLookup(nummappatches);

    for (int i = 0; i < nummappatches; i++)
    {
        string name{name_p + i * 8, 8};
        patchLookup[i] = WadManager::GetLumpId(name.trim().to_upper());
    }

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps, TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    auto* texture1 = WadManager::FindLump("TEXTURE1");
    if (!texture1)
        I_Error("R_InitTextures: Missing lump TEXTURE1");

    auto* maptex1 = texture1->as<int32>();
    auto* maptex = maptex1;
    auto numtextures1 = *maptex;
    int32 maxoff = texture1->size;
    auto* directory = maptex + 1;

    const int32* maptex2 = nullptr;
    int32  numtextures2 = 0;
    int32 maxoff2 = 0;
    if (auto* texture2 = WadManager::FindLump("TEXTURE2"))
    {
        maptex2 = texture2->as<int32>();
        numtextures2 = *maptex2;
        maxoff2 = texture2->size;
    }

    numtextures = numtextures1 + numtextures2;

    textures = Z_Malloc<texture_t*>(numtextures * sizeof(texture_t*), PU_STATIC, 0);
    texturecolumnlump = Z_Malloc<short*>(numtextures * sizeof(short*), PU_STATIC, 0);
    texturecolumnofs = Z_Malloc<unsigned short*>(numtextures * sizeof(unsigned short*), PU_STATIC, 0);
    texturecomposite = Z_Malloc<byte*>(numtextures * sizeof(byte*), PU_STATIC, 0);
    texturecompositesize = Z_Malloc<int>(numtextures * sizeof(int), PU_STATIC, 0);
    texturewidthmask = Z_Malloc<int>(numtextures * sizeof(int), PU_STATIC, 0);
    textureheight = Z_Malloc<fixed_t>(numtextures * sizeof(fixed_t), PU_STATIC, 0);

    int32 totalwidth = 0;

    //	Really complex printing shit...
    auto temp1 = W_GetNumForName("S_START");  // P_???????
    auto temp2 = W_GetNumForName("S_END") - 1;
    auto temp3 = ((temp2 - temp1 + 63) / 64) + ((numtextures + 63) / 64);
    printf("[");
    for (int i = 0; i < temp3; i++)
        printf(" ");
    printf("         ]");
    for (int i = 0; i < temp3; i++)
        printf("\x8");
    printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");

    for (int32 i = 0; i < numtextures; i++, directory++)
    {
        if (!(i & 63))
            printf(".");

        if (i == numtextures1)
        {
            // Start looking in second texture file.
            maptex = maptex2;
            maxoff = maxoff2;
            directory = maptex + 1;
        }

        int32 offset = *directory;

        if (offset > maxoff)
            I_Error("R_InitTextures: bad texture directory");

        maptexture_t* mtexture = (maptexture_t*)((byte*)maptex + offset);

        auto* texture = textures[i] = Z_Malloc<texture_t>(sizeof(texture_t) + sizeof(texpatch_t) * (mtexture->patchcount - 1), PU_STATIC, 0);

        texture->width = mtexture->width;
        texture->height = mtexture->height;
        texture->patchcount = mtexture->patchcount;

        memcpy(texture->name, mtexture->name, sizeof(texture->name));
        mappatch_t* mpatch = &mtexture->patches[0];
        texpatch_t* patch = &texture->patches[0];

        int j = 0;
        for (; j < texture->patchcount; j++, mpatch++, patch++)
        {
            patch->originx = mpatch->originx;
            patch->originy = mpatch->originy;
            patch->patch = patchLookup[mpatch->patch];
            if (patch->patch == -1)
                I_Error("R_InitTextures: Missing patch in texture {}", texture->name);
        }

        texturecolumnlump[i] = Z_Malloc<short>(texture->width * 2, PU_STATIC, 0);
        texturecolumnofs[i] = Z_Malloc<unsigned short>(texture->width * 2, PU_STATIC, 0);

        j = 1;
        while (j * 2 <= texture->width)
            j <<= 1;

        texturewidthmask[i] = j - 1;
        textureheight[i] = texture->height << FRACBITS;

        totalwidth += texture->width;
    }

    // Precalculate whatever possible.	
    for (int i = 0; i < numtextures; i++)
        R_GenerateLookup(i);

    // Create translation table for global animation.
    texturetranslation = Z_Malloc<int>((numtextures + 1) * sizeof(int*), PU_STATIC, 0);

    for (int i = 0; i < numtextures; i++)
        texturetranslation[i] = i;
}

void R_InitFlats()
{
    firstflat = W_GetNumForName("F_START") + 1;
    lastflat = W_GetNumForName("F_END") - 1;
    numflats = lastflat - firstflat + 1;

    // Create translation table for global animation.
    flattranslation = Z_Malloc<int>((numflats + 1) * 4, PU_STATIC, 0);

    for (int i = 0; i < numflats; i++)
        flattranslation[i] = i;
}

//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps()
{
    firstspritelump = W_GetNumForName("S_START") + 1;
    lastspritelump = W_GetNumForName("S_END") - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = Z_Malloc<fixed_t>(numspritelumps * 4, PU_STATIC, 0);
    spriteoffset = Z_Malloc<fixed_t>(numspritelumps * 4, PU_STATIC, 0);
    spritetopoffset = Z_Malloc<fixed_t>(numspritelumps * 4, PU_STATIC, 0);

    for (int32 i = 0; i < numspritelumps; ++i)
    {
        if (!(i & 63))
            printf(".");

        auto* patch = WadManager::GetLumpData<patch_t>(firstspritelump + i);
        spritewidth[i] = (patch->width) << FRACBITS;
        spriteoffset[i] = (patch->leftoffset) << FRACBITS;
        spritetopoffset[i] = (patch->topoffset) << FRACBITS;
    }
}

void R_InitColormaps()
{
    // Load in the light tables,  256 byte align tables.
    auto& lump = WadManager::GetLump("COLORMAP");
    colormaps = Z_Malloc(lump.size + 255, PU_STATIC, 0);
    colormaps = (byte*)(((intptr_t)colormaps + 255) & ~0xff);
    memcpy(colormaps, lump.data, lump.size);
}

// Locates all the lumps that will be used by all views
// Must be called after W_Init.
void R_InitData()
{
    R_InitTextures();
    printf("\nInitTextures");
    R_InitFlats();
    printf("\nInitFlats");
    R_InitSpriteLumps();
    printf("\nInitSprites");
    R_InitColormaps();
    printf("\nInitColormaps");
}

// Retrieval, get a flat number for a flat name.
int32 R_FlatNumForName(string_view name)
{
    name = name.substr(0, 8);
    auto i = WadManager::GetLumpId(name);
    if (i == INVALID_ID)
    {
        I_Error("R_FlatNumForName: {} not found", name);
    }

    return i - firstflat;
}

// Check whether texture is available.
// Filter out NoTexture indicator.
int	R_CheckTextureNumForName(const char* name)
{
    int		i;

    // "NoTexture" marker.
    if (name[0] == '-')
        return 0;

    for (i = 0; i < numtextures; i++)
        if (!_strnicmp(textures[i]->name, name, 8))
            return i;

    return -1;
}

// Calls R_CheckTextureNumForName, aborts with error message.
int	R_TextureNumForName(const char* name)
{
    int i = R_CheckTextureNumForName(name);
    if (i == -1)
    {
        I_Error("R_TextureNumForName: {} not found", name);
    }
    return i;
}

// Preloads all relevant graphics for the level.
int		flatmemory;
int		texturememory;
int		spritememory;

void R_PrecacheLevel()
{
    texture_t* texture;
    thinker_t* th;
    spriteframe_t* sf;

    if (demoplayback)
        return;

    // Precache flats.
    auto* flatpresent = static_cast<char*>(_malloca(numflats));
    memset(flatpresent, 0, numflats);

    for (int32 i = 0; i < numsectors; i++)
    {
        flatpresent[sectors[i].floorpic] = 1;
        flatpresent[sectors[i].ceilingpic] = 1;
    }

    flatmemory = 0;

    for (int32 i = 0; i < numflats; ++i)
    {
        if (!flatpresent[i])
            continue;

        auto& lump = WadManager::GetLump(firstflat + i);
        flatmemory += lump.size;
        WadManager::GetLumpData<patch_t>(lump.id);
    }

    _freea(flatpresent);

    // Precache textures.
    auto* texturepresent = static_cast<char*>(_malloca(numtextures));
    memset(texturepresent, 0, numtextures);

    for (int32 i = 0; i < numsides; i++)
    {
        texturepresent[sides[i].toptexture] = 1;
        texturepresent[sides[i].midtexture] = 1;
        texturepresent[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to indicate a sky floor/ceiling as a flat, while the sky
    // texture is stored like a wall texture, with an episode dependent name.
    texturepresent[skytexture] = 1;

    texturememory = 0;
    for (int32 i = 0; i < numtextures; i++)
    {
        if (!texturepresent[i])
            continue;

        texture = textures[i];

        for (int32 j = 0; j < texture->patchcount; j++)
        {
            auto& lump = WadManager::GetLump(texture->patches[j].patch);
            texturememory += lump.size;
            WadManager::GetLumpData<patch_t>(lump.id);
        }
    }
    _freea(texturepresent);

    // Precache sprites.
    char* spritepresent = nullptr;
    if (numsprites > 0)
    {
        spritepresent = static_cast<char*>(_malloca(numsprites));
        memset(spritepresent, 0, numsprites);
    }

    if (!spritepresent)
        return;

    for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if (th->function.acp1 == (actionf_p1)P_MobjThinker)
            spritepresent[((mobj_t*)th)->sprite] = 1;
    }

    spritememory = 0;
    for (int32 i = 0; i < numsprites; i++)
    {
        if (!spritepresent[i])
            continue;

        for (int32 j = 0; j < sprites[i].numframes; j++)
        {
            sf = &sprites[i].spriteframes[j];
            for (int32 k = 0; k < 8; k++)
            {
                auto& lump = WadManager::GetLump(firstspritelump + sf->lump[k]);
                spritememory += lump.size;
                WadManager::GetLumpData<patch_t>(lump.id);
            }
        }
    }

    _freea(spritepresent);
}
