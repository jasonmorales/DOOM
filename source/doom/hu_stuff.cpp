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
// DESCRIPTION:  Heads-up displays
//
//-----------------------------------------------------------------------------
import std;
#define __STD_MODULE__

#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

#include "dstrings.h"
#include "sounds.h"


// Locally used constants, shortcuts.
#define HU_TITLE	(mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2	(mapnames2[gamemap-1])
#define HU_TITLEP	(mapnamesp[gamemap-1])
#define HU_TITLET	(mapnamest[gamemap-1])
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(167 - (hu_font[0]->height))

#define HU_INPUTTOGGLE	't'
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*((hu_font[0]->height) +1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1

const char* DefaultChatMacros[] = {
    "I'm ready to kick butt!",
    "I'm OK.",
    "I'm not looking too good!",
    "Help!",
    "You suck!",
    "Next time, scumbag...",
    "Come here!",
    "I'll take care of it.",
    "Yes",
    "No",
};

const char* chat_macros[] =
{
    DefaultChatMacros[0],
    DefaultChatMacros[1],
    DefaultChatMacros[2],
    DefaultChatMacros[3],
    DefaultChatMacros[4],
    DefaultChatMacros[5],
    DefaultChatMacros[6],
    DefaultChatMacros[7],
    DefaultChatMacros[8],
    DefaultChatMacros[9]
};

const char* player_names[] =
{
    HUSTR_PLRGREEN,
    HUSTR_PLRINDIGO,
    HUSTR_PLRBROWN,
    HUSTR_PLRRED
};


char			chat_char; // remove later.
static player_t* plr;
const patch_t* hu_font[HU_FONTSIZE];
static hu_textline_t	w_title;
bool			chat_on;
static hu_itext_t	w_chat;
static bool		always_off = false;
static char		chat_dest[MAXPLAYERS];
static hu_itext_t w_inputbuffer[MAXPLAYERS];

static bool		message_on;
bool			message_dontfuckwithme;
static bool		message_nottobefuckedwith;

static hu_stext_t	w_message;
static int		message_counter;

extern int32		showMessages;
extern bool		automapactive;

static bool		headsupactive = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

string_view mapnames[] =	// DOOM shareware/registered/retail (Ultimate) names.
{

    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M6,
    HUSTR_E1M7,
    HUSTR_E1M8,
    HUSTR_E1M9,

    HUSTR_E2M1,
    HUSTR_E2M2,
    HUSTR_E2M3,
    HUSTR_E2M4,
    HUSTR_E2M5,
    HUSTR_E2M6,
    HUSTR_E2M7,
    HUSTR_E2M8,
    HUSTR_E2M9,

    HUSTR_E3M1,
    HUSTR_E3M2,
    HUSTR_E3M3,
    HUSTR_E3M4,
    HUSTR_E3M5,
    HUSTR_E3M6,
    HUSTR_E3M7,
    HUSTR_E3M8,
    HUSTR_E3M9,

    HUSTR_E4M1,
    HUSTR_E4M2,
    HUSTR_E4M3,
    HUSTR_E4M4,
    HUSTR_E4M5,
    HUSTR_E4M6,
    HUSTR_E4M7,
    HUSTR_E4M8,
    HUSTR_E4M9,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};

string_view mapnames2[] =	// DOOM 2 map names.
{
    HUSTR_1,
    HUSTR_2,
    HUSTR_3,
    HUSTR_4,
    HUSTR_5,
    HUSTR_6,
    HUSTR_7,
    HUSTR_8,
    HUSTR_9,
    HUSTR_10,
    HUSTR_11,

    HUSTR_12,
    HUSTR_13,
    HUSTR_14,
    HUSTR_15,
    HUSTR_16,
    HUSTR_17,
    HUSTR_18,
    HUSTR_19,
    HUSTR_20,

    HUSTR_21,
    HUSTR_22,
    HUSTR_23,
    HUSTR_24,
    HUSTR_25,
    HUSTR_26,
    HUSTR_27,
    HUSTR_28,
    HUSTR_29,
    HUSTR_30,
    HUSTR_31,
    HUSTR_32
};

const char shiftxform[] =
{

     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    ' ', '!', '"', '#', '$', '%', '&',
    '"', // shift-'
    '(', ')', '*', '+',
    '<', // shift-,
    '_', // shift--
    '>', // shift-.
    '?', // shift-/
    ')', // shift-0
    '!', // shift-1
    '@', // shift-2
    '#', // shift-3
    '$', // shift-4
    '%', // shift-5
    '^', // shift-6
    '&', // shift-7
    '*', // shift-8
    '(', // shift-9
    ':',
    ':', // shift-;
    '<',
    '+', // shift-=
    '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', // shift-[
    '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
    ']', // shift-]
    '"', '_',
    '\'', // shift-`
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '{', '|', '}', '~', 127
};

void HU_Init()
{
    // load the heads-up font
    int32 j = HU_FONTSTART;
    for (int32 i = 0; i < HU_FONTSIZE; ++i)
    {
        hu_font[i] = WadManager::GetLumpData<patch_t>(std::format("STCFN{:03d}", j++));
    }
}

void HU_Stop()
{
    headsupactive = false;
}

void HU_Start()
{
    int		i;

    if (headsupactive)
        HU_Stop();

    plr = &players[consoleplayer];
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;
    chat_on = false;

    // create the message widget
    HUlib_initSText(&w_message, HU_MSGX, HU_MSGY, HU_MSGHEIGHT, hu_font, HU_FONTSTART, &message_on);

    // create the map title widget
    HUlib_initTextLine(&w_title, HU_TITLEX, HU_TITLEY, hu_font, HU_FONTSTART);

    string_view s;
    switch (gamemode)
    {
    case GameMode::Doom1Shareware:
    case GameMode::Doom1Registered:
    case GameMode::Doom1Retail:
        s = HU_TITLE;
        break;
    case GameMode::Doom2Commercial:
    default:
        s = HU_TITLE2;
        break;
    }

    for (auto c : s)
        HUlib_addCharToTextLine(&w_title, c);

    // create the chat widget
    HUlib_initIText(&w_chat, HU_INPUTX, HU_INPUTY, hu_font, HU_FONTSTART, &chat_on);

    // create the inputbuffer widgets
    for (i = 0; i < MAXPLAYERS; i++)
        HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

    headsupactive = true;

}

void HU_Drawer()
{

    HUlib_drawSText(&w_message);
    HUlib_drawIText(&w_chat);
    if (automapactive)
        HUlib_drawTextLine(&w_title, false);

}

void HU_Erase()
{

    HUlib_eraseSText(&w_message);
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);

}

void HU_Ticker()
{
    char c;

    // tick down message counter if message is up
    if (message_counter && !--message_counter)
    {
        message_on = false;
        message_nottobefuckedwith = false;
    }

    if (showMessages || message_dontfuckwithme)
    {
        // display message if necessary
        if (!plr->message.empty() && (!message_nottobefuckedwith || message_dontfuckwithme))
        {
            HUlib_addMessageToSText(&w_message, {}, plr->message);
            plr->message.clear();
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }

    } // else message_on = false;

    // check for incoming chat characters
    if (netgame)
    {
        for (int32 i = 0; i < MAXPLAYERS; ++i)
        {
            if (!playeringame[i])
                continue;

            c = players[i].cmd.chatchar;
            if (i != consoleplayer && c)
            {
                if (c <= HU_BROADCAST)
                    chat_dest[i] = c;
                else
                {
                    if (c >= 'a' && c <= 'z')
                        c = (char)shiftxform[(unsigned char)c];
                    auto rc = HUlib_keyInIText(&w_inputbuffer[i], c);
                    if (rc && c == KEY_ENTER)
                    {
                        if (w_inputbuffer[i].l.len
                            && (chat_dest[i] == consoleplayer + 1
                                || chat_dest[i] == HU_BROADCAST))
                        {
                            HUlib_addMessageToSText(&w_message, player_names[i], w_inputbuffer[i].l.l);

                            message_nottobefuckedwith = true;
                            message_on = true;
                            message_counter = HU_MSGTIMEOUT;
                            if (gamemode == GameMode::Doom2Commercial)
                                S_StartSound(0, sfx_radio);
                            else
                                S_StartSound(0, sfx_tink);
                        }
                        HUlib_resetIText(&w_inputbuffer[i]);
                    }
                }
                players[i].cmd.chatchar = 0;
            }
        }
    }
}

#define QUEUESIZE		128

static char	chatchars[QUEUESIZE];
static int	head = 0;
static int	tail = 0;


void HU_queueChatChar(char c)
{
    if (((head + 1) & (QUEUESIZE - 1)) == tail)
    {
        plr->message = HUSTR_MSGU;
    }
    else
    {
        chatchars[head] = c;
        head = (head + 1) & (QUEUESIZE - 1);
    }
}

char HU_dequeueChatChar()
{
    char c;

    if (head != tail)
    {
        c = chatchars[tail];
        tail = (tail + 1) & (QUEUESIZE - 1);
    }
    else
    {
        c = 0;
    }

    return c;
}

bool HU_Responder(const event_t& event)
{
    const char* macromessage;
    bool eatkey = false;
    static bool shiftdown = false;
    static bool altdown = false;
    unsigned char c;
    int32 numplayers;

    static char destination_keys[MAXPLAYERS] =
    {
        HUSTR_KEYGREEN,
        HUSTR_KEYINDIGO,
        HUSTR_KEYBROWN,
        HUSTR_KEYRED
    };

    static int32 num_nobrainers = 0;
    numplayers = 0;
    for (int32 i = 0; i < MAXPLAYERS; i++)
        numplayers += playeringame[i];

    if (event.data1 == KEY_RSHIFT)
    {
        shiftdown = event.type == ev_keydown;
        return false;
    }
    else if (event.data1 == KEY_RALT || event.data1 == KEY_LALT)
    {
        altdown = event.type == ev_keydown;
        return false;
    }

    if (event.type != ev_keydown)
        return false;

    if (!chat_on)
    {
        if (event.data1 == HU_MSGREFRESH)
        {
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            eatkey = true;
        }
        else if (netgame && event.data1 == HU_INPUTTOGGLE)
        {
            eatkey = chat_on = true;
            HUlib_resetIText(&w_chat);
            HU_queueChatChar(HU_BROADCAST);
        }
        else if (netgame && numplayers > 2)
        {
            for (char i = 0; i < MAXPLAYERS; i++)
            {
                if (event.data1 == destination_keys[i])
                {
                    if (playeringame[i] && i != consoleplayer)
                    {
                        eatkey = chat_on = true;
                        HUlib_resetIText(&w_chat);
                        HU_queueChatChar(i + 1);
                        break;
                    }
                    else if (i == consoleplayer)
                    {
                        num_nobrainers++;
                        if (num_nobrainers < 3)
                            plr->message = HUSTR_TALKTOSELF1;
                        else if (num_nobrainers < 6)
                            plr->message = HUSTR_TALKTOSELF2;
                        else if (num_nobrainers < 9)
                            plr->message = HUSTR_TALKTOSELF3;
                        else if (num_nobrainers < 32)
                            plr->message = HUSTR_TALKTOSELF4;
                        else
                            plr->message = HUSTR_TALKTOSELF5;
                    }
                }
            }
        }
    }
    else
    {
        c = static_cast<unsigned char>(event.data1);
        // send a macro
        if (altdown)
        {
            c = c - '0';
            if (c > 9)
                return false;
            // fprintf(stderr, "got here\n");
            macromessage = chat_macros[c];

            // kill last message with a '\n'
            HU_queueChatChar(KEY_ENTER); // DEBUG!!!

            // send the macro message
            while (*macromessage)
                HU_queueChatChar(*macromessage++);
            HU_queueChatChar(KEY_ENTER);

            // leave chat mode and notify that it was sent
            chat_on = false;
            plr->message = chat_macros[c];
            eatkey = true;
        }
        else
        {
            if (shiftdown || (c >= 'a' && c <= 'z'))
                c = shiftxform[c];
            eatkey = HUlib_keyInIText(&w_chat, c);
            if (eatkey)
            {
                // static unsigned char buf[20]; // DEBUG
                HU_queueChatChar(c);

                // sprintf(buf, "KEY: %d => %d", event.data1, c);
                //      plr->message = buf;
            }
            if (c == KEY_ENTER)
            {
                chat_on = false;
                if (w_chat.l.len)
                    plr->message = w_chat.l.l;
            }
            else if (c == KEY_ESCAPE)
            {
                chat_on = false;
            }
        }
    }

    return eatkey;
}
