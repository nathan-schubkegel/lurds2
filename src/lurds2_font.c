/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_font.h"

#include "lurds2_errors.h"
#include "lurds2_jsonstream.h"
#include "lurds2_bmp.h"
#include "lurds2_stringutils.h"

#include <string.h>

#define DIAGNOSTIC_FONT_ERROR(message) DIAGNOSTIC_ERROR(message)
#define DIAGNOSTIC_FONT_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2))
#define DIAGNOSTIC_FONT_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3))
#define DIAGNOSTIC_FONT_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4))

typedef struct FontCharacter {
  uint32_t xOrigin; // The 0-based index of the x-coordinate where the left of the letter starts
  uint32_t yOrigin; // The 0-based index of the y-coordinate where the baseline of the letter starts. (baseline = bottom of a, middle of g)
  uint32_t width;   // The width of the letter
  uint32_t heightUp; // The number of pixels up from yOrigin
  uint32_t heightDown; // The number of pixels down from yOrigin
} FontCharacter;

static FontCharacter EmptyFontCharacter = { 0, 0, 0, 0, 0 };

#define FONTDATA_MAXCHARACTERS 128
typedef struct FontData {
  FontCharacter characters[FONTDATA_MAXCHARACTERS];
  Bmp bitmap;
  uint32_t universalHeightUp;
} FontData;

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

  // walk through JsonStream data to determine bmp file name and character points
  int done = 0;
  int inCharacterMap = 0;
  while (!done)
  {
    JsonStreamTokenType t = JsonStream_MoveNext(stream);
    switch (t)
    {
      case JsonStreamError:
        // diagnostic error already reported by JsonStream_MoveNext()
        goto die;

      case JsonStreamEnd:
        done = 1;
        break;

      case JsonStreamPropertyName:
      {
        int32_t propNameLength;
        const char* propName = JsonStream_GetString(stream, &propNameLength);
        if (propName == 0)
        {
          DIAGNOSTIC_FONT_ERROR2("failed to GetString() for JsonStreamPropertyName in ", JsonStream_GetDebugIdentifier(stream));
          goto die;
        }
        else if (inCharacterMap)
        {
          if (propNameLength != 1)
          {
            DIAGNOSTIC_FONT_ERROR4("Unexpected/invalid \"characters\" key \"", propName, "\" (must be single character) in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
          }
          else
          {
            int32_t character = propName[0];
            if (character < 0 || character >= FONTDATA_MAXCHARACTERS)
            {
              DIAGNOSTIC_FONT_ERROR4("Unexpected/invalid \"characters\" key \"", propName, "\" (must be ASCII) in ", JsonStream_GetDebugIdentifier(stream));
              goto die;
            }

            // read the 5 numbers
            if (JsonStream_MoveNext(stream) != JsonStreamArrayStart) goto bonk;

            if (JsonStream_MoveNext(stream) == JsonStreamNumber) data->characters[character].xOrigin = JsonStream_GetNumberInt(stream);
            else goto bonk;
            
            if (JsonStream_MoveNext(stream) == JsonStreamNumber) data->characters[character].yOrigin = JsonStream_GetNumberInt(stream);
            else goto bonk;
            
            if (JsonStream_MoveNext(stream) == JsonStreamNumber) data->characters[character].width = JsonStream_GetNumberInt(stream);
            else goto bonk;
            
            if (JsonStream_MoveNext(stream) == JsonStreamNumber) data->characters[character].heightUp = JsonStream_GetNumberInt(stream);
            else goto bonk;
            
            if (JsonStream_MoveNext(stream) == JsonStreamNumber) data->characters[character].heightDown = JsonStream_GetNumberInt(stream);
            else goto bonk;
            
            if (JsonStream_MoveNext(stream) != JsonStreamArrayEnd) goto bonk;
            goto not_bonk;
            
          bonk:
            DIAGNOSTIC_FONT_ERROR2("expected array of 5 numbers, but got something else in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
            
          not_bonk:
            (void)1;
          }
        }
        else if (strcmp("characters", propName) == 0)
        {
          inCharacterMap = 1;
        }
        else if (strcmp("bitmapFileName", propName) == 0)
        {
          const char* bitmapFileName;
          int32_t bitmapFileNameLength;
          wchar_t* wBitmapFileName;

          if (data->bitmap != 0)
          {
            DIAGNOSTIC_FONT_ERROR2("unexpected multiple \"bitmapFileName\" elements in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
          }
          else if (JsonStream_MoveNext(stream) != JsonStreamString)
          {
            DIAGNOSTIC_FONT_ERROR2("invalid \"bitmapFileName\" element; needs to be string, in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
          }
          else if (0 == (bitmapFileName = JsonStream_GetString(stream, &bitmapFileNameLength)))
          {
            DIAGNOSTIC_FONT_ERROR2("failed to GetString() for \"bitmapFileName\" element in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
          }
          // copy the bitmapFileName
          else if (0 == (wBitmapFileName = StringUtils_MakeWideString(bitmapFileName)))
          {
            DIAGNOSTIC_FONT_ERROR2("failed to StringUtils_MakeWideString() for \"bitmapFileName\" element in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
          }
          else
          {
            data->bitmap = Bmp_LoadMaskingBitmapFromResourceFile(wBitmapFileName);
            free(wBitmapFileName);
            if (data->bitmap == 0)
            {
              // diagnostic error already reported by Bmp_LoadFromResourceFile()
              goto die;
            }
          }
        }
        else if (strcmp("universalHeightUp", propName) == 0)
        {
          if (JsonStream_MoveNext(stream) != JsonStreamNumber)
          {
            DIAGNOSTIC_FONT_ERROR2("invalid \"universalHeightUp\" element; needs to be number, in ", JsonStream_GetDebugIdentifier(stream));
            goto die;
          }
          data->universalHeightUp = JsonStream_GetNumberInt(stream);
        }
        break;
      }
      case JsonStreamObjectEnd:
        if (inCharacterMap)
        {
          inCharacterMap = 0;
        }
        break;
    }
  }

  if (data->bitmap == 0)
  {
    DIAGNOSTIC_FONT_ERROR2("missing \"bitmapFileName\" element in ", JsonStream_GetDebugIdentifier(stream));
    goto die;
  }
  
  if (data->universalHeightUp == 0)
  {
    DIAGNOSTIC_FONT_ERROR2("missing \"universalHeightUp\" element in ", JsonStream_GetDebugIdentifier(stream));
    goto die;
  }
  
  // make all uninitialized characters look like the space character
  for (int i = 0; i < FONTDATA_MAXCHARACTERS; i++)
  {
    if (memcmp(&data->characters[i], &EmptyFontCharacter, sizeof(EmptyFontCharacter) == 0))
    {
      if (i == ' ')
      {
        DIAGNOSTIC_FONT_ERROR2("missing \"characters\" entry for \" \" (space) in ", JsonStream_GetDebugIdentifier(stream));
        goto die;
      }
      else
      {
        memcpy(&data->characters[i], &EmptyFontCharacter, sizeof(EmptyFontCharacter));
      }
    }
  }
  
  JsonStream_Release(stream);
  return data;

die:
  JsonStream_Release(stream);
  if (data->bitmap) Bmp_Release(data->bitmap);
  free(data);
  return 0;
}

void Font_Release(Font font)
{
  FontData* data = (FontData*)font;
  if (!data)
  {
    DIAGNOSTIC_FONT_ERROR("font arg is null");
    return;
  }
  
  Bmp_Release(data->bitmap);
  free(data);
}

static FontMeasurement Font_DoSingleLine(Font font, const char * text, int render)
{
  FontMeasurement result = { 0, 0, 0, 0 };
  
  if (text == 0)
  {
    DIAGNOSTIC_FONT_ERROR("invalid null 'text' arg");
    return result;
  }

  FontData* data = (FontData*)font;
  if (font == 0)
  {
    DIAGNOSTIC_FONT_ERROR("invalid null 'font' arg");
    return result;
  }
  
  GLenum oldMode;
  glGetIntegerv(GL_MATRIX_MODE, &oldMode);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  while (*text != 0)
  {
    FontCharacter* c;
    if (*text >= 0 && *text < FONTDATA_MAXCHARACTERS)
    {
      c = &data->characters[*text];
    }
    else
    {
      c = &data->characters[' '];
    }

    if (render)
    {
      int heightBoost = data->universalHeightUp - c->heightUp;
      glTranslated(result.width, heightBoost, 0);
      Bmp_DrawPortion(data->bitmap, c->xOrigin, c->yOrigin - c->heightUp, c->width, c->heightUp + c->heightDown);
      
      // TODO: is there a more efficient way to do this?
      // I just don't want to get bit by cumulative floaty lossy translation...
      glPopMatrix();
      glPushMatrix();
    }

    result.width += c->width;
    if (c->heightDown > result.descenderHeight)
    {
      result.descenderHeight = c->heightDown;
    }

    text++;
  }
  glPopMatrix();
  glMatrixMode(oldMode);

  result.universalLineHeight = data->universalHeightUp;
  result.success = 1;
  return result;
}

FontMeasurement Font_MeasureSingleLine(Font font, const char * text)
{
  return Font_DoSingleLine(font, text, 0);
}

FontMeasurement Font_RenderSingleLine(Font font, const char * text)
{
  return Font_DoSingleLine(font, text, 1);
}