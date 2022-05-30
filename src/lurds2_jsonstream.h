/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_JSONSTREAM
#define LURDS2_JSONSTREAM

typedef enum JsonStreamTokenType {
  JsonStreamError,
  JsonStreamStart,
  JsonStreamEnd,
  JsonStreamObjectStart,
  JsonStreamObjectEnd,
  JsonStreamArrayStart,
  JsonStreamArrayEnd,
  JsonStreamString,
  JsonStreamNumber,
  JsonStreamTrue,
  JsonStreamFalse,
  JsonStreamNull
} JsonStreamTokenType;

typedef void* JsonStream;

// A JsonStream allows forward iteration through a JSON file's contents (loaded into memory).
JsonStream           JsonStream_LoadFromResourceFile(const wchar_t * fileName);
JsonStreamTokenType  JsonStream_MoveNext(JsonStream stream);
JsonStreamTokenType  JsonStream_GetTokenType(JsonStream stream);
const char*          JsonStream_GetString(JsonStream stream, int* length);
int                  JsonStream_GetNumberInt(JsonStream stream);
void                 JsonStream_Release(JsonStream stream);

#endif