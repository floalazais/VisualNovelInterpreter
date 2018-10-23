#ifndef AUDIO_H
#define AUDIO_H

typedef struct mal_device mal_device;
typedef struct mal_decoder mal_decoder;

typedef struct Sound
{
	int id;
	mal_decoder *decoder;
	mal_device *device;
	float volume;
	bool playing;
} Sound;

Sound *create_sound(char *fileName);
void reset_sound(Sound *sound);
void stop_sound(Sound *sound);
void free_sound(Sound *sound);
void free_audio();

#endif /* end of include guard: AUDIO_H */