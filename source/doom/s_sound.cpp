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
import std;
#define __STD_MODULE__

#include "s_sound.h"

#include "m_misc.h"
#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"

#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "doomstat.h"


// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST		(1200*0x10000)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
#define S_CLOSE_DIST		(160*0x10000)


#define S_ATTENUATOR		((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

// Adjustable by menu.
#define NORM_VOLUME    		snd_MaxVolume

#define NORM_PITCH     		128
#define NORM_PRIORITY		64
#define NORM_SEP		128

#define S_PITCH_PERTURB		1
#define S_STEREO_SWING		(96*0x10000)

// percent attenuation from front to back
#define S_IFRACVOL		30

#define NA			0
#define S_NUMCHANNELS		2


// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
extern int snd_MusicDevice;
extern int snd_SfxDevice;
// Config file? Same disclaimer as above.
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;

struct channel_t
{
    // sound information (if null, channel avail.)
    sfxinfo_t* sfxinfo;

    // origin of sound
    void* origin;

    // handle of the sound being played
    int		handle;

};

// the set of channels available
static channel_t* channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int32 		snd_SfxVolume = 15;

// Maximum volume of music. Useless so far.
int32 		snd_MusicVolume = 15;

// whether songs are mus_paused
static bool mus_paused;

// music currently being played
static musicinfo_t* mus_playing = 0;

static int		nextcleanup;


void S_StopChannel(int32 cnum)
{
    auto* c = &channels[cnum];
    if (!c->sfxinfo)
        return;

    // stop the sound playing
    if (I_SoundIsPlaying(c->handle))
        Sound::Stop(c->handle);

    // check to see if other channels are playing the sound
    for (int32 i = 0; i < numChannels; ++i)
    {
        if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
            break;
    }

    // degrade usefulness of sound data
    c->sfxinfo->usefulness--;

    c->sfxinfo = 0;
}

// S_getChannel : If none available, return -1.  Otherwise channel #.
int32 S_getChannel(void* origin, sfxinfo_t* sfxinfo)
{
    // channel number to use
    int32 cnum = 0;

    // Find an open channel
    for (cnum = 0; cnum < numChannels; cnum++)
    {
        if (!channels[cnum].sfxinfo)
            break;

        if (origin && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }

    // None available
    if (cnum == numChannels)
    {
        // Look for lower priority
        for (cnum = 0; cnum < numChannels; cnum++)
        {
            if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
                break;
        }

        if (cnum == numChannels)
            return -1;  // FUCK!  No lower priority.  Sorry, Charlie.
        else
            S_StopChannel(cnum);  // Otherwise, kick out lower priority.
    }

    auto* c = &channels[cnum];

    // channel is decided to be cnum.
    c->sfxinfo = sfxinfo;
    c->origin = origin;

    return cnum;
}

// Changes volume, stereo-separation, and pitch variables from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
int32 S_AdjustSoundParams(mobj_t* listener, mobj_t* source, int32* vol, int32* sep, [[maybe_unused]] int32* pitch)
{
    fixed_t	approx_dist;
    fixed_t	adx;
    fixed_t	ady;
    angle_t	angle;

    // calculate the distance to sound origin
    //  and clip it if necessary
    adx = std::abs(listener->x - source->x);
    ady = std::abs(listener->y - source->y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = adx + ady - ((adx < ady ? adx : ady) >> 1);

    if (gamemap != 8 && approx_dist > S_CLIPPING_DIST)
        return 0;

    // angle of source to listener
    angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

    if (angle > listener->angle)
        angle = angle - listener->angle;
    else
        angle = angle + (0xffffffff - listener->angle);

    angle >>= ANGLETOFINESHIFT;

    // stereo separation
    *sep = 128 - (FixedMul(S_STEREO_SWING, finesine[angle]) >> FRACBITS);

    // volume calculation
    if (approx_dist < S_CLOSE_DIST)
    {
        *vol = snd_SfxVolume;
    }
    else if (gamemap == 8)
    {
        if (approx_dist > S_CLIPPING_DIST)
            approx_dist = S_CLIPPING_DIST;

        *vol = 15 + ((snd_SfxVolume - 15)
            * ((S_CLIPPING_DIST - approx_dist) >> FRACBITS))
            / S_ATTENUATOR;
    }
    else
    {
        // distance effect
        *vol = (snd_SfxVolume
            * ((S_CLIPPING_DIST - approx_dist) >> FRACBITS))
            / S_ATTENUATOR;
    }

    return (*vol > 0);
}

// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
void S_Init(int sfxVolume, int musicVolume)
{
    std::cerr << std::format("S_Init: default sfx volume {}\n", sfxVolume);

    // Whatever these did with DMX, these are rather dummies now.
    I_SetChannels();

    S_SetSfxVolume(sfxVolume);
    // No music with Linux - another dummy.
    S_SetMusicVolume(musicVolume);

    // Allocating the internal channels for mixing
    // (the maximum number of sounds rendered
    // simultaneously) within zone memory.
    channels = Z_Malloc<channel_t>(numChannels * sizeof(channel_t), PU_STATIC, 0);

    // Free all channels for use
    for (int i = 0; i < numChannels; i++)
        channels[i].sfxinfo = 0;

    // no sounds are playing, and they are not mus_paused
    mus_paused = false;

    // Note that sounds have not been cached (yet).
    for (int32 i = 1; i < NUMSFX; ++i)
        S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
}

// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
void S_Start()
{
    // kill all playing sounds at start of level
    //  (trust me - a good idea)
    for (int32 cnum = 0; cnum < numChannels; cnum++)
    {
        if (channels[cnum].sfxinfo)
            S_StopChannel(cnum);
    }

    // start new music for the level
    mus_paused = false;

    int32 mnum = 0;
    if (gamemode == GameMode::Doom2Commercial)
    {
        mnum = mus_runnin + gamemap - 1;
    }
    else
    {
        int spmus[] =
        {
            // Song - Who? - Where?
            mus_e3m4,	// American	e4m1
            mus_e3m2,	// Romero	e4m2
            mus_e3m3,	// Shawn	e4m3
            mus_e1m5,	// American	e4m4
            mus_e2m7,	// Tim 	e4m5
            mus_e2m4,	// Romero	e4m6
            mus_e2m6,	// J.Anderson	e4m7 CHIRON.WAD
            mus_e2m5,	// Shawn	e4m8
            mus_e1m9	// Tim		e4m9
        };

        if (gameepisode < 4)
            mnum = mus_e1m1 + (gameepisode - 1) * 9 + gamemap - 1;
        else
            mnum = spmus[gamemap - 1];
    }

    // HACK FOR COMMERCIAL
    //  if (commercial && mnum > mus_e3m9)	
    //      mnum -= mus_e3m9;

    S_ChangeMusic(mnum, true);

    nextcleanup = 15;
}

void S_StartSoundAtVolume(void* origin_p, int32 sfx_id, int32 volume)
{
    mobj_t* origin = (mobj_t*)origin_p;

    // check for bogus sound #
    if (sfx_id < 1 || sfx_id > NUMSFX)
        I_Error("Bad sfx #: {}", sfx_id);

    auto* sfx = &S_sfx[sfx_id];

    int32 pitch = 0;
    int32 priority = 0;
    int32 sep = 0;

    // Initialize sound parameters
    if (sfx->link)
    {
        pitch = sfx->pitch;
        priority = sfx->priority;
        volume += sfx->volume;

        if (volume < 1)
            return;

        if (volume > snd_SfxVolume)
            volume = snd_SfxVolume;
    }
    else
    {
        pitch = NORM_PITCH;
        priority = NORM_PRIORITY;
    }

    // Check to see if it is audible, and if not, modify the params
    if (origin && origin != players[consoleplayer].mo)
    {
        auto rc = S_AdjustSoundParams(players[consoleplayer].mo, origin, &volume, &sep, &pitch);
        if (origin->x == players[consoleplayer].mo->x && origin->y == players[consoleplayer].mo->y)
            sep = NORM_SEP;

        if (!rc)
            return;
    }
    else
    {
        sep = NORM_SEP;
    }

    // hacks to vary the sfx pitches
    if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
    {
        pitch += 8 - (M_Random() & 15);

        if (pitch < 0)
            pitch = 0;
        else if (pitch > 255)
            pitch = 255;
    }
    else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
    {
        pitch += 16 - (M_Random() & 31);

        if (pitch < 0)
            pitch = 0;
        else if (pitch > 255)
            pitch = 255;
    }

    // kill old sound
    S_StopSound(origin);

    // try to find a channel
    auto cnum = S_getChannel(origin, sfx);
    if (cnum < 0)
        return;

    // This is supposed to handle the loading/caching.
    // For some odd reason, the caching is done nearly each time the sound is needed?

    // get lumpnum if necessary
    if (sfx->lumpnum < 0)
        sfx->lumpnum = W_GetNumForName(std::format("DS{}", sfx->name.to_upper()));

    // cache data if necessary
    if (!sfx->data)
        std::cerr << "S_StartSoundAtVolume: 16bit and not pre-cached - wtf?\n";

    // increase the usefulness
    if (sfx->usefulness++ < 0)
        sfx->usefulness = 1;

    // Assigns the handle to one of the channels in the mix/output buffer.
    channels[cnum].handle = Sound::Play(sfx_id, /*sfx->data,*/ volume, sep, pitch, priority);
}

void S_StartSound(void* origin, int32 sfx_id)
{
    S_StartSoundAtVolume(origin, sfx_id, snd_SfxVolume);
}

void S_StopSound(void* origin)
{
    for (int32 cnum = 0; cnum < numChannels; ++cnum)
    {
        if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }
}

// Stop and resume music, during game PAUSE.
void S_PauseSound()
{
    if (mus_playing && !mus_paused)
    {
        I_PauseSong(mus_playing->handle);
        mus_paused = true;
    }
}

void S_ResumeSound()
{
    if (mus_playing && mus_paused)
    {
        I_ResumeSong(mus_playing->handle);
        mus_paused = false;
    }
}

// Updates music & sounds
void S_UpdateSounds(void* listener_p)
{
    mobj_t* listener = (mobj_t*)listener_p;

    for (int32 cnum = 0; cnum < numChannels; ++cnum)
    {
        auto* c = &channels[cnum];
        auto* sfx = c->sfxinfo;

        if (!c->sfxinfo)
            continue;

        if (!I_SoundIsPlaying(c->handle))
        {
            // if channel is allocated but sound has stopped, free it
            S_StopChannel(cnum);
            continue;
        }

        // initialize parameters
        auto volume = snd_SfxVolume;
        auto pitch = NORM_PITCH;
        auto sep = NORM_SEP;

        if (sfx->link)
        {
            pitch = sfx->pitch;
            volume += sfx->volume;
            if (volume < 1)
            {
                S_StopChannel(cnum);
                continue;
            }
            else if (volume > snd_SfxVolume)
            {
                volume = snd_SfxVolume;
            }
        }

        // check non-local sounds for distance clipping or modify their params
        if (c->origin && listener_p != c->origin)
        {
            auto audible = S_AdjustSoundParams(listener,
                static_cast<mobj_t*>(c->origin),
                &volume,
                &sep,
                &pitch);

            if (!audible)
                S_StopChannel(cnum);
            else
                I_UpdateSoundParams(c->handle, volume, sep, pitch);
        }
    }
}

void S_SetMusicVolume(int volume)
{
    if (volume < 0 || volume > 127)
        I_Error("Attempt to set music volume at {}", volume);

    I_SetMusicVolume(127);
    I_SetMusicVolume(volume);
    snd_MusicVolume = volume;
}

void S_SetSfxVolume(int volume)
{
    if (volume < 0 || volume > 127)
        I_Error("Attempt to set sfx volume at {}", volume);

    snd_SfxVolume = volume;
}

// Starts some music with the music id found in sounds.h.
void S_StartMusic(int32 m_id)
{
    S_ChangeMusic(m_id, false);
}

void S_ChangeMusic(int32 musicnum, int32 looping)
{
    musicinfo_t* music = nullptr;
    if ((musicnum <= mus_None) || (musicnum >= NUMMUSIC))
        I_Error("Bad music number {}", musicnum);
    else
        music = &S_music[musicnum];

    if (mus_playing == music)
        return;

    // shutdown old music
    S_StopMusic();

    // get lumpnum if necessary
    if (!music->lumpnum)
    {
        string name = std::format("D_{}", music->name.to_upper());
        music->lumpnum = W_GetNumForName(name);
    }

    // load & register it
    music->data = WadManager::GetLumpData(music->lumpnum);
    music->handle = I_RegisterSong(music->data);

    // play it
    I_PlaySong(music->handle, looping);

    mus_playing = music;
}


void S_StopMusic()
{
    if (mus_playing)
    {
        if (mus_paused)
            I_ResumeSong(mus_playing->handle);

        I_StopSong(mus_playing->handle);
        I_UnRegisterSong(mus_playing->handle);
        //Z_ChangeTag(mus_playing->data, PU_CACHE);

        mus_playing->data = 0;
        mus_playing = 0;
    }
}
