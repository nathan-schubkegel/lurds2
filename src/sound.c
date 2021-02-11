#include "sound.h"

#include "errors.h"

#define LURDS2_USE_SOUND_MMEAPI

#define DIAGNOSTIC_SOUND_ERROR(message) DIAGNOSTIC_ERROR(message);

typedef struct __attribute__((packed)) WaveFileHeader {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    unsigned int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    unsigned int fmt_chunk_size; // Should be 16 for PCM
    unsigned short audio_format; // Should be 1 for uncompressed wav (PCM). other values indicate compression.
    unsigned short num_channels; // Mono = 1, Stereo = 2, etc.
    unsigned int sample_rate;    // 8000, 44100, etc.
    unsigned int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    unsigned short block_align; // == NumChannels * BitsPerSample/8 
                                // The number of bytes for one sample including
                                // all channels. I wonder what happens when
                                // this number isn't an integer?
    unsigned short bits_per_sample; // 8 bits = 8, 16 bits = 16, etc.
    
    // Data
    char data_header[4]; // Contains "data"
    int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
} WaveFileHeader;

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

int SoundChannel_Bind(SoundChannelData* channel, SoundBufferData* buffer, int loop)
{
  // if 'buffer' is already bound, then no-op
  if (buffer && buffer == channel->buffer)
  {
    return 1;
  }

  SoundChannel_Unbind(channel);

#ifdef LURDS2_USE_SOUND_MMEAPI
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
#endif
  
  channel->buffer = buffer;
  buffer->refcount++;
  return 1;
}

void SoundChannel_Play(SoundChannel soundChannel, SoundBuffer soundBuffer, int loop)
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

#ifdef LURDS2_USE_SOUND_MMEAPI
  if (!channel->handle)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): invalid soundChannel->handle (likely a dangling pointer defect)");
    return;
  }
#endif

  if (!buffer->data)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundChannel_Play(): null soundBuffer->data (likely a dangling pointer defect)");
    return;
  }

  if (!SoundChannel_Bind(channel, soundBuffer, loop))
  {
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
    // forgive someone calling 'stop' before calling 'play'
    //DIAGNOSTIC_SOUND_ERROR("SoundChannel_Stop(): soundChannel not yet bound to SoundBuffer");
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
  HANDLE h;
  SoundBufferData * buffer;
  char * data;

  data = 0;
  buffer = 0;
  h = INVALID_HANDLE_VALUE;

  buffer = malloc(sizeof(SoundBufferData));
  if (buffer == 0)
  {
    DIAGNOSTIC_SOUND_ERROR("SoundBuffer_LoadFromFileW(): failed to allocate memory SoundBufferData struct");
    goto error;
  }

  memset(buffer, 0, sizeof(SoundBufferData));
  h = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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

  if (size > 2000000 || sizeHigh > 0)
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
  buffer->data = data;
  buffer->length = size;
  buffer->refcount = 1;
  return buffer;
  
error:
  if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
  if (buffer != 0) free(buffer);
  if (data != 0) free(data);
  return 0;
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