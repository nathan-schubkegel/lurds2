#ifndef LURDS2_SOUND
#define LURDS2_SOUND

typedef void* SoundChannel;
typedef void* SoundBuffer;

//SoundBuffer SoundBuffer_Create();
//int         SoundBuffer_LoadFromFile(wchar_t * filePath);

SoundChannel SoundChannel_Create();
int          SoundChannel_OpenDefault(SoundChannel soundChannel);
//int          SoundChannel_StartPlaying(SoundBuffer buffer, int loop);
//int          SoundChannel_StopPlaying();

#endif