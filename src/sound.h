#ifndef LURDS2_SOUND
#define LURDS2_SOUND

typedef void* SoundChannel;
typedef void* SoundBuffer;

// A SoundChannel has the ability to play one sound at a time.
// You must bind a SoundBuffer to it; then play or stop.
// (you may create multiple SoundChannels to play multiple sounds simultaneously)
SoundChannel SoundChannel_Open();
void         SoundChannel_Play(SoundChannel soundChannel, SoundBuffer soundBuffer, int loop);
void         SoundChannel_Stop(SoundChannel soundChannel);
void         SoundChannel_Release(SoundChannel soundChannel);

// A SoundBuffer holds the data for a loaded sound. It may be played by multiple SoundChannels simultaneously.
// It is refcounted, and the refcount is incremented while a SoundChannel is playing the SoundBuffer.
// (you may release the SoundBuffer while it's being played and trust it'll get cleaned up when it's done being played)
SoundBuffer  SoundBuffer_LoadFromFileW(wchar_t * filePath);
void         SoundBuffer_Release(SoundBuffer soundBuffer);


#endif