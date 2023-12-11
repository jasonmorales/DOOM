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
//	System interface, sound.
//
//-----------------------------------------------------------------------------
#pragma once

#include "doomdef.h"

#include "doomstat.h"
#include "sounds.h"

#include "types/numbers.h"
#include "containers/vector.h"

//  SFX I/O

// Initialize channels?
void I_SetChannels();

// Called by S_*() functions to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int32 handle);

// Updates the volume, separation, and pitch of a sound channel.
void I_UpdateSoundParams(int32 handle, int32 vol, int32 sep, int32 pitch);

//  MUSIC I/O
void I_InitMusic();
void I_ShutdownMusic();

// Volume.
void I_SetMusicVolume(int32 volume);

// PAUSE game handling.
void I_PauseSong(int32 handle);
void I_ResumeSong(int32 handle);

// Registers a song handle to song data.
int I_RegisterSong(const void* data);

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int32 handle, int32 looping);

// Stops a song over 3 seconds.
void I_StopSong(int32 handle);

// See above (register), then think backwards
void I_UnRegisterSong(int32 handle);

class Sound
{
public:
    // Init at program start...
    static void Init();

    // ... update sound buffer and audio device at runtime...
    static void Update();

    // ... shut down and relase at program termination.
    static void Shutdown();

    // Starts a sound in a particular sound channel.
    static int32 Play(int32 id, int32 vol, int32 sep, int32 pitch, int32 priority);
    static void Stop(int32 handle);

private:
    static constexpr const double bufferLengthInSeconds = 0.05; //1.0 / 35; //0.05;

    static struct IMMDevice* device;
    static struct IAudioClient* client;
    static struct IAudioRenderClient* renderer;

    static int32 samplesPerSec;
    static int32 bitsPerSample;
    static int32 numChannels;
    static int32 frameBytes;

    static size_t mixBufferSize;
    static byte* mixBuffer;
    static uint32 bufferSizeInFrames;

    //static vector<std::pair<double, Synth::Function*>> functions;
    static vector<uint32> free;
};
