#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "soloud_c.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "stroperation.h"
#include "audio.h"

static Soloud *soloud;

static void **soundSources;
static char **soundSourcesPaths;

float soundVolume;
float musicVolume;

void init_audio()
{
	soloud = Soloud_create();
	Soloud_initEx(soloud, SOLOUD_CLIP_ROUNDOFF, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO, SOLOUD_AUTO);
	Soloud_setGlobalVolume(soloud, 0.1f);

	soundSources = NULL;
	soundSourcesPaths = NULL;

	soundVolume = 1.0f;
	musicVolume = 1.0f;
}

void free_audio()
{
	for (unsigned int i = 0; i < buf_len(soundSourcesPaths); i++)
	{
		buf_free(soundSourcesPaths[i]);
	}
	buf_free(soundSourcesPaths);

	for (unsigned int i = 0; i < buf_len(soundSources); i++)
	{
		Wav_destroy(soundSources[i]);
	}
	buf_free(soundSources);

	Soloud_deinit(soloud);
	Soloud_destroy(soloud);
}

int create_sound(char *fileName)
{
	for (unsigned int i = 0; i < buf_len(soundSourcesPaths); i++)
	{
		if (strmatch(soundSourcesPaths[i], fileName))
		{
			return Soloud_playEx(soloud, soundSources[i], 1.0f, 0.0f, true, 0);
		}
	}
	void *soundSource = Wav_create();
	Wav_load(soundSource, fileName);
	buf_add(soundSources, soundSource);
	char *soundSourcePath = NULL;
	soundSourcePath = strcopy(soundSourcePath, fileName);
	buf_add(soundSourcesPaths, soundSourcePath);

	return Soloud_playEx(soloud, soundSource, 1.0f, 0.0f, true, 0);
}

void set_sound_play_state(int soundHandle, bool playState)
{
	Soloud_setPause(soloud, soundHandle, !playState);
}

float get_sound_volume(int soundHandle)
{
	return Soloud_getVolume(soloud, soundHandle);
}

void set_sound_volume(int soundHandle, float volume)
{
	Soloud_setVolume(soloud, soundHandle, volume);
}

void reset_sound(int soundHandle)
{
	Soloud_setPause(soloud, soundHandle, true);
	Soloud_seek(soloud, soundHandle, 0.0f);
}

void stop_sound(int soundHandle)
{
	Soloud_stop(soloud, soundHandle);
}
