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


// Init at program start...
void I_InitSound();

// ... update sound buffer and audio device at runtime...
void I_UpdateSound();
void I_SubmitSound();

// ... shut down and relase at program termination.
void I_ShutdownSound();

//  SFX I/O

// Initialize channels?
void I_SetChannels();

// Get raw data lump index for sound descriptor.
intptr_t I_GetSfxLumpNum(sfxinfo_t* sfxinfo);

// Starts a sound in a particular sound channel.
int32 I_StartSound(int32 id, int32 vol, int32 sep, int32 pitch, int32 priority);

// Stops a sound channel.
void I_StopSound(int32 handle);

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
int I_RegisterSong(void* data);

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void I_PlaySong(int32 handle, int32 looping);

// Stops a song over 3 seconds.
void I_StopSong(int32 handle);

// See above (register), then think backwards
void I_UnRegisterSong(int32 handle);
