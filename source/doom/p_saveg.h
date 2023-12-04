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
//	Savegame I/O, archiving, persistence.
//
//-----------------------------------------------------------------------------
#pragma once

#include "utility/convert.h"

#ifndef __STD_MODULE__
#include <format>
#endif

enum class SaveFileMarker : uint8
{
    ThinkersEnd = 0,
    MapObject,

    Ceiling = 0,
    Door,
    Floor,
    Plat,
    Flash,
    Strobe,
    Glow,
    End,

    EndSave = 0x1d,

    Invalid = 0xff
};

template<>
struct std::formatter<SaveFileMarker> : std::formatter<string>
{
    auto format(SaveFileMarker mark, std::format_context& ctx) const
    {
        return formatter<string>::format(std::format("{}", to_underlying(mark)), ctx);
    }
};

// Persistent storage/archiving.
// These are the load / save game routines.
void P_ArchivePlayers(std::ofstream& outFile);
void P_UnArchivePlayers(std::ifstream& inFile);
void P_ArchiveWorld(std::ofstream& outFile);
void P_UnArchiveWorld(std::ifstream& inFile);
void P_ArchiveThinkers(std::ofstream& outFile);
void P_UnArchiveThinkers(std::ifstream& inFile);
void P_ArchiveSpecials(std::ofstream& outFile);
void P_UnArchiveSpecials(std::ifstream& inFile);
