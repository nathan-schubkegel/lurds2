/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_BMP
#define LURDS2_BMP

typedef void* Bmp;

// A Bmp holds the bitmap data for MS Paint image loaded from file.
Bmp   Bmp_LoadFromResourceFile(const wchar_t * fileName);
void  Bmp_SetPixelPerfect(Bmp bmp, int newValue); // 1 to render using GL_NEAREST, 0 to render using GL_LINEAR (blend of 4 nearest pixels)
void  Bmp_Draw(Bmp bmp);
void  Bmp_DrawPortion(Bmp bmp, int x, int y, int width, int height);
void  Bmp_Release(Bmp bmp);

#endif