/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_font.h"

#include "lurds2_errors.h"

static FontCharacter oldTimeyFontChars[128] = 
{
  // the first 31 chars are not printable
  {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {},
  
  { 0, 0, 14, 0, 0}, // space
  { 2, 100, 8, 18, 3 }, // !
  { 14, 100, 7, 19, 0 }, // "
  { 0, 0, 14, 0, 0 }, // #
  { 0, 0, 14, 0, 0 }, // $
  { 27, 100, 25, 15, 0 }, // %
  { 0, 0, 14, 0, 0 }, // &
  { 155, 100, 5, 17, 0 }, // '
  { 69, 100, 7, 21, 3 }, // (
  { 81, 100, 8, 21, 3 }, // )
  { 54, 100, 12, 15, 0 }, // *
  { 106, 100, 12, 14, 0 }, // +
  { 208, 100, 6, 5, 3 }, // ,
  { 94, 100, 9, 12, 0 }, // -
  { 219, 100, 8, 3, 0 }, // .
  { 194, 100, 11, 15, 5 }, // /
  { 172, 73, 15, 19, 0 }, // 0
  { 3, 73, 12, 19, 0 }, // 1
  { 20, 73, 14, 19, 0 }, // 2
  { 39, 73, 13, 19, 0 }, // 3
  { 58, 73, 15, 19, 0 }, // 4
  { 77, 73, 14, 19, 0 }, // 5
  { 96, 73, 15, 19, 0 }, // 6
  { 117, 73, 12, 19, 0 }, // 7
  { 134, 73, 14, 19, 0 }, // 8
  { 152, 73, 16, 19, 2 }, // 9
  { 132, 100, 8, 15, 0 }, // :
  { 142, 100, 9, 15, 3 }, // ;
  { 0, 0, 14, 0, 0 }, // <
  { 122, 100, 6, 12, 0 }, // =
  { 0, 0, 14, 0, 0 }, // >
  { 164, 100, 11, 16, 4 }, // ?
  { 0, 0, 14, 0, 0 }, // @
  { 0, 48, 23, 19, 3 }, // A
  { 25, 48, 22, 19, 3 }, // B
  { 49, 48, 18, 19, 0 }, // C
  { 71, 48, 21, 19, 3 }, // D
  { 94, 48, 22, 19, 3 }, // E
  { 118, 48, 24, 19, 3 }, // F
  { 144, 48, 20, 19, 0 }, // G
  { 166, 48, 23, 19, 3 }, // H
  { 192, 48, 20, 19, 3 }, // I
  { 214, 48, 15, 19, 3 }, // J
  { 231, 48, 23, 19, 3 }, // K
  { 171, 127, 21, 19, 3 }, // L
  { 194, 127, 28, 19, 3 }, // M
  { 226, 127, 24, 19, 3 }, // N
  { 0, 155, 20, 19, 0 }, // O
  { 22, 155, 21, 19, 3 }, // P
  { 46, 155, 21, 19, 3 }, // Q
  { 71, 155, 23, 19, 3 }, // R
  { 99, 155, 23, 19, 3 }, // S
  { 124, 155, 20, 19, 0 }, // T
  { 148, 155, 22, 19, 3 }, // U
  { 174, 155, 21, 19, 3 }, // V
  { 198, 155, 27, 19, 3 }, // W
  { 189, 74, 20, 20, 0 }, // X
  { 210, 71, 21, 17, 5 }, // Y
  { 235, 73, 18, 18, 0 }, // Z
  { 0, 0, 14, 0, 0 }, // [
  { 180, 100, 11, 15, 5 }, // \ backslash
  { 0, 0, 14, 0, 0 }, // ]
  { 0, 0, 14, 0, 0 }, // ^
  { 0, 0, 14, 0, 0 }, // _
  { 0, 0, 14, 0, 0 }, // `
  { 0, 18, 14, 13, 0 }, // a
  { 16, 18, 14, 18, 0 }, // b
  { 33, 18, 11, 13, 0 }, // c
  { 46, 18, 15, 18, 0 }, // d
  { 63, 18, 10, 13, 0 }, // e
  { 77, 18, 10, 18, 0 }, // f
  { 89, 18, 14, 13, 8 }, // g
  { 106, 18, 15, 18, 8 }, // h
  { 123, 18, 9, 17, 0 }, // i
  { 134, 18, 9, 17, 8 }, // j
  { 146, 18, 14, 18, 0 }, // k
  { 162, 18, 9, 18, 0 }, // l
  { 173, 18, 23, 13, 0 }, // m
  { 198, 18, 17, 13, 0 }, // n
  { 218, 18, 14, 13, 0 }, // o
  { 235, 18, 15, 13, 4 }, // p
  { 0, 125, 15, 13, 4 }, // q
  { 17, 125, 12, 13, 0 }, // r
  { 33, 125, 13, 13, 0 }, // s
  { 49, 125, 9, 17, 0 }, // t
  { 60, 125, 17, 13, 0 }, // u
  { 79, 125, 15, 13, 0 }, // v
  { 96, 125, 22 13, 0 }, // w
  { 121, 125, 14, 13, 0 }, // x
  { 138, 125, 15, 13, 7 }, // y
  { 155, 125, 12, 13, 0 }, // z
  { 0, 0, 14, 0, 0 }, // {
  { 0, 0, 14, 0, 0 }, // |
  { 0, 0, 14, 0, 0 }, // }
  { 0, 0, 14, 0, 0 }, // ~
};