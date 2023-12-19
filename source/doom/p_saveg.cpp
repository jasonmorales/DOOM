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
//	Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------
import std;

import nstd;

#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"
#include "doomstat.h"
#include "r_state.h"
#include "p_saveg.h"

void P_ArchivePlayers(std::ofstream& outFile)
{
    for (int32 i = 0; i < MAXPLAYERS; ++i)
    {
        if (!playeringame[i])
            continue;

        nstd::align(4, outFile);

        auto* player = players[i].GetSaveData();
        outFile.write(reinterpret_cast<char*>(player), sizeof(player_t));
        delete player;
    }
}

void P_UnArchivePlayers(std::ifstream& inFile)
{
    for (int32 i = 0; i < MAXPLAYERS; ++i)
    {
        if (!playeringame[i])
            continue;

        auto* player = &players[i];

        nstd::align(4, inFile);
        inFile.read(reinterpret_cast<char*>(player), sizeof(player_t));

        // will be set when unarc thinker
        player->mo = nullptr;
        player->message.clear();
        player->attacker = nullptr;

        for (int32 j = 0; j < NUMPSPRITES; j++)
        {
            if (player->psprites[j].state)
            {
                player->psprites[j].state = states + reinterpret_cast<intptr_t>(player->psprites[j].state);
            }
        }
    }
}

void P_ArchiveWorld(std::ofstream& outFile)
{
    // do sectors
    auto* sector = sectors;
    for (int32 i = 0; i < numsectors; ++i, ++sector)
    {
        outFile << static_cast<int16>(sector->floorheight >> FRACBITS);
        outFile << static_cast<int16>(sector->ceilingheight >> FRACBITS);
        outFile << sector->floorpic;
        outFile << sector->ceilingpic;
        outFile << sector->lightlevel;
        outFile << sector->special;		// needed?
        outFile << sector->tag;		// needed?
    }

    // do lines
    for (int32 i = 0; i < numlines; ++i)
    {
        auto* line = lines + i;
        outFile
            << line->flags
            << line->special
            << line->tag;

        for (int32 j = 0; j < 2; ++j)
        {
            if (line->sidenum[j] == -1)
                continue;

            auto* side = sides + line->sidenum[j];
            outFile
                << static_cast<int16>(side->textureoffset >> FRACBITS)
                << static_cast<int16>(side->rowoffset >> FRACBITS)
                << side->toptexture
                << side->bottomtexture
                << side->midtexture;
        }
    }
}

void P_UnArchiveWorld(std::ifstream& inFile)
{
    // do sectors
    for (int32 i = 0; i < numsectors; ++i)
    {
        auto* sector = sectors + i;

        sector->floorheight = 0;
        inFile.read(reinterpret_cast<char*>(&sector->floorheight), 2);
        sector->floorheight <<= FRACBITS;

        sector->ceilingheight = 0;
        inFile.read(reinterpret_cast<char*>(&sector->ceilingheight), 2);
        sector->ceilingheight <<= FRACBITS;

        inFile.read(reinterpret_cast<char*>(&sector->floorpic), 2);
        inFile.read(reinterpret_cast<char*>(&sector->ceilingpic), 2);
        inFile.read(reinterpret_cast<char*>(&sector->lightlevel), 2);
        inFile.read(reinterpret_cast<char*>(&sector->special), 2);
        inFile.read(reinterpret_cast<char*>(&sector->tag), 2);

        sector->specialdata = nullptr;
        sector->soundtarget = nullptr;
    }

    // do lines
    for (int32 i = 0; i < numlines; ++i)
    {
        auto* line = lines + i;

        inFile.read(reinterpret_cast<char*>(&line->flags), 2);
        inFile.read(reinterpret_cast<char*>(&line->special), 2);
        inFile.read(reinterpret_cast<char*>(&line->tag), 2);

        for (int32 j = 0; j < 2; ++j)
        {
            if (line->sidenum[j] == -1)
                continue;

            auto* side = sides + line->sidenum[j];

            side->textureoffset = 0;
            inFile.read(reinterpret_cast<char*>(&side->textureoffset), 2);
            side->textureoffset <<= FRACBITS;

            side->rowoffset = 0;
            inFile.read(reinterpret_cast<char*>(&side->rowoffset), 2);
            side->rowoffset <<= FRACBITS;

            inFile.read(reinterpret_cast<char*>(&side->toptexture), 2);
            inFile.read(reinterpret_cast<char*>(&side->bottomtexture), 2);
            inFile.read(reinterpret_cast<char*>(&side->midtexture), 2);
        }
    }
}

// Thinkers

void P_ArchiveThinkers(std::ofstream& outFile)
{
    // save off the current thinkers
    for (auto* th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if (th->function.acp1 == (actionf_p1)P_MobjThinker)
        {
            outFile << static_cast<uint8>(SaveFileMarker::MapObject);

            nstd::align(4, outFile);

            auto* mobj = reinterpret_cast<mobj_t*>(th)->GetSaveData();
            outFile.write(reinterpret_cast<char*>(mobj), sizeof(mobj_t));
            delete mobj;

            continue;
        }
    }

    // add a terminating marker
    outFile << static_cast<uint8>(SaveFileMarker::ThinkersEnd);
}

void P_UnArchiveThinkers(std::ifstream& inFile)
{
    // remove all the current thinkers
    auto* currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        auto* next = currentthinker->next;

        if (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
            P_RemoveMobj((mobj_t*)currentthinker);
        else
            Z_Free(currentthinker);

        currentthinker = next;
    }
    P_InitThinkers();

    // read in saved thinkers
    for (;;)
    {
        SaveFileMarker tclass;
        inFile.get(reinterpret_cast<char*>(&tclass), 1);

        if (tclass == SaveFileMarker::ThinkersEnd)
            break;

        if (tclass != SaveFileMarker::MapObject)
            I_Error("Unknown tclass {} in savegame", tclass);

        nstd::align(4, inFile);

        auto* mobj = Z_Malloc<mobj_t>(sizeof(mobj_t), PU_LEVEL, nullptr);
        inFile.read(reinterpret_cast<char*>(mobj), sizeof(mobj_t));

        mobj->state = states + reinterpret_cast<intptr_t>(mobj->state);
        mobj->target = nullptr;
        if (mobj->player)
        {
            mobj->player = players + reinterpret_cast<intptr_t>(mobj->player - 1);
            mobj->player->mo = mobj;
        }
        P_SetThingPosition(mobj);
        mobj->info = &mobjinfo[mobj->type];
        mobj->floorz = mobj->subsector->sector->floorheight;
        mobj->ceilingz = mobj->subsector->sector->ceilingheight;
        mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
        P_AddThinker(&mobj->thinker);
    }
}

std::ostream& operator<<(std::ostream& os, SaveFileMarker sp)
{
    os << nstd::to_underlying(sp);
    return os;
}

// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
template<typename T>
inline void writeObject(std::ofstream& outFile, thinker_t* object, SaveFileMarker marker)
{
    outFile << marker;
    nstd::align(4, outFile);

    auto* data = GetSaveData(reinterpret_cast<T*>(object));
    outFile.write(reinterpret_cast<char*>(data), sizeof(T));
    delete data;
}

void P_ArchiveSpecials(std::ofstream& outFile)
{
    // save off the current thinkers
    for (auto* th = thinkercap.next; th != &thinkercap; th = th->next)
    {
        if (th->function.acv == (actionf_v)nullptr)
        {
            int32 i = 0;
            for (; i < MAXCEILINGS; ++i)
            {
                if (activeceilings[i] == (ceiling_t*)th)
                    break;
            }

            if (i < MAXCEILINGS)
                writeObject<ceiling_t>(outFile, th, SaveFileMarker::Ceiling);

            continue;
        }

        if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
            writeObject<ceiling_t>(outFile, th, SaveFileMarker::Ceiling);
        else if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
            writeObject<vldoor_t>(outFile, th, SaveFileMarker::Door);
        else if (th->function.acp1 == (actionf_p1)T_MoveFloor)
            writeObject<floormove_t>(outFile, th, SaveFileMarker::Door);
        else if (th->function.acp1 == (actionf_p1)T_PlatRaise)
            writeObject<plat_t>(outFile, th, SaveFileMarker::Plat);
        else if (th->function.acp1 == (actionf_p1)T_LightFlash)
            writeObject<lightflash_t>(outFile, th, SaveFileMarker::Flash);
        else if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
            writeObject<strobe_t>(outFile, th, SaveFileMarker::Strobe);
        else if (th->function.acp1 == (actionf_p1)T_Glow)
            writeObject<glow_t>(outFile, th, SaveFileMarker::Glow);
    }

    // add a terminating marker
    outFile << SaveFileMarker::End;
}

template<typename T>
T* ReadThinker(std::ifstream& inFile, actionf_p1 action)
{
    nstd::align(4, inFile);
    auto* object = Z_Malloc<T>(sizeof(T), PU_LEVEL, nullptr);
    inFile.read(reinterpret_cast<char*>(object), sizeof(T));
    object->sector = sectors + reinterpret_cast<intptr_t>(object->sector);
    object->sector->specialdata = object;
    object->thinker.function.acp1 = action;
    P_AddThinker(&object->thinker);
    return object;
}

void P_UnArchiveSpecials(std::ifstream& inFile)
{
    // read in saved thinkers
    while (!inFile.eof())
    {
        SaveFileMarker tclass = SaveFileMarker::Invalid;
        inFile.get(reinterpret_cast<char*>(&tclass), 1);

        if (tclass == SaveFileMarker::ThinkersEnd)
            break;

        switch (tclass)
        {
        case SaveFileMarker::End:
            return;	// end of list

        case SaveFileMarker::Ceiling:
        {
            auto* ceiling = ReadThinker<ceiling_t>(inFile, nullptr);

            if (ceiling->thinker.function.acp1)
                ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;

            P_AddActiveCeiling(ceiling);
            break;
        }

        case SaveFileMarker::Door:
            ReadThinker<ceiling_t>(inFile, (actionf_p1)T_VerticalDoor);
            break;

        case SaveFileMarker::Floor:
            ReadThinker<floormove_t>(inFile, (actionf_p1)T_MoveFloor);
            break;

        case SaveFileMarker::Plat:
        {
            auto* platform = ReadThinker<plat_t>(inFile, nullptr);

            if (platform->thinker.function.acp1)
                platform->thinker.function.acp1 = (actionf_p1)T_PlatRaise;

            P_AddActivePlat(platform);
            break;
        }

        case SaveFileMarker::Flash:
            ReadThinker<lightflash_t>(inFile, (actionf_p1)T_LightFlash);
            break;

        case SaveFileMarker::Strobe:
            ReadThinker<strobe_t>(inFile, (actionf_p1)T_StrobeFlash);
            break;

        case SaveFileMarker::Glow:
            ReadThinker<glow_t>(inFile, (actionf_p1)T_Glow);
            break;

        default:
            I_Error("P_UnarchiveSpecials:Unknown tclass {} in savegame", tclass);
        }
    }
}
