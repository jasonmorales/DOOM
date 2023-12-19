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
//	System interface for sound.
//
//-----------------------------------------------------------------------------
import std;
import nstd;
import config;

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"

#include "system/windows.h"

#include <mmdeviceapi.h>
#include <Audioclient.h>


// The number of internal mixing channels, the samples calculated for each mixing step, the size
// of the 16bit, 2 hardware channel (stereo) mixing buffer, and the sample rate of the raw data.

// Needed for calling the actual sound output.
static constexpr const uint32 SAMPLECOUNT = 512;
static constexpr const uint32 NUM_CHANNELS = 8;

// It is 2 for 16bit, and 2 for two channels.
static constexpr const uint32 BUFMUL = 4;
static constexpr const uint32 MIXBUFFERSIZE = SAMPLECOUNT * BUFMUL;

static constexpr const uint32 SAMPLERATE = 11025;	// Hz
static constexpr const uint32 SAMPLESIZE = 2;   	// 16bit

// The actual lengths of all sound effects.
int 		lengths[NUMSFX];

// The global mixing buffer.
// Basically, samples from all active internal channels are modifed and added, and stored in the
// buffer that is submitted to the audio device.
signed short	mixbuffer[MIXBUFFERSIZE];

// The channel step amount...
unsigned int	channelstep[NUM_CHANNELS];
// ... and a 0.16 bit remainder of last step.
unsigned int	channelstepremainder[NUM_CHANNELS];

// The channel data pointers, start and end.
unsigned char* channels[NUM_CHANNELS];
unsigned char* channelsend[NUM_CHANNELS];


// Time/gametic that the channel started playing, used to determine oldest, which automatically
// has lowest priority.
// In case number of active sounds exceeds available channels.
int		channelstart[NUM_CHANNELS];

// The sound in channel handles, determined on registration, might be used to
// unregister/stop/modify, currently unused.
int 		channelhandles[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int		channelids[NUM_CHANNELS];

// Pitch to stepping lookup, unused.
int		steptable[256];

// Volume lookups.
int		vol_lookup[128 * 256];

// Hardware left and right channel volume lookup.
int* channelleftvol_lookup[NUM_CHANNELS];
int* channelrightvol_lookup[NUM_CHANNELS];

// This function loads the sound data from the WAD lump, for single sound.
void* getsfx(string_view sfxname, int32* len)
{
    // Get the sound data from the WAD, allocate lump in zone memory.
    string name(8, '\0');
    name = std::format("DS{}", sfxname);

    // Now, there is a severe problem with the sound handling, in it is not (yet/anymore)
    // gamemode aware. That means, sounds from DOOM II will be requested even with DOOM shareware.
    // The sound list is wired into sounds.c, which sets the external variable.
    // I do not do runtime patches to that variable. Instead, we will use a default sound for replacement.
    int32 sfxlump = 0;
    if (WadManager::GetLumpId(name) == INVALID_ID)
        sfxlump = W_GetNumForName("DSPISTOL");
    else
        sfxlump = W_GetNumForName(name);

    auto size = WadManager::GetLump(sfxlump).size;

    auto* sfx = WadManager::GetLumpData<unsigned char>(sfxlump);

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    auto paddedsize = ((size - 8 + (SAMPLECOUNT - 1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    auto* paddedsfx = (unsigned char*)Z_Malloc(paddedsize + 8, PU_STATIC, 0);
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
    // This should interfere with zone memory handling, which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(paddedsfx, sfx, size);
    for (uint32 i = size; i < paddedsize + 8; ++i)
        paddedsfx[i] = 128;

    // Preserve padded length.
    *len = paddedsize;

    // Return allocated padded data.
    return (void*)(paddedsfx + 8);
}

// SFX API
// Note: this was called by S_Init. However, whatever they did in the old DPMS based DOS version,
// this were simply dummies in the Linux version.
// See soundserver initdata().
void I_SetChannels()
{
    // Init internal lookups (raw data, mixing buffer, channels).
    // This function sets up internal lookups used during
    //  the mixing process. 
    int		i;
    int		j;

    int* steptablemid = steptable + 128;

    // Okay, reset internal mixing channels to zero.
    /*for (i=0; i<NUM_CHANNELS; i++)
    {
      channels[i] = 0;
    }*/

    // This table provides step widths for pitch parameters.
    // I fail to see that this is currently used.
    for (i = -128; i < 128; i++)
        steptablemid[i] = (int)(std::pow(2.0, (i / 64.0)) * 65536.0);


    // Generates volume lookup tables
    //  which also turn the unsigned samples
    //  into signed samples.
    for (i = 0; i < 128; i++)
        for (j = 0; j < 256; j++)
            vol_lookup[i * 256 + j] = (i * (j - 128) * 256) / 127;
}

void I_SetSfxVolume(int volume)
{
    // Identical to DOS.
    // Basically, this should propagate the menu/config file setting to the state variable used
    // in the mixing.
    snd_SfxVolume = volume;
}

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
    // Internal state variable.
    snd_MusicVolume = volume;
    // Now set volume on output device.
    // Whatever( snd_MusciVolume );
}

int32 I_SoundIsPlaying(int32 handle)
{
    // Ouch.
    return gametic < handle;
}

void I_UpdateSoundParams
(int	handle,
    int	vol,
    int	sep,
    int	pitch)
{
    // I fail too see that this is used.
    // Would be using the handle to identify
    //  on which channel the sound might be active,
    //  and resetting the channel parameters.

    // UNUSED.
    handle = vol = sep = pitch = 0;
}

// MUSIC API.
// Still no music done.
// Remains. Dummies.
void I_InitMusic() {}
void I_ShutdownMusic() {}

static int	looping = 0;
static int	musicdies = -1;

void I_PlaySong(int handle, int /*_looping*/)
{
    // UNUSED.
    handle = looping = 0;
    musicdies = gametic + TICRATE * 30;
}

void I_PauseSong(int handle)
{
    // UNUSED.
    handle = 0;
}

void I_ResumeSong(int handle)
{
    // UNUSED.
    handle = 0;
}

void I_StopSong(int handle)
{
    // UNUSED.
    handle = 0;

    looping = 0;
    musicdies = 0;
}

void I_UnRegisterSong(int handle)
{
    // UNUSED.
    handle = 0;
}

int I_RegisterSong(const void* data)
{
    // UNUSED.
    data = nullptr;

    return 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
    // UNUSED.
    handle = 0;
    return looping || musicdies > gametic;
}

IMMDevice* Sound::device = nullptr;
IAudioClient* Sound::client = nullptr;
IAudioRenderClient* Sound::renderer = nullptr;

int32 Sound::samplesPerSec = 0;
int32 Sound::bitsPerSample = 0;
int32 Sound::numChannels = 0;
int32 Sound::frameBytes = 0;

size_t Sound::mixBufferSize = 0;
byte* Sound::mixBuffer = nullptr;
uint32 Sound::bufferSizeInFrames = 0;

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(::IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(::IAudioRenderClient);
const REFERENCE_TIME nsPerSec = 10'000'000; // 1 second

//vector<std::pair<double, Synth::Function*>> Sound::functions;
vector<uint32> Sound::free;

void Sound::Init()
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    IMMDeviceEnumerator* enumerator = nullptr;
    auto result = CoCreateInstance(
        CLSID_MMDeviceEnumerator,
        nullptr,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        reinterpret_cast<LPVOID*>(&enumerator));
    if (FAILED(result))
        I_Error("CoCreateInstance failed: {}", result);

    result = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(result))
        I_Error("GetDefaultAudioEndpoint failed: {}", result);

    enumerator->Release();

    result = device->Activate(
        IID_IAudioClient,
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&client));
    if (FAILED(result))
        I_Error("Activate failed: {}", result);

    WAVEFORMATEX* fmt = nullptr;
    result = client->GetMixFormat(&fmt);
    if (FAILED(result))
        I_Error("GetMixFormat failed: {}", result);

    samplesPerSec = fmt->nSamplesPerSec;
    bitsPerSample = fmt->wBitsPerSample;
    numChannels = fmt->nChannels;

    frameBytes = numChannels * (bitsPerSample / 8);

    result = client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        0,
        std::llround(bufferLengthInSeconds * nsPerSec),
        0,
        fmt,
        nullptr);
    if (FAILED(result))
        I_Error("Initialize failed: {}", result);

    result = client->GetBufferSize(&bufferSizeInFrames);
    if (FAILED(result))
        I_Error("GetBufferSize failed: {}", result);

    mixBufferSize = bufferSizeInFrames * frameBytes;
    mixBuffer = new byte[mixBufferSize];

    result = client->GetService(IID_IAudioRenderClient, reinterpret_cast<void**>(&renderer));
    if (FAILED(result))
        I_Error("GetService failed: {}", result);

    result = client->Start();
    if (FAILED(result))
        I_Error("Start failed: {}", result);

    std::cout << std::format("Sound::Initialize - buffer frames: {} samples/sec: {} bits/sample: {} channels: {} mixBufferSize: {}\n",
        bufferSizeInFrames, samplesPerSec, bitsPerSample, numChannels, mixBufferSize);

    for (int32 i = 1; i < NUMSFX; i++)
    {
        // Alias? Example is the chaingun sound linked to pistol.
        if (!S_sfx[i].link)
        {
            // Load data from WAD file.
            S_sfx[i].data = getsfx(S_sfx[i].name.to_upper(), &lengths[i]);
        }
        else
        {
            // Previously loaded already?
            S_sfx[i].data = S_sfx[i].link->data;
            lengths[i] = lengths[(S_sfx[i].link - S_sfx) / sizeof(sfxinfo_t)];
        }
    }

    std::cout << " pre-cached all sound data\n";

    // Finished initialization.
    std::cout << "Sound::Init: sound module ready\n";
}

void Sound::Update()
{
    // This function loops all active (internal) sound channels, retrieves a given number of samples
    // from the raw sound data, modifies it according to the current (internal) channel parameters,
    // mixes the per channel samples into the global mixbuffer, clamping it to the allowed range, and
    // sets up everything for transferring the contents of the mixbuffer to the (two) hardware
    // channels (left and right, that is).
    //
    // This function currently supports only 16bit.

    // Step in mixbuffer, left and right, thus two.
    int32 step = 2;

    // Left and right channel
    //  are in global mixbuffer, alternating.
    auto* leftout = mixbuffer;
    auto* rightout = mixbuffer + 1;

    // Determine end, for left channel only
    //  (right channel is implicit).
    auto* leftend = mixbuffer + SAMPLECOUNT * step;

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT,
    //  that is 512 values for two channels.
    while (leftout != leftend)
    {
        // Mix current sound data.
        // Data, from raw sound, for right and left.

        // Reset left/right value. 
        int32 dl = 0;
        int32 dr = 0;

        // Love thy L2 chache - made this a loop.
        // Now more channels could be set at compile time as well. Thus loop those  channels.
        for (int32 chan = 0; chan < NUM_CHANNELS; chan++)
        {
            // Check channel, if active.
            if (channels[chan])
            {
                // Get the raw data from the channel. 
                uint32 sample = *channels[chan];
                // Add left and right part
                //  for this channel (sound)
                //  to the current data.
                // Adjust volume accordingly.
                dl += channelleftvol_lookup[chan][sample];
                dr += channelrightvol_lookup[chan][sample];
                // Increment index ???
                channelstepremainder[chan] += channelstep[chan];
                // MSB is next sample???
                channels[chan] += channelstepremainder[chan] >> 16;
                // Limit to LSB???
                channelstepremainder[chan] &= 65536 - 1;

                // Check whether we are done.
                if (channels[chan] >= channelsend[chan])
                    channels[chan] = 0;
            }
        }

        // Clamp to range. Left hardware channel.
        // Has been char instead of short.
        // if (dl > 127) *leftout = 127;
        // else if (dl < -128) *leftout = -128;
        // else *leftout = dl;

        // Pointers in global mixbuffer, left, right, end.

        if (dl > 0x7fff)
            *leftout = 0x7fff;
        else if (dl < -0x8000)
            *leftout = -0x8000;
        else
            *leftout = static_cast<short>(dl);

        // Same for right hardware channel.
        if (dr > 0x7fff)
            *rightout = 0x7fff;
        else if (dr < -0x8000)
            *rightout = -0x8000;
        else
            *rightout = static_cast<short>(dr);

        // Increment current pointers in mixbuffer.
        leftout += step;
        rightout += step;
    }

    // See how much buffer space is available.
    uint32 paddingFrames = 0;
    auto result = client->GetCurrentPadding(&paddingFrames);
    if (FAILED(result))
        I_Error("GetCurrentPadding failed: {}", result);

    uint32 numFramesAvailable = bufferSizeInFrames - paddingFrames;
    //assert(numFramesAvailable / 4 >= SAMPLECOUNT);
    auto writeFrames = std::min(numFramesAvailable / 4, SAMPLECOUNT);

    // Grab all the available space in the shared buffer.
    byte* data = nullptr;
    result = renderer->GetBuffer(writeFrames * 4, &data);
    if (FAILED(result))
        I_Error("GetBuffer failed: {}", result);

    auto* fp = reinterpret_cast<float*>(data);
    auto* mp = mixbuffer;
    auto to_float = [](int16 n){ return static_cast<float>(n) / std::numeric_limits<int16>::max(); };
    for (uint32 n = 0; n < writeFrames; ++n)
    {
        *fp++ = to_float(*mp);
        *fp++ = to_float(*mp);
        *fp++ = to_float(*mp);
        *fp++ = to_float(*mp++);

        *fp++ = to_float(*mp);
        *fp++ = to_float(*mp);
        *fp++ = to_float(*mp);
        *fp++ = to_float(*mp++);
    }

    int flags = 0;
    result = renderer->ReleaseBuffer(writeFrames * 4, flags);
    if (FAILED(result))
        I_Error("ReleaseBuffer failed: {}", result);
}

void Sound::Shutdown()
{
    delete[] mixBuffer;

    client->Stop();

    renderer->Release();
    renderer = nullptr;

    client->Release();
    client = nullptr;

    device->Release();
    device = nullptr;
}

int32 Sound::Play(int32 id, int32 volume, int32 seperation, int32 pitch, [[maybe_unused]] int32 priority)
{
    // Starting a sound means adding it to the current list of active sounds in the internal channels.
    // As the SFX info struct contains e.g. a pointer to the raw data, it is ignored.
    // As our sound handling does not handle priority, it is ignored.
    // Pitching (that is, increased speed of playback) is set, but currently not used by mixing.

    // This function adds a sound to the list of currently active sounds, which is maintained as a
    // given number (eight, usually) of internal channels.
    // Returns a handle.
    static unsigned short handlenums = 0;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if (id == sfx_sawup
        || id == sfx_sawidl
        || id == sfx_sawful
        || id == sfx_sawhit
        || id == sfx_stnmov
        || id == sfx_pistol)
    {
        // Loop all channels, check.
        for (int32 i = 0; i < NUM_CHANNELS; ++i)
        {
            // Active, and using the same SFX?
            if ((channels[i]) && (channelids[i] == id))
            {
                // Reset.
                channels[i] = 0;
                // We are sure that iff, there will only be one.
                break;
            }
        }
    }

    // Loop all channels to find oldest SFX.
    int32 oldest = gametic;
    int32 oldestnum = 0;
    int32 i = 0;
    for (; (i < NUM_CHANNELS) && (channels[i]); ++i)
    {
        if (channelstart[i] < oldest)
        {
            oldestnum = i;
            oldest = channelstart[i];
        }
    }

    // Tales from the cryptic.
    // If we found a channel, fine.
    // If not, we simply overwrite the first one, 0.
    // Probably only happens at startup.
    int32 slot = 0;
    if (i == NUM_CHANNELS)
        slot = oldestnum;
    else
        slot = i;

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    channels[slot] = (unsigned char*)S_sfx[id].data;
    // Set pointer to end of raw data.
    channelsend[slot] = channels[slot] + lengths[id];

    // Reset current handle number, limited to 0..100.
    if (!handlenums)
        handlenums = 100;

    int32 rc = -1;
    // Assign current handle number.
    // Preserved so sounds could be stopped (unused).
    channelhandles[slot] = rc = handlenums++;

    // Set stepping???
    // Kinda getting the impression this is never used.
    channelstep[slot] = steptable[pitch];
    // ???
    channelstepremainder[slot] = 0;
    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    auto leftvol = volume - ((volume * seperation * seperation) >> 16); ///(256*256);
    seperation = seperation - 257;
    auto rightvol = volume - ((volume * seperation * seperation) >> 16);

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
        I_Error("rightvol out of bounds");

    if (leftvol < 0 || leftvol > 127)
        I_Error("leftvol out of bounds");

    // Get the proper lookup table piece
    //  for this volume level???
    channelleftvol_lookup[slot] = &vol_lookup[leftvol * 256];
    channelrightvol_lookup[slot] = &vol_lookup[rightvol * 256];

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = id;

    // You tell me.
    return rc;
}

void Sound::Stop([[maybe_unused]] int32 handle)
{
    /*
    if (handle >= functions.size())
        return;

    if (functions[handle].second == nullptr)
        return;

    functions[handle].second = nullptr;
    free.push_back(handle);
    /**/
}
