/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_FONT
#define LURDS2_FONT

typedef struct FontCharacter {
  int xOrigin; // The 0-based index of the x-coordinate where the left of the letter starts
  int yOrigin; // The 0-based index of the y-coordinate where the baseline of the letter starts. (baseline = bottom of a, middle of g)
  int width;   // The width of the letter
  int heightUp; // The number of pixels up from yOrigin
  int heightDown; // The number of pixels down from yOrigin
} FontCharacter;

typedef void* Font;

// A Font holds the bitmap data for MS Paint image loaded from file.
Font  Font_LoadFromResourceFile(const wchar_t * fileName);
void  Bmp_Draw(Bmp bmp);
void  Bmp_DrawPortion(Bmp bmp, int x, int y, int width, int height);
void  Font_Release(Bmp bmp);





#endif