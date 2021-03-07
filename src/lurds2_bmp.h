/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_BMP
#define LURDS2_BMP

typedef void* Bmp;
typedef void* Bmp;

// A Bmp holds the bitmap data for MS Paint image loaded from file.
Bmp   Bmp_LoadFromResourceFile(const wchar_t * fileName);
void* Bmp_GetPixelData(Bmp bmp);
void  Bmp_Release(Bmp bmp);

#endif