#ifndef LURDS2_SOUND
#define LURDS2_SOUND

typedef void* SoundChannel;
#define SoundChannel_None 0
SoundChannel SoundChannel_Create();
int          SoundChannel_OpenDefault(SoundChannel soundChannel);
//int          SoundChannel_StartPlaying(SoundBuffer buffer, int loop);
//int          SoundChannel_StopPlaying();

typedef void* SoundBuffer;
#define SoundBuffer_None 0
SoundBuffer SoundBuffer_Create();
int         SoundBuffer_LoadFromFileW(SoundBuffer soundBuffer, wchar_t * filePath);



#endif