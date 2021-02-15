/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_PERFORMANCE_COUNTER
#define LURDS2_PERFORMANCE_COUNTER

typedef LARGE_INTEGER PerformanceCounter;

PerformanceCounter PerformanceCounter_Start();
LONGLONG           PerformanceCounter_MeasureTicks(PerformanceCounter start);
double             PerformanceCounter_MeasureSeconds(PerformanceCounter start);
double             PerformanceCounter_MeasureMs(PerformanceCounter start);
double             PerformanceCounter_TicksToSeconds(LONGLONG ticks);
double             PerformanceCounter_TicksToMs(LONGLONG ticks);

#endif