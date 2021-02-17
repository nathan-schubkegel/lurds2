/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_SOUND
#define LURDS2_SOUND

typedef void* SoundChannel;
typedef void* SoundBuffer;

// A SoundChannel has the ability to play one SoundBuffer at a time
// (you may create multiple SoundChannels to play multiple sounds simultaneously).
// It is refcounted (so you don't have to wait for it to stop playing before releasing it)
// and threaded (so it doesn't take your thread's time to load or play the sound)
// and thread-safe (because that's one of the benefits of being threaded).
SoundChannel SoundChannel_Open();
void         SoundChannel_Play(SoundChannel soundChannel, SoundBuffer soundBuffer, long loop);
void         SoundChannel_Stop(SoundChannel soundChannel);
void         SoundChannel_Release(SoundChannel soundChannel);

// A SoundBuffer holds the data for a sound that can be played
// (note that one SoundBuffer may be played by multiple SoundChannels simultaneously).
// It is refcounted, and the refcount is incremented while a SoundChannel references the SoundBuffer.
// You may release the SoundBuffer while it's being played and trust it'll get cleaned up when it's done being played.
// It is threaded (so it doesn't take your thread's time to load the sound).
SoundBuffer  SoundBuffer_LoadFromFileW(const wchar_t * filePath);
void         SoundBuffer_Release(SoundBuffer soundBuffer);

#endif