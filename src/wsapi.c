#include "wsapi.h"

#define DIAGNOSTIC_WSAPI_ERROR(message) MessageBox(0, (message), "error in wsapi.c", 0);

#define LURDS2_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID name = { l,w1,w2,{ b1,b2,b3,b4,b5,b6,b7,b8 } }
LURDS2_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
LURDS2_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
LURDS2_GUID(IID_IMMDeviceCollection, 0x0BD7A1BE, 0x7A1A, 0x44DB, 0x83, 0x97, 0xCC, 0x53, 0x92, 0x38, 0x7B, 0x5E);
LURDS2_GUID(IID_IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48, 0xE8, 0x07, 0x36, 0x3F);
LURDS2_GUID(IID_IPropertyStore, 0x886d8eeb, 0x8cf2, 0x4446, 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99);
LURDS2_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);

IAudioClient* OpenAudioClient()
{
  HRESULT r;
  DWORD state;
  IMMDeviceEnumerator* enumerator;
  IMMDevice* device;
  IAudioClient* audioClient;
  IPropertyStore* properties;
  IMMEndpoint* endpoint;
  WAVEFORMATEXTENSIBLE* mixFormat;
  HANDLE hStreamEvent;
  
  hStreamEvent = 0;
  mixFormat = 0;
  endpoint = 0;
  properties = 0;
  audioClient = 0;
  device = 0;
  enumerator = 0;

  if (!SUCCEEDED(r = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, &enumerator)))
  {
    char* message;
    switch (result)
    {
      case REGDB_E_CLASSNOTREG: message = "CreateMMDeviceEnumerator(): CoCreateInstance(): A specified class is not registered in the registration database. Also can indicate that the type of server you requested in the CLSCTX enumeration is not registered or the values for the server types in the registry are corrupt."; break;
      case CLASS_E_NOAGGREGATION: message = "CreateMMDeviceEnumerator(): CoCreateInstance(): This class cannot be created as part of an aggregate."; break;
      case E_NOINTERFACE: message = "CreateMMDeviceEnumerator(): CoCreateInstance(): The specified class does not implement the requested interface, or the controlling IUnknown does not expose the requested interface."; break;
      case E_POINTER: message = "CreateMMDeviceEnumerator(): CoCreateInstance(): The ppv parameter is NULL."; break;
      default: message = "CreateMMDeviceEnumerator(): CoCreateInstance(): Unknown error."; break;
    }
    DIAGNOSTIC_WSAPI_ERROR(message);
    goto error;
  }

  if (!SUCCEEDED(r = enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device)))
  {
    char* message;
    switch (r)
    {
      case E_NOTFOUND: message = "CreateAndInitializeAudioClient(): IMMDeviceEnumerator.GetDefaultAudioEndpoint(): No device is available."; break;
      case E_OUTOFMEMORY: message = "CreateAndInitializeAudioClient(): IMMDeviceEnumerator.GetDefaultAudioEndpoint(): Out of memory."; break;
      default: message = "CreateAndInitializeAudioClient(): IMMDeviceEnumerator.GetDefaultAudioEndpoint(): unknown error."; break;
    }
    DIAGNOSTIC_WSAPI_ERROR(message);
    goto error;
  }
  
  if (!SUCCEEDED(r = device->lpVtbl->GetState(device, &state)))
  {
    DIAGNOSTIC_WSAPI_ERROR("CreateAndInitializeAudioClient(): IMMDevice.GetState(): failed");
    goto error;
  }

  if (state != DEVICE_STATE_ACTIVE)
  {
    DIAGNOSTIC_WSAPI_ERROR("CreateAndInitializeAudioClient(): default sound device state != active");
    goto error;
  }

  if (!SUCCEEDED(r = device->lpVtbl->Activate(device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, 0, &audioClient)))
  {
    char* message;
    switch (r)
    {
      case E_NOINTERFACE: message = "CreateAndInitializeAudioClient(): IMMDevice.Activate(IAudioClient): The object does not support the requested interface type."; break;
      case E_POINTER: message = "CreateAndInitializeAudioClient(): IMMDevice.Activate(IAudioClient): Parameter ppInterface is NULL."; break;
      case E_INVALIDARG: message = "CreateAndInitializeAudioClient(): IMMDevice.Activate(IAudioClient): The pActivationParams parameter must be NULL for the specified interface; or pActivationParams points to invalid data."; break;
      case E_OUTOFMEMORY: message = "CreateAndInitializeAudioClient(): IMMDevice.Activate(IAudioClient): Out of memory."; break;
      case AUDCLNT_E_DEVICE_INVALIDATED: message = "CreateAndInitializeAudioClient(): IMMDevice.Activate(IAudioClient): The user has removed either the audio endpoint device or the adapter device that the endpoint device connects to."; break;
      default: message = "CreateAndInitializeAudioClient(): IMMDevice.Activate(IAudioClient): unknown error."; break;
    }
    DIAGNOSTIC_WSAPI_ERROR(message);
    goto error;
  }
  
  if (!SUCCEEDED(r = audioClient->lpVtbl->GetMixFormat(audioClient, &mixFormat)))
  {
    char* message;
    switch (r)
    {
      case AUDCLNT_E_DEVICE_INVALIDATED: message = "CreateAndInitializeAudioClient(): IAudioClient.GetMixFormat(): The user has removed either the audio endpoint device or the adapter device that the endpoint device connects to."; break;
      case AUDCLNT_E_SERVICE_NOT_RUNNING: message = "CreateAndInitializeAudioClient(): IAudioClient.GetMixFormat(): The Windows audio service is not running."; break;
      case E_NOINTERFACE: message = "CreateAndInitializeAudioClient(): IAudioClient.GetMixFormat(): The object does not support the requested interface type."; break;
      case E_POINTER: message = "CreateAndInitializeAudioClient(): IAudioClient.GetMixFormat(): Parameter ppInterface is NULL."; break;
      case E_OUTOFMEMORY: message = "CreateAndInitializeAudioClient(): IAudioClient.GetMixFormat(): Out of memory."; break;
      default: message = "CreateAndInitializeAudioClient(): IAudioClient.GetMixFormat(): unknown error."; break;
    }
    DIAGNOSTIC_WSAPI_ERROR(message);
    goto error;
  }

//  WAVEFORMATEX waveFormat;
//  memset(&waveFormat, 0, sizeof(waveFormat));
//  waveFormat.cbSize = sizeof(waveFormat);
//
//  // fill it according to lurds2 resource sounds
//  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
//  waveFormat.nChannels = 1;
//  waveFormat.nSamplesPerSec = 11025;
//  waveFormat.wBitsPerSample = 8;
//  waveFormat.nBlockAlign = waveFormat.wBitsPerSample * waveFormat.nChannels / 8;

  GUID sessionId;
  CoCreateGuid(&sessionId);
  if (!SUCCEEDED(r = audioClient->lpVtbl->Initialize(audioClient, AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
    0, 0, mixFormat, &sessionId)))
  {
    char* message;
    switch (r)
    {
      case AUDCLNT_E_ALREADY_INITIALIZED: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The IAudioClient object is already initialized."; break;
      case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The requested buffer size is not aligned."; break;
      case AUDCLNT_E_BUFFER_SIZE_ERROR: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The buffer duration value is out of range; mode must not be greater than 5000 milliseconds for exclusive mode; for push mode the duration value must not be greater than 2 seconds."; break;
      case AUDCLNT_E_CPUUSAGE_EXCEEDED: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The process-pass duration exceeded the maximum CPU usage."; break;
      case AUDCLNT_E_DEVICE_INVALIDATED: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The audio endpoint device has been unplugged, reconfigured, disabled, removed, or otherwise made unavailable for use."; break;
      case AUDCLNT_E_DEVICE_IN_USE: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The endpoint device is already in use. Either the device is being used in exclusive mode, or the device is being used in shared mode and the caller asked to use the device in exclusive mode."; break;
      case AUDCLNT_E_ENDPOINT_CREATE_FAILED: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The method failed to create the audio endpoint for the render or the capture device. This can occur if the audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been reconfigured, disabled, removed, or otherwise made unavailable for use."; break;
      case AUDCLNT_E_INVALID_DEVICE_PERIOD: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): Indicates that the device period requested by an exclusive-mode client is greater than 5000 milliseconds."; break;
      case AUDCLNT_E_UNSUPPORTED_FORMAT: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The audio engine (shared mode) or audio endpoint device (exclusive mode) does not support the specified format."; break;
      case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The caller is requesting exclusive-mode use of the endpoint device, but the user has disabled exclusive-mode use of the device."; break;
      case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The AUDCLNT_STREAMFLAGS_EVENTCALLBACK flag is set but parameters hnsBufferDuration and hnsPeriodicity are not equal."; break;
      case AUDCLNT_E_SERVICE_NOT_RUNNING: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): The Windows audio service is not running."; break;
      case E_POINTER: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): Parameter pFormat is NULL."; break;
      case E_INVALIDARG: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): E_INVALIDARG. Maybe parameter pFormat points to an invalid format description; or the AUDCLNT_STREAMFLAGS_LOOPBACK flag is set but ShareMode is not equal to AUDCLNT_SHAREMODE_SHARED; or the AUDCLNT_STREAMFLAGS_CROSSPROCESS flag is set but ShareMode is equal to AUDCLNT_SHAREMODE_EXCLUSIVE; Or, a prior call to SetClientProperties was made with an invalid category for audio/render streams."; break;
      case E_OUTOFMEMORY: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): Out of memory."; break;
      default: message = "CreateAndInitializeAudioClient(): IAudioClient.Initialize(): unknown error."; break;
    }
    DIAGNOSTIC_WSAPI_ERROR(message);
    goto error;
  }

  hStreamEvent = CreateEventA(0, 0 /* 0 = auto reset */, 0 /* 0 = initially non-signalled */, 0);
  if (hStreamEvent == 0)
  {
    // TODO: hook up getlasterrormessage
    DIAGNOSTIC_WSAPI_ERROR("CreateAndInitializeAudioClient(): CreateEventA(): unknown error.");
    goto error;
  }

  if (!SUCCEEDED(r = audioClient->lpVtbl->SetEventHandle(audioClient, hStreamEvent)))
  {
    char* message;
    switch (r)
    {
      case E_INVALIDARG: message = "CreateAndInitializeAudioClient(): IAudioClient.SetEventHandle(): Parameter eventHandle is NULL or an invalid handle."; break;
      case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED: message = "CreateAndInitializeAudioClient(): IAudioClient.SetEventHandle(): The audio stream was not initialized for event-driven buffering."; break;
      case AUDCLNT_E_NOT_INITIALIZED: message = "CreateAndInitializeAudioClient(): IAudioClient.SetEventHandle(): The audio stream has not been successfully initialized."; break;
      case AUDCLNT_E_DEVICE_INVALIDATED: message = "CreateAndInitializeAudioClient(): IAudioClient.SetEventHandle(): The audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been reconfigured, disabled, removed, or otherwise made unavailable for use."; break;
      case AUDCLNT_E_SERVICE_NOT_RUNNING: message = "CreateAndInitializeAudioClient(): IAudioClient.SetEventHandle(): The Windows audio service is not running."; break;
      default: message = "CreateAndInitializeAudioClient(): IAudioClient.SetEventHandle(): unknown error."; break;
    }
    DIAGNOSTIC_WSAPI_ERROR(message);
    goto error;
  }
  
  // Get the actual size of the two allocated buffers.
  UINT32 bufferFrameCount;
  if (!SUCCEEDED(r = pAudioClient->GetBufferSize(&bufferFrameCount)))
  {
    DIAGNOSTIC_WSAPI_ERROR("GetBufferSize error");
    goto error;
  }

  if (!SUCCEEDED(r = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient)))
  {
    DIAGNOSTIC_WSAPI_ERROR("GetService(IID_IAudioRenderClient) error");
    goto error;
  }

  // To reduce latency, load the first buffer with data
  // from the audio source before starting the stream.
  hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
  EXIT_ON_ERROR(hr)

  hr = pMySource->LoadData(bufferFrameCount, pData, &flags);
  EXIT_ON_ERROR(hr)

  hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
  EXIT_ON_ERROR(hr)

  // Ask MMCSS to temporarily boost the thread priority
  // to reduce glitches while the low-latency stream plays.
  DWORD taskIndex = 0;
  hTask = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &taskIndex);
  if (hTask == NULL)
  {
      hr = E_FAIL;
      EXIT_ON_ERROR(hr)
  }

  hr = pAudioClient->Start();  // Start playing.
  EXIT_ON_ERROR(hr)

  // Each loop fills one of the two buffers.
  while (flags != AUDCLNT_BUFFERFLAGS_SILENT)
  {
      // Wait for next buffer event to be signaled.
      DWORD retval = WaitForSingleObject(hEvent, 2000);
      if (retval != WAIT_OBJECT_0)
      {
          // Event handle timed out after a 2-second wait.
          pAudioClient->Stop();
          hr = ERROR_TIMEOUT;
          goto Exit;
      }

      // Grab the next empty buffer from the audio device.
      hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
      EXIT_ON_ERROR(hr)

      // Load the buffer with data from the audio source.
      hr = pMySource->LoadData(bufferFrameCount, pData, &flags);
      EXIT_ON_ERROR(hr)

      hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
      EXIT_ON_ERROR(hr)
  }

  // Wait for the last buffer to play before stopping.
  Sleep((DWORD)(hnsRequestedDuration/REFTIMES_PER_MILLISEC));

  hr = pAudioClient->Stop();  // Stop playing.
  EXIT_ON_ERROR(hr)
  
  // TODO: run a thread that blocks on hStreamEvent and on wake, checks IAudioClient.GetCurrentPadding()
  // and where does it put the data? i don't know?


error:
#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }
  CoTaskMemFree(pwfx);
  // Clean up
  SAFE_RELEASE(audioClient);
  return hr;
}

#ifdef CRAZY_GO_NUTS
// these defines are needed to survive the COM MIDL generator's amazingly bad output
/*
#define _COM_Outptr_
#define _In_
#define _In_opt_
#define _Out_
#define _Out_opt_
#define _Outptr_result_buffer_(X)
#define _In_reads_(X)
#define _Out_writes_(X)
#define __field_ecount(X)
#define __RPC__in
#define __RPC__in_ecount_full(X)
#define __RPC__out
#define _Outptr_opt_result_maybenull_
#define __RPC__deref_out_opt
#define __RPC__in_opt
#define __RPC__inout_xcount(X)
#define __RPC__in_xcount(X)
#define __RPC__in_string
#define __RPC_unique_pointer
#define _COM_Outptr_result_maybenull_
#define DEFINE_ENUM_FLAG_OPERATORS(X)
#define __RPC__deref_out_opt_string
#define _Inout_
#define __RPC__out_ecount_full_string(X)
#define __RPC__in_range(X,Y)
#define _In_reads_bytes_(X)
#define _Outptr_result_bytebuffer_(X)
#define _Out_writes_bytes_to_(X,Y)
#define _Outptr_
#define _In_reads_opt_(X)
#define _Outptr_result_maybenull_
*/

#define WINAPI_PARTITION_GAMES 1
#undef WINAPI_FAMILY_PARTITION
#define FACILITY_AUDCLNT                 2185

#include <shobjidl.h>

typedef const PROPVARIANT *REFPROPVARIANT;

//#include <WTypes.h>



//#include "../portaudio/src/hostapi/wasapi/pa_win_wasapi.c"
#include "pa_win_wasapi.c"

#endif
#ifdef CRAZY_GO_NUTS

#include <Objbase.h>

#define DIAGNOSTIC_WSAPI_ERROR(message) MessageBox(0, (message), "error in wsapi.c", 0);

#define LURDS2_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID name = { l,w1,w2,{ b1,b2,b3,b4,b5,b6,b7,b8 } }
LURDS2_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
LURDS2_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
LURDS2_GUID(IID_IMMDeviceCollection, 0x0BD7A1BE, 0x7A1A, 0x44DB, 0x83, 0x97, 0xCC, 0x53, 0x92, 0x38, 0x7B, 0x5E);
LURDS2_GUID(IID_IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48, 0xE8, 0x07, 0x36, 0x3F);
LURDS2_GUID(IID_IPropertyStore, 0x886d8eeb, 0x8cf2, 0x4446, 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99);
LURDS2_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);


/*
 These are fields that can be gathered from IDevice and IAudioDevice PRIOR to Initialize, and
 done in first pass i assume that neither of these will cause the Driver to "load", but again,
 who knows how they implement their stuff
 */
typedef struct PaWasapiDeviceInfo
{
    // Device
#ifndef PA_WINRT
    IMMDevice *device;
#endif

    // device Id
    WCHAR deviceId[PA_WASAPI_DEVICE_ID_LEN];

    // from GetState
    DWORD state;

    // Fields filled from IAudioDevice (_prior_ to Initialize)
    // from GetDevicePeriod(
    REFERENCE_TIME DefaultDevicePeriod;
    REFERENCE_TIME MinimumDevicePeriod;

    // Default format (setup through Control Panel by user)
    WAVEFORMATEXTENSIBLE DefaultFormat;

    // Mix format (internal format used by WASAPI audio engine)
    WAVEFORMATEXTENSIBLE MixFormat;

    // Fields filled from IMMEndpoint'sGetDataFlow
    EDataFlow flow;

    // Form-factor
    EndpointFormFactor formFactor;
}
PaWasapiDeviceInfo;




typedef struct PaWasapiSubStream
{
    IAudioClient        *clientParent;
#ifndef PA_WINRT
    IStream             *clientStream;
#endif
    IAudioClient        *clientProc;

    WAVEFORMATEXTENSIBLE wavex;
    UINT32               bufferSize;
    REFERENCE_TIME       deviceLatency;
    REFERENCE_TIME       period;
    double               latencySeconds;
    UINT32               framesPerHostCallback;
    AUDCLNT_SHAREMODE    shareMode;
    UINT32               streamFlags; // AUDCLNT_STREAMFLAGS_EVENTCALLBACK, ...
    UINT32               flags;
    PaWasapiAudioClientParams params; //!< parameters

    // Buffers
    UINT32               buffers;            //!< number of buffers used (from host side)
    UINT32               framesPerBuffer;    //!< number of frames per 1 buffer
    BOOL                 userBufferAndHostMatch;

    // Used for Mono >> Stereo workaround, if driver does not support it
    // (in Exclusive mode WASAPI usually refuses to operate with Mono (1-ch)
    void                *monoBuffer;     //!< pointer to buffer
    UINT32               monoBufferSize; //!< buffer size in bytes
    MixMonoToStereoF     monoMixer;         //!< pointer to mixer function

    PaUtilRingBuffer    *tailBuffer;       //!< buffer with trailing sample for blocking mode operations (only for Input)
    void                *tailBufferMemory; //!< tail buffer memory region
}
PaWasapiSubStream;

// ------------------------------------------------------------------------------------------


#endif