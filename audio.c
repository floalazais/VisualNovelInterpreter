#define DR_MP3_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#define MINI_AL_IMPLEMENTATION

#include <stdio.h>
#include <stdbool.h>
#include <wchar.h>

#include "dr_mp3.h"
#include "dr_wav.h"
#include "mini_al.h"
#include "error.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "audio.h"
#include "maths.h"
#include "token.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "globals.h"

Sound **sounds = NULL;

static mal_uint32 update_sound(mal_device* device, mal_uint32 frameCount, void* samples)
{
	Sound *sound = (Sound *)device->pUserData;

	if (!sound->playing)
	{
		return 0;
	}

    if (!sound->decoder)
	{
        return 0;
    }

    mal_uint32 nbFramesRead = (mal_uint32)mal_decoder_read(sound->decoder, frameCount, samples);

	if (nbFramesRead == 0)
	{
		sound->playing = false;
		mal_decoder_seek_to_frame(sound->decoder, 0);
	}

	if (isWindowActive)
	{
		for (unsigned int frame = 0; frame < nbFramesRead; frame++)
		{
			for (unsigned int channel = 0; channel < sound->decoder->outputChannels; channel++)
			{
				((float *)samples)[sound->decoder->outputChannels * frame + channel] *= sound->volume;
			}
		}
	} else {
		for (unsigned int frame = 0; frame < nbFramesRead; frame++)
		{
			for (unsigned int channel = 0; channel < sound->decoder->outputChannels; channel++)
			{
				((float *)samples)[sound->decoder->outputChannels * frame + channel] = 0;
			}
		}
	}

	return nbFramesRead;
}

Sound *create_sound(char *fileName)
{
	Sound *sound = xmalloc(sizeof (*sound));
	sound->id = buf_len(sounds);
	sound->playing = false;
	sound->volume = 1.0f;

	sound->decoder = xmalloc(sizeof (*sound->decoder));
    mal_result result = mal_decoder_init_file(fileName, NULL, sound->decoder);
    if (result != MAL_SUCCESS)
	{
        error("could not decode %s.", fileName);
    }

    mal_device_config config = mal_device_config_init_playback(sound->decoder->outputFormat, sound->decoder->outputChannels, sound->decoder->outputSampleRate, update_sound);

	sound->device = xmalloc(sizeof (*sound->device));
    if (mal_device_init(NULL, mal_device_type_playback, NULL, &config, sound, sound->device) != MAL_SUCCESS)
	{
		mal_decoder_uninit(sound->decoder);
        error("Failed to open playback device.");
    }

    if (mal_device_start(sound->device) != MAL_SUCCESS)
	{
        mal_device_uninit(sound->device);
        mal_decoder_uninit(sound->decoder);
        error("Failed to start playback device.");
    }

	buf_add(sounds, sound);
	return sound;
}

void reset_sound(Sound *sound)
{
	mal_decoder_seek_to_frame(sound->decoder, 0);
}

void stop_sound(Sound *sound)
{
    mal_decoder_uninit(sound->decoder);
	mal_device_uninit(sound->device);

	free_sound(sound);
}

void free_sound(Sound *sound)
{
	xfree(sound->device);
    xfree(sound->decoder);

	int soundId = sound->id;

	xfree(sound);

	sounds[soundId] = NULL;
}

void free_audio()
{
	for (unsigned int i = 0; i < buf_len(sounds); i++)
	{
		if (sounds[i])
		{
			stop_sound(sounds[i]);
		}
	}
	buf_free(sounds);
}