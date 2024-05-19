/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_errors.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <errno.h>
#endif

#include <wchar.h>
#include <string.h>
#include <stdlib.h>

#define ERROR_MESSAGE_BUFFER_SIZE 2048

#ifdef _WIN32
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

#else

typedef struct LinuxError
{
  int ErrorCode;
  const char* ErrorMessage;
} LinuxError;

static LinuxError sKnownErrors[] =
{
  { EPERM, "EPERM - Operation not permitted" },
  { ENOENT, "ENOENT - No such file or directory" },
  { ESRCH, "ESRCH - No such process" },
  { EINTR, "EINTR - Interrupted system call" },
  { EIO, "EIO - I/O error" },
  { ENXIO, "ENXIO - No such device or address" },
  { E2BIG, "E2BIG - Argument list too long" },
  { ENOEXEC, "ENOEXEC - Exec format error" },
  { EBADF, "EBADF - Bad file number" },
  { ECHILD, "ECHILD - No child processes" },
  { EAGAIN, "EAGAIN - Try again" },
  { ENOMEM, "ENOMEM - Out of memory" },
  { EACCES, "EACCES - Permission denied" },
  { EFAULT, "EFAULT - Bad address" },
  { ENOTBLK, "ENOTBLK - Block device required" },
  { EBUSY, "EBUSY - Device or resource busy" },
  { EEXIST, "EEXIST - File exists" },
  { EXDEV, "EXDEV - Cross-device link" },
  { ENODEV, "ENODEV - No such device" },
  { ENOTDIR, "ENOTDIR - Not a directory" },
  { EISDIR, "EISDIR - Is a directory" },
  { EINVAL, "EINVAL - Invalid argument" },
  { ENFILE, "ENFILE - File table overflow" },
  { EMFILE, "EMFILE - Too many open files" },
  { ENOTTY, "ENOTTY - Not a typewriter" },
  { ETXTBSY, "ETXTBSY - Text file busy" },
  { EFBIG, "EFBIG - File too large" },
  { ENOSPC, "ENOSPC - No space left on device" },
  { ESPIPE, "ESPIPE - Illegal seek" },
  { EROFS, "EROFS - Read-only file system" },
  { EMLINK, "EMLINK - Too many links" },
  { EPIPE, "EPIPE - Broken pipe" },
  { EDOM, "EDOM - Math argument out of domain of func" },
  { ERANGE, "ERANGE - Math result not representable" },
};

const char* GetLinuxErrorCodeMessage(int errorCode)
{
  LinuxError* current = &sKnownErrors[0];
  LinuxError* end = current + (sizeof(sKnownErrors) / sizeof(sKnownErrors[0]));
  while (current < end)
  {
    if (current->ErrorCode == errorCode)
    {
      return current->ErrorMessage;
    }
    current++;
  }
  return "Unrecognized error code";
}

#endif

#ifdef _WIN32

static DWORD WINAPI ShowFatalErrorThenKillProcessProc(LPVOID lpParameter)
{
  MessageBox(0, (char*)lpParameter, "lurds2 fatal error", 0);
}
#endif

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
#ifdef _WIN32
  strncat(buffer, "\r\n\r\n", sizeof(buffer) - strlen(buffer) - 1);
#else
  strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
#endif

  if (message) strncat(buffer, message, sizeof(buffer) - strlen(buffer) - 1);
  if (message2) strncat(buffer, message2, sizeof(buffer) - strlen(buffer) - 1);
  if (message3) strncat(buffer, message3, sizeof(buffer) - strlen(buffer) - 1);
  if (message4) strncat(buffer, message4, sizeof(buffer) - strlen(buffer) - 1);

#ifdef _WIN32
  HANDLE t;
  t = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ShowFatalErrorThenKillProcessProc, buffer, 0, 0);
  if (t != 0)
  {
    WaitForSingleObject(t, INFINITE);
  }
  ExitProcess(1);
#else
  printf("%s\n\n", buffer);
  exit(1);
#endif
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

#ifdef _WIN32
static DWORD WINAPI ShowDiagnosticErrorProc(LPVOID lpParameter)
{
  MessageBox(0, (char*)lpParameter, "lurds2 diagnostic error", 0);
}
#endif

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
#ifdef _WIN32
  strncat(buffer, "\r\n\r\n", sizeof(buffer) - strlen(buffer) - 1);
#else
  strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
#endif

  if (message) strncat(buffer, message, sizeof(buffer) - strlen(buffer) - 1);
  if (message2) strncat(buffer, message2, sizeof(buffer) - strlen(buffer) - 1);
  if (message3) strncat(buffer, message3, sizeof(buffer) - strlen(buffer) - 1);
  if (message4) strncat(buffer, message4, sizeof(buffer) - strlen(buffer) - 1);

#ifdef _WIN32
  HANDLE t;
  t = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ShowDiagnosticErrorProc, buffer, 0, 0);
  if (t != 0)
  {
    WaitForSingleObject(t, INFINITE);
  }
#else
  printf("%s\n\n", buffer);
#endif
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

void DebugShowInteger(const char* file, const char* function, int line, const char* message, int value, const char * message2, const char * message3)
{
  char buffer[ERROR_MESSAGE_BUFFER_SIZE];
  memset(buffer, 0, sizeof(buffer));

  strncat(buffer, "at line ", sizeof(buffer) - strlen(buffer) - 1);
  sprintf(&buffer[strlen(buffer)], "%d", line);
  strncat(buffer, " of ", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, function, sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, "() in ", sizeof(buffer) - strlen(buffer) - 1);
  strncat(buffer, file, sizeof(buffer) - strlen(buffer) - 1);
#ifdef _WIN32
  strncat(buffer, "\r\n\r\n", sizeof(buffer) - strlen(buffer) - 1);
#else
  strncat(buffer, "\n", sizeof(buffer) - strlen(buffer) - 1);
#endif

  if (message) strncat(buffer, message, sizeof(buffer) - strlen(buffer) - 1);
  char numBuffer[50];
  sprintf(numBuffer, "%d", value);
  strncat(buffer, numBuffer, sizeof(buffer) - strlen(numBuffer) - 1);
  if (message2) strncat(buffer, message2, sizeof(buffer) - strlen(buffer) - 1);
  if (message3) strncat(buffer, message3, sizeof(buffer) - strlen(buffer) - 1);

#ifdef _WIN32
  HANDLE t;
  t = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ShowDiagnosticErrorProc, buffer, 0, 0);
  if (t != 0)
  {
    WaitForSingleObject(t, INFINITE);
  }
#else
  printf("%s\n\n", buffer);
#endif
}
