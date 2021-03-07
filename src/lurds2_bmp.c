/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_bmp.h"

#include "lurds2_errors.h"
#include <wingdi.h>

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

  int rowStride = LURDS2_BMP_PIXEL_ROW_STRIDE(bmp->data->infoHeader);
  if (bmp->data->pixelDataOffset + rowStride * bmp->data->infoHeader.biHeight > bmp->length) {
    DIAGNOSTIC_BMP_ERROR("bmp file is not long enough to hold advertised pixel data");
    goto error;
  }

  return bmp;

error:
  if (bmp->data != 0) free(bmp->data);
  free(bmp);
  return 0;
}

void* Bmp_GetPixelData(Bmp bmp)
{
  BmpData* bitmap;
  bitmap = (BmpData*)bmp;

  if (!bitmap)
  {
    DIAGNOSTIC_BMP_ERROR("bmp arg is null");
    return;
  }

  return ((char*)bitmap->data) + bitmap->data->pixelDataOffset;
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
  
  if (bitmap->data != 0) free(bitmap->data);
  free(bitmap);
}
