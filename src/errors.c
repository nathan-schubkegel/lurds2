#include "errors.h"

char* GetLastErrorMessage()
{
  return GetLastErrorMessageWithPrefix(0);
}

char* GetLastErrorMessageWithPrefix(char * prefix)
{
  static char buffer[1000];
  DWORD errorMessageID;
  DWORD size;
  
  buffer[0] = 0;
  if (prefix)
  {
    strcpy(buffer, prefix);
  }

  //Get the error message ID, if any.
  errorMessageID = GetLastError();
  if(errorMessageID == 0)
  {
    strcat(buffer, "no error");
    return buffer;
  }

  size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        0, errorMessageID, 0, buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), 0);
  if (size == 0)
  {
    strcat(buffer, "unknown error");
  }
  else
  {
    buffer[sizeof(buffer) - 1] = 0;
  }
  return buffer;
}