#ifndef LURDS2_WSAPI
#define LURDS2_WSAPI

#define WSAPI_API_IMPORT extern __stdcall __declspec(dllimport)

#include <Windows.h>
#include <Guiddef.h>
#include <Propidl.h>

typedef enum EDataFlow {
  eRender = 0,
  eCapture = ( eRender + 1 ),
  eAll = ( eCapture + 1 ),
  EDataFlow_enum_count = ( eAll + 1 ) 
} EDataFlow;

typedef enum ERole {
  eConsole = 0,
  eMultimedia = ( eConsole + 1 ),
  eCommunications = ( eMultimedia + 1 ),
  ERole_enum_count = ( eCommunications + 1 ) 
} ERole;

typedef enum _AudioSessionState {
  AudioSessionStateInactive = 0,
  AudioSessionStateActive = 1,
  AudioSessionStateExpired = 2
} AudioSessionState;

typedef enum _AUDCLNT_SHAREMODE {
  AUDCLNT_SHAREMODE_SHARED,
  AUDCLNT_SHAREMODE_EXCLUSIVE
} AUDCLNT_SHAREMODE;

typedef enum _AUDIO_STREAM_CATEGORY {
  AudioCategory_Other = 0,
  AudioCategory_ForegroundOnlyMedia,
  AudioCategory_BackgroundCapableMedia,
  AudioCategory_Communications,
  AudioCategory_Alerts,
  AudioCategory_SoundEffects,
  AudioCategory_GameEffects,
  AudioCategory_GameMedia,
  AudioCategory_GameChat,
  AudioCategory_Speech,
  AudioCategory_Movie,
  AudioCategory_Media
} AUDIO_STREAM_CATEGORY;

#define AUDCLNT_STREAMFLAGS_CROSSPROCESS 0x00010000
#define AUDCLNT_STREAMFLAGS_LOOPBACK 0x00020000
#define AUDCLNT_STREAMFLAGS_EVENTCALLBACK 0x00040000
#define AUDCLNT_STREAMFLAGS_NOPERSIST 0x00080000
#define AUDCLNT_STREAMFLAGS_RATEADJUST 0x00100000
#define AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED 0x10000000
#define AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE 0x20000000
#define AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED 0x40000000

typedef LONGLONG REFERENCE_TIME;

typedef IUnknown IMMNotificationClient;

typedef struct PROPERTYKEY
{
  GUID fmtid;
  DWORD pid;
} PROPERTYKEY;

typedef PROPERTYKEY* REFPROPERTYKEY;
typedef const PROPVARIANT* REFPROPVARIANT;

// errors, for example see https://github.com/wine-mirror/wine/blob/master/include/mferror.h
#define MF_E_BAD_STARTUP_VERSION 0xc00d36e3
#define MF_E_DISABLED_IN_SAFEMODE 0xc00d36ef
#ifndef E_NOTFOUND
#define E_NOTFOUND 0x80070490
#endif

// IAudioClient errors
#define FACILITY_AUDCLNT 0x889
#define AUDCLNT_ERR(n) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_AUDCLNT, n)
#define AUDCLNT_SUCCESS(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_AUDCLNT, n)
#define AUDCLNT_E_NOT_INITIALIZED            AUDCLNT_ERR(0x001)
#define AUDCLNT_E_ALREADY_INITIALIZED        AUDCLNT_ERR(0x002)
#define AUDCLNT_E_WRONG_ENDPOINT_TYPE        AUDCLNT_ERR(0x003)
#define AUDCLNT_E_DEVICE_INVALIDATED         AUDCLNT_ERR(0x004)
#define AUDCLNT_E_NOT_STOPPED                AUDCLNT_ERR(0x005)
#define AUDCLNT_E_BUFFER_TOO_LARGE           AUDCLNT_ERR(0x006)
#define AUDCLNT_E_OUT_OF_ORDER               AUDCLNT_ERR(0x007)
#define AUDCLNT_E_UNSUPPORTED_FORMAT         AUDCLNT_ERR(0x008)
#define AUDCLNT_E_INVALID_SIZE               AUDCLNT_ERR(0x009)
#define AUDCLNT_E_DEVICE_IN_USE              AUDCLNT_ERR(0x00a)
#define AUDCLNT_E_BUFFER_OPERATION_PENDING   AUDCLNT_ERR(0x00b)
#define AUDCLNT_E_THREAD_NOT_REGISTERED      AUDCLNT_ERR(0x00c)
#define AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED AUDCLNT_ERR(0x00e)
#define AUDCLNT_E_ENDPOINT_CREATE_FAILED     AUDCLNT_ERR(0x00f)
#define AUDCLNT_E_SERVICE_NOT_RUNNING        AUDCLNT_ERR(0x010)
#define AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED     AUDCLNT_ERR(0x011)
#define AUDCLNT_E_EXCLUSIVE_MODE_ONLY          AUDCLNT_ERR(0x012)
#define AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL AUDCLNT_ERR(0x013)
#define AUDCLNT_E_EVENTHANDLE_NOT_SET          AUDCLNT_ERR(0x014)
#define AUDCLNT_E_INCORRECT_BUFFER_SIZE        AUDCLNT_ERR(0x015)
#define AUDCLNT_E_BUFFER_SIZE_ERROR            AUDCLNT_ERR(0x016)
#define AUDCLNT_E_CPUUSAGE_EXCEEDED            AUDCLNT_ERR(0x017)
#define AUDCLNT_S_BUFFER_EMPTY              AUDCLNT_SUCCESS(0x001)
#define AUDCLNT_S_THREAD_ALREADY_REGISTERED AUDCLNT_SUCCESS(0x002)
#define AUDCLNT_S_POSITION_STALLED		   AUDCLNT_SUCCESS(0x003)

#if WINVER < 0x0601
#define MF_SDK_VERSION 0x1
#else
#define MF_SDK_VERSION 0x2
#endif

#define MF_API_VERSION 0x0070
#define MF_VERSION (MF_SDK_VERSION << 16 | MF_API_VERSION)

#define MFSTARTUP_NOSOCKET 0x1
#define MFSTARTUP_LITE (MFSTARTUP_NOSOCKET)
#define MFSTARTUP_FULL 0

WSAPI_API_IMPORT HRESULT MFStartup(ULONG Version, DWORD dwFlags);

WSAPI_API_IMPORT HRESULT MFShutdown(void);

// IPropertyStore
extern const GUID IID_IPropertyStore;
#undef  INTERFACE
#define INTERFACE IPropertyStore
DECLARE_INTERFACE_(IPropertyStore, IUnknown)
{
    BEGIN_INTERFACE

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(HRESULT, GetCount)(THIS_ DWORD *cProps) PURE;
    STDMETHOD_(HRESULT, GetAt)(THIS_ DWORD iProp, PROPERTYKEY *pkey) PURE;
    STDMETHOD_(HRESULT, GetValue)(THIS_ REFPROPERTYKEY key, PROPVARIANT *pv) PURE;
    STDMETHOD_(HRESULT, SetValue)(THIS_ REFPROPERTYKEY key, REFPROPVARIANT propvar) PURE;
    STDMETHOD_(HRESULT, Commit)(THIS) PURE;

    END_INTERFACE
};
#undef INTERFACE

// IMMDevice
extern const GUID IID_IMMDevice;
#undef  INTERFACE
#define INTERFACE IMMDevice
DECLARE_INTERFACE_(IMMDevice, IUnknown)
{
    BEGIN_INTERFACE

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(HRESULT, Activate)(THIS_ REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface) PURE;
    STDMETHOD_(HRESULT, OpenPropertyStore)(THIS_ DWORD stgmAccess, IPropertyStore **ppProperties) PURE;
    STDMETHOD_(HRESULT, GetId)(THIS_ LPWSTR *ppstrId) PURE;
    STDMETHOD_(HRESULT, GetState)(THIS_ DWORD *pdwState) PURE;
            
    END_INTERFACE
};
#undef INTERFACE

// IMMDeviceCollection
extern const GUID IID_IMMDeviceCollection;
#undef  INTERFACE
#define INTERFACE IMMDeviceCollection
DECLARE_INTERFACE_(IMMDeviceCollection, IUnknown)
{
    BEGIN_INTERFACE

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(HRESULT, GetCount)(THIS_ UINT *pcDevices) PURE;
    STDMETHOD_(HRESULT, Item)(THIS_ UINT nDevice, IMMDevice **ppDevice) PURE;

    END_INTERFACE
};
#undef INTERFACE

// IMMDeviceEnumerator
extern const GUID CLSID_MMDeviceEnumerator;
extern const GUID IID_IMMDeviceEnumerator;
#undef  INTERFACE
#define INTERFACE IMMDeviceEnumerator
DECLARE_INTERFACE_(IMMDeviceEnumerator, IUnknown)
{
    BEGIN_INTERFACE

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(HRESULT, EnumAudioEndpoints)(THIS_ EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection **ppDevices) PURE;
    STDMETHOD_(HRESULT, GetDefaultAudioEndpoint)(THIS_ EDataFlow dataFlow, ERole role, IMMDevice **ppEndpoint) PURE;
    STDMETHOD_(HRESULT, GetDevice)(THIS_ LPCWSTR pwstrId, IMMDevice **ppDevice) PURE;
    STDMETHOD_(HRESULT, RegisterEndpointNotificationCallback)(THIS_ IMMNotificationClient *pClient) PURE;
    STDMETHOD_(HRESULT, UnregisterEndpointNotificationCallback)(THIS_ IMMNotificationClient *pClient) PURE;

    END_INTERFACE
};
#undef INTERFACE
IMMDeviceEnumerator* CreateMMDeviceEnumerator();

// IAudioClient
extern const GUID IID_IAudioClient;
#undef  INTERFACE
#define INTERFACE IAudioClient
DECLARE_INTERFACE_(IAudioClient, IUnknown)
{
    BEGIN_INTERFACE

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    STDMETHOD_(HRESULT, Initialize)(THIS_ AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration,
      REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid) PURE;
    STDMETHOD_(HRESULT, GetBufferSize)(THIS_ UINT32 *pNumBufferFrames) PURE;
    STDMETHOD_(HRESULT, GetStreamLatency)(THIS_ REFERENCE_TIME *phnsLatency) PURE;
    STDMETHOD_(HRESULT, GetCurrentPadding)(THIS_ UINT32 *pNumPaddingFrames) PURE;
    STDMETHOD_(HRESULT, IsFormatSupported)(THIS_ AUDCLNT_SHAREMODE ShareMode, const WAVEFORMATEX *pFormat, WAVEFORMATEX **ppClosestMatch) PURE;
    STDMETHOD_(HRESULT, GetMixFormat)(THIS_ WAVEFORMATEX **ppDeviceFormat) PURE;
    STDMETHOD_(HRESULT, GetDevicePeriod)(THIS_ REFERENCE_TIME *phnsDefaultDevicePeriod, REFERENCE_TIME *phnsMinimumDevicePeriod) PURE;
    STDMETHOD_(HRESULT, Start)(THIS) PURE;
    STDMETHOD_(HRESULT, Stop)(THIS) PURE;
    STDMETHOD_(HRESULT, Reset)(THIS) PURE;
    STDMETHOD_(HRESULT, SetEventHandle)(THIS_ HANDLE eventHandle) PURE;
    STDMETHOD_(HRESULT, GetService)(THIS_ REFIID riid, void **ppv) PURE;

    END_INTERFACE
};
#undef INTERFACE

#endif

