/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_stringutils.h"

#include <wchar.h>

char* StringUtils_MakeNarrowString(const wchar_t* input)
{
  int len = input == 0 ? 0 : wcslen(input);
  char* result = malloc(len + 1);
  if (result != 0)
  {
    for (int i = 0; i <= len; i++)
    {
      result[i] = (char)input[i];
    }
  }
  return result;
}

wchar_t* StringUtils_MakeWideString(const char* input)
{
  int len = input == 0 ? 0 : strlen(input);
  wchar_t* result = malloc((len + 1) * sizeof(wchar_t));
  if (result != 0)
  {
    for (int i = 0; i <= len; i++)
    {
      result[i] = input[i];
    }
  }
  return result;
}
