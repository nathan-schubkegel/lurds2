#ifndef LURDS2_SOUND
#define LURDS2_SOUND

typedef void* SoundChannel;
typedef void* SoundBuffer;

#define SoundChannel_None 0
SoundChannel SoundChannel_Create();
int          SoundChannel_OpenDefault(SoundChannel soundChannel);
int          SoundChannel_StartPlaying(SoundChannel soundChannel, SoundBuffer soundBuffer, int loop);
//int          SoundChannel_StopPlaying();


#define SoundBuffer_None 0
SoundBuffer SoundBuffer_Create();
int         SoundBuffer_LoadFromFileW(SoundBuffer soundBuffer, wchar_t * filePath);



#endif