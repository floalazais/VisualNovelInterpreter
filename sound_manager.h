typedef struct mal_decoder mal_decoder;
typedef struct mal_device mal_device;

typedef struct SoundHandle
{
	mal_decoder *decoder;
	mal_device *device;
	bool playing;
} SoundHandle;

int play_sound(char *fileName);
void pause_sound(int soundHandleId);
void resume_sound(int soundHandleId);
void stop_sound(int soundHandleId);
