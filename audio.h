#ifndef AUDIO_H
#define AUDIO_H

void init_audio();
void free_audio();
int create_sound(char *fileName);
void set_sound_play_state(int soundHandle, bool playState);
float get_sound_volume(int soundHandle);
void set_sound_volume(int soundHandle, float volume);
void reset_sound(int soundHandle);
void stop_sound(int soundHandle);

extern float soundVolume;
extern float musicVolume;

#endif /* end of include guard: AUDIO_H */
