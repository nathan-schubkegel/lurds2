/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_resourceFile.h"

#include <Windows.h>
#include <stdio.h>
#include "lurds2_errors.h"
#include "lurds2_stringutils.h"
#include <wchar.h>

#define DIAGNOSTIC_RESOURCE_ERROR(message) DIAGNOSTIC_ERROR(message);
#define DIAGNOSTIC_RESOURCE_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2));
#define DIAGNOSTIC_RESOURCE_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3));
#define DIAGNOSTIC_RESOURCE_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4));

#define PathBufferSize 1024
BUILD_ASSERT(PathBufferSize >= MAX_PATH);
static volatile wchar_t gExecutingDir[PathBufferSize];
static volatile int gExecutingDirLength;
static volatile wchar_t gPathSeparator[2];
static volatile long gExecutingDirSpinLock;

static void LoadExecutingDir()
{
  if (gExecutingDir[0] == '\0')
  {
    while (InterlockedCompareExchange(&gExecutingDirSpinLock, 1, 0) != 0)
    {
    }

    GetModuleFileNameW(0, (wchar_t*)gExecutingDir, PathBufferSize);
    gExecutingDir[PathBufferSize - 1] = 0;

    // find last directory separator char
    int i, lastSeparator;
    for (i = 0, lastSeparator = -1; i < PathBufferSize && gExecutingDir[i] != 0; i++)
    {
      if (gExecutingDir[i] == '\\' || gExecutingDir[i] == '/')
      {
        gPathSeparator[0] = gExecutingDir[i];
        gPathSeparator[1] = 0;
        lastSeparator = i;
      }
    }

    if (lastSeparator == -1)
    {
      FATAL_ERROR("couldn't find directory separator in gExecutingDir");
    }

    // clear all characters after the last directory separator char
    memset((void*)&gExecutingDir[lastSeparator + 1], 0, sizeof(wchar_t) * (PathBufferSize - (lastSeparator + 1)));
    gExecutingDirLength = lastSeparator + 1;

    InterlockedExchange(&gExecutingDirSpinLock, 0);
  }
}

int ResourceFile_GetPath(wchar_t* buffer, int bufferSize, const wchar_t * fileName)
{
  if (!buffer)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid null buffer arg");
    return 0;
  }

  if (!fileName)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid null fileName arg");
    return 0;
  }

  if (bufferSize <= 0)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid bufferSize arg <= 0");
    return 0;
  }

  LoadExecutingDir();

  int fileNameLength;
  fileNameLength = wcslen(fileName);
#define RES_DIR_NAME_LEN 4
  if (fileNameLength + RES_DIR_NAME_LEN + gExecutingDirLength + 1 > bufferSize)
  {
    DIAGNOSTIC_RESOURCE_ERROR("insufficient buffer size to hold full file path");
    return 0;
  }

  wcscpy(buffer, (void*)gExecutingDir);
  wcscat(buffer, L"res\\");
  wcscat(buffer, fileName);
  return fileNameLength + gExecutingDirLength;
}

int ResourceFile_GetLords2FilePath(wchar_t* buffer, int bufferSize, const wchar_t * fileName)
{
  static const wchar_t* Lords2Dir = L"C:\\games\\Lords of the Realm II\\";

  if (!buffer)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid null buffer arg");
    return 0;
  }

  if (!fileName)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid null fileName arg");
    return 0;
  }

  if (bufferSize <= 0)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid bufferSize arg <= 0");
    return 0;
  }

  int lords2DirLength = wcslen(Lords2Dir);
  int fileNameLength = wcslen(fileName);
  if (fileNameLength + lords2DirLength + 1 > bufferSize)
  {
    DIAGNOSTIC_RESOURCE_ERROR("insufficient buffer size to hold full file path");
    return 0;
  }

  wcscpy(buffer, Lords2Dir);
  wcscat(buffer, fileName);
  return fileNameLength + lords2DirLength;
}

void* ResourceFile_Load(const wchar_t* fileName, int* fileSize)
{
  wchar_t filePath[PathBufferSize];

  if (!fileName)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid null fileName arg");
    return 0;
  }

  if (!ResourceFile_GetPath(filePath, PathBufferSize, fileName))
  {
    return 0;
  }

  HANDLE h;
  void* data;

  data = 0;
  h = INVALID_HANDLE_VALUE;

  h = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h == INVALID_HANDLE_VALUE)
  {
    char* nFilePath = StringUtils_MakeNarrowString(filePath);
    DIAGNOSTIC_RESOURCE_ERROR4("CreateFileW(): ", GetLastErrorMessage(), " ", nFilePath);
    free(nFilePath);
    goto error;
  }

  DWORD size;
  DWORD sizeHigh;
  size = GetFileSize(h, &sizeHigh);
  if (INVALID_FILE_SIZE == size)
  {
    DIAGNOSTIC_RESOURCE_ERROR2("GetFileSize(): ", GetLastErrorMessage());
    goto error;
  }

  // max 10 megs resource file supported
  if (size > 10000000 || sizeHigh > 0)
  {
    DIAGNOSTIC_RESOURCE_ERROR("resource file too big");
    goto error;
  }

  data = malloc(size + 2);
  if (data == 0)
  {
    DIAGNOSTIC_RESOURCE_ERROR("failed to allocate memory for file data");
    goto error;
  }

  DWORD numBytesRead;
  if (!ReadFile(h, data, size, &numBytesRead, 0))
  {
    DIAGNOSTIC_RESOURCE_ERROR2("ReadFile(): ", GetLastErrorMessage());
    goto error;
  }

  if (numBytesRead != size)
  {
    DIAGNOSTIC_RESOURCE_ERROR("unexpected numByteRead from sound file");
    goto error;
  }

  // ding fries are done
  CloseHandle(h);
  
  // null terminate the data, for convenience if file contains string data
  ((char*)data)[size] = 0;
  ((char*)data)[size + 1] = 0;

  if (fileSize) *fileSize = size;
  return data;
  
error:
  if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
  if (data != 0) free(data);
  return 0;
}

void* ResourceFile_LoadLords2File(const wchar_t* fileName, int* fileSize)
{
  wchar_t filePath[PathBufferSize];

  if (!fileName)
  {
    DIAGNOSTIC_RESOURCE_ERROR("invalid null fileName arg");
    return 0;
  }

  if (!ResourceFile_GetLords2FilePath(filePath, PathBufferSize, fileName))
  {
    return 0;
  }

  HANDLE h;
  void* data;

  data = 0;
  h = INVALID_HANDLE_VALUE;

  h = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h == INVALID_HANDLE_VALUE)
  {
    char* nFilePath = StringUtils_MakeNarrowString(filePath);
    DIAGNOSTIC_RESOURCE_ERROR4("CreateFileW(): ", GetLastErrorMessage(), " ", nFilePath);
    free(nFilePath);
    goto error;
  }

  DWORD size;
  DWORD sizeHigh;
  size = GetFileSize(h, &sizeHigh);
  if (INVALID_FILE_SIZE == size)
  {
    DIAGNOSTIC_RESOURCE_ERROR2("GetFileSize(): ", GetLastErrorMessage());
    goto error;
  }

  // max 10 megs resource file supported
  if (size > 10000000 || sizeHigh > 0)
  {
    DIAGNOSTIC_RESOURCE_ERROR("resource file too big");
    goto error;
  }

  data = malloc(size + 2);
  if (data == 0)
  {
    DIAGNOSTIC_RESOURCE_ERROR("failed to allocate memory for file data");
    goto error;
  }

  DWORD numBytesRead;
  if (!ReadFile(h, data, size, &numBytesRead, 0))
  {
    DIAGNOSTIC_RESOURCE_ERROR2("ReadFile(): ", GetLastErrorMessage());
    goto error;
  }

  if (numBytesRead != size)
  {
    DIAGNOSTIC_RESOURCE_ERROR("unexpected numByteRead from sound file");
    goto error;
  }

  // ding fries are done
  CloseHandle(h);
  
  // null terminate the data, for convenience if file contains string data
  ((char*)data)[size] = 0;
  ((char*)data)[size + 1] = 0;

  if (fileSize) *fileSize = size;
  return data;
  
error:
  if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
  if (data != 0) free(data);
  return 0;
}