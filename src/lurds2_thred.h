/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_THRED
#define LURDS2_THRED

typedef void* Thred;

// Creates a thread w/ a pointer to the function it will run and an argument
// to pass to that function. The thread does not run until Thred_Start() is called.
// The caller must call Thred_Release() once for each call to Thred_Create().
Thred Thred_Create(void* (*func)(void*), void* arg);

// Releases the resources allocated for observing the thread's completion.
// This has no impact on the execution of the thread.
void Thred_Release(Thred thread);

// Starts the given thread (only works once).
void Thred_Start(Thred thread);

// Waits for the given thread to end; returns immediately if already ended;
// returns the value returned by the thread function.
void* Thred_Wait(Thred thread);

#endif
