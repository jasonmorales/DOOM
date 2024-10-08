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
//	DOOM main program (Doom::Main) and game loop (Doom::Loop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------
#include "i_video.h"

#include "doomdef.h"
#include "doomstat.h"
#include "dstrings.h"
#include "sounds.h"
#include "am_map.h"
#include "d_main.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_misc.h"
#include "m_menu.h"
#include "p_setup.h"
#include "r_main.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"
#include "r_draw.h"

//#include <cstdio>

import std;
import config;
import log;
import input;


#define BGCOLOR 7
#define FGCOLOR 8


void G_BuildTiccmd(ticcmd_t* cmd);

extern bool inhelpscreens;
extern int32 showMessages;
extern int forwardmove[2];
extern int sidemove[2];
extern void* statcopy;

bool advancedemo;

bool nomonsters;	 // checkparm of -nomonsters
bool respawnparm; // checkparm of -respawn
bool fastparm;	 // checkparm of -fast

skill_t startskill;
int startepisode;
int startmap;
bool autostart;

std::ofstream debugfile;

char wadfile[1024];		// primary wad file
char mapdir[1024];		// directory of development maps

// EVENT HANDLING

bool Doom::HasEscEventInQueue()
{
    for (auto& event : input::manager::get_event_queue())
    {
        if (event.down("Escape"))
            return true;
    }

    return false;
}

void Doom::Main()
{
    IdentifyVersion();

    std::cout.setf(std::ios::unitbuf);

    nomonsters = CommandLine::HasArg("-nomonsters");
    respawnparm = CommandLine::HasArg("-respawn");
    fastparm = CommandLine::HasArg("-fast");
    isDevMode = CommandLine::HasArg("-devparm");

    if (CommandLine::HasArg("-altdeath"))
        deathmatch = 2;
    else if (CommandLine::HasArg("-deathmatch"))
        deathmatch = 1;

    string title;
    switch (gameMode)
    {
    case GameMode::Doom1Retail:
        title = std::format(
            "                         "
            "The Ultimate DOOM Startup v{}.{}"
            "                           ",
            Version / 100, Version % 100);
        break;
    case GameMode::Doom1Shareware:
        title = std::format(
            "                            "
            "DOOM Shareware Startup v{}.{}"
            "                           ",
            Version / 100, Version % 100);
        break;
    case GameMode::Doom1Registered:
        title = std::format(
            "                            "
            "DOOM Registered Startup v{}.{}"
            "                           ",
            Version / 100, Version % 100);
        break;
    case GameMode::Doom2Commercial:
        title = std::format(
            "                         "
            "DOOM 2: Hell on Earth v{}.{}"
            "                           ",
            Version / 100, Version % 100);
        break;
    default:
        title = std::format(
            "                     "
            "Public DOOM - v{}.{}"
            "                           ",
            Version / 100, Version % 100);
        break;
    }

    logger::write(title);

    if (isDevMode)
        std::printf(D_DEVSTR);

    // turbo option
    if (CommandLine::HasArg("-turbo"))
    {
        int scale = 200;
        CommandLine::TryGetValues("-turbo", scale);

        if (scale < 10)
            scale = 10;
        if (scale > 400)
            scale = 400;

        std::printf("turbo scale: %i%%\n", scale);
        forwardmove[0] = forwardmove[0] * scale / 100;
        forwardmove[1] = forwardmove[1] * scale / 100;
        sidemove[0] = sidemove[0] * scale / 100;
        sidemove[1] = sidemove[1] * scale / 100;
    }

    auto doWarp = [this](int32 ep, int32 map)
        {
            if (gameMode == GameMode::Doom2Commercial)
                startmap = ep;
            else
            {
                startepisode = ep;
                startmap = map;
            }
            autostart = true;
        };

    // add any files specified on the command line with -file wadfile to the wad list
    // convenience hack to allow -wart e m to add a wad file prepend a tilde to the filename so wadfile will be reloadable
    if (int32 ep = 0, map = 0; CommandLine::TryGetValues("-wart", ep, map))
    {
        string file;
        // Map name handling.
        switch (gameMode)
        {
        case GameMode::Doom1Shareware:
        case GameMode::Doom1Retail:
        case GameMode::Doom1Registered:
            file = std::format("~{}E{}M{}.wad", Settings::DevMapPath, ep, map);
            std::cout << "Warping to Episode " << ep << ", Map " << map << ".\n";
            break;

        case GameMode::Doom2Commercial:
        default:
            file = std::format("~{}cdata/map{:02d}.wad", Settings::DevMapPath, ep);
            break;
        }
        WadManager::AddFile(file);

        doWarp(ep,map);
    }

    if (vector<string_view> fileList; CommandLine::GetValueList("-file", fileList))
    {
        isModified = true; // homebrew levels
        for (auto fileName : fileList)
            WadManager::AddFile(fileName);
    }

    if (string_view name;
        CommandLine::TryGetValues("-playdemo", name) ||
        CommandLine::TryGetValues("-timedemo", name))
    {
        string fileName(name);
        fileName += ".lmp";
        WadManager::AddFile(fileName);
        std::cout << "Playing demo " << fileName << ".\n";
    }

    // get skill / episode / map from parms
    startskill = sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;

    if (int32 skill = 0; CommandLine::TryGetValues("-skill", skill))
    {
        startskill = static_cast<skill_t>(skill);
        autostart = true;
    }

    if (CommandLine::TryGetValues("-episode", startepisode))
    {
        startmap = 1;
        autostart = true;
    }

    if (int32 time = 0; CommandLine::TryGetValues("-timer", time) && deathmatch)
        std::cout << std::format("Levels will end after {} minute{}.\n", time, (time > 1) ? "s" : "");

    if (CommandLine::HasArg("-avg") && deathmatch)
        std::cout << "Austin Virtual Gaming: Levels will end after 20 minutes\n";

    if (int32 ep = 0, map = 0; CommandLine::TryGetValues("-warp", ep, map))
        doWarp(ep,map);

    // init subsystems
    logger::write("Z_Init: Init zone memory allocation daemon.");
    Z_Init();

    std::cout << "Video::Init: allocate screens.\n";
    video = new Video(this);
    video->Init();

    std::cout << "Settings::Load: Load system defaults.\n";
    Settings::Init();
    Settings::Load(); // load before initing other systems

    std::cout << "WadManger::LoadAllFiles: Init WADfiles.\n";
    WadManager::LoadAllFiles();

    std::cout << "Init Game\n";
    game = new Game(this);

    // Check for -file in shareware
    if (isModified)
    {
        // These are the lumps that will be checked in IWAD,
        // if any one is not present, execution will be aborted.
        string_view names[] =
        {
            "e2m1", "e2m2", "e2m3", "e2m4", "e2m5", "e2m6", "e2m7", "e2m8", "e2m9",
            "e3m1", "e3m3", "e3m3", "e3m4", "e3m5", "e3m6", "e3m7", "e3m8", "e3m9",
            "dphoof", "bfgga0", "heada1", "cybra1", "spida1d1" };

        if (gameMode == GameMode::Doom1Shareware)
            I_Error("\nYou cannot -file with the shareware version. Register!");

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version.
        if (gameMode == GameMode::Doom1Registered)
        {
            for (int32 i = 0; i < 23; ++i)
            {
                if (WadManager::GetLumpId(names[i]) == INVALID_ID)
                    I_Error("\nThis is not the registered version.");
            }
        }
    }

    // If additional PWAD files are used, print modified banner
    if (isModified)
    {
        /*m*/ std::printf(
            "===========================================================================\n"
            "ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
            "get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
            "        You will not receive technical support for modified games.\n"
            "                      press enter to continue\n"
            "===========================================================================\n");
        std::getchar();
    }

    // Check and print which version is executed.
    switch (gameMode)
    {
    case GameMode::Doom1Shareware:
    case GameMode::Unknown:
        std::printf(
            "===========================================================================\n"
            "                                Shareware!\n"
            "===========================================================================\n");
        break;
    case GameMode::Doom1Registered:
    case GameMode::Doom1Retail:
    case GameMode::Doom2Commercial:
        std::printf(
            "===========================================================================\n"
            "                 Commercial product - do not distribute!\n"
            "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
            "===========================================================================\n");
        break;

    default:
        // Ouch.
        break;
    }

    std::printf("Menu::Init: Init miscellaneous info.\n");
    Menu::Init();

    std::printf("Render::Init: Init DOOM refresh daemon - ");
    render = new Render;
    render->Init();

    std::printf("\nP_Init: Init Playloop state.\n");
    P_Init(this);

    std::printf("I_Init: Setting up machine state.\n");
    I_Init();

    std::printf("Net::CheckGame: Checking network game status.\n");
    Net::CheckGame();

    std::printf("S_Init: Setting up sound.\n");
    S_Init(snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/);

    printf("HU_Init: Setting up heads up display.\n");
    HU_Init();

    printf("ST_Init: Init status bar.\n");
    ST_Init();

    // check for a driver that wants intermission stats
    if (int64 val; CommandLine::TryGetValues("-playdemo", val))
    {
        // for statistics driver
        statcopy = reinterpret_cast<void*>(val);
        printf("External statistics registered.\n");
    }

    // start the appropriate game based on params
    if (string name; CommandLine::TryGetValues("-record", name))
    {
        G_RecordDemo(this, name);
        autostart = true;
    }

    if (string demo; CommandLine::TryGetValues("-playdemo", demo))
    {
        singledemo = true; // quit after one demo
        G_DeferedPlayDemo(demo.c_str());
        Loop(); // never returns
    }

    if (string demo; CommandLine::TryGetValues("-timedemo", demo))
    {
        noDrawers = CommandLine::HasArg("-nodraw");
        G_TimeDemo(demo.c_str());
        Loop(); // never returns
    }

    if (int32 load; CommandLine::TryGetValues("-loadgame", load))
        game->LoadGame(Game::GetSaveFilePath(load));

    if (gameaction != ga_loadgame)
    {
        if (autostart || netgame)
            G_InitNew(startskill, startepisode, startmap);
        else
            StartTitle(); // start up intro loop
    }

    Loop(); // never returns
}

// Not a globally visible function, just included for source reference, called by Doom::Main,
// never exits.
// Manages timing and IO, calls all ?_Responder, ?_Ticker, and ?_Drawer, calls I_GetTime,
// Video::StartFrame, and Video::StartTic
void Doom::Loop()
{
    if (isDemoRecording)
        G_BeginRecording();

    if (CommandLine::HasArg("-debugfile"))
    {
        string fileName = std::format("debug{}.txt", consoleplayer);
        std::cout << "debug output to: " << fileName << "\n";
        debugfile.open(fileName, std::ios_base::out);
    }

    for (;;)
    {
        // frame synchronous IO operations
        video->StartFrame();

        // process one or more tics
        if (useSingleTicks)
        {
            video->StartTick();
            ProcessEvents();
            G_BuildTiccmd(&netcmds[consoleplayer][maketic % BACKUPTICS]);
            if (advancedemo)
                DoAdvanceDemo();
            M_Ticker();
            game->Ticker();
            gametic++;
            maketic++;
        }
        else
        {
            TryRunTics(); // will run at least one tic
        }

        S_UpdateSounds(players[consoleplayer].mo); // move positional sounds

        // Update display, next frame, with current state.
        Display();

        // Sound mixing for the buffer is snychronous.
        Sound::Update();
    }
}

// Send all the events of the given timestamp down the responder chain
void Doom::ProcessEvents()
{
    for (auto& event : input::manager::get_event_queue())
    {
        if (M_Responder(event))
            continue; // menu ate the event

        G_Responder(event);
    }

    input::manager::flush_character_stream();
    input::manager::flush_event_queue();
}

int pagetic;
const char* pagename;

// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
void Doom::DoAdvanceDemo()
{
    players[consoleplayer].playerstate = PST_LIVE; // not reborn
    advancedemo = false;
    usergame = false; // no save / end game here
    paused = false;
    gameaction = ga_nothing;

    if (gameMode == GameMode::Doom1Retail)
        borderDrawCount = (borderDrawCount + 1) % 7;
    else
        borderDrawCount = (borderDrawCount + 1) % 6;

    switch (borderDrawCount)
    {
    case 0:
        if (gameMode == GameMode::Doom2Commercial)
            pagetic = 35 * 11;
        else
            pagetic = 170;
        gameState = GameState::Demo;
        pagename = "TITLEPIC";
        if (gameMode == GameMode::Doom2Commercial)
            S_StartMusic(mus_dm2ttl);
        else
            S_StartMusic(mus_intro);
        break;
    case 1:
        G_DeferedPlayDemo("demo1");
        break;
    case 2:
        pagetic = 200;
        gameState = GameState::Demo;
        pagename = "CREDIT";
        break;
    case 3:
        G_DeferedPlayDemo("demo2");
        break;
    case 4:
        gameState = GameState::Demo;
        if (gameMode == GameMode::Doom2Commercial)
        {
            pagetic = 35 * 11;
            pagename = "TITLEPIC";
            S_StartMusic(mus_dm2ttl);
        }
        else
        {
            pagetic = 200;

            if (gameMode == GameMode::Doom1Retail)
                pagename = "CREDIT";
            else
                pagename = "HELP2";
        }
        break;
    case 5:
        G_DeferedPlayDemo("demo3");
        break;
        // THE DEFINITIVE DOOM Special Edition demo
    case 6:
        G_DeferedPlayDemo("demo4");
        break;
    }
}

void Doom::StartTitle()
{
    gameaction = ga_nothing;
    borderDrawCount = -1;
    D_AdvanceDemo();
}

// Checks availability of IWAD files by name, to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
void Doom::IdentifyVersion()
{
    if (CommandLine::HasArg("-shdev"))
    {
        gameMode = GameMode::Doom1Shareware;
        isDevMode = true;

        WadManager::AddFile(Settings::DevDataPath / "doom1.wad");
        WadManager::AddFile(Settings::DevMapPath / "data_se/texture1.lmp");
        WadManager::AddFile(Settings::DevMapPath / "data_se/pnames.lmp");
        return;
    }

    if (CommandLine::HasArg("-regdev"))
    {
        gameMode = GameMode::Doom1Registered;
        isDevMode = true;

        WadManager::AddFile(Settings::DevDataPath /"doom.wad");
        WadManager::AddFile(Settings::DevMapPath / "data_se/texture1.lmp");
        WadManager::AddFile(Settings::DevMapPath / "data_se/texture2.lmp");
        WadManager::AddFile(Settings::DevMapPath / "data_se/pnames.lmp");
        return;
    }

    if (CommandLine::HasArg("-comdev"))
    {
        gameMode = GameMode::Doom2Commercial;
        isDevMode = true;

        WadManager::AddFile(Settings::DevDataPath /"doom2.wad");
        WadManager::AddFile(Settings::DevMapPath / "cdata/texture1.lmp");
        WadManager::AddFile(Settings::DevMapPath / "cdata/pnames.lmp");
        return;
    }

    gameMode = GameMode::Unknown;
    std::filesystem::path doomWadDir = "data";

    const vector<std::pair<filesys::path, GameMode>> coreWads = {
        { doomWadDir / "doom2.wad", GameMode::Doom2Commercial },    // Commercial
        { doomWadDir / "doomu.wad", GameMode::Doom1Retail },        // Retail
        { doomWadDir / "doom.wad", GameMode::Doom1Registered },     // Registered
        { doomWadDir / "doom1.wad", GameMode::Doom1Shareware },     // Shareware
    };

    for (const auto& entry : coreWads)
    {
        if (!filesys::exists(entry.first))
            continue;

        WadManager::AddFile(entry.first);
        gameMode = entry.second;
        break;
    }

    if (gameMode == GameMode::Unknown)
        std::cout << "Game mode indeterminate.\n";
}

//  draw current display, possibly wiping it from the previous
// wipegamestate can be set to -1 to force a wipe on the next draw
GameState wipegamestate = GameState::Demo;

void Doom::Display()
{
    if (noDrawers)
        return; // for comparative timing / profiling

    // change the view size if needed
    if (render->CheckSetViewSize())
    {
        oldGameState = GameState::ForceWipe; // force background redraw
        borderDrawCount = 3;
    }

    // save the current screen if about to wipe
    bool wipe = false;
    if (gameState != wipegamestate)
    {
        wipe = true;
        wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
    }

    if (gameState == GameState::Level && gametic)
        HU_Erase();

    bool redrawStatusBar = false;

    // do buffered drawing
    switch (gameState)
    {
    case GameState::Level:
        if (!gametic)
            break;
        if (automapactive)
            AM_Drawer();
        if (wipe || (viewheight != 200 && fullScreen))
            redrawStatusBar = true;
        if (inHelpScreensState && !inhelpscreens)
            redrawStatusBar = true; // just put away the help screen
        ST_Drawer(viewheight == 200, redrawStatusBar);
        fullScreen = viewheight == 200;
        break;

    case GameState::Intermission:
        WI_Drawer();
        break;

    case GameState::Finale:
        F_Drawer();
        break;

    case GameState::Demo:
        PageDraw();
        break;
    }

    // draw buffered stuff to screen
    video->UpdateNoBlit();

    // draw the view directly
    if (gameState == GameState::Level && !automapactive && gametic)
        R_RenderPlayerView(&players[displayplayer]);

    if (gameState == GameState::Level && gametic)
        HU_Drawer();

    // clean up border stuff
    if (gameState != oldGameState && gameState != GameState::Level)
        video->SetPalette(WadManager::GetLumpData<byte>("PLAYPAL"));

    // see if the border needs to be initially drawn
    if (gameState == GameState::Level && oldGameState != GameState::Level)
    {
        viewActiveState = false; // view was not active
        R_FillBackScreen();		 // draw the pattern into the back screen
    }

    // see if the border needs to be updated to the screen
    if (gameState == GameState::Level && !automapactive && scaledviewwidth != 320)
    {
        if (menuactive || menuActiveState || !viewActiveState)
            borderDrawCount = 3;
        if (borderDrawCount)
        {
            R_DrawViewBorder(); // erase old menu stuff
            borderDrawCount--;
        }
    }

    menuActiveState = menuactive;
    viewActiveState = viewactive;
    inHelpScreensState = inhelpscreens;
    oldGameState = wipegamestate = gameState;

    // draw pause pic
    if (paused)
    {
        int32 y = 0;
        if (automapactive)
            y = 4;
        else
            y = viewwindowy + 4;
        video->DrawPatch(viewwindowx + (scaledviewwidth - 68) / 2, y, 0, WadManager::GetLumpData<patch_t>("M_PAUSE"));
    }

    // menus go directly to the screen
    M_Drawer();	 // menu is drawn even on top of everything
    NetUpdate(); // send out any new accumulation

    // normal update
    if (!wipe)
    {
        video->FinishUpdate(); // page flip or blit buffer
        return;
    }

    // wipe update
    wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    auto wipestart = I_GetTime() - 1;

    bool done = false;
    do
    {
        time_t tics = 0;
        time_t now = 0;
        do
        {
            now = I_GetTime();
            tics = now - wipestart;
        }
        while (!tics);

        assert(tics > 0);
        wipestart = now;
        done = wipe_ScreenWipe(wipe_Melt, 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
        video->UpdateNoBlit();
        M_Drawer();		  // menu is drawn even on top of wipes
        video->FinishUpdate(); // page flip or blit buffer
    }
    while (!done);
}

void Doom::PageDraw()
{
    video->DrawPatch(0, 0, 0, WadManager::GetLumpData<patch_t>(pagename));
}

//  DEMO LOOP

// Handles timing for warped projection
void D_PageTicker()
{
    if (--pagetic < 0)
        D_AdvanceDemo();
}

// Called after each demo or intro demosequence finishes
void D_AdvanceDemo()
{
    advancedemo = true;
}
