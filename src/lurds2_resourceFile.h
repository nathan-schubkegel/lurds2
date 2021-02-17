/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_RESOURCE_FILE
#define LURDS2_RESOURCE_FILE

// returns 0 on failure
// returns number of characters now used in 'buffer' on success
int ResourceFile_GetPath(wchar_t* buffer, int bufferSize, const wchar_t* fileName);

void* ResourceFile_Load(const wchar_t* fileName, int* fileSize);

#endif