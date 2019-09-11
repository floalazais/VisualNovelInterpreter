#ifndef AUDIO_H
#define AUDIO_H

typedef struct AudioSource
{
	int id;
	float volume;
	bool playing;
} AudioSource;

AudioSource *create_audio_source(const char *fileName);
void reset_audio_source(AudioSource *audioSource);
void stop_audio_source(AudioSource *audioSource);
void free_audio();

#endif /* end of include guard: AUDIO_H */
