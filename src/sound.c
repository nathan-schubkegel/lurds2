#include "sound.h"

#include "errors.h"

#define DIAGNOSTIC_SOUND_ERROR(message) MessageBox(0, (message), "error in sound.c", 0);

typedef struct SoundChannelData {
  HWAVEOUT handle;
  WAVEHDR header; // data associated with currently-playing sound
} SoundChannelData;

typedef struct SoundBufferData {
  char * data;
  int length;
} SoundBufferData;

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

int SoundChannel_StartPlaying(SoundChannel soundChannel, SoundBuffer soundBuffer, int loop)
{
  SoundChannelData* soundChannelData;
  soundChannelData = (SoundChannelData*)soundChannel;
  if (!soundChannelData)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundChannel arg provided to SoundChannel_StartPlaying");
    return 0;
  }
  
  if (soundChannelData->handle == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("soundChannel not yet opened in SoundChannel_StartPlaying");
    return 0;
  }
  
  SoundBufferData* soundBufferData;
  soundBufferData = (SoundBufferData*)soundBuffer;
  if (!soundBufferData)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundBuffer arg provided to SoundChannel_StartPlaying");
    return 0;
  }
  
  if (soundBufferData->data == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("soundBuffer not yet loaded in SoundChannel_StartPlaying");
    return 0;
  }
  
  LPWAVEHDR header;
  header = &soundChannelData->header;
  memset(header, 0, sizeof(*header));
  header->lpData = soundBufferData->data;
  header->dwBufferLength = soundBufferData->length;
  header->dwFlags = 0 | (loop ? (WHDR_BEGINLOOP | WHDR_ENDLOOP) : 0);
  header->dwLoops = loop ? 0xffffffff : 0;
  
  MMRESULT result;
  result = waveOutPrepareHeader(soundChannelData->handle, header, sizeof(*header));
  if (MMSYSERR_NOERROR != result)
  {
    DIAGNOSTIC_SOUND_ERROR("waveOutPrepareHeader failed in SoundChannel_StartPlaying");
    return 0;
  }
  
  result = waveOutWrite(soundChannelData->handle, header, sizeof(*header));
  if (result != MMSYSERR_NOERROR)
  {
    switch (result)
    {
      case MMSYSERR_INVALHANDLE: DIAGNOSTIC_SOUND_ERROR("waveOutWrite error: Specified device handle is invalid."); break;
      case MMSYSERR_NODRIVER: DIAGNOSTIC_SOUND_ERROR("waveOutWrite error: No device driver is present."); break;
      case MMSYSERR_NOMEM: DIAGNOSTIC_SOUND_ERROR("waveOutWrite error: Unable to allocate or lock memory."); break;
      case WAVERR_UNPREPARED: DIAGNOSTIC_SOUND_ERROR("waveOutWrite error: The data block pointed to by the pwh parameter hasn't been prepared."); break;
      default: DIAGNOSTIC_SOUND_ERROR("unknown waveOutWrite error");
    }
    return 0;
  }
}

SoundBuffer SoundBuffer_Create()
{
  SoundBufferData * result;
  result = (SoundBuffer)malloc(sizeof(SoundBufferData));
  if (result != 0)
  {
    memset(result, 0, sizeof(SoundBufferData));
  }
  return result;
}

int SoundBuffer_LoadFromFileW(SoundBuffer soundBuffer, wchar_t * filePath)
{
  SoundBufferData* data;
  data = (SoundBufferData*)soundBuffer;
  if (!data)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundBuffer arg provided to SoundBuffer_LoadFromFileW");
    return 0;
  }
  
  HANDLE h;
  h = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h == INVALID_HANDLE_VALUE)
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessage());
    return 0;
  }
  
  DWORD size;
  DWORD sizeHigh;
  size = GetFileSize(h, &sizeHigh);
  if (INVALID_FILE_SIZE == size)
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessage());
    CloseHandle(h);
    return 0;
  }

  if (size > 100000000 || sizeHigh > 0)
  {
    DIAGNOSTIC_SOUND_ERROR("sound file too big");
    CloseHandle(h);
    return 0;
  }

  char * buffer;
  buffer = malloc(size);
  if (buffer == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("failed to allocate memory for sound from file");
    CloseHandle(h);
    return 0;
  }
  
  DWORD numBytesRead;
  if (!ReadFile(h, buffer, size, &numBytesRead, 0))
  {
    DIAGNOSTIC_SOUND_ERROR("failed to read sound from file");
    CloseHandle(h);
    free(buffer);
    return 0;
  }
  
  if (numBytesRead != size)
  {
    DIAGNOSTIC_SOUND_ERROR("unexpected numByteRead from sound file");
    CloseHandle(h);
    free(buffer);
    return 0;
  }
  
  if (data->data != 0)
  {
    free(data->data);
  }
  
  data->data = buffer;
  data->length = size;
  return 1;
}