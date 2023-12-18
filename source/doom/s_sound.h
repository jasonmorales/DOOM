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
//	The not so system specific sound interface.
//
//-----------------------------------------------------------------------------
#pragma once

import numbers;

// Initializes sound stuff, including volume
// Sets channels, SFX and music volume, allocates channel buffer, sets S_sfx lookup.
void S_Init(int32 sfxVolume, int32 musicVolume);

// Per level startup code.
// Kills playing sounds at start of level, determines music if any, changes music.
void S_Start();

// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
void S_StartSound(void* origin, int32 sound_id);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(void* origin, int32 sound_id, int32 volume);

// Stop sound for thing at <origin>
void S_StopSound(void* origin);

// Start music using <music_id> from sounds.h
void S_StartMusic(int music_id);

// Start music using <music_id> from sounds.h, and set whether looping
void S_ChangeMusic(int32 music_id, int32 looping);

// Stops the music fer sure.
void S_StopMusic();

// Stop and resume music, during game PAUSE.
void S_PauseSound();
void S_ResumeSound();

// Updates music & sounds
void S_UpdateSounds(void* listener);

void S_SetMusicVolume(int volume);
void S_SetSfxVolume(int volume);
