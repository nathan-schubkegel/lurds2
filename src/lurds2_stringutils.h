/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_STRINGUTILS_H
#define LURDS2_STRINGUTILS_H

char* StringUtils_MakeNarrowString(const wchar_t* input);
wchar_t* StringUtils_MakeWideString(const char* input);

#endif