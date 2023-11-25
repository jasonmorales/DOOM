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
//-----------------------------------------------------------------------------

#include "i_system.h"
#include "m_argv.h"

#include <fstream>

string_view CommandLine::commandLine;
vector<string_view> CommandLine::args;
vector<string_view> CommandLine::fileStack;
vector<char*> CommandLine::responseFileContent;

void CommandLine::Initialize(string_view source)
{
    commandLine = source;
    Parse(commandLine);
}

bool CommandLine::HasArg(string_view name)
{
    return args.has(name);
}

bool CommandLine::HasArg(const std::function<bool(string_view)>& condition)
{
    return args.has(condition);
}

void CommandLine::Parse(string_view source)
{
    int32 start = 0;
    int32 end = 0;
    int32 last = source.length();

    while(end < last)
    {
        // Eat whitespace
        while (end < last && is_whitespace(source[end])) ++ end;
        start = end;

        bool inString = false;
        while (end < last && (inString || !is_whitespace(source[end])))
        {
            if (source[end] == '"')
                inString = !inString;

            ++end;
        }

        if (start == end)
            continue;

        auto arg = source.substr(start, end - start);
        if (arg.starts_with('@'))
            ParseResponseFile(arg.substr(1));
        else
            args.push_back(arg);

        start = end = end + 1;
    }
}

void CommandLine::ParseResponseFile(string_view path)
{
    if (fileStack.has(path))
        I_Error("Recursive response file!");

    fileStack.push_back(path);

    auto file = std::ifstream{string{path}, std::ifstream::binary | std::ifstream::ate};
    if (!file.is_open())
        I_Error("No such response file!");

    auto size = file.tellg();
    auto content =new char[size];
    responseFileContent.push_back(content);
    file.seekg(0);
    file.read(content, size);
    Parse(string_view(content, size));

    fileStack.pop_back();
}
