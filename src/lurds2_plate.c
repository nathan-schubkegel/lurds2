/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_plate.h"

#include "lurds2_errors.h"
#include <wingdi.h>
#include <GL/GL.h>

#define DIAGNOSTIC_PLATE_ERROR(message) DIAGNOSTIC_ERROR(message)
#define DIAGNOSTIC_PLATE_ERROR2(m1, m2) DIAGNOSTIC_ERROR2((m1), (m2))
#define DIAGNOSTIC_PLATE_ERROR3(m1, m2, m3) DIAGNOSTIC_ERROR3((m1), (m2), (m3))
#define DIAGNOSTIC_PLATE_ERROR4(m1, m2, m3, m4) DIAGNOSTIC_ERROR4((m1), (m2), (m3), (m4))

typedef struct PaletteFile {
  PaletteFileId id;
  const wchar_t* fileName_w;
  const char* fileName;
  const uint8_t* data;
} PaletteFile;

PaletteFile KnownPaletteFiles[] = {
  { PaletteFileId_ARMITEMS, L"ARMITEMS.256", "ARMITEMS.256", 0 },
  { PaletteFileId_ARMOURY, L"ARMOURY.256", "ARMOURY.256", 0 },
  { PaletteFileId_BACKGRND, L"BACKGRND.256", "BACKGRND.256", 0 },
  { PaletteFileId_BASE01, L"BASE01.256", "BASE01.256", 0 },
  { PaletteFileId_BASE1A, L"BASE1A.256", "BASE1A.256", 0 },
  { PaletteFileId_CAS_BACK, L"CAS_BACK.256", "CAS_BACK.256", 0 },
  { PaletteFileId_CASTLE1, L"CASTLE1.256", "CASTLE1.256", 0 },
  { PaletteFileId_CUSTOM, L"CUSTOM.256", "CUSTOM.256", 0 },
  { PaletteFileId_DEMO, L"DEMO.256", "DEMO.256", 0 },
  { PaletteFileId_DEMO1, L"DEMO1.256", "DEMO1.256", 0 },
  { PaletteFileId_DEMO2, L"DEMO2.256", "DEMO2.256", 0 },
  { PaletteFileId_GATEWAY, L"GATEWAY.256", "GATEWAY.256", 0 },
  { PaletteFileId_GRTNOBLE, L"GRTNOBLE.256", "GRTNOBLE.256", 0 },
  { PaletteFileId_LORDS2, L"LORDS2.256", "LORDS2.256", 0 },
  { PaletteFileId_MERCHANT, L"MERCHANT.256", "MERCHANT.256", 0 },
  { PaletteFileId_MISC_SEL, L"MISC_SEL.256", "MISC_SEL.256", 0 },
  { PaletteFileId_SCORE1, L"SCORE1.256", "SCORE1.256", 0 },
  { PaletteFileId_SCORE2, L"SCORE2.256", "SCORE2.256", 0 },
  { PaletteFileId_SKIRCUST, L"SKIRCUST.256", "SKIRCUST.256", 0 },
  { PaletteFileId_SKIRMISH, L"SKIRMISH.256", "SKIRMISH.256", 0 },
  { PaletteFileId_SPRITE01, L"SPRITE01.256", "SPRITE01.256", 0 },
  { PaletteFileId_SPRITE1A, L"SPRITE1A.256", "SPRITE1A.256", 0 },
  { PaletteFileId_START, L"START.256", "START.256", 0 },
  { PaletteFileId_T32_BAT1, L"T32_BAT1.256", "T32_BAT1.256", 0 },
  { PaletteFileId_T32_STN1, L"T32_STN1.256", "T32_STN1.256", 0 },
  { PaletteFileId_TITLE, L"TITLE.256", "TITLE.256", 0 },
  { PaletteFileId_TREASURY, L"TREASURY.256", "TREASURY.256", 0 },
};

const char* PaletteFile_GetName(PaletteFileId id)
{
  if (id < 0 || id >= PaletteFileId_END) {
    DIAGNOSTIC_PLATE_ERROR("invalid palette file id");
    return 0;
  }
  
  return KnownPaletteFiles[id].fileName;
}

const wchar_t* PaletteFile_GetName_w(PaletteFileId id)
{
  if (id < 0 || id >= PaletteFileId_END) {
    DIAGNOSTIC_PLATE_ERROR("invalid palette file id");
    return 0;
  }
  
  return KnownPaletteFiles[id].fileName_w;
}

// taken from https://stackoverflow.com/a/141943/2221472
static void brightenRgb(uint8_t* rgb)
{
  const double translucence_factor = 3.0f; // this is really a brightness factor; I just call it "translucense" because that's what happens when people try to whiten their teeth too much
  double r = rgb[0] * translucence_factor;
  double g = rgb[1] * translucence_factor;
  double b = rgb[2] * translucence_factor;

  const double threshold = 255.999f;
  double m = r > g ? r : g; m = m > b ? m : b; // max
  if (m > threshold)
  {
    double total = r + g + b;
    if (total >= 3 * threshold)
    {
      r = g = b = 255.0f;
    }
    else
    {
      double x = (3 * threshold - total) / (3 * m - total);
      double gray = threshold - x * m;
      r = gray + x * r;
      g = gray + x * g;
      b = gray + x * b;
      
      // guess I don't 100% trust that math yet
      r = r > 255.0f ? 255.0f : r;
      g = g > 255.0f ? 255.0f : g;
      b = b > 255.0f ? 255.0f : b;
    }
  }
  
  rgb[0] = (uint8_t)r;
  rgb[1] = (uint8_t)g;
  rgb[2] = (uint8_t)b;
}

static const uint8_t* GetPalette(PaletteFileId id)
{
  if (id < 0 || id >= PaletteFileId_END) {
    DIAGNOSTIC_PLATE_ERROR("invalid palette file id");
    return 0;
  }
  
  PaletteFile* f = &KnownPaletteFiles[id];
  if (f->data == 0)
  {
    int fileLength;
    uint8_t* data = ResourceFile_LoadLords2File(f->fileName_w, &fileLength);
    if (data == 0) return 0;
    
    if (fileLength != 256 * 3) // one byte for each of R,G,B for each of the 256 palette indexes
    {
      DIAGNOSTIC_PLATE_ERROR("invalid palette file length");
      free(data);
      return 0;
    }
    
    // Lords2 palettes are all oddly dark... so brighten them up some!
    for (int i = 0; i < 256; i++)
    {
      brightenRgb(&data[i * 3]);
    }
    
    f->data = data;
  }

  return f->data;
}

typedef enum TileDataType {
  TileDataType_BMP,
  TileDataType_ISO,
  TileDataType_RLE
} TileDataType;

typedef struct PlateTileDataTypeIndicator {
  PlateFileId id;
  const wchar_t * fileName_w;
  const char * fileName;
  PaletteFileId paletteFileId;
  TileDataType tileDataType;
} PlateTileDataTypeIndicator;

static PlateTileDataTypeIndicator KnownPlateFiles[] = {
  { PlateFileId_A2B_ARCH, L"A2B_ARCH.PL8", "A2B_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_CROS, L"A2B_CROS.PL8", "A2B_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_KNIG, L"A2B_KNIG.PL8", "A2B_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_MACE, L"A2B_MACE.PL8", "A2B_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_PEAS, L"A2B_PEAS.PL8", "A2B_PEAS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_PIKE, L"A2B_PIKE.PL8", "A2B_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_PSNT, L"A2B_PSNT.PL8", "A2B_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2B_SWOR, L"A2B_SWOR.PL8", "A2B_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_ARCH, L"A2G_ARCH.PL8", "A2G_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_CROS, L"A2G_CROS.PL8", "A2G_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_KNIG, L"A2G_KNIG.PL8", "A2G_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_MACE, L"A2G_MACE.PL8", "A2G_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_PIKE, L"A2G_PIKE.PL8", "A2G_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_PSNT, L"A2G_PSNT.PL8", "A2G_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2G_SWOR, L"A2G_SWOR.PL8", "A2G_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_ARCH, L"A2K_ARCH.PL8", "A2K_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_CROS, L"A2K_CROS.PL8", "A2K_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_KNIG, L"A2K_KNIG.PL8", "A2K_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_MACE, L"A2K_MACE.PL8", "A2K_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_PIKE, L"A2K_PIKE.PL8", "A2K_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_PSNT, L"A2K_PSNT.PL8", "A2K_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2K_SWOR, L"A2K_SWOR.PL8", "A2K_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_ARCH, L"A2P_ARCH.PL8", "A2P_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_CROS, L"A2P_CROS.PL8", "A2P_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_KNIG, L"A2P_KNIG.PL8", "A2P_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_MACE, L"A2P_MACE.PL8", "A2P_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_PIKE, L"A2P_PIKE.PL8", "A2P_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_PSNT, L"A2P_PSNT.PL8", "A2P_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2P_SWOR, L"A2P_SWOR.PL8", "A2P_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_ARCH, L"A2R_ARCH.PL8", "A2R_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_CROS, L"A2R_CROS.PL8", "A2R_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_KNIG, L"A2R_KNIG.PL8", "A2R_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_MACE, L"A2R_MACE.PL8", "A2R_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_PEAS, L"A2R_PEAS.PL8", "A2R_PEAS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_PIKE, L"A2R_PIKE.PL8", "A2R_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_PSNT, L"A2R_PSNT.PL8", "A2R_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2R_SWOR, L"A2R_SWOR.PL8", "A2R_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_ARCH, L"A2W_ARCH.PL8", "A2W_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_CROS, L"A2W_CROS.PL8", "A2W_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_KNIG, L"A2W_KNIG.PL8", "A2W_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_MACE, L"A2W_MACE.PL8", "A2W_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_PIKE, L"A2W_PIKE.PL8", "A2W_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_PSNT, L"A2W_PSNT.PL8", "A2W_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2W_SWOR, L"A2W_SWOR.PL8", "A2W_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_ARCH, L"A2Y_ARCH.PL8", "A2Y_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_CROS, L"A2Y_CROS.PL8", "A2Y_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_KNIG, L"A2Y_KNIG.PL8", "A2Y_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_MACE, L"A2Y_MACE.PL8", "A2Y_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_PIKE, L"A2Y_PIKE.PL8", "A2Y_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_PSNT, L"A2Y_PSNT.PL8", "A2Y_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2Y_SWOR, L"A2Y_SWOR.PL8", "A2Y_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2_HORSE, L"A2_HORSE.PL8", "A2_HORSE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A2_MISS, L"A2_MISS.PL8", "A2_MISS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_ARCH, L"A3B_ARCH.PL8", "A3B_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_CROS, L"A3B_CROS.PL8", "A3B_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_KNIG, L"A3B_KNIG.PL8", "A3B_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_MACE, L"A3B_MACE.PL8", "A3B_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_PIKE, L"A3B_PIKE.PL8", "A3B_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_PSNT, L"A3B_PSNT.PL8", "A3B_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3B_SWOR, L"A3B_SWOR.PL8", "A3B_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_ARCH, L"A3K_ARCH.PL8", "A3K_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_CROS, L"A3K_CROS.PL8", "A3K_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_KNIG, L"A3K_KNIG.PL8", "A3K_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_MACE, L"A3K_MACE.PL8", "A3K_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_PIKE, L"A3K_PIKE.PL8", "A3K_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_PSNT, L"A3K_PSNT.PL8", "A3K_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3K_SWOR, L"A3K_SWOR.PL8", "A3K_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_ARCH, L"A3P_ARCH.PL8", "A3P_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_CROS, L"A3P_CROS.PL8", "A3P_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_KNIG, L"A3P_KNIG.PL8", "A3P_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_MACE, L"A3P_MACE.PL8", "A3P_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_PIKE, L"A3P_PIKE.PL8", "A3P_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_PSNT, L"A3P_PSNT.PL8", "A3P_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3P_SWOR, L"A3P_SWOR.PL8", "A3P_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_ARCH, L"A3R_ARCH.PL8", "A3R_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_CROS, L"A3R_CROS.PL8", "A3R_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_KNIG, L"A3R_KNIG.PL8", "A3R_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_MACE, L"A3R_MACE.PL8", "A3R_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_PIKE, L"A3R_PIKE.PL8", "A3R_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_PSNT, L"A3R_PSNT.PL8", "A3R_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3R_SWOR, L"A3R_SWOR.PL8", "A3R_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_ARCH, L"A3W_ARCH.PL8", "A3W_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_CROS, L"A3W_CROS.PL8", "A3W_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_KNIG, L"A3W_KNIG.PL8", "A3W_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_MACE, L"A3W_MACE.PL8", "A3W_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_PIKE, L"A3W_PIKE.PL8", "A3W_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_PSNT, L"A3W_PSNT.PL8", "A3W_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3W_SWOR, L"A3W_SWOR.PL8", "A3W_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_ARCH, L"A3Y_ARCH.PL8", "A3Y_ARCH.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_CROS, L"A3Y_CROS.PL8", "A3Y_CROS.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_KNIG, L"A3Y_KNIG.PL8", "A3Y_KNIG.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_MACE, L"A3Y_MACE.PL8", "A3Y_MACE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_PIKE, L"A3Y_PIKE.PL8", "A3Y_PIKE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_PSNT, L"A3Y_PSNT.PL8", "A3Y_PSNT.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3Y_SWOR, L"A3Y_SWOR.PL8", "A3Y_SWOR.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_A3_HORSE, L"A3_HORSE.PL8", "A3_HORSE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_ARMITEMS, L"ARMITEMS.PL8", "ARMITEMS.PL8", PaletteFileId_ARMITEMS, TileDataType_BMP },
  { PlateFileId_ARMOURY, L"ARMOURY.PL8", "ARMOURY.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARMTORCH, L"ARMTORCH.PL8", "ARMTORCH.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_BOW, L"ARM_BOW.PL8", "ARM_BOW.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_CROS, L"ARM_CROS.PL8", "ARM_CROS.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_IT_B, L"ARM_IT_B.PL8", "ARM_IT_B.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_IT_K, L"ARM_IT_K.PL8", "ARM_IT_K.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_IT_P, L"ARM_IT_P.PL8", "ARM_IT_P.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_IT_R, L"ARM_IT_R.PL8", "ARM_IT_R.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_IT_Y, L"ARM_IT_Y.PL8", "ARM_IT_Y.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_MACE, L"ARM_MACE.PL8", "ARM_MACE.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_MAIL, L"ARM_MAIL.PL8", "ARM_MAIL.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_PIKE, L"ARM_PIKE.PL8", "ARM_PIKE.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_ARM_SWOR, L"ARM_SWOR.PL8", "ARM_SWOR.PL8", PaletteFileId_ARMOURY, TileDataType_BMP },
  { PlateFileId_BACKGRND, L"BACKGRND.PL8", "BACKGRND.PL8", PaletteFileId_BACKGRND, TileDataType_BMP },
  { PlateFileId_BASE01, L"BASE01.PL8", "BASE01.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_BASE1A, L"BASE1A.PL8", "BASE1A.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_BASE1B, L"BASE1B.PL8", "BASE1B.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_BASE1C, L"BASE1C.PL8", "BASE1C.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_BASE1D, L"BASE1D.PL8", "BASE1D.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_BATFIELD, L"BATFIELD.PL8", "BATFIELD.PL8", PaletteFileId_SPRITE01, TileDataType_BMP },
  { PlateFileId_CASPICS, L"CASPICS.PL8", "CASPICS.PL8", PaletteFileId_CAS_BACK, TileDataType_BMP },
  { PlateFileId_CASTLE1A, L"CASTLE1A.PL8", "CASTLE1A.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_CASTLE1B, L"CASTLE1B.PL8", "CASTLE1B.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_CASTLE1C, L"CASTLE1C.PL8", "CASTLE1C.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_CASTLE1D, L"CASTLE1D.PL8", "CASTLE1D.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_CAS_BACK, L"CAS_BACK.PL8", "CAS_BACK.PL8", PaletteFileId_CAS_BACK, TileDataType_BMP },
  { PlateFileId_CAS_BITS, L"CAS_BITS.PL8", "CAS_BITS.PL8", PaletteFileId_CAS_BACK, TileDataType_BMP },
  { PlateFileId_CATARM1, L"CATARM1.PL8", "CATARM1.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_CATARM2, L"CATARM2.PL8", "CATARM2.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_CROSSBOW, L"CROSSBOW.PL8", "CROSSBOW.PL8", PaletteFileId_TREASURY, TileDataType_BMP },
  { PlateFileId_CUSTOM, L"CUSTOM.PL8", "CUSTOM.PL8", PaletteFileId_CUSTOM, TileDataType_BMP },
  { PlateFileId_ENGINE, L"ENGINE.PL8", "ENGINE.PL8", PaletteFileId_T32_BAT1, TileDataType_RLE },
  { PlateFileId_FACES, L"FACES.PL8", "FACES.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FLAGS, L"FLAGS.PL8", "FLAGS.PL8", PaletteFileId_GRTNOBLE, TileDataType_BMP },
  { PlateFileId_FLAGS1A_BASE, L"FLAGS1A.PL8", "FLAGS1A.PL8", PaletteFileId_BASE01, TileDataType_RLE },
  { PlateFileId_FLAGS1A_SPRITE, L"FLAGS1A.PL8", "FLAGS1A.PL8", PaletteFileId_SPRITE01, TileDataType_RLE },
  { PlateFileId_FLAGS1B, L"FLAGS1B.PL8", "FLAGS1B.PL8", PaletteFileId_BASE01, TileDataType_RLE },
  { PlateFileId_FLAGS1C, L"FLAGS1C.PL8", "FLAGS1C.PL8", PaletteFileId_BASE01, TileDataType_RLE },
  { PlateFileId_FLAGS1D, L"FLAGS1D.PL8", "FLAGS1D.PL8", PaletteFileId_BASE01, TileDataType_RLE },
  { PlateFileId_FLAGS2A, L"FLAGS2A.PL8", "FLAGS2A.PL8", PaletteFileId_BASE01, TileDataType_RLE },
  { PlateFileId_FNTL2_14, L"FNTL2_14.PL8", "FNTL2_14.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FNTL2_22, L"FNTL2_22.PL8", "FNTL2_22.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FNTL2_9, L"FNTL2_9.PL8", "FNTL2_9.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FNT_8, L"FNT_8.PL8", "FNT_8.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FONT3C2, L"FONT3C2.PL8", "FONT3C2.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FONT_10, L"FONT_10.PL8", "FONT_10.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_FONT_C2, L"FONT_C2.PL8", "FONT_C2.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_GATEWAY, L"GATEWAY.PL8", "GATEWAY.PL8", PaletteFileId_GATEWAY, TileDataType_BMP },
  { PlateFileId_GRAPHS, L"GRAPHS.PL8", "GRAPHS.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_GRTNOBLE, L"GRTNOBLE.PL8", "GRTNOBLE.PL8", PaletteFileId_GRTNOBLE, TileDataType_BMP },
  { PlateFileId_HEARTH, L"HEARTH.PL8", "HEARTH.PL8", PaletteFileId_BASE01, TileDataType_RLE },
  { PlateFileId_ICONTRAD, L"ICONTRAD.PL8", "ICONTRAD.PL8", PaletteFileId_MERCHANT, TileDataType_BMP },
  { PlateFileId_ICONVILL, L"ICONVILL.PL8", "ICONVILL.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_ICON_TMP, L"ICON_TMP.PL8", "ICON_TMP.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_LORDS2, L"LORDS2.PL8", "LORDS2.PL8", PaletteFileId_LORDS2, TileDataType_BMP },
  { PlateFileId_MAP01, L"MAP01.PL8", "MAP01.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP02, L"MAP02.PL8", "MAP02.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP03, L"MAP03.PL8", "MAP03.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP04, L"MAP04.PL8", "MAP04.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP05, L"MAP05.PL8", "MAP05.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP06, L"MAP06.PL8", "MAP06.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP11, L"MAP11.PL8", "MAP11.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP12, L"MAP12.PL8", "MAP12.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP13, L"MAP13.PL8", "MAP13.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP14, L"MAP14.PL8", "MAP14.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MAP15, L"MAP15.PL8", "MAP15.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MERCHANT, L"MERCHANT.PL8", "MERCHANT.PL8", PaletteFileId_MERCHANT, TileDataType_BMP },
  { PlateFileId_MISC_BAT, L"MISC_BAT.PL8", "MISC_BAT.PL8", PaletteFileId_T32_BAT1, TileDataType_BMP },
  { PlateFileId_MISC_CTY, L"MISC_CTY.PL8", "MISC_CTY.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MISC_SEL, L"MISC_SEL.PL8", "MISC_SEL.PL8", PaletteFileId_CUSTOM, TileDataType_BMP },
  { PlateFileId_MISC_SKE, L"MISC_SKE.PL8", "MISC_SKE.PL8", PaletteFileId_SKIRMISH, TileDataType_BMP },
  { PlateFileId_MOUSE, L"MOUSE.PL8", "MOUSE.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_MTNS1A, L"MTNS1A.PL8", "MTNS1A.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_MTNS1B, L"MTNS1B.PL8", "MTNS1B.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_MTNS1C, L"MTNS1C.PL8", "MTNS1C.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_MTNS1D, L"MTNS1D.PL8", "MTNS1D.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_PANELS, L"PANELS.PL8", "PANELS.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_PANELS2, L"PANELS2.PL8", "PANELS2.PL8", PaletteFileId_GATEWAY, TileDataType_BMP },
  { PlateFileId_PEASANT, L"PEASANT.PL8", "PEASANT.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_ROADS1A, L"ROADS1A.PL8", "ROADS1A.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_ROADS1B, L"ROADS1B.PL8", "ROADS1B.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_ROADS1C, L"ROADS1C.PL8", "ROADS1C.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_ROADS1D, L"ROADS1D.PL8", "ROADS1D.PL8", PaletteFileId_BASE01, TileDataType_ISO },
  { PlateFileId_SCORE1, L"SCORE1.PL8", "SCORE1.PL8", PaletteFileId_SCORE1, TileDataType_BMP },
  { PlateFileId_SCORE2, L"SCORE2.PL8", "SCORE2.PL8", PaletteFileId_SCORE2, TileDataType_BMP },
  { PlateFileId_SGEPLANS, L"SGEPLANS.PL8", "SGEPLANS.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_SKIRCUST, L"SKIRCUST.PL8", "SKIRCUST.PL8", PaletteFileId_SKIRCUST, TileDataType_BMP },
  { PlateFileId_SKIRMISH, L"SKIRMISH.PL8", "SKIRMISH.PL8", PaletteFileId_SKIRMISH, TileDataType_BMP },
  { PlateFileId_SMITHY, L"SMITHY.PL8", "SMITHY.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_SPRITE1A, L"SPRITE1A.PL8", "SPRITE1A.PL8", PaletteFileId_SPRITE1A, TileDataType_BMP },
  { PlateFileId_SPRITE1B, L"SPRITE1B.PL8", "SPRITE1B.PL8", PaletteFileId_SPRITE1A, TileDataType_BMP },
  { PlateFileId_SPRITE2A, L"SPRITE2A.PL8", "SPRITE2A.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_SPRITE2B, L"SPRITE2B.PL8", "SPRITE2B.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_START, L"START.PL8", "START.PL8", PaletteFileId_START, TileDataType_BMP },
  { PlateFileId_STNFIELD, L"STNFIELD.PL8", "STNFIELD.PL8", PaletteFileId_GRTNOBLE, TileDataType_BMP },
  { PlateFileId_SYSTEM, L"SYSTEM.PL8", "SYSTEM.PL8", PaletteFileId_ARMITEMS, TileDataType_BMP },
  { PlateFileId_SYSTEM2, L"SYSTEM2.PL8", "SYSTEM2.PL8", PaletteFileId_GATEWAY, TileDataType_BMP },
  { PlateFileId_T32_BAT, L"T32_BAT.PL8", "T32_BAT.PL8", PaletteFileId_T32_STN1, TileDataType_BMP },
  { PlateFileId_T32_BAT1, L"T32_BAT1.PL8", "T32_BAT1.PL8", PaletteFileId_T32_BAT1, TileDataType_BMP },
  { PlateFileId_T32_STN1, L"T32_STN1.PL8", "T32_STN1.PL8", PaletteFileId_T32_STN1, TileDataType_BMP },
  { PlateFileId_T32_STN2, L"T32_STN2.PL8", "T32_STN2.PL8", PaletteFileId_T32_STN1, TileDataType_BMP },
  { PlateFileId_T32_STNA, L"T32_STNA.PL8", "T32_STNA.PL8", PaletteFileId_T32_STN1, TileDataType_BMP },
  { PlateFileId_T32_WOD1, L"T32_WOD1.PL8", "T32_WOD1.PL8", PaletteFileId_T32_STN1, TileDataType_BMP },
  { PlateFileId_T32_WOD2, L"T32_WOD2.PL8", "T32_WOD2.PL8", PaletteFileId_T32_STN1, TileDataType_BMP },
  { PlateFileId_TOWN1A, L"TOWN1A.PL8", "TOWN1A.PL8", PaletteFileId_BASE1A, TileDataType_ISO },
  { PlateFileId_TOWN1B, L"TOWN1B.PL8", "TOWN1B.PL8", PaletteFileId_BASE1A, TileDataType_ISO },
  { PlateFileId_TOWN1C, L"TOWN1C.PL8", "TOWN1C.PL8", PaletteFileId_BASE1A, TileDataType_ISO },
  { PlateFileId_TOWN1D, L"TOWN1D.PL8", "TOWN1D.PL8", PaletteFileId_BASE1A, TileDataType_ISO },
  { PlateFileId_TREASURY, L"TREASURY.PL8", "TREASURY.PL8", PaletteFileId_TREASURY, TileDataType_BMP },
  { PlateFileId_TROOP1, L"TROOP1.PL8", "TROOP1.PL8", PaletteFileId_ARMITEMS, TileDataType_BMP },
  { PlateFileId_TRP_AR_B, L"TRP_AR_B.PL8", "TRP_AR_B.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_AR_K, L"TRP_AR_K.PL8", "TRP_AR_K.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_AR_P, L"TRP_AR_P.PL8", "TRP_AR_P.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_AR_R, L"TRP_AR_R.PL8", "TRP_AR_R.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_AR_Y, L"TRP_AR_Y.PL8", "TRP_AR_Y.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_KN_B, L"TRP_KN_B.PL8", "TRP_KN_B.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_KN_K, L"TRP_KN_K.PL8", "TRP_KN_K.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_KN_P, L"TRP_KN_P.PL8", "TRP_KN_P.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_KN_R, L"TRP_KN_R.PL8", "TRP_KN_R.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_KN_Y, L"TRP_KN_Y.PL8", "TRP_KN_Y.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_MA_B, L"TRP_MA_B.PL8", "TRP_MA_B.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_MA_K, L"TRP_MA_K.PL8", "TRP_MA_K.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_MA_P, L"TRP_MA_P.PL8", "TRP_MA_P.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_MA_R, L"TRP_MA_R.PL8", "TRP_MA_R.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_MA_Y, L"TRP_MA_Y.PL8", "TRP_MA_Y.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_PI_B, L"TRP_PI_B.PL8", "TRP_PI_B.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_PI_K, L"TRP_PI_K.PL8", "TRP_PI_K.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_PI_P, L"TRP_PI_P.PL8", "TRP_PI_P.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_PI_R, L"TRP_PI_R.PL8", "TRP_PI_R.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_PI_Y, L"TRP_PI_Y.PL8", "TRP_PI_Y.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_SW_B, L"TRP_SW_B.PL8", "TRP_SW_B.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_SW_K, L"TRP_SW_K.PL8", "TRP_SW_K.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_SW_P, L"TRP_SW_P.PL8", "TRP_SW_P.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_SW_R, L"TRP_SW_R.PL8", "TRP_SW_R.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_SW_Y, L"TRP_SW_Y.PL8", "TRP_SW_Y.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_XB_B, L"TRP_XB_B.PL8", "TRP_XB_B.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_XB_K, L"TRP_XB_K.PL8", "TRP_XB_K.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_XB_P, L"TRP_XB_P.PL8", "TRP_XB_P.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_XB_R, L"TRP_XB_R.PL8", "TRP_XB_R.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_TRP_XB_Y, L"TRP_XB_Y.PL8", "TRP_XB_Y.PL8", PaletteFileId_ARMITEMS, TileDataType_RLE },
  { PlateFileId_VILL, L"VILL.PL8", "VILL.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLAGE, L"VILLAGE.PL8", "VILLAGE.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLAGE3, L"VILLAGE3.PL8", "VILLAGE3.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLAGE4, L"VILLAGE4.PL8", "VILLAGE4.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLANI1, L"VILLANI1.PL8", "VILLANI1.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLANI2, L"VILLANI2.PL8", "VILLANI2.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLBORD, L"VILLBORD.PL8", "VILLBORD.PL8", PaletteFileId_BASE01, TileDataType_BMP },
  { PlateFileId_VILLTOPS, L"VILLTOPS.PL8", "VILLTOPS.PL8", PaletteFileId_BASE01, TileDataType_BMP },
};

const char* PlateFile_GetName(PlateFileId id)
{
  if (id < 0 || id >= PlateFileId_END) {
    DIAGNOSTIC_PLATE_ERROR("invalid Plate file id");
    return 0;
  }
  
  return KnownPlateFiles[id].fileName;
}

const wchar_t* PlateFile_GetName_w(PlateFileId id)
{
  if (id < 0 || id >= PlateFileId_END) {
    DIAGNOSTIC_PLATE_ERROR("invalid Plate file id");
    return 0;
  }
  
  return KnownPlateFiles[id].fileName_w;
}

int PlateFile_IsSupported(PlateFileId id)
{
  if (id < 0 || id >= PlateFileId_END) {
    DIAGNOSTIC_PLATE_ERROR("invalid Plate file id");
    return 0;
  }
  
  return !(KnownPlateFiles[id].tileDataType == TileDataType_BMP ||
    KnownPlateFiles[id].tileDataType == TileDataType_RLE);
}

typedef struct __attribute__((packed)) PlateHeader {
  uint16_t unknown1;
  uint16_t numTiles;
  uint32_t unknown2;
} PlateHeader;

typedef struct __attribute__((packed)) TileHeader {
  uint16_t width;
  uint16_t height;
  uint32_t offset;
  uint16_t x;
  uint16_t y;
  uint8_t extraType;
  uint8_t extraRows;
  uint8_t unknown1; // always 0?
  uint8_t unknown2; // always 0 except in FONT3C2.PL8 and FNTL2_22.PL8?
} TileHeader;

Bmp* Plate_LoadFromFileWithCustomPalette(PlateFileId id, PaletteFileId customPalette)
{
  if (id < 0 || id >= PlateFileId_END)
  {
    DIAGNOSTIC_PLATE_ERROR("invalid plate file id");
    return 0;
  }
  
  if ((customPalette < 0 || customPalette >= PaletteFileId_END) && customPalette != PaletteFileId_NONE)
  {
    DIAGNOSTIC_PLATE_ERROR("invalid customPalette arg");
    return 0;
  }

  Bmp* bitmaps = 0;
  PlateHeader* data = 0;
  int fileLength = 0;

  data = (PlateHeader*)ResourceFile_LoadLords2File(KnownPlateFiles[id].fileName_w, &fileLength);
  if (data == 0) goto error;

  if (fileLength < sizeof(PlateHeader)) {
    DIAGNOSTIC_PLATE_ERROR2("unexpected too-small size of plate file", KnownPlateFiles[id].fileName);
    goto error;
  }
  
  if (data->numTiles > 5000) {
    DIAGNOSTIC_PLATE_ERROR2("unexpected too-large numTiles in plate file ", KnownPlateFiles[id].fileName);
    goto error;
  }

  bitmaps = malloc(sizeof(Bmp) * (data->numTiles + 1));
  if (bitmaps == 0) {
    DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for bitmaps in plate file ", KnownPlateFiles[id].fileName);
    goto error;
  }
  memset(bitmaps, 0, sizeof(Bmp) * (data->numTiles + 1));

  // load a Bmp for every tile
  // (TODO: I might need to be more clever and load them all into a single Bmp, but we'll do that when it becomes obviously necessary)
  uint8_t* start = (uint8_t*)data;
  uint8_t* current = start + sizeof(PlateHeader);
  uint8_t* end = start + fileLength;
  for (int i = 0; i < data->numTiles; i++, current += sizeof(TileHeader))
  {
    if (current + sizeof(TileHeader) > end) {
      DIAGNOSTIC_PLATE_ERROR2("invalid plate file data in ", KnownPlateFiles[id].fileName);
      goto error;
    }

    TileHeader* t = (TileHeader*)current;

    if (start + t->offset > end) {
      DIAGNOSTIC_PLATE_ERROR2("invalid plate file data in ", KnownPlateFiles[id].fileName);
      goto error;
    }
    
    if (t->width == 0 || t->height == 0) {
      DIAGNOSTIC_PLATE_ERROR2("invalid width/height of plate ", KnownPlateFiles[id].fileName);
      goto error;
    }

    // the palette contains the RGB values to use for each of the available 256 palette indexes
    const uint8_t* palette = GetPalette(customPalette == PaletteFileId_NONE ? KnownPlateFiles[id].paletteFileId : customPalette);
    if (palette == 0) goto error;

    switch (KnownPlateFiles[id].tileDataType)
    {
      case TileDataType_BMP:
      {
        // each item in 'paletteNumbers' is a 0-255 index in the palette
        uint8_t* paletteNumbers = start + t->offset;
        if (paletteNumbers + t->width * t->height > end) {
          DIAGNOSTIC_PLATE_ERROR2("invalid plate file data in ", KnownPlateFiles[id].fileName);
          goto error;
        }

        // allocate space for RGBA for each pixel
        uint8_t* rgbaData = malloc(t->height * t->width * 4);
        if (rgbaData == 0) {
          DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for rgbaData for plate ", KnownPlateFiles[id].fileName);
          goto error;
        }

        for (int h = 0; h < t->height; h++)
        {
          int wStart = h * t->width * 4;
          for (int w = 0; w < t->width; w++)
          {
            int p = wStart + w * 4;
            int index = paletteNumbers[h * t->width + w] * 3;
            if (index == 0)
            {
              *(uint32_t*)&rgbaData[p] = 0x00FFFFFF; // transparent white
            }
            else
            {
              rgbaData[p]     = palette[index]; // R
              rgbaData[p + 1] = palette[index + 1]; // G
              rgbaData[p + 2] = palette[index + 2];  // B
              rgbaData[p + 3] = 0xFF; // A - opaque
            }
          }
        }

        Bmp bitmap = Bmp_LoadFromRgba(rgbaData, t->width, t->height);
        free(rgbaData);
        if (bitmap == 0) goto error;
        bitmaps[i] = bitmap;
      }
      break;

      case TileDataType_RLE:
      {
        // allocate space for RGBA for each pixel
        uint8_t* rgbaData = malloc(t->height * t->width * 4);
        if (rgbaData == 0) {
          DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for rgbaData for plate ", KnownPlateFiles[id].fileName);
          goto error;
        }
        memset(rgbaData, 0, t->height * t->width * 4);
        
#define CHECK_PAST_END(b) \
  if ((b) >= end) { \
    DIAGNOSTIC_PLATE_ERROR2("insufficient/invalid data in plate ", KnownPlateFiles[id].fileName); \
    free(rgbaData); \
    goto error; \
  }

        uint8_t* b = start + t->offset;
        for (int h = 0; h < t->height; h++)
        {
          int wStart = h * t->width * 4;
          for (int w = 0; w < t->width; )
          {
            CHECK_PAST_END(b);
            uint8_t numOpaquePixels = *(b++);
            if (numOpaquePixels == 0)
            {
              CHECK_PAST_END(b);
              int numTransparentPixels = *(b++);
              w += numTransparentPixels;
            }
            else
            {
              CHECK_PAST_END(b + numOpaquePixels - 1);
              if (w + numOpaquePixels - 1 >= t->width)
              {
                DIAGNOSTIC_PLATE_ERROR2("invalid aggregate cell count in a row in RLE plate ", KnownPlateFiles[id].fileName);
                free(rgbaData);
                goto error;
              }
              
              for (int z = 0; z < numOpaquePixels; z++)
              {
                int p = wStart + w * 4;
                int index = *(b++) * 3;
                rgbaData[p]     = palette[index]; // R
                rgbaData[p + 1] = palette[index + 1]; // G
                rgbaData[p + 2] = palette[index + 2];  // B
                rgbaData[p + 3] = 0xFF; // A - opaque
                
                w++;
              }
            }
          }
        }
        
        Bmp bitmap = Bmp_LoadFromRgba(rgbaData, t->width, t->height);
        free(rgbaData);
        if (bitmap == 0) goto error;
        bitmaps[i] = bitmap;
      }
      break;
      
      case TileDataType_ISO:
      {
        // see below for these requirements
        if (t->height < 30 || t->width < 58)
        {
          DIAGNOSTIC_PLATE_ERROR2("invalid height/width for plate ", KnownPlateFiles[id].fileName);
          goto error;
        }
        
        // allocate space for final RGBA data
        int finalRgbaDataLength = 64 * 64 * 4; // I guess these things are always 64 wide, 64 tall
        uint8_t* finalRgbaData = malloc(finalRgbaDataLength);
        if (finalRgbaData == 0) {
          DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for finalRgbaData for plate ", KnownPlateFiles[id].fileName);
          goto error;
        }
        memset(finalRgbaData, 0, finalRgbaDataLength);

        // allocate space for first-pass RGBA for each pixel
        int rgbaDataLength = t->height * t->width * 4;
        uint8_t* rgbaData = malloc(rgbaDataLength);
        if (rgbaData == 0) {
          DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for rgbaData for plate ", KnownPlateFiles[id].fileName);
          free(finalRgbaData);
          goto error;
        }
        memset(rgbaData, 0, t->height * t->width * 4);
        
        // this gets used later
        int extraHeight = 0;
        int extraLength = 0;
        uint8_t * extra = 0;
        
#define CHECK_PAST_RGBA_LENGTH(p) \
  if ((p) >= rgbaDataLength) { \
    DIAGNOSTIC_PLATE_ERROR2("insufficient/invalid data in plate ", KnownPlateFiles[id].fileName); \
    free(rgbaData); \
    free(finalRgbaData); \
    goto error; \
  }
        
        // upper part of the tile
        uint8_t* b = start + t->offset;
        for (int h = 0; h < 15; h++)
        {
          int wStart = h * t->width * 4;
          for (int w = 0; w < (h*4)+2; w++)
          {
            int ww = ((14-h)*2 + w) * 4;
            int p = wStart + ww;
            CHECK_PAST_RGBA_LENGTH(p);

            CHECK_PAST_END(b);
            int index = *(b++) * 3;
            rgbaData[p]     = palette[index]; // R
            rgbaData[p + 1] = palette[index + 1]; // G
            rgbaData[p + 2] = palette[index + 2];  // B
            rgbaData[p + 3] = 0xFF; // A - opaque
          }
        }
        
        // lower part of the tile
        for (int h = 0; h < 15; h++)
        {
          int hh = h + 15;
          int wStart = hh * t->width * 4;
          for (int w = 0; w<((14-h)*4)+2; w++)
          {
            int ww = (h*2 + w) * 4;
            int p = wStart + ww;
            CHECK_PAST_RGBA_LENGTH(p);
            
            CHECK_PAST_END(b);
            int index = *(b++) * 3;
            rgbaData[p]     = palette[index]; // R
            rgbaData[p + 1] = palette[index + 1]; // G
            rgbaData[p + 2] = palette[index + 2];  // B
            rgbaData[p + 3] = 0xFF; // A - opaque
          }
        }
        
        if (t->extraType != 1)
        {
          // read extra rows
          int leftOffset = 0;
          int rightOffset = t->width;
          
          if (t->extraType == 3) { // left only
            rightOffset = t->width/2 + 1;
          } else if (t->extraType == 4) { // right only
            leftOffset = t->width/2 - 1;
          }
          
          int halfHeight = t->height >> 1;
          int halfWidth = t->width >> 1;
          extraHeight = t->extraRows + halfHeight;
          extraLength = extraHeight * t->width * 4;
          extra = malloc(extraLength);
          if (extra == 0)
          {
            DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for extra for plate ", KnownPlateFiles[id].fileName);
            free(rgbaData);
            free(finalRgbaData);
            goto error;
          }
          memset(extra, 0, extraLength);
          
          uint8_t * row = malloc(t->width);
          if (row == 0)
          {
            DIAGNOSTIC_PLATE_ERROR2("failed to allocate memory for extra row for plate ", KnownPlateFiles[id].fileName);
            free(rgbaData);
            free(finalRgbaData);
            free(extra);
            goto error;
          }
          memset(row, 0, t->width);
          
#define CHECK_PAST_EXTRA_END(b) \
  if ((b) >= end) { \
    DIAGNOSTIC_PLATE_ERROR2("insufficient/invalid data in plate ", KnownPlateFiles[id].fileName); \
    free(rgbaData); \
    free(finalRgbaData); \
    free(extra); \
    free(row); \
    goto error; \
  }
  
#define CHECK_PAST_EXTRA_LENGTH(p) \
  if ((p) >= extraLength) { \
    DIAGNOSTIC_PLATE_ERROR2("insufficient/invalid data in plate ", KnownPlateFiles[id].fileName); \
    free(rgbaData); \
    free(finalRgbaData); \
    free(extra); \
    free(row); \
    goto error; \
  }

          for (int h = 0; h < t->extraRows; h++)
          {
            memset(row, 0, t->width);
            for (int z = leftOffset; z < rightOffset; z++)
            {
              CHECK_PAST_EXTRA_END(b);
              row[z] = *(b++);
            }

            for (int w = leftOffset; w < rightOffset; w++)
            {
              int yPos = t->extraRows - h;
              
              if (w <= halfWidth)
              {
                yPos += (halfHeight-1) - (w/2);
              } 
              else
              {
                yPos += (w/2) - (halfHeight-1);
              }
              
              int index = row[w] * 3;
              
              if (index != 0)
              {
                int wStart = yPos * t->width * 4;
                int p = wStart + w * 4;
                CHECK_PAST_EXTRA_LENGTH(p);
                extra[p]     = palette[index]; // R
                extra[p + 1] = palette[index + 1]; // G
                extra[p + 2] = palette[index + 2];  // B
                extra[p + 3] = 0xFF; // A - opaque
              }
            }
          }
          
          free(row);
        }
        
        // remember that finalRgbaData has height=64, width=64
        for (int j = 0; j < 30; j++)
        {
          for (int k = 0; k < 58 * 4; k++)
          {
            finalRgbaData[(j+34) * 64 * 4 + k] = rgbaData[j * t->width + k]; // NOTE: not bounds-checking rgbaData[] accessor because j=30, k=58 were checked 2 pages up
          }
        }

        if (t->extraType != 1 && t->extraType != 0)
        {
          for (int j = 0; j < extraHeight; j++)
          {
            for (int k = 0; k < 58*4; k+=4)
            {
              if (extra[j * t->width * 4 + k + 3] != 0)
              {
                for (int m = 0; m < 4; m++)
                {
                  finalRgbaData[(j + 33 - t->extraRows) * 64 + k + m] = extra[j * t->width + k + m];
                }
              }
            }
          }
        }
        
        // NOTE: original code added to tileset at t->y - 34
        free(rgbaData);
        free(extra);
        Bmp bitmap = Bmp_LoadFromRgba(finalRgbaData, 64, 64);
        free(finalRgbaData);
        if (bitmap == 0) goto error;
        bitmaps[i] = bitmap;
      }
      break;

      default:
      {
        DIAGNOSTIC_PLATE_ERROR2("invalid tile data type for ", KnownPlateFiles[id].fileName);
        goto error;
      }
      break;
    }
  }

  free(data);
  return bitmaps;

error:
  if (data != 0) free(data);
  if (bitmaps != 0)
  {
    Plate_Release(bitmaps);
  }
  return 0;
}

Bmp* Plate_LoadFromFile(PlateFileId id)
{
  return Plate_LoadFromFileWithCustomPalette(id, PaletteFileId_NONE);
}

void Plate_Release(Bmp* bitmaps)
{
  if (bitmaps == 0)
  {
    DIAGNOSTIC_PLATE_ERROR("invalid null bitmaps arg");
    return;
  }

  // free any bitmaps allocated so far
  Bmp* bitmapsIterator = bitmaps;
  while (*bitmapsIterator != 0)
  {
    Bmp_Release(*bitmapsIterator);
    bitmapsIterator++;
  }
  
  // then free the bitmaps
  free(bitmaps);
}