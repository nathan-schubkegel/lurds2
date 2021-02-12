#include "sound.h"

#include "errors.h"

#define LURDS2_USE_SOUND_MMEAPI

#define DIAGNOSTIC_SOUND_ERROR(message) DIAGNOSTIC_ERROR(message);

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
        DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundThreadProc(): GetMessage(): "));
        return;
    }
        
    switch (msg.message)
    {
      case WM_SOUNDCHANNEL_OPEN: SoundChannel_Open_Handler(*(SoundChannelData**)&msg.wParam); break;
      case WM_SOUNDCHANNEL_PLAY: 
        SoundChannel_Play_Handler(*(SoundChannelData**)&msg.wParam, *(SoundBufferData**)&msg.lParam, 0);
        break;
      case WM_SOUNDCHANNEL_PLAYLOOP: 
        SoundChannel_Play_Handler(*(SoundChannelData**)&msg.wParam, *(SoundBufferData**)&msg.lParam, 1);
        break;
      case WM_SOUNDCHANNEL_STOP: SoundChannel_Stop_Handler(*(SoundChannelData**)&msg.wParam); break;
      case WM_SOUNDCHANNEL_RELEASE: SoundChannel_Release_Handler(*(SoundChannelData**)&msg.wParam); break;
      case WM_SOUNDBUFFER_LOADFROMFILE: SoundBuffer_LoadFromFileW_Handler(*(SoundBufferData**)&msg.wParam, *(wchar_t**)&msg.lParam); break;
      case WM_SOUNDBUFFER_RELEASE: SoundBuffer_Release_Handler(*(SoundBufferData**)&msg.wParam); break;
    }
  }
}

void SoundThreadSetup()
{
  if (SoundThread == 0)
  {
    while (0 != InterlockedCompareExchange(&SoundThreadInitSpinLock, 1, 0))
    {
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
        DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundThreadSetup(): CreateThread(): "));
      }
      else
      {
        // wait for thread to establish message queue, to guarantee that no calls to PostThreadMessage() fail for that reason
        while (!established)
        {
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
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Open(): failed to allocate memory for SoundChannelData");
    return 0;
  }
  memset(channel, 0, sizeof(SoundChannelData));
  InterlockedIncrement(&channel->refcount); // 1 for caller holding the reference
  InterlockedIncrement(&channel->refcount); // 1 for SoundThread using it

  if (!PostThreadMessage(SoundThreadId, WM_SOUNDCHANNEL_OPEN, *(WPARAM*)&channel, 0))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundChannel_Open(): PostThreadMessage(): "));
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
      case MMSYSERR_ALLOCATED: message = "SoundChannel_Open(): waveOutOpen(): Specified resource is already allocated."; break;
      case MMSYSERR_BADDEVICEID: message = "SoundChannel_Open(): waveOutOpen(): Specified device identifier is out of range."; break;
      case MMSYSERR_NODRIVER: message = "SoundChannel_Open(): waveOutOpen(): No device driver is present."; break;
      case MMSYSERR_NOMEM: message = "SoundChannel_Open(): waveOutOpen(): Unable to allocate or lock memory."; break;
      case WAVERR_BADFORMAT: message = "SoundChannel_Open(): waveOutOpen(): Attempted to open with an unsupported waveform-audio format."; break;
      case WAVERR_SYNC: message = "SoundChannel_Open(): waveOutOpen(): The device is synchronous but waveOutOpen was called without using the WAVE_ALLOWSYNC flag."; break;
      default: message = "SoundChannel_Open(): waveOutOpen(): Unknown error."; break;
    }
    DIAGNOSTIC_SOUND_ERROR(message);
  }
  else
  {
    channel->handle = h;
  }
#endif
  SoundChannel_Release_Handler(channel);
  LeaveCriticalSection(&SoundThreadCriticalSection);
}

static void SoundChannel_Unbind_Handler(SoundChannelData* channel)
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
    }
#endif

    SoundBuffer_Release_Handler(channel->buffer);
    channel->buffer = 0;
  }
}

static long SoundChannel_Bind_Handler(SoundChannelData* channel, SoundBufferData* buffer, long loop)
{
  // if 'buffer' is already bound to this channel, then no-op
  if (buffer && buffer == channel->buffer)
  {
    return 1;
  }

  SoundChannel_Unbind_Handler(channel);

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
        case MMSYSERR_INVALHANDLE: message = "SoundChannel_Bind(): waveOutPrepareHeader(): Specified device handle is invalid."; break;
        case MMSYSERR_NODRIVER: message = "SoundChannel_Bind(): waveOutPrepareHeader(): No device driver is present."; break;
        case MMSYSERR_NOMEM: message = "SoundChannel_Bind(): waveOutPrepareHeader(): Unable to allocate or lock memory."; break;
        default: message = "SoundChannel_Bind(): waveOutPrepareHeader(): unknown error"; break;
      }
      DIAGNOSTIC_SOUND_ERROR(message);
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
  SoundChannelData* channel = (SoundChannelData*)soundChannel;
  SoundBufferData* buffer = (SoundBufferData*)soundBuffer;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): null soundChannel arg provided");
    return;
  }

  if (!buffer)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): null soundBuffer arg provided");
    return;
  }

  InterlockedIncrement(&channel->refcount);
  InterlockedIncrement(&buffer->refcount);
  if (!PostThreadMessage(SoundThreadId, loop ? WM_SOUNDCHANNEL_PLAYLOOP : WM_SOUNDCHANNEL_PLAY, *(WPARAM*)&channel, *(LPARAM*)&buffer))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundChannel_Stop(): PostThreadMessage(): "));
    SoundChannel_Play_Handler(channel, buffer, loop);
  }
}

void SoundChannel_Play_Handler(SoundChannelData* channel, SoundBufferData* buffer, long loop)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  if (!buffer->data)
  {
    // be graceful - can happen when SoundBuffer_LoadFromFileW_Handler() fails
  }
  else if (!SoundChannel_Bind_Handler(channel, buffer, loop))
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
  }
#endif

  SoundChannel_Release_Handler(channel);
  SoundBuffer_Release_Handler(buffer);
  LeaveCriticalSection(&SoundThreadCriticalSection);
}

void SoundChannel_Stop(SoundChannel soundChannel)
{
  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Stop(): null soundChannel arg provided");
    return;
  }

  InterlockedIncrement(&channel->refcount);
  if (!PostThreadMessage(SoundThreadId, WM_SOUNDCHANNEL_STOP, *(WPARAM*)&channel, 0))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundChannel_Stop(): PostThreadMessage(): "));
    SoundChannel_Stop_Handler(channel);
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
          case MMSYSERR_INVALHANDLE: message = "SoundChannel_Stop(): waveOutReset(): Specified device handle is invalid."; break;
          case MMSYSERR_NODRIVER: message = "SoundChannel_Stop(): waveOutReset(): No device driver is present."; break;
          case MMSYSERR_NOMEM: message = "SoundChannel_Stop(): waveOutReset(): Unable to allocate or lock memory."; break;
          case MMSYSERR_NOTSUPPORTED: message = "SoundChannel_Stop(): waveOutReset(): Specified device is synchronous and does not support pausing."; break;
          default: message = "SoundChannel_Stop(): waveOutReset(): Unknown error."; break;
        }
        DIAGNOSTIC_SOUND_ERROR(message);
        // TODO: how to robustly recover from this kind of error?
      }
    }
  #endif
  }
  
  SoundChannel_Release_Handler(channel);
  LeaveCriticalSection(&SoundThreadCriticalSection);
}

void SoundChannel_Release(SoundChannel soundChannel)
{
  SoundChannelData* channel = (SoundChannelData*)soundChannel;

  if (!channel)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Release(): null soundChannel arg provided");
    return;
  }
  
  if (!PostThreadMessage(SoundThreadId, WM_SOUNDCHANNEL_RELEASE, *(WPARAM*)&channel, 0))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundChannel_Release(): PostThreadMessage(): "));
    SoundChannel_Release_Handler(channel);
  }
}

void SoundChannel_Release_Handler(SoundChannelData* channel)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  long result = InterlockedDecrement(&channel->refcount);
  if (result < 0)
  {
    InterlockedIncrement(&channel->refcount);
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Release_Handler(): invalid refcount (bad pointer management)");
  }
  else if (result > 0)
  {
    // something else is still holding this alive
  }
  else
  {
    SoundChannel_Unbind_Handler(channel);

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
    }
#endif

    free(channel);
  }

  LeaveCriticalSection(&SoundThreadCriticalSection);
}

SoundBuffer SoundBuffer_LoadFromFileW(const wchar_t * filePath)
{
  if (filePath == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): null filePath arg");
    return 0;
  }
  
  // copy filePath
  long len;
  len = wcslen(filePath);
  wchar_t * filePathCopy;
  filePathCopy = malloc((len + 1) * sizeof(wchar_t));
  if (filePathCopy == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): failed to allocate memory for filePathCopy");
    return 0;
  }
  memcpy(filePathCopy, filePath, (len + 1) * sizeof(wchar_t));
  
  SoundBufferData * buffer;
  buffer = malloc(sizeof(SoundBufferData));
  if (buffer == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): failed to allocate memory SoundBufferData struct");
    free(filePathCopy);
    return 0;
  }
  memset(buffer, 0, sizeof(SoundBufferData));
  
  InterlockedIncrement(&buffer->refcount); // 1 for caller holding the reference
  InterlockedIncrement(&buffer->refcount); // 1 for SoundThread using it

  if (!PostThreadMessage(SoundThreadId, WM_SOUNDBUFFER_LOADFROMFILE, *(WPARAM*)&buffer, *(LPARAM*)&filePathCopy))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): PostThreadMessage(): "));
    SoundBuffer_LoadFromFileW_Handler(buffer, filePathCopy);
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
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): CreateFileW(): "));
    goto error;
  }

  DWORD size;
  DWORD sizeHigh;
  size = GetFileSize(h, &sizeHigh);
  if (INVALID_FILE_SIZE == size)
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): GetFileSize(): "));
    goto error;
  }

  // max 10 megs sound file supported
  if (size > 10000000 || sizeHigh > 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): sound file too big");
    goto error;
  }

  data = malloc(size);
  if (data == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): failed to allocate memory for sound from file");
    goto error;
  }

  DWORD numBytesRead;
  if (!ReadFile(h, data, size, &numBytesRead, 0))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_LoadFromFileW(): ReadFile(): "));
    goto error;
  }

  if (numBytesRead != size)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected numByteRead from sound file");
    goto error;
  }

  // inspect *.wav file header
  WaveFileHeader* header;
  header = (WaveFileHeader*)data;
  if (size < sizeof(WaveFileHeader)) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): sound file not large enough to hold wave file header");
    goto error;
  }

  if (memcmp(header->riff_header, "RIFF", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->riff_header in sound file");
    goto error;
  }

  if (header->wav_size > size - 8) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): too large header->wav_size in sound file");
    goto error;
  }
  
  if (memcmp(header->wave_header, "WAVE", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->wave_header in sound file");
    goto error;
  }
  
  if (memcmp(header->fmt_header, "fmt ", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->fmt_header in sound file");
    goto error;
  }
  
  if (header->fmt_chunk_size != 16) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->fmt_chunk_size in sound file");
    goto error;
  }

  if (header->audio_format != 1) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->audio_format in sound file");
    goto error;
  }

  if (header->num_channels > 2 || header->num_channels == 0) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->num_channels in sound file");
    goto error;
  }

  if (header->sample_rate < 11025 || header->sample_rate > 50000) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->sample_rate in sound file");
    goto error;
  }

  if (header->bits_per_sample % 8 != 0 || header->bits_per_sample > 32) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->bits_per_sample in sound file");
    goto error;
  }

  if (header->block_align != header->num_channels * (header->bits_per_sample / 8)) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->block_align in sound file");
    goto error;
  }

  if (header->byte_rate != header->sample_rate * header->num_channels * (header->bits_per_sample / 8)) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->byte_rate in sound file");
    goto error;
  }

  if (memcmp(header->data_header, "data", 4) != 0) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->data_header in sound file");
    goto error;
  }

  if (header->data_bytes > size - sizeof(WaveFileHeader)) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): unexpected header->data_bytes in sound file");
    goto error;
  }
  
  // now the disappointing part... right now the code is structured to assume lurds2 resource wav format
  if (header->num_channels != 1) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): expected lurds2 sounds to have header->num_channels == 1");
    goto error;
  }
  if (header->sample_rate != 11025) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): expected lurds2 sounds to have header->sample_rate == 11025");
    goto error;
  }
  if (header->bits_per_sample != 8) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): expected lurds2 sounds to have header->bits_per_sample == 8");
    goto error;
  }
  if (header->block_align != header->bits_per_sample * header->num_channels / 8) {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): expected lurds2 sounds to have header->block_align == 1");
    goto error;
  }

  // ding fries are done
  CloseHandle(h);
  free(filePathCopy);

  EnterCriticalSection(&SoundThreadCriticalSection);
  buffer->data = data;
  buffer->length = size;
  SoundBuffer_Release_Handler(buffer);
  LeaveCriticalSection(&SoundThreadCriticalSection);
  return;
  
error:
  if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
  if (data != 0) free(data);
  if (filePathCopy != 0) free(filePathCopy);
  SoundBuffer_Release_Handler(buffer);
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
  
  if (!PostThreadMessage(SoundThreadId, WM_SOUNDBUFFER_RELEASE, *(WPARAM*)&buffer, 0))
  {
    DIAGNOSTIC_SOUND_ERROR(GetLastErrorMessageWithPrefix("SoundBuffer_Release(): PostThreadMessage(): "));
    SoundBuffer_Release_Handler(buffer);
  }
}

void SoundBuffer_Release_Handler(SoundBufferData* buffer)
{
  EnterCriticalSection(&SoundThreadCriticalSection);

  long result = InterlockedDecrement(&buffer->refcount);
  if (result < 0)
  {
    InterlockedIncrement(&buffer->refcount);
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_Release_Handler(): invalid refcount (bad pointer management)");
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