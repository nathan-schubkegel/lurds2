/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_sound.h"

#include "lurds2_errors.h"

#define LURDS2_USE_SOUND_MMEAPI

#define DIAGNOSTIC_SOUND_ERROR(message) DIAGNOSTIC_ERROR(message);
#define DIAGNOSTIC_SOUND_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2));
#define DIAGNOSTIC_SOUND_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3));
#define DIAGNOSTIC_SOUND_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4));

typedef struct __attribute__((packed)) WaveFileHeader {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    unsigned long wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    unsigned long fmt_chunk_size; // Should be 16 for PCM
    unsigned short audio_format; // Should be 1 for uncompressed wav (PCM). other values indicate compression.
    unsigned short num_channels; // Mono = 1, Stereo = 2, etc.
    unsigned long sample_rate;    // 8000, 44100, etc.
    unsigned long byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    unsigned short block_align; // == NumChannels * BitsPerSample/8 
                                // The number of bytes for one sample including
                                // all channels. I wonder what happens when
                                // this number isn't an integer?
    unsigned short bits_per_sample; // 8 bits = 8, 16 bits = 16, etc.
    
    // Data
    char data_header[4]; // Contains "data"
    long data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
} WaveFileHeader;

typedef struct SoundBufferData {
  volatile long refcount;
  char* data;
  long length;
} SoundBufferData;

typedef struct SoundChannelData {
  volatile long refcount;
#ifdef LURDS2_USE_SOUND_MMEAPI
  HWAVEOUT handle;
  WAVEHDR header; // data associated with currently-playing sound
#endif
  SoundBufferData* buffer;
} SoundChannelData;

void SoundChannel_Open_Handler(SoundChannelData* channel);
void SoundChannel_Play_Handler(SoundChannelData* channel, SoundBufferData* buffer, long loop);
void SoundChannel_Stop_Handler(SoundChannelData* channel);
void SoundChannel_Release_Handler(SoundChannelData* channel);
void SoundBuffer_LoadFromFileW_Handler(SoundBufferData * buffer, wchar_t * filePath);
void SoundBuffer_Release_Handler(SoundBufferData* buffer);


// the threadHandle that's running to do the work
static volatile HANDLE SoundThread;
static volatile DWORD SoundThreadId;
static volatile long SoundThreadInitSpinLock;
static CRITICAL_SECTION SoundThreadCriticalSection;
static volatile long SoundThreadCriticalSectionInitialized;

#define WM_SOUNDCHANNEL_OPEN (WM_USER + 1)
#define WM_SOUNDCHANNEL_PLAY (WM_USER + 2)
#define WM_SOUNDCHANNEL_PLAYLOOP (WM_USER + 3)
#define WM_SOUNDCHANNEL_STOP (WM_USER + 4)
#define WM_SOUNDCHANNEL_RELEASE (WM_USER + 5)
#define WM_SOUNDBUFFER_LOADFROMFILE (WM_USER + 6)
#define WM_SOUNDBUFFER_RELEASE (WM_USER + 7)

static DWORD WINAPI SoundThreadProc(LPVOID lpParameter)
{
  // establish the thread's message queue by calling one of the message queue methods
  MSG msg;
  PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
  
  // record that SoundThreadProc is now established
  volatile long* established;
  established = (long*)lpParameter;
  *established = 1;

  // process messages forever
  while (1)
  {
    BOOL result = GetMessage(&msg, 0, 0, 0);
    switch (result)
    {
      case 0: return; // application requested to terminate
      case -1: 
        DIAGNOSTIC_SOUND_ERROR2("GetMessage: ", GetLastErrorMessage());
        return;
    }
        
    switch (msg.message)
    {
      case WM_SOUNDCHANNEL_OPEN:
        SoundChannel_Open_Handler(*(SoundChannelData**)&msg.wParam);
        SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam);
        break;

      case WM_SOUNDCHANNEL_PLAY:
        SoundChannel_Play_Handler(*(SoundChannelData**)&msg.wParam, *(SoundBufferData**)&msg.lParam, 0);
        SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam);
        SoundBuffer_Release_Handler(*(SoundBufferData**)&msg.lParam);
        break;
        
      case WM_SOUNDCHANNEL_PLAYLOOP:
        SoundChannel_Play_Handler(*(SoundChannelData**)&msg.wParam, *(SoundBufferData**)&msg.lParam, 1);
        SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam);
        SoundBuffer_Release_Handler(*(SoundBufferData**)&msg.lParam);
        break;
        
      case WM_SOUNDCHANNEL_STOP:
        SoundChannel_Stop_Handler(*(SoundChannelData**)&msg.wParam);
        SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam);
        break;
        
      case WM_SOUNDCHANNEL_RELEASE:
        SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam); // once to do what the user asked
        SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam); // once for the refcount incremented for PostThreadMessage
        break;
        
      case WM_SOUNDBUFFER_LOADFROMFILE:
        SoundBuffer_LoadFromFileW_Handler(*(SoundBufferData**)&msg.wParam, *(wchar_t**)&msg.lParam);
        SoundBuffer_Release_Handler(*(SoundBufferData**)&msg.wParam);
        break;
        
      case WM_SOUNDBUFFER_RELEASE:
        SoundBuffer_Release_Handler(*(SoundBufferData**)&msg.wParam); // once to do what the user asked
        SoundBuffer_Release_Handler(*(SoundBufferData**)&msg.wParam); // once for the refcount incremented for PostThreadMessage
        break;
    }
  }
}

void SoundThreadSetup()
{
  if (SoundThread == 0)
  {
    while (0 != InterlockedCompareExchange(&SoundThreadInitSpinLock, 1, 0))
    {
      SwitchToThread();
    }

    if (!SoundThreadCriticalSectionInitialized)
    {
      InitializeCriticalSection(&SoundThreadCriticalSection);
      SoundThreadCriticalSectionInitialized = 1;
    }

    if (SoundThread == 0)
    {
      HWND handle;
      volatile long established;
      DWORD id;
      established = 0;
      if (0 == (handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SoundThreadProc, (void*)&established, 0, &id)))
      {
        DIAGNOSTIC_SOUND_ERROR2("CreateThread: ", GetLastErrorMessage());
      }
      else
      {
        // wait for thread to establish message queue, to guarantee that no calls to PostThreadMessage() fail for that reason
        while (!established)
        {
          SwitchToThread();
        }
        SoundThread = handle;
        SoundThreadId = id;
      }
    }
    InterlockedDecrement(&SoundThreadInitSpinLock);    
  }
}

SoundChannel SoundChannel_Open()
{
  SoundThreadSetup();

  SoundChannelData* channel;
  channel = malloc(sizeof(SoundChannelData));
  if (channel == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("failed to allocate memory for SoundChannelData");
    return 0;
  }
  memset(channel, 0, sizeof(SoundChannelData));
  InterlockedIncrement(&channel->refcount); // 1 for caller holding the reference
  InterlockedIncrement(&channel->refcount); // 1 for SoundThread using it, since we're calling PostThreadMessage next

  if (!PostThreadMessage(SoundThreadId, WM_SOUNDCHANNEL_OPEN, *(WPARAM*)&channel, 0))
  {
    DIAGNOSTIC_SOUND_ERROR2("PostThreadMessage: ", GetLastErrorMessage());
    free(channel);
    return 0;
  }
  return channel;
}

void SoundChannel_Open_Handler(SoundChannelData* channel)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

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
      case MMSYSERR_ALLOCATED: message = "Specified resource is already allocated."; break;
      case MMSYSERR_BADDEVICEID: message = "Specified device identifier is out of range."; break;
      case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
      case WAVERR_BADFORMAT: message = "Attempted to open with an unsupported waveform-audio format."; break;
      case WAVERR_SYNC: message = "The device is synchronous but waveOutOpen was called without using the WAVE_ALLOWSYNC flag."; break;
      default: message = "Unknown error."; break;
    }
    DIAGNOSTIC_SOUND_ERROR2("waveOutOpen: ", message);
  }
  else
  {
    channel->handle = h;
  }
#endif
  LeaveCriticalSection(&SoundThreadCriticalSection);
}

static void SoundChannel_UnbindFromSoundBuffer(SoundChannelData* channel)
{
  // NOTE: it's important this method ignore channel->refcount
  // (because sometimes it'll be 0 while this method runs, and that's valid)
  
  if (channel->buffer)
  {
#ifdef LURDS2_USE_SOUND_MMEAPI
    // be graceful - it's possible that SoundChannel_Open_Handler couldn't open the handle
    if (channel->handle)
    {
      // in case a sound is currently playing, gotta stop it first
      MMRESULT result;
      result = waveOutReset(channel->handle);
      if (result != MMSYSERR_NOERROR)
      {
        char* message;
        switch (result)
        {
          case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
          case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
          case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
          case MMSYSERR_NOTSUPPORTED: message = "Specified device is synchronous and does not support pausing."; break;
          default: message = "Unknown error."; break;
        }
        DIAGNOSTIC_SOUND_ERROR2("waveOutReset: ", message);
        // TODO: how to robustly recover from this kind of error?
      }

      result = waveOutUnprepareHeader(channel->handle, &channel->header, sizeof(channel->header));
      if (result != MMSYSERR_NOERROR)
      {
        char* message;
        switch (result)
        {
          case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
          case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
          case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
          case WAVERR_STILLPLAYING: message = "The data block pointed to by the pwh parameter is still in the queue."; break;
          default: message = "Unknown error."; break;
        }
        DIAGNOSTIC_SOUND_ERROR2("waveOutUnprepareHeader: ", message);
        // TODO: how to robustly recover from this kind of error?
      }
    }
#endif

    SoundBuffer_Release_Handler(channel->buffer);
    channel->buffer = 0;
  }
}

static long SoundChannel_BindToSoundBuffer(SoundChannelData* channel, SoundBufferData* buffer, long loop)
{
  // if 'buffer' is already bound to this channel, then no-op
  if (buffer && buffer == channel->buffer)
  {
    return 1;
  }

  SoundChannel_UnbindFromSoundBuffer(channel);

#ifdef LURDS2_USE_SOUND_MMEAPI
  // be graceful - it's possible that SoundChannel_Open_Handler couldn't open the handle
  // be graceful - it's possible that SoundBuffer_LoadFromFileW_Handler couldn't load the file data
  if (channel->handle && buffer->data)
  {
    // inspect *.wav file header
    WaveFileHeader* fileHeader;
    fileHeader = (WaveFileHeader*)buffer->data;

    LPWAVEHDR header;
    header = &channel->header;
    memset(header, 0, sizeof(*header));
    header->lpData = buffer->data + sizeof(WaveFileHeader);
    header->dwBufferLength = fileHeader->data_bytes;
    header->dwFlags = 0 | (loop ? (WHDR_BEGINLOOP | WHDR_ENDLOOP) : 0);
    header->dwLoops = loop ? 0xffffffff : 0;

    MMRESULT result;
    result = waveOutPrepareHeader(channel->handle, header, sizeof(*header));
    if (MMSYSERR_NOERROR != result)
    {
      char * message;
      switch (result)
      {
        case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
        case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
        case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
        default: message = "unknown error"; break;
      }
      DIAGNOSTIC_SOUND_ERROR2("waveOutPrepareHeader: ", message);
      return 0;
    }
  }
#endif
  
  channel->buffer = buffer;
  InterlockedIncrement(&buffer->refcount);
  return 1;
}

void SoundChannel_Play(SoundChannel soundChannel, SoundBuffer soundBuffer, long loop)
{
  SoundThreadSetup();

  SoundChannelData* channel = (SoundChannelData*)soundChannel;
  SoundBufferData* buffer = (SoundBufferData*)soundBuffer;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundChannel arg provided");
    return;
  }

  if (!buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundBuffer arg provided");
    return;
  }

  if (1 >= InterlockedIncrement(&channel->refcount))
  {
    InterlockedDecrement(&channel->refcount);
    DIAGNOSTIC_SOUND_ERROR("soundChannel arg has bad refcount - won't use");
    return;
  }
  
  if (1 >= InterlockedIncrement(&buffer->refcount))
  {
    InterlockedDecrement(&buffer->refcount);
    DIAGNOSTIC_SOUND_ERROR("soundBuffer arg has bad refcount - won't use");
    SoundChannel_Release_Handler(channel);
    return;
  }

  if (!PostThreadMessage(SoundThreadId, loop ? WM_SOUNDCHANNEL_PLAYLOOP : WM_SOUNDCHANNEL_PLAY, *(WPARAM*)&channel, *(LPARAM*)&buffer))
  {
    // better to just not play than to be out of order vs other operations
    DIAGNOSTIC_SOUND_ERROR2("PostThreadMessage: ", GetLastErrorMessage());
    
    // decrement the refcounts incremented for PostThreadMessage
    SoundChannel_Release_Handler(channel);
    SoundBuffer_Release_Handler(buffer);
  }
}

void SoundChannel_Play_Handler(SoundChannelData* channel, SoundBufferData* buffer, long loop)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  if (!buffer->data)
  {
    // be graceful - can happen when SoundBuffer_LoadFromFileW_Handler() fails
  }
  else if (!SoundChannel_BindToSoundBuffer(channel, buffer, loop))
  {
    // it speaks for itself
  }
  // be graceful - it's possible that SoundChannel_Open_Handler couldn't open the handle
  else if (channel->handle)
  {
#ifdef LURDS2_USE_SOUND_MMEAPI
    // in case a sound is currently playing, stop it first
    MMRESULT result;
    result = waveOutReset(channel->handle);
    if (result != MMSYSERR_NOERROR)
    {
      char* message;
      switch (result)
      {
        case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
        case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
        case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
        case MMSYSERR_NOTSUPPORTED: message = "Specified device is synchronous and does not support pausing."; break;
        default: message = "Unknown error."; break;
      }
      DIAGNOSTIC_SOUND_ERROR2("waveOutReset: ", message);
      // TODO: how to robustly recover from this kind of error?
    }

    result = waveOutWrite(channel->handle, &channel->header, sizeof(channel->header));
    if (result != MMSYSERR_NOERROR)
    {
      char* message;
      switch (result)
      {
        case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
        case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
        case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
        case WAVERR_UNPREPARED: message = "The data block pointed to by the pwh parameter hasn't been prepared."; break;
        default: message = "unknown error";
      }
      DIAGNOSTIC_SOUND_ERROR2("waveOutWrite: ", message);
    }
  }
#endif

  LeaveCriticalSection(&SoundThreadCriticalSection);
}

void SoundChannel_Stop(SoundChannel soundChannel)
{
  SoundThreadSetup();

  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundChannel arg provided");
    return;
  }

  if (1 >= InterlockedIncrement(&channel->refcount))
  {
    InterlockedDecrement(&channel->refcount);
    DIAGNOSTIC_SOUND_ERROR("soundChannel arg has bad refcount - won't use");
    return;
  }
  
  if (!PostThreadMessage(SoundThreadId, WM_SOUNDCHANNEL_STOP, *(WPARAM*)&channel, 0))
  {
    // better to just not stop the sound than to perform this operation out of order vs. others
    DIAGNOSTIC_SOUND_ERROR2("PostThreadMessage: ", GetLastErrorMessage());
    
    // decrement the refcount incremented for PostThreadMessage
    SoundChannel_Release_Handler(channel);
  }
}

void SoundChannel_Stop_Handler(SoundChannelData* channel)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  // forgive someone calling 'stop' before calling 'play'
  if (channel->buffer)
  {
  #ifdef LURDS2_USE_SOUND_MMEAPI
    // be graceful - it's possible that SoundChannel_Open_Handler couldn't open the handle
    if (channel->handle)
    {
      MMRESULT result;
      result = waveOutReset(channel->handle);
      if (result != MMSYSERR_NOERROR)
      {
        char* message;
        switch (result)
        {
          case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
          case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
          case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
          case MMSYSERR_NOTSUPPORTED: message = "Specified device is synchronous and does not support pausing."; break;
          default: message = "Unknown error."; break;
        }
        DIAGNOSTIC_SOUND_ERROR2("waveOutReset: ", message);
        // TODO: how to robustly recover from this kind of error?
      }
    }
  #endif
  }
  
  LeaveCriticalSection(&SoundThreadCriticalSection);
}

void SoundChannel_Release(SoundChannel soundChannel)
{
  SoundThreadSetup();

  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("null soundChannel arg provided");
    return;
  }
  
  if (1 >= InterlockedIncrement(&channel->refcount))
  {
    InterlockedDecrement(&channel->refcount);
    DIAGNOSTIC_SOUND_ERROR("soundChannel arg has bad refcount - won't use");
    return;
  }
  
  if (!PostThreadMessage(SoundThreadId, WM_SOUNDCHANNEL_RELEASE, *(WPARAM*)&channel, 0))
  {
    DIAGNOSTIC_SOUND_ERROR2("PostThreadMessage: ", GetLastErrorMessage());

    // decrement the refcounts
    SoundChannel_Release_Handler(channel); // once for the increment done for PostThreadMessage
    SoundChannel_Release_Handler(channel); // once to do what the user asked
  }
}

void SoundChannel_Release_Handler(SoundChannelData* channel)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  long result = InterlockedDecrement(&channel->refcount);
  if (result < 0)
  {
    InterlockedIncrement(&channel->refcount);
    DIAGNOSTIC_SOUND_ERROR("invalid refcount (bad pointer management)");
  }
  else if (result > 0)
  {
    // something else is still holding this alive
  }
  else
  {
    SoundChannel_UnbindFromSoundBuffer(channel);

#ifdef LURDS2_USE_SOUND_MMEAPI
    // be graceful - it's possible that SoundChannel_Open_Handler couldn't open the handle
    if (channel->handle)
    {
      MMRESULT result;
      result = waveOutClose(channel->handle);
      if (result != MMSYSERR_NOERROR)
      {
        char* message;
        switch (result)
        {
          case MMSYSERR_INVALHANDLE: message = "Specified device handle is invalid."; break;
          case MMSYSERR_NODRIVER: message = "No device driver is present."; break;
          case MMSYSERR_NOMEM: message = "Unable to allocate or lock memory."; break;
          case WAVERR_STILLPLAYING: message = "There are still buffers in the queue."; break;
          default: message = "Unknown error."; break;
        }
        DIAGNOSTIC_SOUND_ERROR2("waveOutClose: ", message);
        // TODO: how to robustly recover from this kind of error?
      }
      channel->handle = 0;
    }
#endif

    free(channel);
  }

  LeaveCriticalSection(&SoundThreadCriticalSection);
}

SoundBuffer SoundBuffer_LoadFromFileW(const wchar_t * filePath)
{
  SoundThreadSetup();

  if (filePath == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("null filePath arg");
    return 0;
  }
  
  // copy filePath
  long len;
  len = wcslen(filePath);
  wchar_t * filePathCopy;
  filePathCopy = malloc((len + 1) * sizeof(wchar_t));
  if (filePathCopy == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("failed to allocate memory for filePathCopy");
    return 0;
  }
  memcpy(filePathCopy, filePath, (len + 1) * sizeof(wchar_t));
  
  SoundBufferData * buffer;
  buffer = malloc(sizeof(SoundBufferData));
  if (buffer == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("failed to allocate memory SoundBufferData struct");
    free(filePathCopy);
    return 0;
  }
  memset(buffer, 0, sizeof(SoundBufferData));
  
  InterlockedIncrement(&buffer->refcount); // 1 for caller holding the reference
  InterlockedIncrement(&buffer->refcount); // 1 for SoundThread using it, since we're calling PostThreadMessage next

  if (!PostThreadMessage(SoundThreadId, WM_SOUNDBUFFER_LOADFROMFILE, *(WPARAM*)&buffer, *(LPARAM*)&filePathCopy))
  {
    DIAGNOSTIC_SOUND_ERROR2("PostThreadMessage: ", GetLastErrorMessage());

    // decrement the refcount incremented for PostThreadMessage
    SoundBuffer_Release_Handler(buffer);
    free(filePathCopy);
  }
  return buffer;
}

void SoundBuffer_LoadFromFileW_Handler(SoundBufferData * buffer, wchar_t * filePathCopy)
{
  HANDLE h;
  char * data;

  data = 0;
  h = INVALID_HANDLE_VALUE;
  
  h = CreateFileW(filePathCopy, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h == INVALID_HANDLE_VALUE)
  {
    DIAGNOSTIC_SOUND_ERROR2("CreateFileW: ", GetLastErrorMessage());
    goto error;
  }

  DWORD size;
  DWORD sizeHigh;
  size = GetFileSize(h, &sizeHigh);
  if (INVALID_FILE_SIZE == size)
  {
    DIAGNOSTIC_SOUND_ERROR2("GetFileSize: ", GetLastErrorMessage());
    goto error;
  }

  // max 10 megs sound file supported
  if (size > 10000000 || sizeHigh > 0)
  {
    DIAGNOSTIC_SOUND_ERROR("sound file too big");
    goto error;
  }

  data = malloc(size);
  if (data == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("failed to allocate memory for sound from file");
    goto error;
  }

  DWORD numBytesRead;
  if (!ReadFile(h, data, size, &numBytesRead, 0))
  {
    DIAGNOSTIC_SOUND_ERROR2("ReadFile(): ", GetLastErrorMessage());
    goto error;
  }

  if (numBytesRead != size)
  {
    DIAGNOSTIC_SOUND_ERROR("unexpected numByteRead from sound file");
    goto error;
  }

  // inspect *.wav file header
  WaveFileHeader* header;
  header = (WaveFileHeader*)data;
  if (size < sizeof(WaveFileHeader)) {
    DIAGNOSTIC_SOUND_ERROR("sound file not large enough to hold wave file header");
    goto error;
  }

  if (memcmp(header->riff_header, "RIFF", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->riff_header in sound file");
    goto error;
  }

  if (header->wav_size > size - 8) {
    DIAGNOSTIC_SOUND_ERROR("too large header->wav_size in sound file");
    goto error;
  }
  
  if (memcmp(header->wave_header, "WAVE", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->wave_header in sound file");
    goto error;
  }
  
  if (memcmp(header->fmt_header, "fmt ", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->fmt_header in sound file");
    goto error;
  }
  
  if (header->fmt_chunk_size != 16) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->fmt_chunk_size in sound file");
    goto error;
  }

  if (header->audio_format != 1) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->audio_format in sound file");
    goto error;
  }

  if (header->num_channels > 2 || header->num_channels == 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->num_channels in sound file");
    goto error;
  }

  if (header->sample_rate < 11025 || header->sample_rate > 50000) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->sample_rate in sound file");
    goto error;
  }

  if (header->bits_per_sample % 8 != 0 || header->bits_per_sample > 32) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->bits_per_sample in sound file");
    goto error;
  }

  if (header->block_align != header->num_channels * (header->bits_per_sample / 8)) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->block_align in sound file");
    goto error;
  }

  if (header->byte_rate != header->sample_rate * header->num_channels * (header->bits_per_sample / 8)) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->byte_rate in sound file");
    goto error;
  }

  if (memcmp(header->data_header, "data", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->data_header in sound file");
    goto error;
  }

  if (header->data_bytes > size - sizeof(WaveFileHeader)) {
    DIAGNOSTIC_SOUND_ERROR("unexpected header->data_bytes in sound file");
    goto error;
  }
  
  // now the disappointing part... right now the code is structured to assume lurds2 resource wav format
  if (header->num_channels != 1) {
    DIAGNOSTIC_SOUND_ERROR("expected lurds2 sounds to have header->num_channels == 1");
    goto error;
  }
  if (header->sample_rate != 11025) {
    DIAGNOSTIC_SOUND_ERROR("expected lurds2 sounds to have header->sample_rate == 11025");
    goto error;
  }
  if (header->bits_per_sample != 8) {
    DIAGNOSTIC_SOUND_ERROR("expected lurds2 sounds to have header->bits_per_sample == 8");
    goto error;
  }
  if (header->block_align != header->bits_per_sample * header->num_channels / 8) {
    DIAGNOSTIC_SOUND_ERROR("expected lurds2 sounds to have header->block_align == 1");
    goto error;
  }

  // ding fries are done
  CloseHandle(h);
  free(filePathCopy);

  EnterCriticalSection(&SoundThreadCriticalSection);
  buffer->data = data;
  buffer->length = size;
  LeaveCriticalSection(&SoundThreadCriticalSection);
  return;
  
error:
  if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
  if (data != 0) free(data);
  if (filePathCopy != 0) free(filePathCopy);
}

void SoundBuffer_Release(SoundBuffer soundBuffer)
{
  SoundThreadSetup();

  SoundBufferData* buffer;
  buffer = (SoundBufferData*)soundBuffer;

  if (!buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("soundBuffer arg is null");
    return;
  }
  
  if (1 >= InterlockedIncrement(&buffer->refcount))
  {
    InterlockedDecrement(&buffer->refcount);
    DIAGNOSTIC_SOUND_ERROR("soundBuffer arg has bad refcount - won't use");
    return;
  }
  
  if (!PostThreadMessage(SoundThreadId, WM_SOUNDBUFFER_RELEASE, *(WPARAM*)&buffer, 0))
  {
    DIAGNOSTIC_SOUND_ERROR2("PostThreadMessage: ", GetLastErrorMessage());
    SoundBuffer_Release_Handler(buffer); // once for the increment done for PostThreadMessage
    SoundBuffer_Release_Handler(buffer); // once to do what the user asked
  }
}

void SoundBuffer_Release_Handler(SoundBufferData* buffer)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  long result = InterlockedDecrement(&buffer->refcount);
  if (result < 0)
  {
    InterlockedIncrement(&buffer->refcount);
    DIAGNOSTIC_SOUND_ERROR("invalid refcount (bad pointer management)");
  }
  else if (result > 0)
  {
    // something else is still holding this alive
  }
  else
  {
    if (buffer->data)
    {
      free(buffer->data);
      buffer->data = 0;
    }
    free(buffer);
  }

  LeaveCriticalSection(&SoundThreadCriticalSection);
}