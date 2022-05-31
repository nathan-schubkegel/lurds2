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
  JsonStreamPropertyName,
  JsonStreamString,
  JsonStreamNumber,
  JsonStreamTrue,
  JsonStreamFalse,
  JsonStreamNull
} JsonStreamTokenType;

typedef void* JsonStream;

// A JsonStream allows forward iteration through a JSON file's contents (loaded into memory).
JsonStream           JsonStream_Parse(const char* jsonData, const char* debugIdentifier);
JsonStream           JsonStream_LoadFromResourceFile(const wchar_t * fileName);
void                 JsonStream_Release(JsonStream stream);

JsonStreamTokenType  JsonStream_MoveNext(JsonStream stream);
JsonStreamTokenType  JsonStream_GetTokenType(JsonStream stream);

// returns borrowed string data; you have to copy it if you want to keep it
const char*          JsonStream_GetString(JsonStream stream, int32_t* length);
int64_t              JsonStream_GetNumberInt(JsonStream stream);


#endif