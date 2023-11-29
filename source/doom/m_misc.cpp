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
//
// DESCRIPTION:
//	Main loop menu stuff.
//	Default Config File.
//	PCX Screenshots.
//
//-----------------------------------------------------------------------------

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


//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
extern patch_t* hu_font[HU_FONTSIZE];

int
M_DrawText
(int		x,
    int		y,
    boolean	direct,
    char* string)
{
    int 	c;
    int		w;

    while (*string)
    {
        c = toupper(*string) - HU_FONTSTART;
        string++;
        if (c < 0 || c> HU_FONTSIZE)
        {
            x += 4;
            continue;
        }

        w = SHORT(hu_font[c]->width);
        if (x + w > SCREENWIDTH)
            break;
        if (direct)
            g_doom->GetVideo()->DrawPatch(x, y, 0, hu_font[c]);
        else
            g_doom->GetVideo()->DrawPatch(x, y, 0, hu_font[c]);
        x += w;
    }

    return x;
}

//
// M_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

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

int M_ReadFile(char const* name, byte** buffer)
{
    int	count, length;
    struct stat	fileinfo;
    byte* buf;

    int handle = -1;
    if (_sopen_s(&handle, name, O_RDONLY | O_BINARY, _SH_DENYWR, _S_IREAD) != 0)
        I_Error("Couldn't read file {}", name);

    if (fstat(handle, &fileinfo) == -1)
        I_Error("Couldn't read file {}", name);

    length = fileinfo.st_size;
    buf = Z_Malloc(length, PU_STATIC, nullptr);
    count = _read(handle, buf, length);
    _close(handle);

    if (count < length)
        I_Error("Couldn't read file {}", name);

    *buffer = buf;
    return length;
}

// DEFAULTS

int32 usemouse;
int32 usejoystick;

extern int	key_right;
extern int	key_left;
extern int	key_up;
extern int	key_down;

extern int	key_strafeleft;
extern int	key_straferight;

extern int	key_fire;
extern int	key_use;
extern int	key_strafe;
extern int	key_speed;

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

extern int32	detailLevel;

extern int32	screenblocks;

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

    {"key_right",&key_right, KEY_RIGHTARROW},
    {"key_left",&key_left, KEY_LEFTARROW},
    {"key_up",&key_up, KEY_UPARROW},
    {"key_down",&key_down, KEY_DOWNARROW},
    {"key_strafeleft",&key_strafeleft, ','},
    {"key_straferight",&key_straferight, '.'},

    {"key_fire",&key_fire, KEY_RCTRL},
    {"key_use",&key_use, ' '},
    {"key_strafe",&key_strafe, KEY_RALT},
    {"key_speed",&key_speed, KEY_RSHIFT},

    {"use_mouse",&usemouse, 1},
    {"mouseb_fire",&mousebfire,0},
    {"mouseb_strafe",&mousebstrafe,1},
    {"mouseb_forward",&mousebforward,2},

    {"use_joystick",&usejoystick, 0},
    {"joyb_fire",&joybfire,0},
    {"joyb_strafe",&joybstrafe,1},
    {"joyb_use",&joybuse,3},
    {"joyb_speed",&joybspeed,2},

    {"screenblocks",&screenblocks, 9},
    {"detaillevel",&detailLevel, 0},

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

int	numdefaults;
string defaultfile;

//
// M_SaveDefaults
//
void M_SaveDefaults()
{
    FILE* f = nullptr;
    fopen_s(&f, defaultfile.c_str(), "w");
    if (!f)
        return; // can't write the file, but don't complain

    for (int i = 0; i < numdefaults; i++)
    {
        if (defaults[i].defaultvalue & 0x8000)
        {
            fprintf(f, "%s\t\t\"%s\"\n", defaults[i].name, *(char**)(defaults[i].location));
        }
        else
        {
            int v = *static_cast<int32*>(defaults[i].location);
            fprintf(f, "%s\t\t%i\n", defaults[i].name, v);
        }
    }

    fclose(f);
}

//
// M_LoadDefaults
//
extern byte	scantokey[128];

void M_LoadDefaults()
{
    char	def[80];
    char	strparm[100];

    // set everything to base values
    numdefaults = sizeof(defaults) / sizeof(defaults[0]);
    for (int32 i = 0; i < numdefaults; i++)
    {
        if (defaults[i].defaultvalue & 0x8000)
            *static_cast<const char**>(defaults[i].location) = DefaultChatMacros[defaults[i].defaultvalue & 0x7fff];
        else
            *static_cast<int32*>(defaults[i].location) = defaults[i].defaultvalue;
    }

    // check for a custom default file
    if (CommandLine::TryGetValues("-config", defaultfile))
        printf("	default file: %s\n", defaultfile.c_str());
    else
        defaultfile = basedefault;

    // read the file in, overriding any set defaults
    FILE* f = nullptr;
    fopen_s(&f, defaultfile.c_str(), "r");
    if (f)
    {
        while (!feof(f))
        {
            if (fscanf_s(f, "%79s %[^\n]\n", def, static_cast<unsigned int>(_countof(def)), strparm, static_cast<unsigned int>(_countof(strparm))) == 2)
            {
                int32 parm = 0;
                char* newString = nullptr;
                if (strparm[0] == '"')
                {
                    // get a string default
                    auto len = strlen(strparm);
                    newString = (char*)malloc(len);
                    strparm[len - 1] = 0;
                    strcpy_s(newString, len, strparm + 1);
                }
                else if (strparm[0] == '0' && strparm[1] == 'x')
                    sscanf_s(strparm + 2, "%x", &parm);
                else
                    sscanf_s(strparm, "%i", &parm);

                for (int32 i = 0; i < numdefaults; i++)
                {
                    if (!strcmp(def, defaults[i].name))
                    {
                        if (newString == nullptr)
                            *static_cast<int32*>(defaults[i].location) = parm;
                        else
                            *static_cast<const char**>(defaults[i].location) = newString;
                        break;
                    }
                }
            }
        }

        fclose(f);
    }
}

//
// SCREEN SHOTS
//
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


//
// WritePCXfile
//
void
WritePCXfile
(char* filename,
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
