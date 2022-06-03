/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_FONT
#define LURDS2_FONT

typedef void* Font;

typedef struct FontMeasurement
{
  int success;
  uint32_t width; // the aggregate width of all rendered letters
  uint32_t universalLineHeight; // The amount of vertical space reserved by the font to render capital letters
  //uint32_t suggestedOutsideUpperPadding; // The amount of additional vertical space above 'universalLineHeight' suggested by the font that capital letters may render into
  //uint32_t suggestedOutsideLowerPadding; // The amount of additional vertical space below 'universalLineHeight' suggested by the font that letters may render into
  //uint32_t suggestedBetweenLinePadding; // The amount of additional vertical space suggested by the font between multiple rendered lines of text
} FontMeasurement;

// A Font holds all data loaded from resource files needed to render text to an opengl surface
Font Font_LoadFromResourceFile(const wchar_t * fileName);
void Font_Release(Font font);

FontMeasurement Font_MeasureSingleLine(Font font, const char * text);
FontMeasurement Font_RenderSingleLine(Font font, const char * text);

FontMeasurement Font_MeasureLinesConstrainedByWidth(Font font, const char * text, int32_t width);
FontMeasurement Font_RenderLinesConstrainedByWidth(Font font, const char * text, int32_t width);

#endif