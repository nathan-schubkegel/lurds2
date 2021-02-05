#include "sound.h"

#include "defines.h"

#define DIAGNOSTIC_SOUND_ERROR(message) MessageBox(0, (message), "error in sound.c", 0);

typedef struct SoundChannelData {
  HWAVEOUT handle;
} SoundChannelData;

SoundChannel SoundChannel_Create()
{
  SoundChannelData * result;
  result = (SoundChannel)malloc(sizeof(SoundChannelData));
  if (result != 0)
  {
    memset(result, 0, sizeof(SoundChannelData));
  }
  return result;
}

int SoundChannel_OpenDefault(SoundChannel soundChannel)
{
  SoundChannelData* data;
  data = (SoundChannelData*)soundChannel;
  if (!data)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundChannel arg provided to SoundChannel_OpenDefault");
    return 0;
  }
  
  if (data->handle != 0)
  {
    DIAGNOSTIC_SOUND_ERROR("soundChannel already opened in SoundChannel_OpenDefault");
    return 0;
  }
  
  WAVEFORMATEX waveFormat;
  memset(&waveFormat, 0, sizeof(waveFormat));
  waveFormat.cbSize = sizeof(waveFormat);

  // fill it for lurds2 resource sounds
  // FUTURE: inspect audio files
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = 1;
  waveFormat.nSamplesPerSec = 11025;
  waveFormat.wBitsPerSample = 8;
  waveFormat.nBlockAlign = waveFormat.wBitsPerSample * waveFormat.nChannels / 8;

  HWAVEOUT h;
  MMRESULT result;
  result = waveOutOpen(&h, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
  if (MMSYSERR_NOERROR == result)
  {
    data->handle = h;
    return 1;
  }
  
  switch (result)
  {
    case MMSYSERR_ALLOCATED: DIAGNOSTIC_SOUND_ERROR("Specified resource is already allocated."); break;
    case MMSYSERR_BADDEVICEID: DIAGNOSTIC_SOUND_ERROR("Specified device identifier is out of range."); break;
    case MMSYSERR_NODRIVER: DIAGNOSTIC_SOUND_ERROR("No device driver is present."); break;
    case MMSYSERR_NOMEM: DIAGNOSTIC_SOUND_ERROR("Unable to allocate or lock memory."); break;
    case WAVERR_BADFORMAT: DIAGNOSTIC_SOUND_ERROR("Attempted to open with an unsupported waveform-audio format."); break;
    case WAVERR_SYNC: DIAGNOSTIC_SOUND_ERROR("The device is synchronous but waveOutOpen was called without using the WAVE_ALLOWSYNC flag."); break;
    default: DIAGNOSTIC_SOUND_ERROR("Unknown error."); break;
  }
  
  return 0;
}

/*
SoundBuffer SoundBuffer_Create()
int         SoundBuffer_LoadFromFile(wchar_t * filePath);

SoundChannel SoundChannel_Create();
int          SoundChannel_OpenDefault();
int          SoundChannel_StartPlaying(SoundBuffer buffer, int loop);
int          SoundChannel_StopPlaying();
*/