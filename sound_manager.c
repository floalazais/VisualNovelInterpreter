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
#include "sound_manager.h"

SoundHandle **soundHandles = NULL;

static mal_uint32 update_sound(mal_device* pDevice, mal_uint32 frameCount, void* pSamples)
{
	for (unsigned int i = 0; i < buf_len(soundHandles); i++)
	{
		if (soundHandles[i]->device == pDevice)
		{
			if (soundHandles[i]->playing)
			{
				break;
			} else {
				return 0;
			}
		}
	}

    mal_decoder* pDecoder = (mal_decoder*)pDevice->pUserData;
    if (!pDecoder)
	{
        return 0;
    }

    return (mal_uint32)mal_decoder_read(pDecoder, frameCount, pSamples);
}

int play_sound(char *fileName)
{
	SoundHandle *soundHandle = xmalloc(sizeof (*soundHandle));
	soundHandle->playing = true;

	soundHandle->decoder = xmalloc(sizeof (*soundHandle->decoder));
    mal_result result = mal_decoder_init_file(fileName, NULL, soundHandle->decoder);
    if (result != MAL_SUCCESS)
	{
        error("could not decode %s.", fileName);
    }

    mal_device_config config = mal_device_config_init_playback(soundHandle->decoder->outputFormat, soundHandle->decoder->outputChannels, soundHandle->decoder->outputSampleRate, update_sound);

	soundHandle->device = xmalloc(sizeof (*soundHandle->device));
    if (mal_device_init(NULL, mal_device_type_playback, NULL, &config, soundHandle->decoder, soundHandle->device) != MAL_SUCCESS)
	{
		mal_decoder_uninit(soundHandle->decoder);
        error("Failed to open playback device.");
    }

    if (mal_device_start(soundHandle->device) != MAL_SUCCESS)
	{
        mal_device_uninit(soundHandle->device);
        mal_decoder_uninit(soundHandle->decoder);
        error("Failed to start playback device.");
    }

	buf_add(soundHandles, soundHandle);
	return buf_len(soundHandles) - 1;
}

void pause_sound(int soundHandleId)
{
	if (soundHandleId >= buf_len(soundHandles))
	{
		error("incorrect sound handle id %d provided.", soundHandleId);
	} else if (!soundHandles[soundHandleId]) {
		error("sound handle id %d identifies an already stopped sound.", soundHandleId);
	}
	soundHandles[soundHandleId]->playing = false;
}

void resume_sound(int soundHandleId)
{
	if (soundHandleId >= buf_len(soundHandles))
	{
		error("incorrect sound handle id %d provided.", soundHandleId);
	} else if (!soundHandles[soundHandleId]) {
		error("sound handle id %d identifies an already stopped sound.", soundHandleId);
	}
	soundHandles[soundHandleId]->playing = true;
}

void stop_sound(int soundHandleId)
{
	if (soundHandleId >= buf_len(soundHandles))
	{
		error("incorrect sound handle id %d provided.", soundHandleId);
	} else if (!soundHandles[soundHandleId]) {
		error("sound handle id %d identifies an already stopped sound.", soundHandleId);
	}
	mal_device_uninit(soundHandles[soundHandleId]->device);
    mal_decoder_uninit(soundHandles[soundHandleId]->decoder);

	xfree(soundHandles[soundHandleId]->device);
    xfree(soundHandles[soundHandleId]->decoder);

	xfree(soundHandles[soundHandleId]);
	soundHandles[soundHandleId] = NULL;
}
