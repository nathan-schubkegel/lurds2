/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "performanceCounter.h"

#include <Windows.h>

// frequency = ticks per second
static LARGE_INTEGER PerformanceCounter_Frequency;

static PerformanceCounter_RecordFrequency()
{
  if (PerformanceCounter_Frequency.QuadPart == 0)
  {
    QueryPerformanceFrequency(&PerformanceCounter_Frequency);
  }
}

PerformanceCounter PerformanceCounter_Start()
{
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  return now;
}

LONGLONG PerformanceCounter_MeasureTicks(PerformanceCounter start)
{
  LARGE_INTEGER now;
  QueryPerformanceCounter(&now);
  return now.QuadPart - start.QuadPart;
}

double PerformanceCounter_MeasureSeconds(PerformanceCounter start)
{
  return PerformanceCounter_TicksToSeconds(PerformanceCounter_MeasureTicks(start));
}

double PerformanceCounter_MeasureMs(PerformanceCounter start)
{
  return PerformanceCounter_TicksToMs(PerformanceCounter_MeasureTicks(start));
}

double PerformanceCounter_TicksToSeconds(LONGLONG ticks)
{
  PerformanceCounter_RecordFrequency();
  return (double)ticks / (double)PerformanceCounter_Frequency.QuadPart;
}

double PerformanceCounter_TicksToMs(LONGLONG ticks)
{
  PerformanceCounter_RecordFrequency();
  return (double)ticks * 1000.0 / (double)PerformanceCounter_Frequency.QuadPart;
}