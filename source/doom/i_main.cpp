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
//	Main program, simply calls Doom::Main high level loop.
//
//-----------------------------------------------------------------------------
import std;
import nstd;
import config;

#include "system/windows.h"

#include "d_main.h"

Doom* g_doom = nullptr;

template<typename T>
consteval string_view get_type_name()
{
    string_view fn_name = std::source_location::current().function_name();
    auto start = fn_name.find_last_of('<') + 1;
    auto type_name = fn_name.substr(start, fn_name.find_last_of('>') - start);
    if (type_name.starts_with("class "))
        type_name = type_name.substr(6);

    return type_name;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR args, int)
{
    CommandLine::Initialize(args);

    g_doom = new Doom;
    g_doom->Main();

    return 0;
}
