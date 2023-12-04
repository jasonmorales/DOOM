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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------
import std;
#define __STD_MODULE__

#include "doomtype.h"

#include "i_system.h"
#include "m_swap.h"
#include "w_wad.h"
#include "z_zone.h"

#include <malloc.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <ctype.h>

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t* lumpinfo;
intptr_t numlumps = 0;

void** lumpcache;
#define strcmpi	_stricmp

#if 0
void strupr(char* s)
{
    while (*s) { *s = (char)toupper(*s); s++; }
}

int filelength(int handle)
{
    struct _stat fileinfo;
    if (_fstat(handle, &fileinfo) == -1)
        I_Error("Error fstating");

    return fileinfo.st_size;
}
#endif

void ExtractFileBase(const char* path, char* dest)
{
    const char* src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path
        && *(src - 1) != '\\'
        && *(src - 1) != '/')
    {
        src--;
    }

    // copy up to eight characters
    memset(dest, 0, 8);
    int length = 0;

    while (*src && *src != '.')
    {
        if (++length == 9)
            I_Error("Filename base of {} >8 chars", path);

        *dest++ = (char)toupper(*src++);
    }
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

intptr_t reloadlump;
const char* reloadname;

void W_AddFile(const char* filename)
{
    // open the file and add to directory

    // handle reload indicator.
    if (filename[0] == '~')
    {
        filename++;
        reloadname = filename;
        reloadlump = numlumps;
    }

    int handle = -1;
    if (_sopen_s(&handle, filename, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD) == -1)
    {
        printf(" couldn't open %s\n", filename);
        return;
    }

    printf(" adding %s\n", filename);
    intptr_t startlump = numlumps;

    filelump_t* fileinfo = nullptr;
    if (_stricmp(filename + strlen(filename) - 3, "wad"))
    {
        // single lump file
        filelump_t singleinfo;
        fileinfo = &singleinfo;
        singleinfo.filepos = 0;
        singleinfo.size = (_filelength(handle));
        ExtractFileBase(filename, singleinfo.name);
        numlumps++;
    }
    else
    {
        // WAD file
        wadinfo_t header;
        _read(handle, &header, sizeof(header));
        if (strncmp(header.identification, "IWAD", 4))
        {
            // Homebrew levels?
            if (strncmp(header.identification, "PWAD", 4))
            {
                I_Error("Wad file {} doesn't have IWAD "
                    "or PWAD id\n", filename);
            }
        }
        header.numlumps = header.numlumps;
        header.infotableofs = (header.infotableofs);
        int length = header.numlumps * sizeof(filelump_t);
        fileinfo = static_cast<filelump_t*>(alloca(length));
        _lseek(handle, header.infotableofs, SEEK_SET);
        _read(handle, fileinfo, length);
        numlumps += header.numlumps;
    }

    // Fill in lumpinfo
    lumpinfo = static_cast<decltype(lumpinfo)>(realloc(lumpinfo, numlumps * sizeof(lumpinfo_t)));

    if (!lumpinfo)
        I_Error("Couldn't realloc lumpinfo");

    lumpinfo_t* lump_p = &lumpinfo[startlump];

    int storehandle = reloadname ? -1 : handle;

    for (int64_t i = startlump; i < numlumps; i++, lump_p++, fileinfo++)
    {
        lump_p->handle = storehandle;
        lump_p->position = fileinfo->filepos;
        lump_p->size = fileinfo->size;
        memset(lump_p->name, 0, 9);
        strncpy_s(lump_p->name, fileinfo->name, 8);
    }

    if (reloadname)
        _close(handle);
}

//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload()
{
    if (!reloadname)
        return;

    int handle = -1;
    if ((_sopen_s(&handle, reloadname, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD)) == -1)
        I_Error("W_Reload: couldn't open {}", reloadname);

    wadinfo_t header;
    _read(handle, &header, sizeof(header));
    intptr_t lumpcount = header.numlumps;
    header.infotableofs = (header.infotableofs);
    size_t length = lumpcount * sizeof(filelump_t);
    filelump_t* fileinfo = static_cast<decltype(fileinfo)>(alloca(length));
    _lseek(handle, header.infotableofs, SEEK_SET);
    _read(handle, fileinfo, (unsigned int)length);

    // Fill in lumpinfo
    lumpinfo_t* lump_p = &lumpinfo[reloadlump];

    for (intptr_t i = reloadlump; i < reloadlump + lumpcount; i++, lump_p++, fileinfo++)
    {
        if (lumpcache[i])
            Z_Free(lumpcache[i]);

        lump_p->position = (fileinfo->filepos);
        lump_p->size = (fileinfo->size);
    }

    _close(handle);
}

// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles(const vector<filesys::path>& files)
{
    // open all the files, load headers, and count lumps
    numlumps = 0;

    // will be realloced as lumps are added
    lumpinfo = static_cast<decltype(lumpinfo)>(malloc(1));

    for (auto& file : files)
        W_AddFile(file.string().c_str());

    if (!numlumps)
        I_Error("W_InitFiles: no files found");

    // set up caching
    size_t size = numlumps * sizeof(*lumpcache);
    lumpcache = static_cast<decltype(lumpcache)>(malloc(size));

    if (!lumpcache)
        I_Error("Couldn't allocate lumpcache");

    memset(lumpcache, 0, size);
}

uint64_t W_NumLumps()
{
    return numlumps;
}

// Returns -1 if name not found.
int32 W_CheckNumForName(string_view name)
{
    union
    {
        char s[9] = { 0 };
        int	x[2];
    } name8;

    // make the name into two integers for easy compares
    strncpy_s(name8.s, name.data(), 8);

    // in case the name was a fill 8 chars
    name8.s[8] = 0;

    // case insensitive
    _strupr_s(name8.s);

    int v1 = name8.x[0];
    int v2 = name8.x[1];

    // scan backwards so patch lump files take precedence
    lumpinfo_t* lump_p = lumpinfo + numlumps;

    while (lump_p-- != lumpinfo)
    {
        if (*(int*)lump_p->name == v1
            && *(int*)&lump_p->name[4] == v2)
        {
            return static_cast<int32>(lump_p - lumpinfo);
        }
    }

    // TFB. Not found.
    return -1;
}

// Calls W_CheckNumForName, but bombs out if not found.
int32 W_GetNumForName(string_view name)
{
    auto i = W_CheckNumForName(name);

    if (i == -1)
        I_Error("W_GetNumForName: {} not found!", name);

    return i;
}

// Returns the buffer size needed to load the given lump.
int W_LumpLength(intptr_t lump)
{
    if (lump >= numlumps)
        I_Error("W_LumpLength: {} >= numlumps", lump);

    return lumpinfo[lump].size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(intptr_t lump, void* dest)
{
    if (lump >= numlumps)
        I_Error("W_ReadLump: {} >= numlumps", lump);

    lumpinfo_t* l = lumpinfo + lump;

    int handle = l->handle;
    if (handle == -1)
    {
        // reloadable file, so use open / read / close
        if (_sopen_s(&handle, reloadname, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD) == -1)
            I_Error("W_ReadLump: couldn't open {}", reloadname);
    }

    _lseek(handle, l->position, SEEK_SET);
    int c = _read(handle, dest, l->size);

    if (c < l->size)
        I_Error("W_ReadLump: only read {} of {} on lump {}", c, l->size, lump);

    if (l->handle == -1)
        _close(handle);
}

//
// W_CacheLumpNum
//
void* W_CacheLumpNum_internal(intptr_t lump, int tag)
{
    if (lump >= numlumps)
        I_Error("W_CacheLumpNum: {} >= numlumps", lump);

    if (!lumpcache[lump])
    {
        // read the lump in
        Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
        W_ReadLump(lump, lumpcache[lump]);
    }
    else
    {
        //printf ("cache hit on lump %i\n",lump);
        Z_ChangeTag(lumpcache[lump], tag);
    }

    return lumpcache[lump];
}

//
// W_Profile
//
int		info[2500][10];

void W_Profile()
{
    static int profilecount = 0;

    for (int i = 0; i < numlumps; i++)
    {
        void* lump = lumpcache[i];
        if (!lump)
            continue;

        memblock_t* block = (memblock_t*)((byte*)lump - sizeof(memblock_t));
        char ch = (block->tag < PU_PURGELEVEL) ? 'S' : 'P';
        info[i][profilecount] = ch;
    }
    profilecount++;

    FILE* f = nullptr;
    fopen_s(&f, "waddump.txt", "w");

    char name[9] = { 0 };

    int j = 0;
    for (int i = 0; i < numlumps; i++)
    {
        memcpy(name, lumpinfo[i].name, 8);

        for (j = 0; j < 8; j++)
            if (!name[j])
                break;

        for (; j < 8; j++)
            name[j] = ' ';

        fprintf(f, "%s ", name);

        for (j = 0; j < profilecount; j++)
            fprintf(f, "    %c", info[i][j]);

        fprintf(f, "\n");
    }

    fclose(f);
}
