/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_font.h"

#include "lurds2_errors.h"
#include "lurds2_jsonstream.h"

#define DIAGNOSTIC_FONT_ERROR(message) DIAGNOSTIC_ERROR(message)
#define DIAGNOSTIC_FONT_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2))
#define DIAGNOSTIC_FONT_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3))
#define DIAGNOSTIC_FONT_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4))

typedef struct FontCharacter {
  int32_t xOrigin; // The 0-based index of the x-coordinate where the left of the letter starts
  int32_t yOrigin; // The 0-based index of the y-coordinate where the baseline of the letter starts. (baseline = bottom of a, middle of g)
  int32_t width;   // The width of the letter
  int32_t heightUp; // The number of pixels up from yOrigin
  int32_t heightDown; // The number of pixels down from yOrigin
} FontCharacter;

typedef struct FontData {
  FontCharacter characters[128];
}

Font Font_LoadFromResourceFile(const wchar_t * fileName)
{
  JsonStream stream = JsonStream_LoadFromResourceFile(fileName);
  if (stream == 0)
  {
    // diagnostic error already reported by JsonStream constructor
    return 0;
  }
  
  FontData * data = malloc(sizeof(FontData));
  if (data == 0)
  {
    DIAGNOSTIC_FONT_ERROR("Failed to allocate memory for FontData");
    JsonStream_Release(stream);
    return 0;
  }
  memset(data, 0, sizeof(FontData));

  // walk through JsonStream data to determine bmp file name and points
  int done = 0;
  int inCharacterMap = 0;
  int character = 0;
  while (!done) {
    JsonStreamTokenType t = JsonStream_MoveNext(stream);
    switch
    {
      case JsonStreamError:
        // diagnostic error already reported by JsonStream_MoveNext()
        JsonStream_Release(stream);
        free(data);
        return 0;

      case JsonStreamEnd:
        done = 1;
        break;
        
      case JsonStreamObjectStart,
      case JsonStreamObjectEnd:
        if (inCharacterMap)
        {
          inCharacterMap = 0;
        }
      case JsonStreamArrayStart,
      case JsonStreamArrayEnd,
      case JsonStreamPropertyName:
        if (strcmp("characters", JsonStream_GetString(stream, 0)) == 0)
        {
          inCharacterMap = 1;
        }
        else if (inCharacterMap)
        {
          
        }
      case JsonStreamString,
      case JsonStreamNumber,
      case JsonStreamTrue,
      case JsonStreamFalse,
      case JsonStreamNull
      
    }
  }
  
  int32_t fileSize;
  char* jsonData = ResourceFile_Load(fileName, &fileSize);
}

void Font_Release(Font font);

FontMeasurement Font_MeasureSingleLine(Font font, const char * text);
FontMeasurement Font_RenderSingleLine(Font font, const char * text);

FontMeasurement Font_MeasureLinesConstrainedByWidth(Font font, const char * text, int32_t width);
FontMeasurement Font_RenderLinesConstrainedByWidth(Font font, const char * text, int32_t width);