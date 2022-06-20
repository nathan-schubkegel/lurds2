/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_bmp.h"

#include "lurds2_errors.h"
#include <wingdi.h>
#include <GL/GL.h>

#define DIAGNOSTIC_BMP_ERROR(message) DIAGNOSTIC_ERROR(message)
#define DIAGNOSTIC_BMP_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2))
#define DIAGNOSTIC_BMP_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3))
#define DIAGNOSTIC_BMP_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4))

typedef struct __attribute__((packed)) BmpHeader {
  unsigned char bm[2]; // Contains "BM"
  unsigned long fileSize; // The size of the BMP file in bytes
  unsigned long reserved;
  unsigned long pixelDataOffset; // The position in the file data where the pixel array starts.
  union {
    unsigned long headerSize; // Number of bytes in the DIB header (from this point). This size indicates header version
    // 12 = BITMAPCOREHEADER
    //BITMAPCOREHEADER coreHeader;
    // 40 = BITMAPINFOHEADER
    BITMAPINFOHEADER infoHeader;    
    // 108 = BITMAPV4HEADER
    //BITMAPV4HEADER v4Header;
    // 124 = BITMAPV5HEADER
    //BITMAPV5HEADER v5Header;
  };
} BmpHeader;

// BITMAPINFOHEADER at https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader

/*
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth; // the width of the bitmap, in pixels.
  LONG  biHeight; // the height of the bitmap, in pixels. For uncompressed RGB bitmaps,
                  // If biHeight is positive, the bitmap is a bottom-up DIB with the origin at the lower left corner.
                  // If biHeight is negative, the bitmap is a top-down DIB with the origin at the upper left corner.
  WORD  biPlanes; // This value must be set to 1.
  WORD  biBitCount; // the number of bits per pixel (bpp). (mspaints to 24)
  DWORD biCompression; // BI_RGB = 0x0000, others at https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-wmf/4e588f70-bd92-4a6f-b77f-35d0feaf7a57
  DWORD biSizeImage; // Specifies the size, in bytes, of the image. This can be set to 0 for uncompressed RGB bitmaps.
  LONG  biXPelsPerMeter; // Specifies the horizontal resolution, in pixels per meter, of the target device for the bitmap.
  LONG  biYPelsPerMeter; // Specifies the vertical resolution, in pixels per meter, of the target device for the bitmap.
  DWORD biClrUsed; // Specifies the number of color indices in the color table that are actually used by the bitmap.
  DWORD biClrImportant; // Specifies the number of color indices that are considered important for displaying the bitmap. If this value is zero, all colors are important.
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
*/

// From https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader#calculating-surface-stride
// For uncompressed RGB formats, the minimum stride is always the image width in bytes, rounded up to the nearest DWORD. 
// You can use the following formula to calculate the stride:
#define LURDS2_BMP_PIXEL_ROW_STRIDE(infoHeader) (((((infoHeader).biWidth * (infoHeader).biBitCount) + 31) & ~31) >> 3)

typedef struct BmpData {
  int width; // in pixels
  int height; // in pixels
  unsigned int glTextureId;
  int pixelPerfect;
  int isMaskingBitmap;
} BmpData;

static int Bmp_LoadToOpenGLTexture(BmpData* bitmap, uint8_t* rgbaData);

static Bmp Bmp_LoadFromResourceFile_Internal(const wchar_t * fileName, int isMaskingBitmap)
{
  BmpData* bmp;
  bmp = malloc(sizeof(BmpData));
  if (bmp == 0)
  {
    DIAGNOSTIC_BMP_ERROR("failed to allocate memory for BmpData");
    return 0;
  }
  memset(bmp, 0, sizeof(BmpData));

  BmpHeader* data = 0;
  int fileLength = 0;

  data = (BmpHeader*)ResourceFile_Load(fileName, &fileLength);
  if (data == 0) goto error;

  if (fileLength < sizeof(BmpHeader)) {
    DIAGNOSTIC_BMP_ERROR("unexpected too-small size of bmp file");
    goto error;
  }

  if (data->fileSize != fileLength) {
    DIAGNOSTIC_BMP_ERROR("bmp header fileSize != actual bmp file size");
    goto error;
  }

  if (data->pixelDataOffset > fileLength) {
    DIAGNOSTIC_BMP_ERROR("bmp header pixelDataOffset too large");
    goto error;
  }

  if (memcmp(&data->bm, "BM", 2) != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-bm signature in bmp file");
    goto error;
  }

  if (data->headerSize != 40) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-BITMAPINFOHEADER header in bmp file");
    goto error;
  }

  if (data->infoHeader.biWidth < 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-positive biWidth in bmp file");
    goto error;
  }

  if (data->infoHeader.biWidth > 2000) {
    DIAGNOSTIC_SOUND_ERROR("unexpected > 2000 biWidth in bmp file");
    goto error;
  }

  if (data->infoHeader.biHeight < 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-positive biHeight in bmp file");
    goto error;
  }

  if (data->infoHeader.biHeight > 2000) {
    DIAGNOSTIC_SOUND_ERROR("unexpected > 2000 biHeight in bmp file");
    goto error;
  }

  if (data->infoHeader.biPlanes != 1) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biPlanes != 1 in bmp file");
    goto error;
  }

  if (data->infoHeader.biBitCount != 24) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biBitCount != 24 in bmp file");
    goto error;
  }

  if (data->infoHeader.biCompression != BI_RGB) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biCompression != BI_RGB (0) in bmp file");
    goto error;
  }

  if (data->infoHeader.biClrUsed != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biClrUsed != 0 in bmp file");
    goto error;
  }

  int rowStride;
  rowStride = LURDS2_BMP_PIXEL_ROW_STRIDE(data->infoHeader);
  if (data->pixelDataOffset + rowStride * data->infoHeader.biHeight > fileLength) {
    DIAGNOSTIC_BMP_ERROR("bmp file is not long enough to hold advertised pixel data");
    goto error;
  }

  // bitmaps store pixels in order Blue-Green-Red
  // but opengl needs Red-Green-Blue, so swap them
  char* start = (char*)data + data->pixelDataOffset;
  int rowCount = data->infoHeader.biHeight;
  int columnCount = data->infoHeader.biWidth;
  int bytesPerElement = data->infoHeader.biBitCount / 8;
  for (int row = 0; row < rowCount; row++)
  {
    char* rowStart = start + row * rowStride;
    for (int column = 0; column < columnCount; column++)
    {
      char* c = rowStart + column * bytesPerElement;
      char t = c[0];
      c[0] = c[2];
      c[2] = t;
    }
  }

  // bitmaps are 24-bit RGB but I want 32-bit RGBA so I can fake in my own transparency
  // so relloc and shift the bytes
  int oldBytesPerElement = bytesPerElement; // 3
  bytesPerElement += 1;
  int oldRowStride = rowStride; // biWidth * 3 plus some alignment fudging;
  rowStride = data->infoHeader.biWidth * bytesPerElement;
  fileLength = fileLength + (rowStride * rowCount) - (oldRowStride * rowCount);
  void* newData = realloc(data, fileLength);
  if (newData == 0) {
    DIAGNOSTIC_BMP_ERROR("failed to reallocate for RGB to RGBA expansion");
    goto error;
  }
  data = newData;
  start = (char*)data + data->pixelDataOffset;
  for (int row = rowCount - 1; row >= 0; row--)
  {
    char* oldRowStart = start + row * oldRowStride;
    char* rowStart = start + row * rowStride;
    for (int column = columnCount - 1; column >= 0; column--)
    {
      char* oldPixel = oldRowStart + column * oldBytesPerElement;
      char* newPixel = rowStart + column * bytesPerElement;
      newPixel[0] = oldPixel[0];
      newPixel[1] = oldPixel[1];
      newPixel[2] = oldPixel[2];
      newPixel[3] = 255; // make it opaque
      if (isMaskingBitmap)
      {
        // is the pixel pure white?
        if (*newPixel == 0xFFFFFFFF)
        {
          // pick a color that does not render
          *(DWORD*)newPixel = 0x00FFFFFF; // transparent white
        }
        else // pick a color that is convenient to GL_MODULATE with later runtime-specified glColor4f()
        {
          // FUTURE: there's opportunity for neat specular effect here if we would let
          // the bitmap's color data contribute to the GL_MODULATE
          *(DWORD*)newPixel = 0xFFFFFFFF; // opaque white
        }
      }
    }
  }

  // bitmaps store pixel data "last row first"
  // but I want the "top row first" because that helps my opengl commands make sense without melting the mind
  // so swap them
  char * temp = malloc(rowStride);
  if (temp == 0)
  {
    DIAGNOSTIC_BMP_ERROR("failed to allocate memory for bitmap row shuffling");
    goto error;
  }
  for (int row = 0; row < rowCount / 2; row++)
  {
    int otherRow = rowCount - row - 1;
    char* rowStart = start + row * rowStride;
    char* otherRowStart = start + otherRow * rowStride;
    
    memcpy(temp, rowStart, rowStride);
    memcpy(rowStart, otherRowStart, rowStride);
    memcpy(otherRowStart, temp, rowStride);
  }
  free(temp);

  bmp->width = data->infoHeader.biWidth;
  bmp->height = data->infoHeader.biHeight;
  bmp->isMaskingBitmap = isMaskingBitmap;
  bmp->pixelPerfect = 1;
  
  if (!Bmp_LoadToOpenGLTexture(bmp, (uint8_t*)data + data->pixelDataOffset))
  {
    goto error;
  }

  free(data);
  return bmp;

error:
  if (data != 0) free(data);
  free(bmp);
  return 0;
}

Bmp Bmp_LoadFromRgba(uint8_t* rgbaData, int width, int height)
{
  if (rgbaData == 0) {
    DIAGNOSTIC_BMP_ERROR("invalid null rgbaData param");
    return 0;
  }
  
  // aint nobody gonna make a bitmap 5000 pixels wide or tall
  if (width <= 0 || height <= 0 || width >= 5000 || height >= 5000) {
    DIAGNOSTIC_BMP_ERROR("invalid width or height param");
    return 0;
  }
  
  BmpData* bmp = malloc(sizeof(BmpData));
  if (bmp == 0)
  {
    DIAGNOSTIC_BMP_ERROR("failed to allocate memory for BmpData");
    return 0;
  }
  memset(bmp, 0, sizeof(BmpData));
  bmp->width = width;
  bmp->height = height;
  bmp->pixelPerfect = 1;

  if (!Bmp_LoadToOpenGLTexture(bmp, rgbaData))
  {
    free(bmp);
    return 0;
  }

  return bmp;
}

void Bmp_Release(Bmp bmp)
{
  BmpData* bitmap;
  bitmap = (BmpData*)bmp;

  if (!bitmap)
  {
    DIAGNOSTIC_BMP_ERROR("bmp arg is null");
    return;
  }
  
  if (bitmap->glTextureId != 0) glDeleteTextures(1, &bitmap->glTextureId);
  free(bitmap);
}

Bmp Bmp_LoadMaskingBitmapFromResourceFile(const wchar_t * fileName)
{
  return Bmp_LoadFromResourceFile_Internal(fileName, 1);
}

Bmp Bmp_LoadFromResourceFile(const wchar_t * fileName)
{
  return Bmp_LoadFromResourceFile_Internal(fileName, 0);
}

static int Bmp_LoadToOpenGLTexture(BmpData* bitmap, uint8_t* rgbaData)
{
  // TODO: use gluErrorString() in this method, from glu32.dll and glu32.lib
  glGetError(); // clear error flag
  glGenTextures(1, &bitmap->glTextureId);
  if (glGetError() != NO_ERROR)
  {
    DIAGNOSTIC_BMP_ERROR("glGenTextures() refused to produce a texture id");
    bitmap->glTextureId = 0;
    return 0;
  }

  glBindTexture(GL_TEXTURE_2D, bitmap->glTextureId);
  if (glGetError() != NO_ERROR)
  {
    DIAGNOSTIC_BMP_ERROR("glBindTexture() failed");
    glDeleteTextures(1, &bitmap->glTextureId);
    bitmap->glTextureId = 0;
    return 0;
  }
  
  glTexImage2D(
    GL_TEXTURE_2D, // target
    0, // level (has to do with mip mapping)
    GL_RGBA, // internalFormat
    bitmap->width,
    bitmap->height,
    0, // border
    GL_RGBA, // format of the passed-in data
    GL_UNSIGNED_BYTE, // type
    rgbaData);

  if (glGetError() != NO_ERROR)
  {
    DIAGNOSTIC_BMP_ERROR("glTexImage2D() failed");
    glDeleteTextures(1, &bitmap->glTextureId);
    bitmap->glTextureId = 0;
    return 0;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  
  return 1;
}

void Bmp_SetPixelPerfect(Bmp bmp, int newValue)
{
  BmpData* bitmap = (BmpData*)bmp;

  if (!bitmap) {
    DIAGNOSTIC_BMP_ERROR("bmp arg is null");
    return;
  }

  bitmap->pixelPerfect = newValue;
}

static BmpData* Bmp_DrawStart(Bmp bmp, int* oldTexEnv)
{
  BmpData* bitmap = (BmpData*)bmp;

  if (!bitmap) {
    DIAGNOSTIC_BMP_ERROR("bmp arg is null");
    return 0;
  }

  if (bitmap->glTextureId == 0) {
    DIAGNOSTIC_BMP_ERROR("bmp has not yet been loaded to opengl");
    return 0;
  }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, bitmap->glTextureId);

  // I'm grumpy I need to provide these for the bitmap to show up, but whatev okay --nathschu
  // (what are the defaults if not "something that makes the texture appear"?)
  int paramValue = bitmap->pixelPerfect ? GL_NEAREST : GL_LINEAR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, paramValue);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, paramValue);

  glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, oldTexEnv);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, bitmap->isMaskingBitmap ? GL_MODULATE : GL_REPLACE);

  // FUTURE: the texture environment color could contribute if we used GL_BLEND instead of GL_MODULATE
  //GLfloat green[] = { 1.0f, 0.0f, 0.0f, 0.0f };
  //glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &green[0]);
  
  // NOTE: when isMaskingBitmap, callers are expected to specify glColor to pick text color and transparency
  //glColor4f(0.0f, 1.0f, 0.0f, 0.5f); // green
  
  // openGL doesn't respect alpha until blending is enabled
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return bitmap;
}

static void Bmp_DrawEnd(int* oldTexEnv)
{
  glDisable(GL_BLEND);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, *oldTexEnv);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
}

void Bmp_Draw(Bmp bmp)
{
  int oldTexEnv;
  BmpData* bitmap = Bmp_DrawStart(bmp, &oldTexEnv);
  if (bitmap == 0) return;

  glBegin(GL_QUADS);
    glTexCoord2d(0, 0);
    glVertex2d(0, 0);

    glTexCoord2d(1, 0);
    glVertex2d(bitmap->width, 0);

    glTexCoord2d(1, 1);
    glVertex2d(bitmap->width, bitmap->height);

    glTexCoord2d(0, 1);
    glVertex2d(0, bitmap->height);
  glEnd();

  Bmp_DrawEnd(&oldTexEnv);
}

void Bmp_DrawPortion(Bmp bmp, int x, int y, int width, int height)
{
  int oldTexEnv;
  BmpData* bitmap = Bmp_DrawStart(bmp, &oldTexEnv);
  if (bitmap == 0) return;

  glBegin(GL_QUADS);
    float u = (float)x / (float)bitmap->width;
    float v = (float)y / (float)bitmap->height;
    float uWidth = (float)width / (float)bitmap->width;
    float vHeight = (float)height / (float)bitmap->height;

    glTexCoord2f(u, v);
    glVertex2d(0, 0);

    glTexCoord2d(u + uWidth, v);
    glVertex2d(width, 0);

    glTexCoord2d(u + uWidth, v + vHeight);
    glVertex2d(width, height);

    glTexCoord2d(u, v + vHeight);
    glVertex2d(0, height);
  glEnd();

  Bmp_DrawEnd(&oldTexEnv);
}