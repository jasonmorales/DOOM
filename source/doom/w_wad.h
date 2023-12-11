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

#include "containers/vector.h"
#include "types/strings.h"

#ifndef __STD_MODULE__
#include <filesystem>
#endif

int32 W_GetNumForName(string_view name);

struct FileInfo
{
    FileInfo() = default;
    FileInfo(const FileInfo&) = delete;
    FileInfo(FileInfo&& other)
        : id{other.id}
        , path{other.path}
        , size{other.size}
        , data{other.data}
    {
        other.data = nullptr;
    }

    ~FileInfo() { delete[] data; }

    void ReadFile(std::ifstream& file)
    {
        file.seekg(0, std::ios_base::end);
        size = static_cast<int32>(file.tellg());
        data = new byte[size];
        file.seekg(0);
        file.read(reinterpret_cast<char*>(data), size);
    }

    int32 id = INVALID_ID;
    filesys::path path;
    int32 size = 0;
    byte* data = nullptr;
};

struct WadInfo
{
    WadInfo(const FileInfo& info)
        : fileId{info.id}
        , tag{info.data, 4}
        , lumpCount{*reinterpret_cast<int32*>(info.data + 4)}
        , directoryOffset{*reinterpret_cast<int32*>(info.data + 8)}
    {        
    }

    bool HasValidTag() const { return tag == "IWAD" || tag == "PWAD"; }

    int32 id = INVALID_ID;
    int32 fileId = INVALID_ID;

    string tag;
    int32 lumpCount = 0;
    int32 directoryOffset = 0;

    bool reloadable = false;
};

struct LumpHeader
{
    int32 offset = 0;
    int32 size = 0;
    char name[8] = {0};
};

struct LumpInfo
{
    int32 id = INVALID_ID;
    int32 fileId = INVALID_ID;
    int32 wadId = INVALID_ID;

    string name;
    int32 size = 0;
    byte* data = nullptr;

    template<typename T = void>
    const T* as() const { return reinterpret_cast<T*>(data); }
};

class WadManager
{
public:
    static void LoadAllFiles();

    static void AddFile(const filesys::path& path) { loadList.push_back(path); }

    static int32 GetLumpId(string_view name)
    {
        auto it = lumpDirectory.find(string{name});
        if (it == lumpDirectory.end())
            return INVALID_ID;

        return it->second;
    }

    static const LumpInfo* FindLump(string_view name) { return FindLump(GetLumpId(name)); }
    static const LumpInfo* FindLump(int32 id) { return (id < 0 || id >= lumps.size()) ? nullptr : &lumps[id]; }

    static const LumpInfo& GetLump(string_view name) { return GetLump(GetLumpId(name)); }
    static const LumpInfo& GetLump(int32 id) { assert(id >= 0 && id < lumps.size()); return lumps[id]; }

    template<typename T = void>
    static const T* GetLumpData(string_view name) { return GetLumpData<T>(GetLumpId(name)); }
    template<typename T = void>
    static const T* GetLumpData(int32 id) { return GetLump(id).as<T>(); }

private:
    static void LoadFile(const filesys::path& path);
    static void StoreLump(int32 fileId, string_view name, int32 size, byte* data, int32 wadId = INVALID_ID);

    static vector<filesys::path> loadList;
    static vector<FileInfo> files;
    static vector<WadInfo> wads;
    static vector<LumpInfo> lumps;
    static std::map<string, int32> lumpDirectory;
};
