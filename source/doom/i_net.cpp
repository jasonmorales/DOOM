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
#include "system/windows.h"
#include <winsock.h>

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "doomstat.h"
#include "i_net.h"

import std;
import nstd;
import config;


// For some odd reason...
#define ntohl(x) \
        ((unsigned long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))

#define ntohs(x) \
        ((unsigned short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8))) \

#define htonl(x) ntohl(x)
#define htons(x) ntohs(x)

void	NetSend();

// NETWORKING

#define IPPORT_USERRESERVED 16384
int	DOOMPORT = (IPPORT_USERRESERVED + 0x1d);

SOCKET sendsocket;
SOCKET insocket;

struct	sockaddr_in	sendaddress[Net::MaxNodes];

void	(*netget) ();
void	(*netsend) ();

SOCKET UDPsocket()
{
    // allocate a socket
    auto s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
    {
        static char errorBuffer[1024];
        strerror_s(errorBuffer, errno);
        I_Error("can't create socket: {}", errorBuffer);
    }

    return s;
}

void BindToLocalPort(SOCKET s, u_short port)
{
    sockaddr_in	address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = port;

    auto v = bind(s, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (v == -1)
    {
        static char errorBuffer[1024];
        strerror_s(errorBuffer, errno);
        I_Error("BindToPort: bind: {}", errorBuffer);
    }
}

void PacketSend()
{
    int		c;
    doomdata_t	sw;

    // byte swap
    sw.checksum = htonl(netbuffer->checksum);
    sw.player = netbuffer->player;
    sw.retransmitfrom = netbuffer->retransmitfrom;
    sw.starttic = netbuffer->starttic;
    sw.numtics = netbuffer->numtics;
    for (c = 0; c < netbuffer->numtics; c++)
    {
        sw.cmds[c].forwardmove = netbuffer->cmds[c].forwardmove;
        sw.cmds[c].sidemove = netbuffer->cmds[c].sidemove;
        sw.cmds[c].angleturn = htons(netbuffer->cmds[c].angleturn);
        sw.cmds[c].consistancy = htons(netbuffer->cmds[c].consistancy);
        sw.cmds[c].chatchar = netbuffer->cmds[c].chatchar;
        sw.cmds[c].buttons = netbuffer->cmds[c].buttons;
    }

    //printf ("sending %i\n",gametic);		
    c = sendto(
        sendsocket,
        reinterpret_cast<char*>(&sw),
        doomcom->datalength,
        0,
        reinterpret_cast<sockaddr*>(&sendaddress[doomcom->remotenode]),
        sizeof(sendaddress[doomcom->remotenode]));

    //	if (c == -1)
    //		I_Error ("SendPacket error: %s",strerror(errno));
}


//
// PacketGet
//
void PacketGet()
{
    struct sockaddr_in	fromaddress;
    int32 fromlen = sizeof(fromaddress);
    doomdata_t sw;
    auto c = recvfrom(insocket, reinterpret_cast<char*>(&sw), sizeof(sw), 0, (sockaddr*)&fromaddress, &fromlen);
    if (c == -1)
    {
        static char errorBuffer[1024];
        if (_strerror_s(errorBuffer, nullptr) != EWOULDBLOCK)
            I_Error("GetPacket: {}", errorBuffer);

        doomcom->remotenode = -1;		// no packet
        return;
    }

    {
        static int first = 1;
        if (first)
            std::cout << std::format("len={}:p=[{:#010x} {:#010x}] \n", c, *(int*)&sw, *((int*)&sw + 1));
        first = 0;
    }

    // find remote node number
    int16 i;
    for (i = 0; i < doomcom->numnodes; ++i)
        if (fromaddress.sin_addr.s_addr == sendaddress[i].sin_addr.s_addr)
            break;

    if (i == doomcom->numnodes)
    {
        // packet is not from one of the players (new game broadcast)
        doomcom->remotenode = -1;		// no packet
        return;
    }

    doomcom->remotenode = i; // good packet from a game player
    doomcom->datalength = static_cast<int16>(c);

    // byte swap
    netbuffer->checksum = ntohl(sw.checksum);
    netbuffer->player = sw.player;
    netbuffer->retransmitfrom = sw.retransmitfrom;
    netbuffer->starttic = sw.starttic;
    netbuffer->numtics = sw.numtics;

    for (c = 0; c < netbuffer->numtics; c++)
    {
        netbuffer->cmds[c].forwardmove = sw.cmds[c].forwardmove;
        netbuffer->cmds[c].sidemove = sw.cmds[c].sidemove;
        netbuffer->cmds[c].angleturn = ntohs(sw.cmds[c].angleturn);
        netbuffer->cmds[c].consistancy = ntohs(sw.cmds[c].consistancy);
        netbuffer->cmds[c].chatchar = sw.cmds[c].chatchar;
        netbuffer->cmds[c].buttons = sw.cmds[c].buttons;
    }
}



int GetLocalAddress()
{
    char		hostname[1024];
    struct hostent* hostentry;	// host information entry
    int			v;

    // get local address
    v = gethostname(hostname, sizeof(hostname));
    if (v == -1)
        I_Error("GetLocalAddress : gethostname: errno {}", errno);

    hostentry = gethostbyname(hostname);
    if (!hostentry)
        I_Error("GetLocalAddress : gethostbyname: couldn't get local host");

    return *(int*)hostentry->h_addr_list[0];
}

//
// I_InitNetwork
//
void I_InitNetwork()
{
    doomcom = static_cast<doomcom_t*>(std::malloc(sizeof(*doomcom)));
    memset(doomcom, 0, sizeof(*doomcom));

    // set up for network
    if (CommandLine::TryGetValues("-dup", doomcom->ticdup))
    {
        if (doomcom->ticdup < 1)
            doomcom->ticdup = 1;
        if (doomcom->ticdup > 9)
            doomcom->ticdup = 9;
    }
    else
        doomcom->ticdup = 1;

    if (CommandLine::HasArg("-extratic"))
        doomcom->extratics = 1;
    else
        doomcom->extratics = 0;

    if (CommandLine::TryGetValues("-port", DOOMPORT))
        std::cout << std::format("using alternate port {}\n", DOOMPORT);

    // parse network game options,
    //  -net <consoleplayer> <host> <host> ...
    vector<string_view> netSettings;
    if (!CommandLine::GetValueList("-net", netSettings) || netSettings.empty())
    {
        // single player game
        netgame = false;
        doomcom->id = DOOMCOM_ID;
        doomcom->numplayers = doomcom->numnodes = 1;
        doomcom->deathmatch = false;
        doomcom->consoleplayer = 0;
        return;
    }

    netsend = PacketSend;
    netget = PacketGet;
    netgame = true;

    // parse player number and host list
    doomcom->consoleplayer = nstd::convert<short>(netSettings[0]) - 1;

    doomcom->numnodes = 1;	// this node for sure

    for (auto arg = netSettings.begin() + 1; arg != netSettings.end(); ++arg)
    {
        auto argStr = string(*arg);
        sendaddress[doomcom->numnodes].sin_family = AF_INET;
        sendaddress[doomcom->numnodes].sin_port = htons(DOOMPORT);
        if (argStr[0] == '.')
        {
            sendaddress[doomcom->numnodes].sin_addr.s_addr = inet_addr(argStr.substr(1).c_str());
        }
        else
        {
            struct hostent* hostentry = gethostbyname(argStr.c_str());
            if (!hostentry)
                I_Error("gethostbyname: couldn't find {}", argStr.c_str());
            sendaddress[doomcom->numnodes].sin_addr.s_addr = *(int*)hostentry->h_addr_list[0];
        }
        doomcom->numnodes++;
    }

    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes;

    // build message to receive
    insocket = UDPsocket();
    BindToLocalPort(insocket, htons(DOOMPORT));
    //ioctl (insocket, FIONBIO, &trueval);

    sendsocket = UDPsocket();
}

void I_NetCmd()
{
    if (doomcom->command == CMD_SEND)
        netsend();
    else if (doomcom->command == CMD_GET)
        netget();
    else
        I_Error("Bad net cmd: {}\n", doomcom->command);
}
