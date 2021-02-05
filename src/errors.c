#include "errors.h"

char* GetLastErrorMessage()
{
  static char buffer[1000];
  DWORD errorMessageID;
  DWORD size;
  
  //Get the error message ID, if any.
  errorMessageID = GetLastError();
  if(errorMessageID == 0)
  {
    strcpy(buffer, "no error");
    return buffer;
  }

  size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        0, errorMessageID, 0, buffer, sizeof(buffer), 0);
  if (size == 0)
  {
    strcpy(buffer, "unknown error");
  }
  else
  {
    buffer[sizeof(buffer) - 1] = 0;
  }
  return buffer;
}