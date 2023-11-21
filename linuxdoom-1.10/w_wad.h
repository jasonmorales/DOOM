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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------
#pragma once

#include <stdint.h>

//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char identification[4];		
    int numlumps;
    int infotableofs;
    
} wadinfo_t;


typedef struct
{
    int			filepos;
    int			size;
    char		name[8];
    
} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char	name[8];
    int		handle;
    int		position;
    int		size;
} lumpinfo_t;


extern	void**		lumpcache;
extern	lumpinfo_t*	lumpinfo;
extern intptr_t numlumps;

void W_InitMultipleFiles (char** filenames);
void W_Reload (void);

intptr_t W_CheckNumForName(char* name);
intptr_t W_GetNumForName(char* name);

int	W_LumpLength(intptr_t lump);
void W_ReadLump(intptr_t lump, void *dest);

void* W_CacheLumpNum(intptr_t lump, int tag);
void* W_CacheLumpName(char* name, int tag);
