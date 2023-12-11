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

#include "i_system.h"
#include "m_swap.h"
#include "w_wad.h"
#include "z_zone.h"


vector<filesys::path> WadManager::loadList;
vector<FileInfo> WadManager::files;
vector<WadInfo> WadManager::wads;
vector<LumpInfo> WadManager::lumps;
std::map<string, int32> WadManager::lumpDirectory;

// LUMP BASED ROUTINES.

// All files are optional, but at least one file must be found (PWAD, if all required lumps are
// present).
// Files with a .wad extension are wadlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
void WadManager::LoadFile(const filesys::path& path)
{
    std::cout << "Checking for " << path << " ... " << filesys::exists(path) << "\n";

    // open the file and add to directory
    std::ifstream file{path, std::ios_base::binary};
    if (!file.is_open())
    {
        std::cout << " couldn't open" << path << "\n";
        return;
    }

    std::cout << " adding " << path << "\n";

    FileInfo info;
    info.path = path;
    info.ReadFile(file);
    info.id = files.size();

    if (path.extension().string().to_lower() == ".wad")
    {
        // WAD file
        WadInfo wad{info};
        if (!wad.HasValidTag())
        {
            std::cerr << "Wad file " << path << " doesn't have IWAD or PWAD id\n";
            return;
        }

        wad.id = wads.size();
        wads.push_back(wad);

        auto* entry = reinterpret_cast<LumpHeader*>(info.data + wad.directoryOffset);
        for (int32 n = 0; n < wad.lumpCount; ++n, ++entry)
        {
            StoreLump(info.id, {entry->name, 8}, entry->size, info.data + entry->offset, wad.id);
        }
    }
    else
    {
        StoreLump(info.id, path.stem().string(), info.size, info.data);
    }

    files.push_back(std::move(info));
}

void WadManager::StoreLump(int32 fileId, string_view name, int32 size, byte* data, int32 wadId /*= INVALID_ID*/)
{
    LumpInfo lump;
    lump.fileId = fileId;
    lump.wadId = wadId;
    lump.name = name.trim().to_upper();
    lump.size = size;
    lump.data = data;
    lump.id = lumps.size();
    lumps.push_back(lump);
    lumpDirectory[lump.name] = lump.id;
}

// Pass a null terminated list of files to use.
// All files are optional, but at least one file must be found.
// Files with a .wad extension are idlink files with multiple lumps.
// Other files are single lumps with the base filename for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file does override all earlier ones.
void WadManager::LoadAllFiles()
{
    std::cout << "Current path: " << filesys::current_path() << "\n";

    for (auto& file : loadList)
        LoadFile(file);

    if (lumps.empty())
        I_Error("WadManager::LoadAllFiles: no files found");
}

// Calls WadManager::GetLumpId, but bombs out if not found.
int32 W_GetNumForName(string_view name)
{
    auto i = WadManager::GetLumpId(name);

    if (i == INVALID_ID)
         I_Error("W_GetNumForName: {} not found!", name);

    return i;
}
