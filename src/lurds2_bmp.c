/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_bmp.h"

#include "lurds2_errors.h"
#include <wingdi.h>
#include <GL/GL.h>

#define DIAGNOSTIC_BMP_ERROR(message) DIAGNOSTIC_ERROR(message);
#define DIAGNOSTIC_BMP_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2));
#define DIAGNOSTIC_BMP_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3));
#define DIAGNOSTIC_BMP_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4));

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
  BmpHeader* data;
  int length;
  unsigned int glTextureId;
} BmpData;

Bmp Bmp_LoadFromResourceFile(const wchar_t * fileName)
{
  BmpData* bmp;
  bmp = malloc(sizeof(BmpData));
  if (bmp == 0)
  {
    DIAGNOSTIC_BMP_ERROR("failed to allocate memory for BmpData");
    return 0;
  }
  memset(bmp, 0, sizeof(BmpData));

  bmp->data = (BmpHeader*)ResourceFile_Load(fileName, &bmp->length);
  if (bmp->data == 0) goto error;

  if (bmp->length < sizeof(BmpHeader)) {
    DIAGNOSTIC_BMP_ERROR("unexpected too-small size of bmp file");
    goto error;
  }

  if (bmp->data->fileSize != bmp->length) {
    DIAGNOSTIC_BMP_ERROR("bmp header fileSize != actual bmp file size");
    goto error;
  }

  if (bmp->data->pixelDataOffset > bmp->length) {
    DIAGNOSTIC_BMP_ERROR("bmp header pixelDataOffset too large");
    goto error;
  }

  if (memcmp(&bmp->data->bm, "BM", 2) != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-bm signature in bmp file");
    goto error;
  }

  if (bmp->data->headerSize != 40) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-BITMAPINFOHEADER header in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biWidth < 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-positive biWidth in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biWidth > 2000) {
    DIAGNOSTIC_SOUND_ERROR("unexpected > 2000 biWidth in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biHeight < 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected non-positive biHeight in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biHeight > 2000) {
    DIAGNOSTIC_SOUND_ERROR("unexpected > 2000 biHeight in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biPlanes != 1) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biPlanes != 1 in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biBitCount != 24) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biBitCount != 24 in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biCompression != BI_RGB) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biCompression != BI_RGB (0) in bmp file");
    goto error;
  }

  if (bmp->data->infoHeader.biClrUsed != 0) {
    DIAGNOSTIC_SOUND_ERROR("unexpected biClrUsed != 0 in bmp file");
    goto error;
  }

  int rowStride;
  rowStride = LURDS2_BMP_PIXEL_ROW_STRIDE(bmp->data->infoHeader);
  if (bmp->data->pixelDataOffset + rowStride * bmp->data->infoHeader.biHeight > bmp->length) {
    DIAGNOSTIC_BMP_ERROR("bmp file is not long enough to hold advertised pixel data");
    goto error;
  }
  
  // bitmaps store pixels in order Blue-Green-Red
  // but opengl needs Red-Green-Blue, so swap them
  char* start = (char*)bmp->data + bmp->data->pixelDataOffset;
  int rowCount = bmp->data->infoHeader.biHeight;
  int bytesPerElement = bmp->data->infoHeader.biBitCount / 8;
  for (int row = 0; row < rowCount; row++)
  {
    char* rowStart = start + row * rowStride;
    int columnCount = bmp->data->infoHeader.biWidth;
    for (int column = 0; column < columnCount; column++)
    {
      char* c = rowStart + column * bytesPerElement;
      char t = c[0];
      c[0] = c[2];
      c[2] = t;
    }
  }
  
  return bmp;

error:
  if (bmp->data != 0) free(bmp->data);
  free(bmp);
  return 0;
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
  if (bitmap->data != 0) free(bitmap->data);
  free(bitmap);
}

int Bmp_LoadToOpenGL(Bmp bmp)
{
  // TODO: use gluErrorString() in this method, from glu32.dll and glu32.lib
  BmpData* bitmap;
  bitmap = (BmpData*)bmp;

  if (!bitmap) {
    DIAGNOSTIC_BMP_ERROR("bmp arg is null");
    return 0;
  }

  if (bitmap->glTextureId != 0) {
    DIAGNOSTIC_BMP_ERROR("bmp has already been loaded to opengl");
    return 0;
  }
  
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
    return 0;
  }
  
  int rowStride;
  rowStride = LURDS2_BMP_PIXEL_ROW_STRIDE(bitmap->data->infoHeader);
  if (glGetError() != NO_ERROR)
  {
    DIAGNOSTIC_BMP_ERROR("glPixelStorei() failed");
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
  }

  glTexImage2D(
    GL_TEXTURE_2D, // target
    0, // level (has to do with mip mapping)
    GL_RGBA, // internalFormat
    bitmap->data->infoHeader.biWidth,
    bitmap->data->infoHeader.biHeight,
    0, // border
    GL_RGB, // format
    GL_UNSIGNED_BYTE, // type
    ((char*)bitmap->data) + bitmap->data->pixelDataOffset);

  if (glGetError() != NO_ERROR)
  {
    DIAGNOSTIC_BMP_ERROR("glTexImage2D() failed");
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
  }
  
  // I'm grumpy I need to provide these for the bitmap to show up, but whatev okay --nathschu
  // (what are the defaults if not "something that makes the texture appear"?)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  
  glBindTexture(GL_TEXTURE_2D, 0);
  
  // TODO: theoretically could dump the texture data now that it's been loaded to opengl?
  
  return 1;
}

void Bmp_Draw(Bmp bmp)
{
  BmpData* bitmap;
  bitmap = (BmpData*)bmp;

  if (!bitmap) {
    DIAGNOSTIC_BMP_ERROR("bmp arg is null");
    return;
  }

  if (bitmap->glTextureId == 0) {
    DIAGNOSTIC_BMP_ERROR("bmp has not yet been loaded to opengl");
    return;
  }
  
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, bitmap->glTextureId);
  
  int oldTexEnv;
  glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &oldTexEnv);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glBegin(GL_QUADS);
    // NOTE: not sure whether it's rendering upside-down because my projection matrix is dumb
    // or because my bitmap data is backwards from how opengl naturally renders... but I'm
    // choosing to "fix" the problem via vertically-reversed texture coordinates
    glTexCoord2d(0, 1);
    glVertex2d(0, 0);

    glTexCoord2d(1, 1);
    glVertex2d(bitmap->data->infoHeader.biWidth, 0);

    glTexCoord2d(1, 0);
    glVertex2d(bitmap->data->infoHeader.biWidth, bitmap->data->infoHeader.biHeight);

    glTexCoord2d(0, 0);
    glVertex2d(0, bitmap->data->infoHeader.biHeight);
  glEnd();

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, oldTexEnv);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
}