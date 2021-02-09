#include "sound.h"

#include "errors.h"

#define LURDS2_USE_SOUND_MMEAPI

#define DIAGNOSTIC_SOUND_ERROR(message) MessageBox(0, (message), "error in sound.c", 0);

typedef struct SoundBufferData {
  int refcount;
  char* data;
  int length;
} SoundBufferData;

typedef struct SoundChannelData {
#ifdef LURDS2_USE_SOUND_MMEAPI
  HWAVEOUT handle;
  WAVEHDR header; // data associated with currently-playing sound
#endif
  SoundBufferData* buffer;
} SoundChannelData;

SoundChannel SoundChannel_Open()
{
  SoundChannelData* channel;
  channel = malloc(sizeof(SoundChannelData));
  if (channel == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Open(): failed to allocate memory for SoundChannelData");
    return 0;
  }
  memset(channel, 0, sizeof(SoundChannelData));

#ifdef LURDS2_USE_SOUND_MMEAPI
  WAVEFORMATEX waveFormat;
  memset(&waveFormat, 0, sizeof(waveFormat));
  waveFormat.cbSize = sizeof(waveFormat);

  // fill it according to lurds2 resource sounds
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = 1;
  waveFormat.nSamplesPerSec = 11025;
  waveFormat.wBitsPerSample = 8;
  waveFormat.nBlockAlign = waveFormat.wBitsPerSample * waveFormat.nChannels / 8;

  HWAVEOUT h;
  MMRESULT result;
  result = waveOutOpen(&h, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
  if (MMSYSERR_NOERROR != result)
  {
    char* message;
    switch (result)
    {
      case MMSYSERR_ALLOCATED: message = "SoundChannel_Open(): waveOutOpen(): Specified resource is already allocated."; break;
      case MMSYSERR_BADDEVICEID: message = "SoundChannel_Open(): waveOutOpen(): Specified device identifier is out of range."; break;
      case MMSYSERR_NODRIVER: message = "SoundChannel_Open(): waveOutOpen(): No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "SoundChannel_Open(): waveOutOpen(): Unable to allocate or lock memory."; break;
      case WAVERR_BADFORMAT: message = "SoundChannel_Open(): waveOutOpen(): Attempted to open with an unsupported waveform-audio format."; break;
      case WAVERR_SYNC: message = "SoundChannel_Open(): waveOutOpen(): The device is synchronous but waveOutOpen was called without using the WAVE_ALLOWSYNC flag."; break;
      default: message = "SoundChannel_Open(): waveOutOpen(): Unknown error."; break;
    }
    DIAGNOSTIC_SOUND_ERROR(message);
    free(channel);
    return 0;
  }

  channel->handle = h;
#endif

  return channel;
}

void SoundChannel_Unbind(SoundChannelData* channel)
{
  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Unbind(): null channel arg provided");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  if (!channel->handle)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Unbind(): invalid soundChannel->handle (likely a dangling pointer defect)");
    return;
  }
#endif

  if (channel->buffer)
  {
#ifdef LURDS2_USE_SOUND_MMEAPI
    // in case a sound is currently playing, gotta stop it first
    MMRESULT result;
    result = waveOutReset(channel->handle);
    if (result != MMSYSERR_NOERROR)
    {
      char* message;
      switch (result)
      {
        case MMSYSERR_INVALHANDLE: message = "SoundChannel_Unbind(): waveOutReset(): Specified device handle is invalid."; break;
        case MMSYSERR_NODRIVER: message = "SoundChannel_Unbind(): waveOutReset(): No device driver is present."; break;
        case MMSYSERR_NOMEM: message = "SoundChannel_Unbind(): waveOutReset(): Unable to allocate or lock memory."; break;
        case MMSYSERR_NOTSUPPORTED: message = "SoundChannel_Unbind(): waveOutReset(): Specified device is synchronous and does not support pausing."; break;
        default: message = "SoundChannel_Unbind(): waveOutReset(): Unknown error."; break;
      }
      DIAGNOSTIC_SOUND_ERROR(message);
      // TODO: how to robustly recover from this kind of error?
    }

    result = waveOutUnprepareHeader(channel->handle, &channel->header, sizeof(channel->header));
    if (result != MMSYSERR_NOERROR)
    {
      char* message;
      switch (result)
      {
        case MMSYSERR_INVALHANDLE: message = "SoundChannel_Unbind(): waveOutUnprepareHeader(): Specified device handle is invalid."; break;
        case MMSYSERR_NODRIVER: message = "SoundChannel_Unbind(): waveOutUnprepareHeader(): No device driver is present."; break;
        case MMSYSERR_NOMEM: message = "SoundChannel_Unbind(): waveOutUnprepareHeader(): Unable to allocate or lock memory."; break;
        case WAVERR_STILLPLAYING: message = "SoundChannel_Unbind(): waveOutUnprepareHeader(): The data block pointed to by the pwh parameter is still in the queue."; break;
        default: message = "SoundChannel_Unbind(): waveOutUnprepareHeader(): Unknown error."; break;
      }
      DIAGNOSTIC_SOUND_ERROR(message);
      // TODO: how to robustly recover from this kind of error?
    }
#endif

    SoundBuffer_Release(channel->buffer);
    channel->buffer = 0;
  }
}

void SoundChannel_Bind(SoundChannel soundChannel, SoundBuffer soundBuffer, int loop)
{
  SoundChannelData* channel = (SoundChannelData*)soundChannel;
  SoundBufferData* buffer = (SoundBufferData*)soundBuffer;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Bind(): null soundChannel arg provided");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  if (!channel->handle)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Bind(): invalid soundChannel->handle (likely a dangling pointer defect)");
    return;
  }
#endif
  
  if (!buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Bind(): null soundBuffer arg provided");
    return;
  }

  // in case a sound is currently playing, gotta stop it first
  SoundChannel_Unbind(channel);

  // check these after unbinding because it catches gross corner case of rebinding to same buffer w/ dangling pointer
  if (buffer->refcount < 1)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Bind(): soundBuffer arg has bad refcount");
    return;
  }

  if (!buffer->data)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Bind(): soundBuffer arg has no data");
    return;
  }

  if (buffer->length < 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Bind(): soundBuffer arg has negative length");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  LPWAVEHDR header;
  header = &channel->header;
  memset(header, 0, sizeof(*header));
  header->lpData = buffer->data;
  header->dwBufferLength = buffer->length;
  header->dwFlags = 0 | (loop ? (WHDR_BEGINLOOP | WHDR_ENDLOOP) : 0);
  header->dwLoops = loop ? 0xffffffff : 0;

  MMRESULT result;
  result = waveOutPrepareHeader(channel->handle, header, sizeof(*header));
  if (MMSYSERR_NOERROR != result)
  {
    char * message;
    switch (result)
    {
      case MMSYSERR_INVALHANDLE: message = "SoundChannel_Bind(): waveOutPrepareHeader(): Specified device handle is invalid."; break;
      case MMSYSERR_NODRIVER: message = "SoundChannel_Bind(): waveOutPrepareHeader(): No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "SoundChannel_Bind(): waveOutPrepareHeader(): Unable to allocate or lock memory."; break;
      default: message = "SoundChannel_Bind(): waveOutPrepareHeader(): unknown error"; break;
    }
    DIAGNOSTIC_SOUND_ERROR(message);
    return;
  }
#endif
  
  channel->buffer = buffer;
  buffer->refcount++;
}

void SoundChannel_Play(SoundChannel soundChannel)
{
  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): null soundChannel arg provided");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  if (!channel->handle)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): invalid soundChannel->handle (likely a dangling pointer defect)");
    return;
  }
#endif

  if (!channel->buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): soundChannel not yet bound to SoundBuffer");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  // in case a sound is currently playing, stop it first
  MMRESULT result;
  result = waveOutReset(channel->handle);
  if (result != MMSYSERR_NOERROR)
  {
    char* message;
    switch (result)
    {
      case MMSYSERR_INVALHANDLE: message = "SoundChannel_Play(): waveOutReset(): Specified device handle is invalid."; break;
      case MMSYSERR_NODRIVER: message = "SoundChannel_Play(): waveOutReset(): No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "SoundChannel_Play(): waveOutReset(): Unable to allocate or lock memory."; break;
      case MMSYSERR_NOTSUPPORTED: message = "SoundChannel_Play(): waveOutReset(): Specified device is synchronous and does not support pausing."; break;
      default: message = "SoundChannel_Play(): waveOutReset(): Unknown error."; break;
    }
    DIAGNOSTIC_SOUND_ERROR(message);
    // TODO: how to robustly recover from this kind of error?
  }

  result = waveOutWrite(channel->handle, &channel->header, sizeof(channel->header));
  if (result != MMSYSERR_NOERROR)
  {
    switch (result)
    {
      case MMSYSERR_INVALHANDLE: DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): waveOutWrite(): Specified device handle is invalid."); break;
      case MMSYSERR_NODRIVER: DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): waveOutWrite(): No device driver is present."); break;
      case MMSYSERR_NOMEM: DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): waveOutWrite(): Unable to allocate or lock memory."); break;
      case WAVERR_UNPREPARED: DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): waveOutWrite(): The data block pointed to by the pwh parameter hasn't been prepared."); break;
      default: DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): waveOutWrite(): unknown error");
    }
  }
#endif
}

void SoundChannel_Stop(SoundChannel soundChannel)
{
  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Stop(): null soundChannel arg provided");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  if (!channel->handle)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Stop(): invalid soundChannel->handle (likely a dangling pointer defect)");
    return;
  }
#endif

  if (!channel->buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Stop(): soundChannel not yet bound to SoundBuffer");
    return;
  }
  
#ifdef LURDS2_USE_SOUND_MMEAPI
  MMRESULT result;
  result = waveOutReset(channel->handle);
  if (result != MMSYSERR_NOERROR)
  {
    char* message;
    switch (result)
    {
      case MMSYSERR_INVALHANDLE: message = "SoundChannel_Stop(): waveOutReset(): Specified device handle is invalid."; break;
      case MMSYSERR_NODRIVER: message = "SoundChannel_Stop(): waveOutReset(): No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "SoundChannel_Stop(): waveOutReset(): Unable to allocate or lock memory."; break;
      case MMSYSERR_NOTSUPPORTED: message = "SoundChannel_Stop(): waveOutReset(): Specified device is synchronous and does not support pausing."; break;
      default: message = "SoundChannel_Stop(): waveOutReset(): Unknown error."; break;
    }
    DIAGNOSTIC_SOUND_ERROR(message);
    // TODO: how to robustly recover from this kind of error?
  }
#endif
}

void SoundChannel_Release(SoundChannel soundChannel)
{
  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Release(): null soundChannel arg provided");
    return;
  }

#ifdef LURDS2_USE_SOUND_MMEAPI
  if (!channel->handle)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Release(): invalid soundChannel->handle (likely a dangling pointer defect)");
    return;
  }
#endif
  
  SoundChannel_Unbind(channel);

#ifdef LURDS2_USE_SOUND_MMEAPI
  MMRESULT result;
  result = waveOutClose(channel->handle);
  if (result != MMSYSERR_NOERROR)
  {
    char* message;
    switch (result)
    {
      case MMSYSERR_INVALHANDLE: message = "SoundChannel_Release(): waveOutClose(): Specified device handle is invalid."; break;
      case MMSYSERR_NODRIVER: message = "SoundChannel_Release(): waveOutClose(): No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "SoundChannel_Release(): waveOutClose(): Unable to allocate or lock memory."; break;
      case WAVERR_STILLPLAYING: message = "SoundChannel_Release(): waveOutClose(): There are still buffers in the queue."; break;
      default: message = "SoundChannel_Release(): waveOutClose(): Unknown error."; break;
    }
    DIAGNOSTIC_SOUND_ERROR(message);
    // TODO: how to robustly recover from this kind of error?
  }
  channel->handle = 0;
#endif

  free(channel);
}

SoundBuffer SoundBuffer_LoadFromFileW(wchar_t * filePath)
{
  SoundBufferData * buffer;
  buffer = malloc(sizeof(SoundBufferData));
  if (buffer == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): failed to allocate memory SoundBufferData struct");
    return 0;
  }

  memset(buffer, 0, sizeof(SoundBufferData));

  HANDLE h;
  h = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h == INVALID_HANDLE_VALUE)
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): CreateFileW(): "));
    free(buffer);
    return 0;
  }
  
  DWORD size;
  DWORD sizeHigh;
  size = GetFileSize(h, &sizeHigh);
  if (INVALID_FILE_SIZE == size)
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): GetFileSize(): "));
    free(buffer);
    CloseHandle(h);
    return 0;
  }

  if (size > 100000000 || sizeHigh > 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): sound file too big");
    free(buffer);
    CloseHandle(h);
    return 0;
  }

  char * data;
  data = malloc(size);
  if (data == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): failed to allocate memory for sound from file");
    free(buffer);
    CloseHandle(h);
    return 0;
  }
  
  DWORD numBytesRead;
  if (!ReadFile(h, data, size, &numBytesRead, 0))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): ReadFile(): "));
    CloseHandle(h);
    free(buffer);
    free(data);
    return 0;
  }
  
  if (numBytesRead != size)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected numByteRead from sound file");
    CloseHandle(h);
    free(buffer);
    free(data);
    return 0;
  }

  buffer->data = data;
  buffer->length = size;
  buffer->refcount = 1;
  return buffer;
}

void SoundBuffer_Release(SoundBuffer soundBuffer)
{
  SoundBufferData* buffer;
  buffer = (SoundBufferData*)soundBuffer;

  if (!buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_Release(): soundBuffer arg is null");
    return;
  }
  
  if (buffer->refcount < 1)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_Release(): soundBuffer arg has bad refcount");
    return;
  }

  buffer->refcount--;
  if (buffer->refcount == 0)
  {
    if (buffer->data)
    {
      free(buffer->data);
      buffer->data = 0;
    }
    free(buffer);
  }
}