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
//	DOOM Network game communication and protocol,
//	all OS independent parts.
//
//-----------------------------------------------------------------------------
#include "i_video.h"

#include "i_system.h"
#include "i_net.h"
#include "g_game.h"
#include "m_menu.h"
#include "doomstat.h"
#include "d_main.h"

import std;


extern Doom* g_doom;


void G_BuildTiccmd(ticcmd_t* cmd);

int32 Net::ticks[Net::MaxNodes] = {0};

#define	NCMD_EXIT		0x80000000
#define	NCMD_RETRANSMIT		0x40000000
#define	NCMD_SETUP		0x20000000
#define	NCMD_KILL		0x10000000	// kill game
#define	NCMD_CHECKSUM	 	0x0fffffff

doomcom_t* doomcom;
doomdata_t* netbuffer;		// points inside doomcom

// NETWORKING

// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// Net::ticks[] has the maketics for all players 
//
// a gametic cannot be run until Net::ticks[] > gametic for all players

#define	RESENDCOUNT	10
#define	PL_DRONE	0x80	// bit flag in doomdata->player

ticcmd_t	localcmds[BACKUPTICS];

ticcmd_t netcmds[MAXPLAYERS][BACKUPTICS];
bool nodeingame[Net::MaxNodes];		// set false as nodes leave game
bool remoteresend[Net::MaxNodes];		// set when local needs tics
int32 resendto[Net::MaxNodes];			// set when remote needs tics
int32 resendcount[Net::MaxNodes];

int		nodeforplayer[MAXPLAYERS];

int             maketic;
int		lastnettic;
time_t skiptics;
int		ticdup;
int		maxsend;	// BACKUPTICS/(2*ticdup)-1

bool		reboundpacket;
doomdata_t	reboundstore;

//
//
//
intptr_t NetbufferSize()
{
    return reinterpret_cast<intptr_t>(&static_cast<doomdata_t*>(nullptr)->cmds[netbuffer->numtics]);
    //return (int)&(((doomdata_t*)0)->cmds[netbuffer->numtics]); 
}

//
// Checksum 
//
unsigned NetbufferChecksum()
{
    uint32_t c = 0x1234567;

    auto l = (NetbufferSize() - reinterpret_cast<intptr_t>(&(((doomdata_t*)0)->retransmitfrom)) / 4);
    for (int i = 0; i < l; i++)
        c += ((unsigned*)&netbuffer->retransmitfrom)[i] * (i + 1);

    return c & NCMD_CHECKSUM;
}

//
//
//
int ExpandTics(int low)
{
    int	delta;

    delta = low - (maketic & 0xff);

    if (delta >= -64 && delta <= 64)
        return (maketic & ~0xff) + low;
    if (delta > 64)
        return (maketic & ~0xff) - 256 + low;
    if (delta < -64)
        return (maketic & ~0xff) + 256 + low;

    I_Error("ExpandTics: strange value {} at maketic {}", low, maketic);
    return 0;
}



//
// HSendPacket
//
void HSendPacket(int node, int	flags)
{
    netbuffer->checksum = NetbufferChecksum() | flags;

    if (!node)
    {
        reboundstore = *netbuffer;
        reboundpacket = true;
        return;
    }

    if (demoplayback)
        return;

    if (!netgame)
        I_Error("Tried to transmit to another node");

    doomcom->command = CMD_SEND;
    doomcom->remotenode = static_cast<short>(node);
    doomcom->datalength = static_cast<short>(NetbufferSize());

    if (debugfile.is_open())
    {
        int32 realretrans = -1;
        if (netbuffer->checksum & NCMD_RETRANSMIT)
            realretrans = ExpandTics(netbuffer->retransmitfrom);

        debugfile << std::format("send ({} + {}, R {}) [{}] ",
            ExpandTics(netbuffer->starttic),
            netbuffer->numtics, realretrans, doomcom->datalength);

        for (int32 i = 0; i < doomcom->datalength; ++i)
            debugfile << std::format("{} ", ((byte*)netbuffer)[i]);

        debugfile << "\n";
    }

    I_NetCmd();
}

// Returns false if no packet is waiting
bool HGetPacket()
{
    if (reboundpacket)
    {
        *netbuffer = reboundstore;
        doomcom->remotenode = 0;
        reboundpacket = false;
        return true;
    }

    if (!netgame)
        return false;

    if (demoplayback)
        return false;

    doomcom->command = CMD_GET;
    I_NetCmd();

    if (doomcom->remotenode == -1)
        return false;

    if (doomcom->datalength != NetbufferSize())
    {
        if (debugfile.is_open())
            debugfile << std::format("bad packet length {}\n", doomcom->datalength);
        return false;
    }

    if (NetbufferChecksum() != (netbuffer->checksum & NCMD_CHECKSUM))
    {
        if (debugfile.is_open())
            debugfile << "bad packet checksum\n";
        return false;
    }

    if (debugfile.is_open())
    {
        int		realretrans;
        int	i;

        if (netbuffer->checksum & NCMD_SETUP)
            debugfile << "setup packet\n";
        else
        {
            if (netbuffer->checksum & NCMD_RETRANSMIT)
                realretrans = ExpandTics(netbuffer->retransmitfrom);
            else
                realretrans = -1;

            debugfile << std::format("get {} = ({} + {}, R {})[{}] ",
                doomcom->remotenode,
                ExpandTics(netbuffer->starttic),
                netbuffer->numtics, realretrans, doomcom->datalength);

            for (i = 0; i < doomcom->datalength; i++)
                debugfile << std::format("{} ", ((byte*)netbuffer)[i]);
            debugfile << "\n";
        }
    }
    return true;
}

void GetPackets()
{
    int		netconsole;
    int		netnode;
    ticcmd_t* src, * dest;
    int		realend;
    int		realstart;

    while (HGetPacket())
    {
        if (netbuffer->checksum & NCMD_SETUP)
            continue;		// extra setup packet

        netconsole = netbuffer->player & ~PL_DRONE;
        netnode = doomcom->remotenode;

        // to save bytes, only the low byte of tic numbers are sent
        // Figure out what the rest of the bytes are
        realstart = ExpandTics(netbuffer->starttic);
        realend = (realstart + netbuffer->numtics);

        // check for exiting the game
        if (netbuffer->checksum & NCMD_EXIT)
        {
            if (!nodeingame[netnode])
                continue;
            nodeingame[netnode] = false;
            playeringame[netconsole] = false;
            players[consoleplayer].message = std::format("Player {} left the game", static_cast<char>(netconsole) + 1);
            if (g_doom->IsDemoRecording())
                G_CheckDemoStatus(g_doom);
            continue;
        }

        // check for a remote game kill
        if (netbuffer->checksum & NCMD_KILL)
            I_Error("Killed by network driver");

        nodeforplayer[netconsole] = netnode;

        // check for retransmit request
        if (resendcount[netnode] <= 0
            && (netbuffer->checksum & NCMD_RETRANSMIT))
        {
            resendto[netnode] = ExpandTics(netbuffer->retransmitfrom);
            if (debugfile.is_open())
                debugfile << std::format("retransmit from {}\n", resendto[netnode]);
            resendcount[netnode] = RESENDCOUNT;
        }
        else
            resendcount[netnode]--;

        // check for out of order / duplicated packet		
        if (realend == Net::ticks[netnode])
            continue;

        if (realend < Net::ticks[netnode])
        {
            if (debugfile.is_open())
                debugfile << std::format("out of order packet ({} + {})\n", realstart, netbuffer->numtics);
            continue;
        }

        // check for a missed packet
        if (realstart > Net::ticks[netnode])
        {
            // stop processing until the other system resends the missed tics
            if (debugfile.is_open())
                debugfile << std::format("missed tics from {} ({} - {})\n", netnode, realstart, Net::ticks[netnode]);
            remoteresend[netnode] = true;
            continue;
        }

        // update command store from the packet
        {
            int		start;

            remoteresend[netnode] = false;

            start = Net::ticks[netnode] - realstart;
            src = &netbuffer->cmds[start];

            while (Net::ticks[netnode] < realend)
            {
                dest = &netcmds[netconsole][Net::ticks[netnode] % BACKUPTICS];
                Net::ticks[netnode]++;
                *dest = *src;
                src++;
            }
        }
    }
}

// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
time_t      gametime;

void NetUpdate()
{
    // check time
    auto nowtime = I_GetTime() / ticdup;
    auto newtics = nowtime - gametime;
    gametime = nowtime;

    int gameticdiv = gametic / ticdup;

    if (newtics <= 0) 	// nothing new to update
        goto listen;

    if (skiptics <= newtics)
    {
        newtics -= skiptics;
        skiptics = 0;
    }
    else
    {
        skiptics -= newtics;
        newtics = 0;
    }

    netbuffer->player = static_cast<byte>(consoleplayer);

    // build new ticcmds for console player
    for (int i = 0; i < newtics; i++)
    {
        g_doom->GetVideo()->StartTick();
        g_doom->ProcessEvents();
        if (maketic - gameticdiv >= BACKUPTICS / 2 - 1)
            break;          // can't hold any more

        //printf ("mk:%i ",maketic);
        G_BuildTiccmd(&localcmds[maketic % BACKUPTICS]);
        maketic++;
    }

    if (g_doom->UseSingleTicks())
        return;	// singletic update is synchronous

    // send the packet to the other nodes
    for (int i = 0; i < doomcom->numnodes; i++)
        if (nodeingame[i])
        {
            auto realstart = netbuffer->starttic = static_cast<byte>(resendto[i]);
            netbuffer->numtics = static_cast<byte>(maketic - realstart);
            if (netbuffer->numtics > BACKUPTICS)
                I_Error("NetUpdate: netbuffer->numtics > BACKUPTICS");

            resendto[i] = maketic - doomcom->extratics;

            for (int j = 0; j < netbuffer->numtics; j++)
                netbuffer->cmds[j] =
                localcmds[(realstart + j) % BACKUPTICS];

            if (remoteresend[i])
            {
                netbuffer->retransmitfrom = static_cast<byte>(Net::ticks[i]);
                HSendPacket(i, NCMD_RETRANSMIT);
            }
            else
            {
                netbuffer->retransmitfrom = 0;
                HSendPacket(i, 0);
            }
        }

    // listen for other packets
listen:
    GetPackets();
}

void CheckAbort()
{
    auto stoptic = I_GetTime() + 2;
    while (I_GetTime() < stoptic)
        g_doom->GetVideo()->StartTick();

    g_doom->GetVideo()->StartTick();
    if (g_doom->HasEscEventInQueue())
        I_Error("Network game synchronization aborted.");
}

void D_ArbitrateNetStart()
{
    int		i;
    bool	gotinfo[Net::MaxNodes];

    autostart = true;
    memset(gotinfo, 0, sizeof(gotinfo));

    if (doomcom->consoleplayer)
    {
        // listen for setup info from key player
        std::cout << "listening for network start info...\n";
        while (1)
        {
            CheckAbort();
            if (!HGetPacket())
                continue;
            if (netbuffer->checksum & NCMD_SETUP)
            {
                if (netbuffer->player != Doom::Version)
                    I_Error("Different DOOM versions cannot play a net game!");

                startskill = static_cast<skill_t>(netbuffer->retransmitfrom & 15);
                deathmatch = (netbuffer->retransmitfrom & 0xc0) >> 6;
                nomonsters = (netbuffer->retransmitfrom & 0x20) > 0;
                respawnparm = (netbuffer->retransmitfrom & 0x10) > 0;
                startmap = netbuffer->starttic & 0x3f;
                startepisode = netbuffer->starttic >> 6;
                return;
            }
        }
    }
    else
    {
        // key player, send the setup info
        std::cout << "sending network start info...\n";
        do
        {
            CheckAbort();
            for (i = 0; i < doomcom->numnodes; i++)
            {
                netbuffer->retransmitfrom = static_cast<byte>(startskill);
                if (deathmatch)
                    netbuffer->retransmitfrom |= (deathmatch << 6);
                if (nomonsters)
                    netbuffer->retransmitfrom |= 0x20;
                if (respawnparm)
                    netbuffer->retransmitfrom |= 0x10;
                netbuffer->starttic = static_cast<byte>(startepisode * 64 + startmap);
                netbuffer->player = Doom::Version;
                netbuffer->numtics = 0;
                HSendPacket(i, NCMD_SETUP);
            }

#if 1
            for (i = 10; i && HGetPacket(); --i)
            {
                if ((netbuffer->player & 0x7f) < Net::MaxNodes)
                    gotinfo[netbuffer->player & 0x7f] = true;
            }
#else
            while (HGetPacket())
            {
                gotinfo[netbuffer->player & 0x7f] = true;
            }
#endif

            for (i = 1; i < doomcom->numnodes; i++)
                if (!gotinfo[i])
                    break;
        } while (i < doomcom->numnodes);
    }
}

// Works out player numbers among the net participants
void Net::CheckGame()
{
    for (int i = 0; i < Net::MaxNodes; ++i)
    {
        nodeingame[i] = false;
        Net::ticks[i] = 0;
        remoteresend[i] = false;	// set when local needs tics
        resendto[i] = 0;		// which tic to start sending
    }

    // I_InitNetwork sets doomcom and netgame
    I_InitNetwork();
    if (doomcom->id != DOOMCOM_ID)
        I_Error("Doomcom buffer invalid!");

    netbuffer = &doomcom->data;
    consoleplayer = displayplayer = doomcom->consoleplayer;
    if (netgame)
        D_ArbitrateNetStart();

    std::cout << std::format("startskill {}  deathmatch: {}  startmap: {}  startepisode: {}\n",
        nstd::to_underlying(startskill), deathmatch, startmap, startepisode);

    // read values out of doomcom
    ticdup = doomcom->ticdup;
    maxsend = BACKUPTICS / (2 * ticdup) - 1;
    if (maxsend < 1)
        maxsend = 1;

    for (int i = 0; i < doomcom->numplayers; i++)
        playeringame[i] = true;

    for (int i = 0; i < doomcom->numnodes; i++)
        nodeingame[i] = true;

    std::cout << std::format("player {} of {} ({} nodes)\n", consoleplayer + 1, doomcom->numplayers, doomcom->numnodes);
}

// Called before quitting to leave a net game
// without hanging the other players
void D_QuitNetGame()
{
    if (debugfile.is_open())
        debugfile.close();

    if (!netgame || !usergame || consoleplayer == -1 || demoplayback)
        return;

    // send a bunch of packets for security
    netbuffer->player = static_cast<byte>(consoleplayer);
    netbuffer->numtics = 0;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 1; j < doomcom->numnodes; j++)
            if (nodeingame[j])
                HSendPacket(j, NCMD_EXIT);
        I_WaitVBL(1);
    }
}

//
// TryRunTics
//
int	frametics[4];
int	frameon;
int	frameskip[4];
int	oldnettics;

extern	bool	advancedemo;

void TryRunTics()
{
    static time_t oldentertics = 0;
    int		i;
    int		numplaying;

    // get real tics		
    auto entertic = I_GetTime() / ticdup;
    auto realtics = entertic - oldentertics;
    oldentertics = entertic;

    // get available tics
    NetUpdate();

    auto lowtic = std::numeric_limits<time_t>::max();
    numplaying = 0;
    for (i = 0; i < doomcom->numnodes; i++)
    {
        if (nodeingame[i])
        {
            numplaying++;
            if (Net::ticks[i] < lowtic)
                lowtic = Net::ticks[i];
        }
    }
    auto availabletics = lowtic - gametic / ticdup;

    // decide how many tics to run
    time_t counts = 0;
    if (realtics < availabletics - 1)
        counts = realtics + 1;
    else if (realtics < availabletics)
        counts = realtics;
    else
        counts = availabletics;

    if (counts < 1)
        counts = 1;

    frameon++;

    if (debugfile.is_open())
        debugfile << std::format("=======real: {}  avail: {}  game: {}\n", realtics, availabletics, counts);

    if (!demoplayback)
    {
        // ideally Net::ticks[0] should be 1 - 3 tics above lowtic
        // if we are consistently slower, speed up time
        for (i = 0; i < MAXPLAYERS; i++)
            if (playeringame[i])
                break;
        if (consoleplayer == i)
        {
            // the key player does not adapt
        }
        else
        {
            if (Net::ticks[0] <= Net::ticks[nodeforplayer[i]])
            {
                gametime--;
                // printf ("-");
            }
            frameskip[frameon & 3] = (oldnettics > Net::ticks[nodeforplayer[i]]);
            oldnettics = Net::ticks[0];
            if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
            {
                skiptics = 1;
                // printf ("+");
            }
        }
    }// demoplayback

    // wait for new tics if needed
    while (lowtic < gametic / ticdup + counts)
    {
        NetUpdate();
        lowtic = std::numeric_limits<int>::max();

        for (i = 0; i < doomcom->numnodes; i++)
            if (nodeingame[i] && Net::ticks[i] < lowtic)
                lowtic = Net::ticks[i];

        if (lowtic < gametic / ticdup)
            I_Error("TryRunTics: lowtic < gametic");

        // don't stay in here forever -- give the menu a chance to work
        if (I_GetTime() / ticdup - entertic >= 20)
        {
            M_Ticker();
            return;
        }
    }

    // run the count * ticdup dics
    while (counts--)
    {
        for (i = 0; i < ticdup; i++)
        {
            if (gametic / ticdup > lowtic)
                I_Error("gametic>lowtic");
            if (advancedemo)
                g_doom->DoAdvanceDemo();
            M_Ticker();
            g_doom->GetGame()->Ticker();
            gametic++;

            // modify command for duplicated tics
            if (i != ticdup - 1)
            {
                ticcmd_t* cmd;
                int			buf;
                int			j;

                buf = (gametic / ticdup) % BACKUPTICS;
                for (j = 0; j < MAXPLAYERS; j++)
                {
                    cmd = &netcmds[j][buf];
                    cmd->chatchar = 0;
                    if (cmd->buttons & BT_SPECIAL)
                        cmd->buttons = 0;
                }
            }
        }
        NetUpdate();	// check for new console commands
    }
}
