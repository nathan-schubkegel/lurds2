#include "wsapi.h"

#include <Objbase.h>

#define DIAGNOSTIC_WSAPI_ERROR(message) MessageBox(0, (message), "error in wsapi.c", 0);

#define LURDS2_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID name = { l,w1,w2,{ b1,b2,b3,b4,b5,b6,b7,b8 } }
LURDS2_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
LURDS2_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
LURDS2_GUID(IID_IMMDeviceCollection, 0x0BD7A1BE, 0x7A1A, 0x44DB, 0x83, 0x97, 0xCC, 0x53, 0x92, 0x38, 0x7B, 0x5E);
LURDS2_GUID(IID_IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48, 0xE8, 0x07, 0x36, 0x3F);
LURDS2_GUID(IID_IPropertyStore, 0x886d8eeb, 0x8cf2, 0x4446, 0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99);
LURDS2_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);

IMMDeviceEnumerator* CreateMMDeviceEnumerator()
{
  HRESULT result;
  IMMDeviceEnumerator* obj;
  result = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, &obj);
  if (!SUCCEEDED(result))
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
    return 0;
  }
  return obj;
}