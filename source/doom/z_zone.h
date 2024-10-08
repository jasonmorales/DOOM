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
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//	Remark: this was the only stuff that, according
//	 to John Carmack, might have been useful for
//	 Quake.
//
//---------------------------------------------------------------------
#pragma once

import nstd;

//#include <cstdio>

//
// ZONE MEMORY
// PU - purge tags.
// Tags < 100 are not overwritten until freed.
#define PU_STATIC		1	// static entire execution time
#define PU_SOUND		2	// static while playing
#define PU_MUSIC		3	// static while playing
#define PU_DAVE		4	// anything else Dave wants static
#define PU_LEVEL		50	// static until level exited
#define PU_LEVSPEC		51      // a special thinker in a level
// Tags >= 100 are purgable whenever needed.
#define PU_PURGELEVEL	100
#define PU_CACHE		101


void Z_Init();

void* Z_Malloc_internal(std::intptr_t size, int tag, void* p);
template<typename T = byte>
T* Z_Malloc(intptr_t size, int tag, void* p)
{
    return static_cast<T*>(Z_Malloc_internal(size, tag, p));
}

void Z_Free(void* p);
void Z_FreeTags(int lowtag, int hightag);
void Z_DumpHeap(int lowtag, int hightag);
void Z_FileDumpHeap(std::ofstream& file);
void Z_CheckHeap();
void Z_ChangeTag2(void* p, int tag);
intptr_t Z_FreeMemory();


struct memblock_t
{
    intptr_t size;	// including the header and possibly tiny fragments
    void** user;	// nullptr if a free block
    int tag;	// purgelevel
    int id;	// should be ZONEID
    memblock_t* next;
    memblock_t* prev;
};

// This is used to get the local FILE:LINE info from CPP
// prior to really call the function in question.
#define Z_ChangeTag(p, t) \
{ \
      if (( (memblock_t*)((intptr_t)(p) - sizeof(memblock_t)))->id!=0x1d4a11) \
	  I_Error("Z_CT at " __FILE__ ":{}",__LINE__); \
	  Z_ChangeTag2(p, t); \
};
