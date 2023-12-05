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
//	Main loop menu stuff.
//	Default Config File.
//	PCX Screenshots.
//
//-----------------------------------------------------------------------------
import std;
#define __STD_MODULE__

#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"
#include "m_argv.h"

#include "w_wad.h"

#include "i_system.h"
#include "i_video.h"
#include "v_video.h"

#include "hu_stuff.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "m_misc.h"
#include "d_main.h"

#include "types/strings.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#include <ctype.h>
#include <io.h>


extern Doom* g_doom;


Settings::DirectoryType* Settings::settings = nullptr;

boolean M_WriteFile(char const* name, void* source, uint32 length)
{
    int handle = 0;
    _sopen_s(&handle, name, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _SH_DENYWR, _S_IREAD);

    if (handle == -1)
        return false;

    uint32 count = _write(handle, source, length);
    _close(handle);

    if (count < length)
        return false;

    return true;
}

vector<byte> M_ReadFile(const filesys::path& path)
{
    auto file = std::ifstream{path, std::ifstream::binary | std::ifstream::ate};
    if (!file.is_open())
        return {};

    auto size = file.tellg();
    vector<byte> contents(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(contents.data()), size);

    return contents;
}

// DEFAULTS
Setting<bool> useMouse{"useMouse", false};
Setting<bool> useJoystick{"use_joystick", false};

// Blocky mode, has default, 0 = high, 1 = normal
Setting<int32> detailLevel{"detail_level", 0};

Setting<int32> key_right{"key_right", KEY_RIGHTARROW};
Setting<int32> key_left{"key_left", KEY_LEFTARROW};
Setting<int32> key_up{"key_up", KEY_UPARROW};
Setting<int32> key_down{"key_down", KEY_DOWNARROW};

Setting<int32> key_strafeleft{"key_strafeleft", ','};
Setting<int32> key_straferight{"key_straferight", '.'};

Setting<int32> key_fire{"key_fire", KEY_RCTRL};
Setting<int32> key_use{"key_use", ' '};
Setting<int32> key_strafe{"key_strafe", KEY_RALT};
Setting<int32> key_speed{"key_speed", KEY_RSHIFT};

extern int32	mousebfire;
extern int32	mousebstrafe;
extern int32	mousebforward;

extern int32	joybfire;
extern int32	joybstrafe;
extern int32	joybuse;
extern int32	joybspeed;

extern int	viewwidth;
extern int	viewheight;

extern int32	mouseSensitivity;
extern int32	showMessages;

// machine-independent sound params
extern	int32	numChannels;


// UNIX hack, to be removed.
#ifdef SNDSERV
extern const char* sndserver_filename;
extern int	mb_used;
#endif

extern const char* chat_macros[];

struct default_t
{
    const char* name;
    void* location;
    int32 defaultvalue;
};

default_t	defaults[] =
{
    {"mouse_sensitivity",&mouseSensitivity, 5},
    {"sfx_volume",&snd_SfxVolume, 8},
    {"music_volume",&snd_MusicVolume, 8},
    {"show_messages",&showMessages, 1},

    {"mouseb_fire",&mousebfire,0},
    {"mouseb_strafe",&mousebstrafe,1},
    {"mouseb_forward",&mousebforward,2},

    {"joyb_fire",&joybfire,0},
    {"joyb_strafe",&joybstrafe,1},
    {"joyb_use",&joybuse,3},
    {"joyb_speed",&joybspeed,2},

    {"snd_channels",&numChannels, 3},

    {"usegamma",&usegamma, 0},

    {"chatmacro0", chat_macros + 0, 0 | 0x8000 },
    {"chatmacro1", chat_macros + 1, 1 | 0x8000 },
    {"chatmacro2", chat_macros + 2, 2 | 0x8000 },
    {"chatmacro3", &chat_macros[3], 3 | 0x8000 },
    {"chatmacro4", &chat_macros[4], 4 | 0x8000 },
    {"chatmacro5", &chat_macros[5], 5 | 0x8000 },
    {"chatmacro6", &chat_macros[6], 6 | 0x8000 },
    {"chatmacro7", &chat_macros[7], 7 | 0x8000 },
    {"chatmacro8", &chat_macros[8], 8 | 0x8000 },
    {"chatmacro9", &chat_macros[9], 9 | 0x8000 },
};

Setting<int32> screenBlocks{"screenblocks", 9};
Setting<int32> viewWidth{"viewWidth", 0};

const std::filesystem::path Settings::DevDataPath = "devdata";
const filesys::path Settings::DefaultConfigFile = Settings::DevDataPath / "default.cfg";

void Settings::Save(const filesys::path& path /*= DefaultConfigFile*/)
{
    std::ofstream outFile(path);
    if (!outFile.is_open())
        return; // can't write the file, but don't complain

    for (auto& [name, var] : Directory())
    {
        std::visit(
            [&outFile, &name](auto&& val){
                outFile << std::format("{}   {}\n", name, val);
            },
            var->GetVar());
    }
}

void Settings::Load()
{
    filesys::path configPath = DefaultConfigFile;
    // check for a custom default file
    if (string arg; CommandLine::TryGetValues("-config", arg))
        configPath = arg;

    // read the file in, overriding any set defaults
    std::ifstream source(configPath);
    if (!source.is_open())
        return;

    auto& directoy = Directory();

    for (string line; std::getline(source, line);)
    {
        string_view key = line;
        key = key.substr(key.find_first_not_of(" \t\n\r"));
        auto split = key.find_first_of(" \t\n\r");
        key = key.substr(0, split);
        string_view value = line;
        value = value.substr(value.find_first_not_of(" \t\n\r", split));
        value = value.substr(0, value.find_last_not_of(" \t\n\r") + 1);

        auto entry = directoy.find(key);
        if (entry == directoy.end())
            continue;

        entry->second->Set(value);
    }
}

// SCREEN SHOTS
typedef struct
{
    char		manufacturer;
    char		version;
    char		encoding;
    char		bits_per_pixel;

    unsigned short	xmin;
    unsigned short	ymin;
    unsigned short	xmax;
    unsigned short	ymax;

    unsigned short	hres;
    unsigned short	vres;

    unsigned char	palette[48];

    char		reserved;
    char		color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;

    char		filler[58];
    unsigned char	data;		// unbounded
} pcx_t;

// WritePCXfile
void WritePCXfile(char* filename,
    byte* data,
    int		width,
    int		height,
    byte* palette)
{
    pcx_t* pcx;
    byte* pack;

    pcx = Z_Malloc<pcx_t>(width * height * 2 + 1000, PU_STATIC, nullptr);

    pcx->manufacturer = 0x0a;		// PCX id
    pcx->version = 5;			// 256 color
    pcx->encoding = 1;			// uncompressed
    pcx->bits_per_pixel = 8;		// 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = SHORT(width - 1);
    pcx->ymax = SHORT(height - 1);
    pcx->hres = SHORT(width);
    pcx->vres = SHORT(height);
    memset(pcx->palette, 0, sizeof(pcx->palette));
    pcx->color_planes = 1;		// chunky image
    pcx->bytes_per_line = SHORT(width);
    pcx->palette_type = SHORT(2);	// not a grey scale
    memset(pcx->filler, 0, sizeof(pcx->filler));


    // pack the image
    pack = &pcx->data;

    for (int i = 0; i < width * height; i++)
    {
        if ((*data & 0xc0) != 0xc0)
            *pack++ = *data++;
        else
        {
            *pack++ = 0xc1;
            *pack++ = *data++;
        }
    }

    // write the palette
    *pack++ = 0x0c;	// palette ID byte
    for (int i = 0; i < 768; i++)
        *pack++ = *palette++;

    // write output file
    auto length = static_cast<uint32>(pack - reinterpret_cast<byte*>(pcx));
    M_WriteFile(filename, pcx, length);

    Z_Free(pcx);
}

void M_ScreenShot()
{
    // munge planar buffer to linear
    auto* linear = g_doom->GetVideo()->CopyScreen(2);

    // find a file name to save it to
    char lbmname[12];
    strcpy_s(lbmname, "DOOM00.pcx");

    char i = 0;
    for (; i <= 99; i++)
    {
        lbmname[4] = i / 10 + '0';
        lbmname[5] = i % 10 + '0';
        if (_access(lbmname, 0) == -1)
            break;	// file doesn't exist
    }
    if (i == 100)
        I_Error("M_ScreenShot: Couldn't create a PCX");

    // save the pcx file
    WritePCXfile(lbmname, linear, SCREENWIDTH, SCREENHEIGHT, W_CacheLumpName<byte>("PLAYPAL", PU_CACHE));

    players[consoleplayer].message = "screen shot";
}

SettingBase::SettingBase(string_view name)
    : name{ name }
{
    Settings::Register(this);
}
