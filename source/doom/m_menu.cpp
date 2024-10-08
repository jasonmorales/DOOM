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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------
#include "i_video.h"

#include "doomdef.h"
#include "dstrings.h"
#include "d_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "m_swap.h"
#include "s_sound.h"
#include "doomstat.h"
#include "sounds.h"
#include "m_menu.h"
#include "m_misc.h"
#include "r_main.h"

import std;
import nstd;
import config;


extern Doom* g_doom;


extern const patch_t* hu_font[HU_FONTSIZE];
extern bool		message_dontfuckwithme;

extern bool		chat_on;		// in heads-up code

//
// defaulted values
//
int32			mouseSensitivity;       // has default

// Show messages has default, 0 = off, 1 = on
int32			showMessages;

// temp for screenblocks (0-9)
int			screenSize;

// -1 = no quicksave slot picked!
int			quickSaveSlot;

// 1 = message to be printed
int			messageToPrint;

// message x & y
int			messx;
int			messy;
int			messageLastMenuActive;

// timed message = no input from user
bool			messageNeedsInput;

void    (*messageRoutine)(bool response);

char gammamsg[5][26] =
{
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4
};

// we are going to be entering a savegame string
int			saveStringEnter;
int             	saveSlot;	// which slot to save in
size_t saveCharIndex;	// which char we're editing
// old save description before edit
string saveOldString{Game::SaveStringSize, '\0'};

bool			inhelpscreens;
bool			menuactive;

#define SKULLXOFF		-32
#define LINEHEIGHT		16

extern bool		sendpause;
string savegamestrings[10];

char	endstring[160];


//
// MENU TYPEDEFS
//
struct menuitem_t
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    short	status;

    char	name[10];

    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void (*routine)(int choice);

    // hotkey in menu
    char	alphaKey;
};



typedef struct menu_s
{
    short		numitems;	// # of menu items
    struct menu_s* prevMenu;	// previous menu
    menuitem_t* menuitems;	// menu items
    void		(*routine)();	// draw routine
    short		x;
    short		y;		// x,y of menu
    short		lastOn;		// last item user was on in menu
} menu_t;

short		itemOn;			// menu item skull is on
short		skullAnimCounter;	// skull animation counter
short		whichSkull;		// which skull to draw

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char    skullName[2][/*8*/9] = { "M_SKULL1","M_SKULL2" };

// current menudef
menu_t* currentMenu;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_ChangeSensitivity(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_ChangeDetail(int choice);
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int32 choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings();
void M_QuickSave();
void M_QuickLoad();

void M_DrawMainMenu();
void M_DrawReadThis1();
void M_DrawReadThis2();
void M_DrawNewGame();
void M_DrawEpisode();
void M_DrawOptions();
void M_DrawSound();
void M_DrawLoad();
void M_DrawSave();

void M_DrawSaveLoadBorder(int x, int y);
void M_SetupNextMenu(menu_t* menudef);
void M_DrawThermo(int x, int y, int thermWidth, int thermDot);
void M_DrawEmptyCell(menu_t* menu, int item);
void M_DrawSelCell(menu_t* menu, int item);
void M_WriteText(int32 x, int32 y, string_view text);
int32  M_StringWidth(string_view str);
int32  M_StringHeight(string_view str);
void M_StartControlPanel();
void M_ClearMenus();

// DOOM MENU
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
    readthis,
    quitdoom,
    main_end
} main_e;

menuitem_t MainMenu[] =
{
    {1,"M_NGAME",M_NewGame,'n'},
    {1,"M_OPTION",M_Options,'o'},
    {1,"M_LOADG",M_LoadGame,'l'},
    {1,"M_SAVEG",M_SaveGame,'s'},
    // Another hickup with Special edition.
    {1,"M_RDTHIS",M_ReadThis,'r'},
    {1,"M_QUITG",M_QuitDOOM,'q'}
};

menu_t  MainDef =
{
    main_end,
    nullptr,
    MainMenu,
    M_DrawMainMenu,
    97,64,
    0
};


//
// EPISODE SELECT
//
enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[] =
{
    {1,"M_EPI1", M_Episode,'k'},
    {1,"M_EPI2", M_Episode,'t'},
    {1,"M_EPI3", M_Episode,'i'},
    {1,"M_EPI4", M_Episode,'t'}
};

menu_t  EpiDef =
{
    ep_end,		// # of menu items
    &MainDef,		// previous menu
    EpisodeMenu,	// menuitem_t ->
    M_DrawEpisode,	// drawing routine ->
    48,63,              // x,y
    ep1			// lastOn
};

//
// NEW GAME
//
enum newgame_e
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
};

menuitem_t NewGameMenu[] =
{
    {1,"M_JKILL",	M_ChooseSkill, 'i'},
    {1,"M_ROUGH",	M_ChooseSkill, 'h'},
    {1,"M_HURT",	M_ChooseSkill, 'h'},
    {1,"M_ULTRA",	M_ChooseSkill, 'u'},
    {1,"M_NMARE",	M_ChooseSkill, 'n'}
};

menu_t  NewDef =
{
    newg_end,		// # of menu items
    &EpiDef,		// previous menu
    NewGameMenu,	// menuitem_t ->
    M_DrawNewGame,	// drawing routine ->
    48,63,              // x,y
    hurtme		// lastOn
};



//
// OPTIONS MENU
//
enum
{
    endgame,
    messages,
    detail,
    scrnsize,
    option_empty1,
    mousesens,
    option_empty2,
    soundvol,
    opt_end
} options_e;

menuitem_t OptionsMenu[] =
{
    {1,"M_ENDGAM",	M_EndGame,'e'},
    {1,"M_MESSG",	M_ChangeMessages,'m'},
    {1,"M_DETAIL",	M_ChangeDetail,'g'},
    {2,"M_SCRNSZ",	M_SizeDisplay,'s'},
    {-1,"",0},
    {2,"M_MSENS",	M_ChangeSensitivity,'m'},
    {-1,"",0},
    {1,"M_SVOL",	M_Sound,'s'}
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,37,
    0
};

//
// Read This! MENU 1 & 2
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
{
    {1,"",M_ReadThis2,0}
};

menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[] =
{
    {1,"",M_FinishReadThis,0}
};

menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    330,175,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    sound_end
} sound_e;

menuitem_t SoundMenu[] =
{
    {2,"M_SFXVOL",M_SfxVol,'s'},
    {-1,"",0},
    {2,"M_MUSVOL",M_MusicVol,'m'},
    {-1,"",0}
};

menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    80,64,
    0
};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
} load_e;

menuitem_t LoadMenu[] =
{
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'}
};

menu_t  LoadDef =
{
    load_end,
    &MainDef,
    LoadMenu,
    M_DrawLoad,
    80,54,
    0
};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[] =
{
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'}
};

menu_t  SaveDef =
{
    load_end,
    &MainDef,
    SaveMenu,
    M_DrawSave,
    80,54,
    0
};

//  read the strings from the savegame files
void M_ReadSaveStrings()
{
    for (int32 i = 0;i < load_end; ++i)
    {
        LoadMenu[i].status = 0;
        savegamestrings[i].clear();
        savegamestrings[i].reserve(Game::SaveStringSize);

        auto path = Game::GetSaveFilePath(i);
        std::ifstream file(path);

        if (!file.is_open())
            continue;

        file.read(savegamestrings[i].data(), Game::SaveStringSize);
        LoadMenu[i].status = 1;
    }
}

void M_DrawLoad()
{
    g_doom->GetVideo()->DrawPatch(72, 28, 0, WadManager::GetLumpData<patch_t>("M_LOADG"));
    for (int32 i = 0;i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
    }
}

// Draw border for the savegame description
void M_DrawSaveLoadBorder(int x, int y)
{
    g_doom->GetVideo()->DrawPatch(x - 8, y + 7, 0, WadManager::GetLumpData<patch_t>("M_LSLEFT"));

    for (int i = 0;i < 24;i++)
    {
        g_doom->GetVideo()->DrawPatch(x, y + 7, 0, WadManager::GetLumpData<patch_t>("M_LSCNTR"));
        x += 8;
    }

    g_doom->GetVideo()->DrawPatch(x, y + 7, 0, WadManager::GetLumpData<patch_t>("M_LSRGHT"));
}

// User wants to load this game
void M_LoadSelect(int32 choice)
{
    g_doom->GetGame()->LoadGame(Game::GetSaveFilePath(choice));
    M_ClearMenus();
}

// Selected from DOOM menu
void M_LoadGame(int)
{
    if (netgame)
    {
        Menu::StartMessage(LOADNET, nullptr, false);
        return;
    }

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}

void M_DrawSave()
{
    int             i;

    g_doom->GetVideo()->DrawPatch(72, 28, 0, WadManager::GetLumpData<patch_t>("M_SAVEG"));
    for (i = 0;i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
    }

    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[saveSlot]);
        M_WriteText(LoadDef.x + i, LoadDef.y + LINEHEIGHT * saveSlot, "_");
    }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    G_SaveGame(slot, savegamestrings[slot]);
    M_ClearMenus();

    // PICK QUICKSAVE SLOT YET?
    if (quickSaveSlot == -2)
        quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;

    saveSlot = choice;
    saveOldString = savegamestrings[choice];
    if (saveOldString == EMPTYSTRING)
        savegamestrings[choice].clear();
    saveCharIndex = savegamestrings[choice].length();
}

//
// Selected from DOOM menu
//
void M_SaveGame(int)
{
    if (!usergame)
    {
        Menu::StartMessage(SAVEDEAD, nullptr, false);
        return;
    }

    if (g_doom->GetGameState() != GameState::Level)
        return;

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

void M_QuickSaveResponse(bool response)
{
    if (response)
    {
        M_DoSave(quickSaveSlot);
        S_StartSound(nullptr, sfx_swtchx);
    }
}

void M_QuickSave()
{
    if (!usergame)
    {
        S_StartSound(nullptr, sfx_oof);
        return;
    }

    if (g_doom->GetGameState() != GameState::Level)
        return;

    if (quickSaveSlot < 0)
    {
        M_StartControlPanel();
        M_ReadSaveStrings();
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2;	// means to pick a slot now
        return;
    }

    Menu::StartMessage(std::format(QSPROMPT, savegamestrings[quickSaveSlot]), M_QuickSaveResponse, true);
}

void M_QuickLoadResponse(bool response)
{
    if (response)
    {
        M_LoadSelect(quickSaveSlot);
        S_StartSound(nullptr, sfx_swtchx);
    }
}


void M_QuickLoad()
{
    if (netgame)
    {
        Menu::StartMessage(QLOADNET, nullptr, false);
        return;
    }

    if (quickSaveSlot < 0)
    {
        Menu::StartMessage(QSAVESPOT, nullptr, false);
        return;
    }

    Menu::StartMessage(std::format(QLPROMPT, savegamestrings[quickSaveSlot]), M_QuickLoadResponse, true);
}

// Read This Menus
// Had a "quick hack to fix romero bug"
void M_DrawReadThis1()
{
    inhelpscreens = true;
    switch (g_doom->GetGameMode())
    {
    case GameMode::Doom2Commercial:
        g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("HELP"));
        break;
    case GameMode::Doom1Shareware:
    case GameMode::Doom1Registered:
    case GameMode::Doom1Retail:
        g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("HELP1"));
        break;
    default:
        break;
    }
    return;
}

// Read This Menus - optional second page.
void M_DrawReadThis2()
{
    inhelpscreens = true;
    switch (g_doom->GetGameMode())
    {
    case GameMode::Doom1Retail:
    case GameMode::Doom2Commercial:
        // This hack keeps us from having to change menus.
        g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("CREDIT"));
        break;
    case GameMode::Doom1Shareware:
    case GameMode::Doom1Registered:
        g_doom->GetVideo()->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>("HELP2"));
        break;
    default:
        break;
    }
}

// Change Sfx & Music volumes
void M_DrawSound()
{
    g_doom->GetVideo()->DrawPatch(60, 38, 0, WadManager::GetLumpData<patch_t>("M_SVOL"));

    M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1), 16, snd_SfxVolume);

    M_DrawThermo(SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1), 16, snd_MusicVolume);
}

void M_Sound(int)
{
    M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (snd_SfxVolume)
            snd_SfxVolume--;
        break;
    case 1:
        if (snd_SfxVolume < 15)
            snd_SfxVolume++;
        break;
    }

    S_SetSfxVolume(snd_SfxVolume /* *8 */);
}

void M_MusicVol(int choice)
{
    switch (choice)
    {
    case 0:
        if (snd_MusicVolume)
            snd_MusicVolume--;
        break;
    case 1:
        if (snd_MusicVolume < 15)
            snd_MusicVolume++;
        break;
    }

    S_SetMusicVolume(snd_MusicVolume /* *8 */);
}

void M_DrawMainMenu()
{
    g_doom->GetVideo()->DrawPatch(94, 2, 0, WadManager::GetLumpData<patch_t>("M_DOOM"));
}

void M_DrawNewGame()
{
    g_doom->GetVideo()->DrawPatch(96, 14, 0, WadManager::GetLumpData<patch_t>("M_NEWG"));
    g_doom->GetVideo()->DrawPatch(54, 38, 0, WadManager::GetLumpData<patch_t>("M_SKILL"));
}

void M_NewGame(int)
{
    if (netgame && !demoplayback)
    {
        Menu::StartMessage(NEWGAME, nullptr, false);
        return;
    }

    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
        M_SetupNextMenu(&NewDef);
    else
        M_SetupNextMenu(&EpiDef);
}

//      M_Episode
int     epi;

void M_DrawEpisode()
{
    g_doom->GetVideo()->DrawPatch(54, 38, 0, WadManager::GetLumpData<patch_t>("M_EPISOD"));
}

void M_VerifyNightmare(bool response)
{
    if (!response)
        return;

    G_DeferedInitNew(static_cast<skill_t>(nightmare), epi + 1, 1);
    M_ClearMenus();
}

void M_ChooseSkill(int choice)
{
    if (choice == nightmare)
    {
        Menu::StartMessage(NIGHTMARE, M_VerifyNightmare, true);
        return;
    }

    G_DeferedInitNew(static_cast<skill_t>(choice), epi + 1, 1);
    M_ClearMenus();
}

void M_Episode(int choice)
{
    if ((g_doom->GetGameMode() == GameMode::Doom1Shareware) && choice)
    {
        Menu::StartMessage(SWSTRING, nullptr, false);
        M_SetupNextMenu(&ReadDef1);
        return;
    }

    // Yet another hack...
    if ((g_doom->GetGameMode() == GameMode::Doom1Registered) && (choice > 2))
    {
        std::cerr << "M_Episode: 4th episode requires UltimateDOOM\n";
        choice = 0;
    }

    epi = choice;
    M_SetupNextMenu(&NewDef);
}

// M_Options
char    detailNames[2][9] = { "M_GDHIGH","M_GDLOW" };
char	msgNames[2][9] = { "M_MSGOFF","M_MSGON" };

void M_DrawOptions()
{
    g_doom->GetVideo()->DrawPatch(108, 15, 0, WadManager::GetLumpData<patch_t>("M_OPTTTL"));

    g_doom->GetVideo()->DrawPatch(OptionsDef.x + 175, OptionsDef.y + LINEHEIGHT * detail, 0, WadManager::GetLumpData<patch_t>(detailNames[detailLevel]));

    g_doom->GetVideo()->DrawPatch(OptionsDef.x + 120, OptionsDef.y + LINEHEIGHT * messages, 0, WadManager::GetLumpData<patch_t>(msgNames[showMessages]));

    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (mousesens + 1), 10, mouseSensitivity);

    M_DrawThermo(OptionsDef.x, OptionsDef.y + LINEHEIGHT * (scrnsize + 1), 9, screenSize);
}

void M_Options(int)
{
    M_SetupNextMenu(&OptionsDef);
}



//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;

    if (!showMessages)
        players[consoleplayer].message = MSGOFF;
    else
        players[consoleplayer].message = MSGON;

    message_dontfuckwithme = true;
}

void M_EndGameResponse(bool response)
{
    if (!response)
        return;

    currentMenu->lastOn = itemOn;
    M_ClearMenus();
    g_doom->StartTitle();
}

void M_EndGame(int choice)
{
    choice = 0;
    if (!usergame)
    {
        S_StartSound(nullptr, sfx_oof);
        return;
    }

    if (netgame)
    {
        Menu::StartMessage(NETEND, nullptr, false);
        return;
    }

    Menu::StartMessage(ENDGAME, M_EndGameResponse, true);
}

//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}




//
// M_QuitDOOM
//
int     quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

int     quitsounds2[8] =
{
    sfx_vilact,
    sfx_getpow,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};



void M_QuitResponse(bool response)
{
    if (!response)
        return;

    if (!netgame)
    {
        if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
            S_StartSound(nullptr, quitsounds2[(gametic >> 2) & 7]);
        else
            S_StartSound(nullptr, quitsounds[(gametic >> 2) & 7]);
        I_WaitVBL(105);
    }

    I_Quit();
}

void M_QuitDOOM(int)
{
    Menu::StartMessage(std::format("{}\n\n" DOSY, endmsg[(gametic % (NUM_QUITMESSAGES - 2)) + 1]), M_QuitResponse, true);
}

void M_ChangeSensitivity(int choice)
{
    switch (choice)
    {
    case 0:
        if (mouseSensitivity)
            mouseSensitivity--;
        break;
    case 1:
        if (mouseSensitivity < 9)
            mouseSensitivity++;
        break;
    }
}

void M_ChangeDetail(int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    g_doom->GetRender()->RequestSetViewSize(screenBlocks, detailLevel);
}

void M_SizeDisplay(int choice)
{
    switch (choice)
    {
    case 0:
        if (screenSize > 0)
        {
            screenBlocks--;
            screenSize--;
        }
        break;
    case 1:
        if (screenSize < 8)
        {
            screenBlocks++;
            screenSize++;
        }
        break;
    }

    g_doom->GetRender()->RequestSetViewSize(screenBlocks, detailLevel);
}

//      Menu Functions
void M_DrawThermo(int x, int y, int thermWidth, int thermDot)
{
    int		xx;
    int		i;

    xx = x;
    g_doom->GetVideo()->DrawPatch(xx, y, 0, WadManager::GetLumpData<patch_t>("M_THERML"));
    xx += 8;
    for (i = 0;i < thermWidth;i++)
    {
        g_doom->GetVideo()->DrawPatch(xx, y, 0, WadManager::GetLumpData<patch_t>("M_THERMM"));
        xx += 8;
    }
    g_doom->GetVideo()->DrawPatch(xx, y, 0, WadManager::GetLumpData<patch_t>("M_THERMR"));

    g_doom->GetVideo()->DrawPatch((x + 8) + thermDot * 8, y, 0, WadManager::GetLumpData<patch_t>("M_THERMO"));
}

void M_DrawEmptyCell(menu_t* menu, int32 item)
{
    g_doom->GetVideo()->DrawPatch(menu->x - 10, menu->y + item * LINEHEIGHT - 1, 0, WadManager::GetLumpData<patch_t>("M_CELL1"));
}

void M_DrawSelCell(menu_t* menu, int32 item)
{
    g_doom->GetVideo()->DrawPatch(menu->x - 10, menu->y + item * LINEHEIGHT - 1, 0, WadManager::GetLumpData<patch_t>("M_CELL2"));
}

// Find string width from hu_font chars
int32 M_StringWidth(string_view str)
{
    int32 width = 0;
    for (auto c : str)
    {
        c = static_cast<char>(toupper(c)) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
            width += 4;
        else
            width += hu_font[c]->width;
    }

    return width;
}

// Find string height from hu_font chars
int32 M_StringHeight(string_view str)
{
    int32 height = hu_font[0]->height;

    auto out = height;
    for (auto c : str)
    {
        if (c == '\n')
            out += height;
    }

    return out;
}

// Write a string using the hu_font
void M_WriteText(int32 x, int32 y, string_view text)
{
    auto cx = x;
    auto cy = y;

    for (auto c : text)
    {
        if (c == '\n')
        {
            cx = x;
            cy += 12;
            continue;
        }

        c = static_cast<char>(toupper(c)) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        auto w = hu_font[c]->width;
        if (cx + w > SCREENWIDTH)
            break;

        g_doom->GetVideo()->DrawPatch(cx, cy, 0, hu_font[c]);
        cx += w;
    }
}

// CONTROL PANEL

bool M_Responder(const input::event& event)
{
    static  time_t joywait = 0;
    static float x_count = 0;
    static float y_count = 0;

    int32 ch = -1;

    auto nav_up_down = [&](int32 dir)
    {
        do
        {
            itemOn = (itemOn + currentMenu->numitems + dir) % currentMenu->numitems;
            S_StartSound(nullptr, sfx_pstop);
        }
        while (currentMenu->menuitems[itemOn].status == -1);
    };

    auto nav_left_right = [&](int32 dir)
    {
        auto& item = currentMenu->menuitems[itemOn];
        if (item.routine && item.status == 2)
        {
            S_StartSound(nullptr, sfx_stnmov);
            item.routine(dir);
        }
    };

    if (event.is_controller() && joywait < I_GetTime())
    {
        if (event.is("JoyYNeg"))
        {
            nav_up_down(-1);
            joywait = I_GetTime() + 5;
        }
        else if (event.is("JoyYPos"))
        {
            nav_up_down(1);
            joywait = I_GetTime() + 5;
        }

        if (event.is("JoyXNeg"))
        {
            nav_left_right(0);
            joywait = I_GetTime() + 2;
        }
        else if (event.is("JoyXPos"))
        {
            nav_left_right(1);
            joywait = I_GetTime() + 2;
        }

        if (event.down("Button1"))
        {
            ch = KEY_ENTER;
            joywait = I_GetTime() + 5;
        }
        if (event.down("Button2"))
        {
            ch = KEY_BACKSPACE;
            joywait = I_GetTime() + 5;
        }
    }
    else if (event.is_mouse())
    {
        if (event.is("MouseDeltaY"))
        {
            static const float delta_threshold = 100.f;

            y_count += event.i_value;
            if (menuactive && std::abs(y_count) > delta_threshold)
            {
                nav_up_down(nstd::sign(event.i_value));
                y_count -= std::copysign(delta_threshold, y_count);
            }
        }

        if (event.is("MouseDeltaX"))
        {
            static const float delta_threshold = 100.f;

            x_count += event.i_value;
            if (std::abs(x_count) > delta_threshold)
            {
                nav_left_right(nstd::sign((event.i_value) + 1) / 2);
                x_count -= std::copysign(delta_threshold, x_count);
            }
        }

        if (event.down("MouseLeft"))
            ch = KEY_ENTER;

        if (event.down("MouseRight"))
            ch = KEY_BACKSPACE;
    }
    else if (event.is_keyboard() && event.down())
    {
        ch = event.id.value;
    }

    if (ch == -1)
        return false;

    // Save Game string input
    if (saveStringEnter)
    {
        switch (ch)
        {
        case KEY_BACKSPACE:
            if (saveCharIndex > 0)
            {
                saveCharIndex--;
                savegamestrings[saveSlot].pop_back();
            }
            break;

        case KEY_ESCAPE:
            saveStringEnter = 0;
            savegamestrings[saveSlot] = saveOldString;
            break;

        case KEY_ENTER:
            saveStringEnter = 0;
            if (!savegamestrings[saveSlot].empty())
                M_DoSave(saveSlot);
            break;

        default:
            ch = toupper(ch);
            if (ch != 32)
                if (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE)
                    break;
            if (ch >= 32 && ch <= 127 &&
                saveCharIndex < Game::SaveStringSize - 1 &&
                M_StringWidth(savegamestrings[saveSlot]) <
                (Game::SaveStringSize - 2) * 8)
            {
                savegamestrings[saveSlot] += static_cast<char>(ch);
            }
            break;
        }
        return true;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
        bool is_affirmative = event.down("Space") || event.down("Y");
        bool is_relevant = is_affirmative || event.down("N") || event.down("Escape");

        if (messageNeedsInput && !is_relevant)
            return false;

        menuactive = messageLastMenuActive;
        messageToPrint = 0;
        if (messageRoutine)
            messageRoutine(is_affirmative);

        menuactive = false;
        S_StartSound(nullptr, sfx_swtchx);
        return true;
    }

    if (g_doom->IsDevMode() && event.down("F1"))
    {
        G_ScreenShot();
        return true;
    }

    // F-Keys
    if (!menuactive)
    {
        switch (event.id.value)
        {
        case input::event_id("Minus"):         // Screen size down
            if (automapactive || chat_on)
                return false;
            M_SizeDisplay(0);
            S_StartSound(nullptr, sfx_stnmov);
            return true;

        case input::event_id("Plus"):        // Screen size up
            if (automapactive || chat_on)
                return false;
            M_SizeDisplay(1);
            S_StartSound(nullptr, sfx_stnmov);
            return true;

        case input::event_id("F1"):            // Help key
            M_StartControlPanel();

            if (g_doom->GetGameMode() == GameMode::Doom1Retail)
                currentMenu = &ReadDef2;
            else
                currentMenu = &ReadDef1;

            itemOn = 0;
            S_StartSound(nullptr, sfx_swtchn);
            return true;

        case input::event_id("F2"):            // Save
            M_StartControlPanel();
            S_StartSound(nullptr, sfx_swtchn);
            M_SaveGame(0);
            return true;

        case input::event_id("F3"):            // Load
            M_StartControlPanel();
            S_StartSound(nullptr, sfx_swtchn);
            M_LoadGame(0);
            return true;

        case input::event_id("F4"):            // Sound Volume
            M_StartControlPanel();
            currentMenu = &SoundDef;
            itemOn = sfx_vol;
            S_StartSound(nullptr, sfx_swtchn);
            return true;

        case input::event_id("F5"):            // Detail toggle
            M_ChangeDetail(0);
            S_StartSound(nullptr, sfx_swtchn);
            return true;

        case input::event_id("F6"):            // Quicksave
            S_StartSound(nullptr, sfx_swtchn);
            M_QuickSave();
            return true;

        case input::event_id("F7"):            // End game
            S_StartSound(nullptr, sfx_swtchn);
            M_EndGame(0);
            return true;

        case input::event_id("F8"):            // Toggle messages
            M_ChangeMessages(0);
            S_StartSound(nullptr, sfx_swtchn);
            return true;

        case input::event_id("F9"):            // Quickload
            S_StartSound(nullptr, sfx_swtchn);
            M_QuickLoad();
            return true;

        case input::event_id("F10"):           // Quit DOOM
            S_StartSound(nullptr, sfx_swtchn);
            M_QuitDOOM(0);
            return true;

        case input::event_id("F11"):           // gamma toggle
            usegamma++;
            if (usegamma > 4)
                usegamma = 0;
            players[consoleplayer].message = gammamsg[usegamma];
            g_doom->GetVideo()->SetPalette(WadManager::GetLumpData<byte>("PLAYPAL"));
            return true;
        }
    }

    // Pop-up menu?
    if (!menuactive)
    {
        if (event.is("Escape"))
        {
            M_StartControlPanel();
            S_StartSound(nullptr, sfx_swtchn);
            return true;
        }
        return false;
    }

    // Keys usable within menu
    switch (event.id.value)
    {
    case input::event_id("DownArrow"):
        nav_up_down(1);
        return true;

    case input::event_id("UpArrow"):
        nav_up_down(-1);
        return true;

    case input::event_id("LeftArrow"):
        nav_left_right(0);
        return true;

    case input::event_id("RightArrow"):
        nav_left_right(1);
        return true;

    case input::event_id("Enter"):
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                currentMenu->menuitems[itemOn].routine(1);      // right arrow
                S_StartSound(nullptr, sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(itemOn);
                S_StartSound(nullptr, sfx_pistol);
            }
        }
        return true;

    case input::event_id("Escape"):
        currentMenu->lastOn = itemOn;
        M_ClearMenus();
        S_StartSound(nullptr, sfx_swtchx);
        return true;

    case input::event_id("Backspace"):
        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(nullptr, sfx_swtchn);
        }
        return true;

    default:
        for (short i = itemOn + 1;i < currentMenu->numitems;i++)
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(nullptr, sfx_pstop);
                return true;
            }
        for (short i = 0;i <= itemOn;i++)
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(nullptr, sfx_pstop);
                return true;
            }
        break;

    }

    return false;
}

void M_StartControlPanel()
{
    // intro might call this repeatedly
    if (menuactive)
        return;

    menuactive = 1;
    currentMenu = &MainDef;         // JDC
    itemOn = currentMenu->lastOn;   // JDC
}

// Called after the view has been rendered,
// but before it has been blitted.
void M_Drawer()
{
    static int x = 0;
    static int y = 0;

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        string_view drawString = Menu::messageString;
        string_view::size_type start = 0;
        y = 100 - M_StringHeight(drawString) / 2;
        while (start != string_view::npos && start < Menu::messageString.length())
        {
            drawString = Menu::messageString;

            auto end = drawString.find('\n', start);
            if (end != string::npos)
            {
                drawString = drawString.substr(start, end - start);
                start = end + 1;
            }
            else
            {
                drawString = drawString.substr(start);
                start = end;
            }

            x = 160 - M_StringWidth(drawString) / 2;
            M_WriteText(x, y, drawString);
            y += hu_font[0]->height;
        }

        return;
    }

    if (!menuactive)
        return;

    if (currentMenu->routine)
        currentMenu->routine();         // call Draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    short max = currentMenu->numitems;

    for (short i = 0;i < max;i++)
    {
        if (currentMenu->menuitems[i].name[0])
            g_doom->GetVideo()->DrawPatch(x, y, 0, WadManager::GetLumpData<patch_t>(currentMenu->menuitems[i].name));
        y += LINEHEIGHT;
    }

    // DRAW SKULL
    g_doom->GetVideo()->DrawPatch(x + SKULLXOFF, currentMenu->y - 5 + itemOn * LINEHEIGHT, 0, WadManager::GetLumpData<patch_t>(skullName[whichSkull]));
}

void M_ClearMenus()
{
    menuactive = 0;
    // if (!netgame && usergame && paused)
    //       sendpause = true;
}

void M_SetupNextMenu(menu_t* menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}

void M_Ticker()
{
    if (--skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8;
    }
}

string Menu::messageString;

void Menu::Init()
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = 10;
    screenSize = screenBlocks - 3;
    messageToPrint = 0;
    messageString = "";
    messageLastMenuActive = menuactive;
    quickSaveSlot = -1;

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.


    switch (g_doom->GetGameMode())
    {
    case GameMode::Doom2Commercial:
        // This is used because DOOM 2 had only one HELP
            //  page. I use CREDIT as second page now, but
        //  kept this hack for educational purposes.
        MainMenu[readthis] = MainMenu[quitdoom];
        MainDef.numitems--;
        MainDef.y += 8;
        NewDef.prevMenu = &MainDef;
        ReadDef1.routine = M_DrawReadThis1;
        ReadDef1.x = 330;
        ReadDef1.y = 165;
        ReadMenu1[0].routine = M_FinishReadThis;
        break;
    case GameMode::Doom1Shareware:
        // Episode 2 and 3 are handled,
        //  branching to an ad screen.
    case GameMode::Doom1Registered:
        // We need to remove the fourth episode.
        EpiDef.numitems--;
        break;
    case GameMode::Doom1Retail:
        // We are fine.
    default:
        break;
    }

}

void Menu::StartMessage(string_view message, void(*routine)(bool), bool input)
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = message;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = true;
}
