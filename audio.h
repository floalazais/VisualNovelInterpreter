#ifndef AUDIO_H
#define AUDIO_H

typedef enum SoundType
{
	SOUND_SOUND,
	SOUND_MUSIC
} SoundType;

typedef struct Sound
{
	SoundType soundType;
	void *decoder;
	void *device;
	float volume;
	bool playing;
	bool stopping;
} Sound;

Sound create_sound(SoundType soundType, char *fileName);
void reset_sound(Sound *sound);
void stop_sound(Sound *sound);

extern float soundVolume;
extern float musicVolume;

#endif /* end of include guard: AUDIO_H */
