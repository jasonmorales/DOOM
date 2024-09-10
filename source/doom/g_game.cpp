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
#include "i_video.h"

#include "doomdef.h" 
#include "doomstat.h"
#include "dstrings.h"
#include "sounds.h"
#include "am_map.h"
#include "d_main.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "p_local.h" 
#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "r_main.h"
#include "r_data.h"
#include "r_sky.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"
#include "r_draw.h"

import std;
import config;
import log;
import input;
import nstd;

extern Doom* g_doom;


const filesys::path Game::SavePath = "./saves";

bool	G_CheckDemoStatus(Doom* doom);
void	G_ReadDemoTiccmd(ticcmd_t* cmd);
void	G_WriteDemoTiccmd(ticcmd_t* cmd);
void	G_PlayerReborn(int player);
void	G_InitNew(skill_t skill, int episode, int map);

void	G_DoReborn(int playernum);

void	G_DoLoadLevel();
void	G_DoNewGame();
void	G_DoPlayDemo();
void	G_DoCompleted();
void	G_DoVictory();
void	G_DoWorldDone();


gameaction_t    gameaction;
skill_t         gameskill;
bool		respawnmonsters;
int             gameepisode;
int             gamemap;

bool         paused;
bool         sendpause;             	// send a pause event next tic 
bool         sendsave;             	// send a save event next tic 
bool         usergame;               // ok to save / end game 

bool         timingdemo;             // if true, exit with report on completion 
time_t starttime;          	// for comparative timing purposes  	 

bool         viewactive;

int deathmatch;           	// only if started as net death 
bool         netgame;                // only true if packets are broadcast 
bool playeringame[MAXPLAYERS];
player_t        players[MAXPLAYERS];

int             consoleplayer;          // player taking events and displaying 
int             displayplayer;          // view being displayed 
int             gametic;
int             levelstarttic;          // gametic at level start 
int             totalkills, totalitems, totalsecret;    // for intermission 

string demoname;
bool         demoplayback;
bool		netdemo;
const byte* demo_ibuffer;
const byte* demo_g;
byte* demo_obuffer;
byte* demo_p;
const byte* demoend;
bool         singledemo;            	// quit after playing a demo from cmdline 

bool         precache = true;        // if true, load all graphics at start 

wbstartstruct_t wminfo;               	// parms for world map / intermission 

short		consistancy[MAXPLAYERS][BACKUPTICS];

// controls (have defaults) 
int32             mousebfire;
int32             mousebstrafe;
int32             mousebforward;

int32             joybfire;
int32             joybstrafe;
int32             joybuse;
int32             joybspeed;



#define MAXPLMOVE		(forwardmove[1]) 

#define TURBOTHRESHOLD	0x32

fixed_t		forwardmove[2] = { 0x19, 0x32 };
fixed_t		sidemove[2] = { 0x18, 0x28 };
fixed_t		angleturn[3] = { 640, 1280, 320 };	// + slow turn 

#define SLOWTURNTICS	6 

bool gamekeydown[input::event_id::count] = {};
int32 turnheld;				// for accelerative turning 

bool mousearray[4];
bool* mousebuttons = &mousearray[1];		// allow [-1]

// mouse values are used once 
int             mousex;
int		mousey;

int             dclicktime;
bool		dclickstate;
int		dclicks;
int             dclicktime2;
bool		dclickstate2;
int		dclicks2;

// joystick values are repeated 
int             joyxmove;
int		joyymove;
bool         joyarray[5];
bool* joybuttons = &joyarray[1];		// allow [-1] 

int		savegameslot;
string saveDescription(24, '\0');


#define	BODYQUESIZE	32

mobj_t* bodyque[BODYQUESIZE];
int		bodyqueslot;

void* statcopy;				// for statistics driver



int G_CmdChecksum(ticcmd_t* cmd)
{
    int		i;
    int		sum = 0;

    for (i = 0; i < sizeof(*cmd) / 4 - 1; i++)
        sum += ((int*)cmd)[i];

    return sum;
}

// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
void G_BuildTiccmd(ticcmd_t* cmd)
{
    ticcmd_t* base = I_BaseTiccmd();		// empty, or external driver
    memcpy(cmd, base, sizeof(*cmd));

    cmd->consistancy = consistancy[consoleplayer][maketic % BACKUPTICS];

    auto strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
    auto speed = gamekeydown[key_speed] || joybuttons[joybspeed];

    // use two stage accelerative turning
    // on the keyboard and joystick
    if (joyxmove < 0
        || joyxmove > 0
        || gamekeydown[key_right]
        || gamekeydown[key_left])
        turnheld += ticdup;
    else
        turnheld = 0;

    auto tspeed = (turnheld < SLOWTURNTICS) ? 2 : speed; // slow turn 

    fixed_t side = 0;

    // let movement keys cancel each other out
    if (strafe)
    {
        if (gamekeydown[key_right])
        {
            // fprintf(stderr, "strafe right\n");
            side += sidemove[speed];
        }
        if (gamekeydown[key_left])
        {
            //	fprintf(stderr, "strafe left\n");
            side -= sidemove[speed];
        }
        if (joyxmove > 0)
            side += sidemove[speed];
        if (joyxmove < 0)
            side -= sidemove[speed];
    }
    else
    {
        if (gamekeydown[key_right])
            cmd->angleturn -= static_cast<short>(angleturn[tspeed]);
        if (gamekeydown[key_left])
            cmd->angleturn += static_cast<short>(angleturn[tspeed]);
        if (joyxmove > 0)
            cmd->angleturn -= static_cast<short>(angleturn[tspeed]);
        if (joyxmove < 0)
            cmd->angleturn += static_cast<short>(angleturn[tspeed]);
    }

    fixed_t forward = 0;

    if (gamekeydown[key_up])
    {
        // fprintf(stderr, "up\n");
        forward += forwardmove[speed];
    }
    if (gamekeydown[key_down])
    {
        // fprintf(stderr, "down\n");
        forward -= forwardmove[speed];
    }

    if (joyymove < 0)
        forward += forwardmove[speed];
    if (joyymove > 0)
        forward -= forwardmove[speed];
    if (gamekeydown[key_straferight])
        side += sidemove[speed];
    if (gamekeydown[key_strafeleft])
        side -= sidemove[speed];

    // buttons
    cmd->chatchar = HU_dequeueChatChar();

    if (gamekeydown[key_fire] || mousebuttons[mousebfire]
        || joybuttons[joybfire])
        cmd->buttons |= BT_ATTACK;

    if (gamekeydown[key_use] || joybuttons[joybuse])
    {
        cmd->buttons |= BT_USE;
        // clear double clicks if hit use button 
        dclicks = 0;
    }

    // chainsaw overrides 
    for (int32 i = 0; i < NUMWEAPONS - 1; i++)
        if (gamekeydown['1' + i])
        {
            cmd->buttons |= BT_CHANGE;
            cmd->buttons |= i << BT_WEAPONSHIFT;
            break;
        }

    // mouse
    if (mousebuttons[mousebforward])
        forward += forwardmove[speed];

    // forward double click
    if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1)
    {
        dclickstate = mousebuttons[mousebforward];
        if (dclickstate)
            dclicks++;

        if (dclicks == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks = 0;
        }
        else
            dclicktime = 0;
    }
    else
    {
        dclicktime += ticdup;
        if (dclicktime > 20)
        {
            dclicks = 0;
            dclickstate = false;
        }
    }

    // strafe double click
    bool bstrafe = mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
    if (bstrafe != dclickstate2 && dclicktime2 > 1)
    {
        dclickstate2 = bstrafe;
        if (dclickstate2)
            dclicks2++;
        if (dclicks2 == 2)
        {
            cmd->buttons |= BT_USE;
            dclicks2 = 0;
        }
        else
            dclicktime2 = 0;
    }
    else
    {
        dclicktime2 += ticdup;
        if (dclicktime2 > 20)
        {
            dclicks2 = 0;
            dclickstate2 = false;
        }
    }

    forward += mousey;
    if (strafe)
        side += mousex * 2;
    else
        cmd->angleturn -= static_cast<short>(mousex * 0x8);

    mousex = mousey = 0;

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += static_cast<char>(forward);
    cmd->sidemove += static_cast<char>(side);

    // special buttons
    if (sendpause)
    {
        sendpause = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (sendsave)
    {
        sendsave = false;
        cmd->buttons = static_cast<byte>(BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT));
    }
}

extern  GameState wipegamestate;

void G_DoLoadLevel()
{
    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.
    skyflatnum = R_FlatNumForName(SKYFLATNAME);

    // DOOM determines the sky texture to be used
    // depending on the current episode, and the game version.
    if ((g_doom->GetGameMode() == GameMode::Doom2Commercial))
    {
        skytexture = R_TextureNumForName("SKY3");
        if (gamemap < 12)
            skytexture = R_TextureNumForName("SKY1");
        else
            if (gamemap < 21)
                skytexture = R_TextureNumForName("SKY2");
    }

    levelstarttic = gametic;        // for time calculation

    if (wipegamestate == GameState::Level)
        wipegamestate = GameState::ForceWipe;             // force a wipe 

    g_doom->SetGameState(GameState::Level);

    for (int i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i] && players[i].playerstate == PST_DEAD)
            players[i].playerstate = PST_REBORN;
        memset(players[i].frags, 0, sizeof(players[i].frags));
    }

    P_SetupLevel(gameepisode, gamemap, 0, gameskill);
    displayplayer = consoleplayer;		// view the guy you are playing    
    starttime = I_GetTime();
    gameaction = ga_nothing;
    Z_CheckHeap();

    // clear cmd building stuff
    memset(gamekeydown, 0, sizeof(gamekeydown));
    joyxmove = joyymove = 0;
    mousex = mousey = 0;
    sendpause = sendsave = paused = false;
    memset(mousebuttons, 0, sizeof(mousebuttons));
    memset(joybuttons, 0, sizeof(joybuttons));
}

// Get info needed to make ticcmd_ts for the players.
bool G_Responder(const input::event& event)
{
    // allow spy mode changes even during the demo
    if (g_doom->GetGameState() == GameState::Level && event.is_keyboard() && event.down("F12") && (singledemo || !deathmatch))
    {
        // spy mode
        do
        {
            displayplayer++;
            if (displayplayer == MAXPLAYERS)
                displayplayer = 0;
        }
        while (!playeringame[displayplayer] && displayplayer != consoleplayer);
        return true;
    }

    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo &&
        (demoplayback || g_doom->GetGameState() == GameState::Demo)
        )
    {
        if ((event.is_keyboard() && event.down())||
            (event.is_mouse() && event.down()) ||
            (event.is_controller() && event.down()))
        {
            M_StartControlPanel();
            return true;
        }
        return false;
    }

    if (g_doom->GetGameState() == GameState::Level)
    {
        if (HU_Responder(event))
            return true;	// chat ate the event 
        if (ST_Responder(event))
            return true;	// status window ate it 
        if (AM_Responder(event))
            return true;	// automap ate it 
    }

    if (g_doom->GetGameState() == GameState::Finale)
    {
        if (F_Responder(event))
            return true;	// finale ate the event 
    }

    switch (event.device)
    {
    case input::enhanced_enum_base_type_device_id::Keyboard:
        if (event.down("Pause"))
        {
            sendpause = true;
            return true;
        }

        gamekeydown[event.id.value] = event.down();
        if (event.down())
            return true;    // eat key down events 
        
        return false;   // always let key up events filter down 

    case input::enhanced_enum_base_type_device_id::Mouse:
        if (event.is("MouseLeft")) mousebuttons[0] = event.down();
        if (event.is("MouseRight")) mousebuttons[1] = event.down();
        if (event.is("MouseMiddle")) mousebuttons[2] = event.down();
        if (event.is("MouseDeltaX")) mousex = event.i_value * (mouseSensitivity + 5) / 10;
        if (event.is("MouseDeltaY")) mousey = event.i_value * (mouseSensitivity + 5) / 10;
        return true;    // eat events 

    case input::enhanced_enum_base_type_device_id::Controller:
        if (event.is("Button1")) joybuttons[0] = event.down();
        if (event.is("Button2")) joybuttons[1] = event.down();
        if (event.is("Button3")) joybuttons[2] = event.down();
        if (event.is("Button4")) joybuttons[3] = event.down();
        if (event.is("JoyX")) joyxmove = event.i_value;
        if (event.is("JoyY")) joyymove = event.i_value;
        return true;    // eat events 

    default:
        break;
    }

    return false;
}

// Make ticcmd_ts for the players.
void Game::Ticker()
{
    // do player reborns if needed
    for (int32 i = 0; i < MAXPLAYERS; ++i)
        if (playeringame[i] && players[i].playerstate == PST_REBORN)
            G_DoReborn(i);

    // do things to change the game state
    while (gameaction != ga_nothing)
    {
        switch (gameaction)
        {
        case ga_loadlevel:
            G_DoLoadLevel();
            break;
        case ga_newgame:
            G_DoNewGame();
            break;
        case ga_loadgame:
            DoLoadGame();
            break;
        case ga_savegame:
            DoSaveGame();
            break;
        case ga_playdemo:
            G_DoPlayDemo();
            break;
        case ga_completed:
            G_DoCompleted();
            break;
        case ga_victory:
            F_StartFinale();
            break;
        case ga_worlddone:
            G_DoWorldDone();
            break;
        case ga_screenshot:
            M_ScreenShot();
            gameaction = ga_nothing;
            break;
        case ga_nothing:
            break;
        }
    }

    // get commands, check consistancy,
    // and build new consistancy check
    int32 buf = (gametic / ticdup) % BACKUPTICS;

    for (int32 i = 0; i < MAXPLAYERS; ++i)
    {
        if (playeringame[i])
        {
            auto cmd = &players[i].cmd;

            memcpy(cmd, &netcmds[i][buf], sizeof(ticcmd_t));

            if (demoplayback)
                G_ReadDemoTiccmd(cmd);
            if (doom->IsDemoRecording())
                G_WriteDemoTiccmd(cmd);

            // check for turbo cheats
            if (cmd->forwardmove > TURBOTHRESHOLD
                && !(gametic & 31) && ((gametic >> 5) & 3) == i)
            {
                extern const char* player_names[4];
                players[consoleplayer].message = std::format("{} is turbo!", player_names[i]);
            }

            if (netgame && !netdemo && !(gametic % ticdup))
            {
                if (gametic > BACKUPTICS && consistancy[i][buf] != cmd->consistancy)
                    I_Error("consistency failure ({} should be {})", cmd->consistancy, consistancy[i][buf]);

                if (players[i].mo)
                    consistancy[i][buf] = static_cast<short>(players[i].mo->x);
                else
                    consistancy[i][buf] = static_cast<short>(rndindex);
            }
        }
    }

    // check for special buttons
    for (int32 i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            if (players[i].cmd.buttons & BT_SPECIAL)
            {
                switch (players[i].cmd.buttons & BT_SPECIALMASK)
                {
                case BTS_PAUSE:
                    paused = !paused;
                    if (paused)
                        S_PauseSound();
                    else
                        S_ResumeSound();
                    break;

                case BTS_SAVEGAME:
                    if (!saveDescription[0])
                        saveDescription = "NET GAME";
                    savegameslot = (players[i].cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
                    gameaction = ga_savegame;
                    break;
                }
            }
        }
    }

    // do main actions
    switch (doom->GetGameState())
    {
    case GameState::Level:
        P_Ticker();
        ST_Ticker();
        AM_Ticker();
        HU_Ticker();
        break;

    case GameState::Intermission:
        WI_Ticker();
        break;

    case GameState::Finale:
        F_Ticker();
        break;

    case GameState::Demo:
        D_PageTicker();
        break;
    }
}

// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things

// Called at the start.
// Called by the game initialization functions.
void G_InitPlayer(int player)
{
    // clear everything else to defaults 
    G_PlayerReborn(player);
}

// Can when a player completes a level.
void G_PlayerFinishLevel(int player)
{
    auto* p = &players[player];

    memset(p->powers, 0, sizeof(p->powers));
    memset(p->cards, 0, sizeof(p->cards));
    p->mo->flags &= ~MF_SHADOW;		// cancel invisibility 
    p->extralight = 0;			// cancel gun flashes 
    p->fixedcolormap = 0;		// cancel ir gogles 
    p->damagecount = 0;			// no palette changes 
    p->bonuscount = 0;
}

// Called after a player dies 
// almost everything is cleared and initialized 
void G_PlayerReborn(int player)
{
    int		i;
    int		frags[MAXPLAYERS];
    int		killcount;
    int		itemcount;
    int		secretcount;

    memcpy(frags, players[player].frags, sizeof(frags));
    killcount = players[player].killcount;
    itemcount = players[player].itemcount;
    secretcount = players[player].secretcount;

    auto* p = &players[player];
    memset(p, 0, sizeof(*p));

    memcpy(players[player].frags, frags, sizeof(players[player].frags));
    players[player].killcount = killcount;
    players[player].itemcount = itemcount;
    players[player].secretcount = secretcount;

    p->usedown = p->attackdown = true;	// don't do anything immediately 
    p->playerstate = PST_LIVE;
    p->health = MAXHEALTH;
    p->readyweapon = p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = 50;

    for (i = 0; i < NUMAMMO; i++)
        p->maxammo[i] = maxammo[i];
}

// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
void P_SpawnPlayer(mapthing_t* mthing);

bool G_CheckSpot(int32 playernum, mapthing_t* mthing)
{
    subsector_t* ss;
    unsigned		an;
    mobj_t* mo;
    int			i;

    if (!players[playernum].mo)
    {
        // first spawn of level, before corpses
        for (i = 0; i < playernum; i++)
            if (players[i].mo->x == mthing->x << FRACBITS
                && players[i].mo->y == mthing->y << FRACBITS)
                return false;
        return true;
    }

    int32 x = mthing->x << FRACBITS;
    int32 y = mthing->y << FRACBITS;

    if (!P_CheckPosition(players[playernum].mo, x, y))
        return false;

    // flush an old corpse if needed 
    if (bodyqueslot >= BODYQUESIZE)
        P_RemoveMobj(bodyque[bodyqueslot % BODYQUESIZE]);
    bodyque[bodyqueslot % BODYQUESIZE] = players[playernum].mo;
    bodyqueslot++;

    // spawn a teleport fog 
    ss = R_PointInSubsector(x, y);
    an = (ANG45 * (mthing->angle / 45)) >> ANGLETOFINESHIFT;

    mo = P_SpawnMobj(x + 20 * finecosine[an], y + 20 * finesine[an]
        , ss->sector->floorheight
        , MT_TFOG);

    if (players[consoleplayer].viewz != 1)
        S_StartSound(mo, sfx_telept);	// don't start sound on first frame 

    return true;
}

// Spawns a player at one of the random death match spots 
// called at level load and each death 
void G_DeathMatchSpawnPlayer(int playernum)
{
    auto selections = deathmatch_p - deathmatchstarts;
    if (selections < 4)
        I_Error("Only {} deathmatch spots, 4 required", selections);

    for (int32 j = 0; j < 20; j++)
    {
        int32 i = P_Random() % selections;
        if (G_CheckSpot(playernum, &deathmatchstarts[i]))
        {
            deathmatchstarts[i].type = static_cast<short>(playernum + 1);
            P_SpawnPlayer(&deathmatchstarts[i]);
            return;
        }
    }

    // no good spot, so the player will probably get stuck 
    P_SpawnPlayer(&playerstarts[playernum]);
}

void G_DoReborn(int playernum)
{
    if (!netgame)
    {
        // reload the level from scratch
        gameaction = ga_loadlevel;
    }
    else
    {
        // respawn at the start

        // first dissasociate the corpse 
        players[playernum].mo->player = nullptr;

        // spawn at random spot if in death match 
        if (deathmatch)
        {
            G_DeathMatchSpawnPlayer(playernum);
            return;
        }

        if (G_CheckSpot(playernum, &playerstarts[playernum]))
        {
            P_SpawnPlayer(&playerstarts[playernum]);
            return;
        }

        // try to spawn at one of the other players spots 
        for (int32 i = 0; i < MAXPLAYERS; ++i)
        {
            if (G_CheckSpot(playernum, &playerstarts[i]))
            {
                playerstarts[i].type = static_cast<short>(playernum + 1);	// fake as other player 
                P_SpawnPlayer(&playerstarts[i]);
                playerstarts[i].type = static_cast<short>(i + 1);		// restore 
                return;
            }
            // he's going to be inside something.  Too bad.
        }
        P_SpawnPlayer(&playerstarts[playernum]);
    }
}

void G_ScreenShot()
{
    gameaction = ga_screenshot;
}

// DOOM Par Times
int pars[4][10] =
{
    {0},
    {0,30,75,120,90,165,180,180,30,165},
    {0,90,90,90,120,90,360,240,30,170},
    {0,90,45,90,150,90,90,165,30,135}
};

// DOOM II Par Times
int cpars[32] =
{
    30,90,120,120,90,150,120,120,270,90,	//  1-10
    210,150,150,150,210,150,420,150,210,150,	// 11-20
    240,150,180,150,150,300,330,420,300,180,	// 21-30
    120,30					// 31-32
};

// G_DoCompleted 

bool		secretexit;
extern char* pagename;

void G_ExitLevel()
{
    secretexit = false;
    gameaction = ga_completed;
}

void G_SecretExitLevel()
{
    secretexit = true;
    gameaction = ga_completed;
}

void G_DoCompleted()
{
    gameaction = ga_nothing;

    for (int32 i = 0; i < MAXPLAYERS; ++i)
        if (playeringame[i])
            G_PlayerFinishLevel(i);        // take away cards and stuff 

    if (automapactive)
        AM_Stop();

    if (g_doom->GetGameMode() != GameMode::Doom2Commercial)
    {
        switch (gamemap)
        {
        case 8:
            gameaction = ga_victory;
            return;
        case 9:
            for (int32 i = 0; i < MAXPLAYERS; i++)
                players[i].didsecret = true;
            break;
        }
    }

    if ((gamemap == 8) && (g_doom->GetGameMode() != GameMode::Doom2Commercial))
    {
        // victory 
        gameaction = ga_victory;
        return;
    }

    if ((gamemap == 9) && (g_doom->GetGameMode() != GameMode::Doom2Commercial))
    {
        // exit secret level 
        for (int32 i = 0; i < MAXPLAYERS; ++i)
            players[i].didsecret = true;
    }

    wminfo.didsecret = players[consoleplayer].didsecret;
    wminfo.epsd = gameepisode - 1;
    wminfo.last = gamemap - 1;

    // wminfo.next is 0 biased, unlike gamemap
    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
    {
        if (secretexit)
            switch (gamemap)
            {
            case 15: wminfo.next = 30; break;
            case 31: wminfo.next = 31; break;
            }
        else
            switch (gamemap)
            {
            case 31:
            case 32: wminfo.next = 15; break;
            default: wminfo.next = gamemap;
            }
    }
    else
    {
        if (secretexit)
            wminfo.next = 8; 	// go to secret level 
        else if (gamemap == 9)
        {
            // returning from secret level 
            switch (gameepisode)
            {
            case 1:
                wminfo.next = 3;
                break;
            case 2:
                wminfo.next = 5;
                break;
            case 3:
                wminfo.next = 6;
                break;
            case 4:
                wminfo.next = 2;
                break;
            }
        }
        else
            wminfo.next = gamemap;          // go to next level 
    }

    wminfo.maxkills = totalkills;
    wminfo.maxitems = totalitems;
    wminfo.maxsecret = totalsecret;
    wminfo.maxfrags = 0;
    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
        wminfo.partime = 35 * cpars[gamemap - 1];
    else
        wminfo.partime = 35 * pars[gameepisode][gamemap];
    wminfo.pnum = consoleplayer;

    for (int32 i = 0; i < MAXPLAYERS; ++i)
    {
        wminfo.plyr[i].in = playeringame[i];
        wminfo.plyr[i].skills = players[i].killcount;
        wminfo.plyr[i].sitems = players[i].itemcount;
        wminfo.plyr[i].ssecret = players[i].secretcount;
        wminfo.plyr[i].stime = leveltime;
        memcpy(wminfo.plyr[i].frags, players[i].frags, sizeof(wminfo.plyr[i].frags));
    }

    g_doom->SetGameState(GameState::Intermission);
    viewactive = false;
    automapactive = false;

    if (statcopy)
        memcpy(statcopy, &wminfo, sizeof(wminfo));

    WI_Start(&wminfo);
}

void G_WorldDone()
{
    gameaction = ga_worlddone;

    if (secretexit)
        players[consoleplayer].didsecret = true;

    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
    {
        switch (gamemap)
        {
        case 15:
        case 31:
            if (!secretexit)
                break;
        case 6:
        case 11:
        case 20:
        case 30:
            F_StartFinale();
            break;
        }
    }
}

void G_DoWorldDone()
{
    g_doom->SetGameState(GameState::Level);
    gamemap = wminfo.next + 1;
    G_DoLoadLevel();
    gameaction = ga_nothing;
    viewactive = true;
}

void Game::LoadGame(const filesys::path& fileName)
{
    loadFileName = fileName;
    gameaction = ga_loadgame;
}

void Game::DoLoadGame()
{
    gameaction = ga_nothing;

    auto inFile = std::ifstream{loadFileName, std::ifstream::binary};
    if (!inFile.is_open())
    {
        logger::write(logger::Verbosity::Error, "Failed to open load file: ", loadFileName);
        return;
    }

    // skip the description field
    inFile.seekg(SaveStringSize, std::ios_base::beg);

    string versionCheck(VersionSize, '\0');
    inFile.read(versionCheck.data(), VersionSize);
    if (versionCheck != std::format("version {}", Doom::Version))
        return;

    auto read_byte = [&inFile](auto* val){
        using T = nstd::int_rep<nstd::rm_ptr<decltype(val)>>;
        *reinterpret_cast<T*>(val) = nstd::zero<T>;
        inFile.read(reinterpret_cast<char*>(val), 1);
    };

    read_byte(&gameskill);
    read_byte(&gameepisode);
    read_byte(&gamemap);

    for (int32 i = 0; i < MAXPLAYERS; ++i)
        read_byte(playeringame + i);

    // load a base level 
    G_InitNew(gameskill, gameepisode, gamemap);

    // get the times
    leveltime = 0;
    inFile.read(reinterpret_cast<char*>(&leveltime) + 2, 1);
    inFile.read(reinterpret_cast<char*>(&leveltime) + 1, 1);
    inFile.read(reinterpret_cast<char*>(&leveltime) + 0, 1);

    // dearchive all the modifications
    P_UnArchivePlayers(inFile);
    P_UnArchiveWorld(inFile);
    P_UnArchiveThinkers(inFile);
    P_UnArchiveSpecials(inFile);

    SaveFileMarker check;
    read_byte(&check);
    if (check != SaveFileMarker::End)
        I_Error("Bad savegame");

    g_doom->GetRender()->CheckSetViewSize();

    // draw the pattern into the back screen
    R_FillBackScreen();
}

// Called by the menu task.
// Description is a 24 byte text string 
void G_SaveGame(int slot, string_view description)
{
    savegameslot = slot;
    saveDescription = description;
    sendsave = true;
}

void Game::DoSaveGame()
{
    auto fileName = Game::GetSaveFilePath(savegameslot);
    std::ofstream outFile(fileName, std::ofstream::binary);
    if (!outFile.is_open())
    {
        std::cerr << "Failed to open save file: " << fileName << "\n";
        return;
    }

    outFile << std::setfill('\0')
        << std::setw(SaveStringSize) << saveDescription
        << std::setw(VersionSize) << std::format("version {}", Doom::Version)

        << static_cast<byte>(gameskill)
        <<static_cast<byte>(gameepisode)
        << static_cast<byte>(gamemap);

    for (int32 i = 0; i < MAXPLAYERS; ++i)
        outFile << playeringame[i];

    outFile << static_cast<byte>(leveltime >> 16)
        << static_cast<byte>(leveltime >> 8)
        << static_cast<byte>(leveltime);

    P_ArchivePlayers(outFile);
    P_ArchiveWorld(outFile);
    P_ArchiveThinkers(outFile);
    P_ArchiveSpecials(outFile);

    outFile << static_cast<uint8>(SaveFileMarker::End);

    gameaction = ga_nothing;
    saveDescription = "";

    players[consoleplayer].message = GGSAVED;

    // draw the pattern into the back screen
    R_FillBackScreen();
}

// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set. 
skill_t	d_skill;
int     d_episode;
int     d_map;

void G_DeferedInitNew(skill_t	skill,
    int		episode,
    int		map)
{
    d_skill = skill;
    d_episode = episode;
    d_map = map;
    gameaction = ga_newgame;
}


void G_DoNewGame()
{
    demoplayback = false;
    netdemo = false;
    netgame = false;
    deathmatch = false;
    playeringame[1] = playeringame[2] = playeringame[3] = false;
    respawnparm = false;
    fastparm = false;
    nomonsters = false;
    consoleplayer = 0;
    G_InitNew(d_skill, d_episode, d_map);
    gameaction = ga_nothing;
}

// The sky texture to be used instead of the F_SKY1 dummy.
extern  int	skytexture;

void G_InitNew(skill_t skill, int32 episode, int32 map)
{
    int             i;

    if (paused)
    {
        paused = false;
        S_ResumeSound();
    }


    if (skill > sk_nightmare)
        skill = sk_nightmare;


    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.
    if (episode < 1)
        episode = 1;

    if (g_doom->GetGameMode() == GameMode::Doom1Retail)
    {
        if (episode > 4)
            episode = 4;
    }
    else if (g_doom->GetGameMode() == GameMode::Doom1Shareware)
    {
        if (episode > 1)
            episode = 1;	// only start episode 1 on shareware
    }
    else
    {
        if (episode > 3)
            episode = 3;
    }



    if (map < 1)
        map = 1;

    if ((map > 9) && (g_doom->GetGameMode() != GameMode::Doom2Commercial))
        map = 9;

    M_ClearRandom();

    if (skill == sk_nightmare || respawnparm)
        respawnmonsters = true;
    else
        respawnmonsters = false;

    if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare))
    {
        for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics >>= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20 * FRACUNIT;
    }
    else if (skill != sk_nightmare && gameskill == sk_nightmare)
    {
        for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 10 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 10 * FRACUNIT;
    }


    // force players to be initialized upon first level load         
    for (i = 0; i < MAXPLAYERS; i++)
        players[i].playerstate = PST_REBORN;

    usergame = true;                // will be set false if a demo 
    paused = false;
    demoplayback = false;
    automapactive = false;
    viewactive = true;
    gameepisode = episode;
    gamemap = map;
    gameskill = skill;

    viewactive = true;

    // set the sky map for the episode
    if (g_doom->GetGameMode() == GameMode::Doom2Commercial)
    {
        skytexture = R_TextureNumForName("SKY3");
        if (gamemap < 12)
            skytexture = R_TextureNumForName("SKY1");
        else
            if (gamemap < 21)
                skytexture = R_TextureNumForName("SKY2");
    }
    else
        switch (episode)
        {
        case 1:
            skytexture = R_TextureNumForName("SKY1");
            break;
        case 2:
            skytexture = R_TextureNumForName("SKY2");
            break;
        case 3:
            skytexture = R_TextureNumForName("SKY3");
            break;
        case 4:	// Special Edition sky
            skytexture = R_TextureNumForName("SKY4");
            break;
        }

    G_DoLoadLevel();
}

// DEMO RECORDING 

#define DEMOMARKER		0x80


void G_ReadDemoTiccmd(ticcmd_t* cmd)
{
    if (*demo_g == DEMOMARKER)
    {
        // end of demo data stream 
        G_CheckDemoStatus(g_doom);
        return;
    }
    cmd->forwardmove = ((signed char)*demo_g++);
    cmd->sidemove = ((signed char)*demo_g++);
    cmd->angleturn = ((unsigned char)*demo_g++) << 8;
    cmd->buttons = (unsigned char)*demo_g++;
}


void G_WriteDemoTiccmd(ticcmd_t* cmd)
{
    if (gamekeydown['q'])           // press q to end demo recording 
        G_CheckDemoStatus(g_doom);
    *demo_p++ = cmd->forwardmove;
    *demo_p++ = cmd->sidemove;
    *demo_p++ = (cmd->angleturn + 128) >> 8;
    *demo_p++ = cmd->buttons;
    demo_p -= 4;
    if (demo_p > demoend - 16)
    {
        // no more space 
        G_CheckDemoStatus(g_doom);
        return;
    }

    G_ReadDemoTiccmd(cmd);         // make SURE it is exactly the same 
}

void G_RecordDemo(Doom* doom, string_view name)
{
    usergame = false;
    demoname = name;
    demoname += ".lmp";
    int maxsize = 0x20000;

    if (CommandLine::TryGetValues("-maxdemo", maxsize))
        maxsize *= 1024;

    demo_obuffer = static_cast<byte*>(Z_Malloc(maxsize, PU_STATIC, nullptr));
    demoend = demo_obuffer + maxsize;

    doom->SetDemoRecording(true);
}

void G_BeginRecording()
{
    demo_p = demo_obuffer;

    *demo_p++ = Doom::Version;
    *demo_p++ = static_cast<byte>(gameskill);
    *demo_p++ = static_cast<byte>(gameepisode);
    *demo_p++ = static_cast<byte>(gamemap);
    *demo_p++ = static_cast<byte>(deathmatch);
    *demo_p++ = respawnparm;
    *demo_p++ = fastparm;
    *demo_p++ = nomonsters;
    *demo_p++ = static_cast<byte>(consoleplayer);

    for (int i = 0; i < MAXPLAYERS; i++)
        *demo_p++ = playeringame[i];
}

// G_PlayDemo 

static string_view defdemoname;

void G_DeferedPlayDemo(const char* name)
{
    defdemoname = name;
    gameaction = ga_playdemo;
}

void G_DoPlayDemo()
{
    skill_t skill;
    int             i, episode, map;

    gameaction = ga_nothing;
    demo_ibuffer = demo_g = WadManager::GetLumpData<byte>(defdemoname.to_upper());
    if (*demo_g++ != Doom::Version)
    {
        std::cerr << "Demo is from a different game version!\n";
        gameaction = ga_nothing;
        return;
    }

    skill = static_cast<skill_t>(*demo_g++);
    episode = *demo_g++;
    map = *demo_g++;
    deathmatch = *demo_g++;
    respawnparm = *demo_g++;
    fastparm = *demo_g++;
    nomonsters = *demo_g++;
    consoleplayer = *demo_g++;

    for (i = 0; i < MAXPLAYERS; i++)
        playeringame[i] = *demo_g++;
    if (playeringame[1])
    {
        netgame = true;
        netdemo = true;
    }

    // don't spend a lot of time in loadlevel 
    precache = false;
    G_InitNew(skill, episode, map);
    precache = true;

    usergame = false;
    demoplayback = true;
}

void G_TimeDemo(const char* name)
{
    timingdemo = true;
    g_doom->SetUseSingleTicks(true);

    defdemoname = name;
    gameaction = ga_playdemo;
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

bool G_CheckDemoStatus(Doom* doom)
{
    if (timingdemo)
    {
        auto endtime = I_GetTime();
        I_Error("timed {} gametics in {} realtics", gametic, endtime - starttime);
    }

    if (demoplayback)
    {
        if (singledemo)
            I_Quit();

        //Z_ChangeTag(demobuffer, PU_CACHE);
        demoplayback = false;
        netdemo = false;
        netgame = false;
        deathmatch = false;
        playeringame[1] = playeringame[2] = playeringame[3] = false;
        respawnparm = false;
        fastparm = false;
        nomonsters = false;
        consoleplayer = 0;
        D_AdvanceDemo();
        return true;
    }

    if (doom->IsDemoRecording())
    {
        *demo_p++ = DEMOMARKER;
        M_WriteFile(demoname, reinterpret_cast<char*>(demo_obuffer), static_cast<uint32>(demo_p - demo_obuffer));
        Z_Free(demo_obuffer);
        doom->SetDemoRecording(false);
        I_Error("Demo {} recorded", demoname);
    }

    return false;
}
