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
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------
import std;

#include "z_zone.h"
#include "i_system.h"
#include "doomdef.h"


// ZONE MEMORY ALLOCATION

// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
// 

#define ZONEID	0x1d4a11

typedef struct
{
    // total bytes malloced, including header
    intptr_t size;

    // start / end cap for linked list
    memblock_t blocklist;

    memblock_t* rover;

} memzone_t;

memzone_t* mainzone;

void Z_ClearZone(memzone_t* zone)
{
    // set the entire zone to one free block
    memblock_t* block = zone->blocklist.next = zone->blocklist.prev = (memblock_t*)((intptr_t)zone + sizeof(memzone_t));

    zone->blocklist.user = reinterpret_cast<void**>(zone);
    zone->blocklist.tag = PU_STATIC;
    zone->rover = block;

    block->prev = block->next = &zone->blocklist;

    // nullptr indicates a free block.
    block->user = nullptr;

    block->size = zone->size - sizeof(memzone_t);
}

void Z_Init()
{
    intptr_t size = 0;
    mainzone = (memzone_t*)I_ZoneBase(&size);
    mainzone->size = size;

    // set the entire zone to one free block
    memblock_t* block = mainzone->blocklist.next = mainzone->blocklist.prev = (memblock_t*)((intptr_t)mainzone + sizeof(memzone_t));

    mainzone->blocklist.user = reinterpret_cast<void**>(mainzone);
    mainzone->blocklist.tag = PU_STATIC;
    mainzone->rover = block;

    block->prev = block->next = &mainzone->blocklist;

    // nullptr indicates a free block.
    block->user = nullptr;

    block->size = mainzone->size - sizeof(memzone_t);
}

//
// Z_Free
//
void Z_Free(void* p)
{
    memblock_t* block = (memblock_t*)((intptr_t)p - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error("Z_Free: freed a pointer without ZONEID");

    if (block->user > (void**)0x100)
    {
        // smaller values are not pointers
        // Note: OS-dependend?

        // clear the user's mark
        *block->user = 0;
    }

    // mark as free
    block->user = nullptr;
    block->tag = 0;
    block->id = 0;

    memblock_t* other = block->prev;

    if (!other->user)
    {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;

        if (block == mainzone->rover)
            mainzone->rover = other;

        block = other;
    }

    other = block->next;
    if (!other->user)
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;

        if (other == mainzone->rover)
            mainzone->rover = block;
    }
}

//
// Z_Malloc
// You can pass a nullptr user if the tag is < PU_PURGELEVEL.
//
#define MINFRAGMENT		64

void* Z_Malloc_internal(intptr_t size, int tag, void* user)
{
    size = (size + 3) & ~3;

    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.

    // account for size of block header
    size += sizeof(memblock_t);

    // if there is a free block behind the rover,
    //  back up over them
    memblock_t* base = mainzone->rover;

    if (!base->prev->user)
        base = base->prev;

    memblock_t* rover = base;
    memblock_t* start = base->prev;

    do
    {
        if (rover == start)
        {
            // scanned all the way around the list
            I_Error("Z_Malloc: failed on allocation of {} bytes", static_cast<int64>(size));
        }

        if (rover->user)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                //  so move base past it
                base = rover = rover->next;
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base = base->prev;
                Z_Free((byte*)rover + sizeof(memblock_t));
                base = base->next;
                rover = base->next;
            }
        }
        else
            rover = rover->next;
    } while (base->user || base->size < size);

    // found a block big enough
    auto extra = base->size - size;

    if (extra > MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        memblock_t* newblock = (memblock_t*)((byte*)base + size);
        newblock->size = extra;

        // nullptr indicates free block.
        newblock->user = nullptr;
        newblock->tag = 0;
        newblock->prev = base;
        newblock->next = base->next;
        newblock->next->prev = newblock;

        base->next = newblock;
        base->size = size;
    }

    if (user)
    {
        // mark as an in use block
        base->user = reinterpret_cast<void**>(user);
        *(void**)user = (void*)((byte*)base + sizeof(memblock_t));
    }
    else
    {
        if (tag >= PU_PURGELEVEL)
            I_Error("Z_Malloc: an owner is required for purgable blocks");

        // mark as in use, but unowned	
        base->user = reinterpret_cast<void**>(2);
    }
    base->tag = tag;

    // next allocation will start looking here
    mainzone->rover = base->next;

    base->id = ZONEID;

    return (void*)((byte*)base + sizeof(memblock_t));
}

void Z_FreeTags(int lowtag, int hightag)
{
    memblock_t* next = nullptr;
    for (memblock_t* block = mainzone->blocklist.next; block != &mainzone->blocklist; block = next)
    {
        // get link before freeing
        next = block->next;

        // free block?
        if (!block->user)
            continue;

        if (block->tag >= lowtag && block->tag <= hightag)
            Z_Free((void*)((intptr_t)block + sizeof(memblock_t)));
    }
}

void Z_DumpHeap(int32 lowtag, int32 hightag)
{
    memblock_t* block;

    std::cout << std::format("zone size: {}  location: {}\n", mainzone->size, static_cast<void*>(mainzone));

    std::cout << std::format("tag range: {} to {}\n", lowtag, hightag);

    for (block = mainzone->blocklist.next; ; block = block->next)
    {
        if (block->tag >= lowtag && block->tag <= hightag)
            std::cout << std::format("block:{}    size:{:7}    user:{}    tag:{:3}\n",
                static_cast<void*>(block), block->size, static_cast<void*>(block->user), block->tag);

        if (block->next == &mainzone->blocklist)
        {
            // all blocks have been hit
            break;
        }

        if ((byte*)block + block->size != (byte*)block->next)
            std::cerr << "ERROR: block size does not touch the next block\n";

        if (block->next->prev != block)
            std::cerr << "ERROR: next block doesn't have proper back link\n";

        if (!block->user && !block->next->user)
            std::cerr << "ERROR: two consecutive free blocks\n";
    }
}

void Z_FileDumpHeap(std::ofstream& file)
{
    file << std::format("zone size: {}  location: {}\n", mainzone->size, static_cast<void*>(mainzone));

    for (auto* block = mainzone->blocklist.next; ; block = block->next)
    {
        file << std::format("block:{}    size:{:7}    user:{}    tag:{:3}\n",
            static_cast<void*>(block), block->size, static_cast<void*>(block->user), block->tag);

        if (block->next == &mainzone->blocklist)
        {
            // all blocks have been hit
            break;
        }

        if ((byte*)block + block->size != (byte*)block->next)
            file << "ERROR: block size does not touch the next block\n";

        if (block->next->prev != block)
            file << "ERROR: next block doesn't have proper back link\n";

        if (!block->user && !block->next->user)
            file << "ERROR: two consecutive free blocks\n";
    }
}

void Z_CheckHeap()
{
    for (auto* block = mainzone->blocklist.next; ; block = block->next)
    {
        if (block->next == &mainzone->blocklist)
            break;  // all blocks have been hit

        if ((byte*)block + block->size != (byte*)block->next)
            I_Error("Z_CheckHeap: block size does not touch the next block\n");

        if (block->next->prev != block)
            I_Error("Z_CheckHeap: next block doesn't have proper back link\n");

        if (!block->user && !block->next->user)
            I_Error("Z_CheckHeap: two consecutive free blocks\n");
    }
}

void Z_ChangeTag2(void* p, int32 tag)
{
    auto* block = reinterpret_cast<memblock_t*>((byte*)p - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error("Z_ChangeTag: freed a pointer without ZONEID");

    if (tag >= PU_PURGELEVEL && reinterpret_cast<intptr_t>(block->user) < 0x100)
        I_Error("Z_ChangeTag: an owner is required for purgable blocks");

    block->tag = tag;
}

intptr_t Z_FreeMemory()
{
    intptr_t free = 0;
    for (auto* block = mainzone->blocklist.next; block != &mainzone->blocklist; block = block->next)
    {
        if (!block->user || block->tag >= PU_PURGELEVEL)
            free += block->size;
    }
    return free;
}
