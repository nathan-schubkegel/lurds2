/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_errors.h"

#include <Windows.h>
#include <wchar.h>

#define ERROR_MESSAGE_BUFFER_SIZE 2048

char* GetLastErrorMessage()
{
  //Get the error message ID, if any.
  DWORD errorMessageID;
  errorMessageID = GetLastError();

  static char buffer[ERROR_MESSAGE_BUFFER_SIZE];
  memset(buffer, 0, sizeof(buffer));

  if(errorMessageID == 0)
  {
    strcpy(buffer, "no error");
    return buffer;
  }

  DWORD size;
  size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        0, errorMessageID, 0, buffer, sizeof(buffer) - 1, 0);
  if (size == 0)
  {
    strcpy(buffer, "unknown error");
  }
  return buffer;
}

char* CopyWstrToCstr(const wchar_t* input)
{
  if (!input) return 0; 
  int len;
  len = wcslen(input);
  char* result;
  result = malloc(len + 1);
  if (!result) return 0;
  int i;
  for (i = 0; i <= len; i++)
  {
    result[i] = (char)input[i];
  }
  return result;
}

static DWORD WINAPI ShowFatalErrorThenKillProcessProc(LPVOID lpParameter)
{
  MessageBox(0, (char*)lpParameter, "lurds2 fatal error", 0);
}

void ShowFatalErrorThenKillProcess4(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3, const char* message4)
{
  char buffer[ERROR_MESSAGE_BUFFER_SIZE];
  memset(buffer, 0, sizeof(buffer));

  strncat(buffer, "at line ", sizeof(buffer) - strlen(buffer) - 1);
  sprintf(&buffer[strlen(buffer)], "%d", line);
  strncat(buffer, " of ", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, function, sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, " in ", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, file, sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, "\r\n\r\n", sizeof(buffer) - strlen(buffer) - 1);

  if (message) strncat(buffer, message, sizeof(buffer) - strlen(buffer) - 1);
  if (message2) strncat(buffer, message2, sizeof(buffer) - strlen(buffer) - 1);
  if (message3) strncat(buffer, message3, sizeof(buffer) - strlen(buffer) - 1);
  if (message4) strncat(buffer, message4, sizeof(buffer) - strlen(buffer) - 1);

  HANDLE t;
  t = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ShowFatalErrorThenKillProcessProc, buffer, 0, 0);
  if (t != 0)
  {
    WaitForSingleObject(t, INFINITE);
  }
  ExitProcess(1);
}

void ShowFatalErrorThenKillProcess3(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3)
{
  ShowFatalErrorThenKillProcess4(file, function, line, message, message2, message3, 0);
}

void ShowFatalErrorThenKillProcess2(const char* file, const char* function, int line, const char* message, const char* message2)
{
  ShowFatalErrorThenKillProcess4(file, function, line, message, message2, 0, 0);
}

void ShowFatalErrorThenKillProcess(const char* file, const char* function, int line, const char* message)
{
  ShowFatalErrorThenKillProcess4(file, function, line, message, 0, 0, 0);
}

static DWORD WINAPI ShowDiagnosticErrorProc(LPVOID lpParameter)
{
  MessageBox(0, (char*)lpParameter, "lurds2 diagnostic error", 0);
}

void ShowDiagnosticError4(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3, const char* message4)
{
  char buffer[ERROR_MESSAGE_BUFFER_SIZE];
  memset(buffer, 0, sizeof(buffer));

  strncat(buffer, "at line ", sizeof(buffer) - strlen(buffer) - 1);
  sprintf(&buffer[strlen(buffer)], "%d", line);
  strncat(buffer, " of ", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, function, sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, "() in ", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, file, sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, "\r\n\r\n", sizeof(buffer) - strlen(buffer) - 1);

  if (message) strncat(buffer, message, sizeof(buffer) - strlen(buffer) - 1);
  if (message2) strncat(buffer, message2, sizeof(buffer) - strlen(buffer) - 1);
  if (message3) strncat(buffer, message3, sizeof(buffer) - strlen(buffer) - 1);
  if (message4) strncat(buffer, message4, sizeof(buffer) - strlen(buffer) - 1);

  HANDLE t;
  t = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ShowDiagnosticErrorProc, buffer, 0, 0);
  if (t != 0)
  {
    WaitForSingleObject(t, INFINITE);
  }
}

void ShowDiagnosticError3(const char* file, const char* function, int line, const char* message, const char* message2, const char* message3)
{
  ShowDiagnosticError4(file, function, line, message, message2, message3, 0);
}

void ShowDiagnosticError2(const char* file, const char* function, int line, const char* message, const char* message2)
{
  ShowDiagnosticError4(file, function, line, message, message2, 0, 0);
}

void ShowDiagnosticError(const char* file, const char* function, int line, const char* message)
{
  ShowDiagnosticError4(file, function, line, message, 0, 0, 0);
}