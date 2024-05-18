/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_thred.h"
#include "lurds2_atomic.h"
#include "lurds2_errors.h"
#include <stdint.h>

#ifdef _WIN32
#else
#include <pthread.h>
#include <unistd.h> // for usleep; FUTURE: remove when notifications by mutex are added
#endif

typedef struct ThredData
{
  void* (*func)(void*); // the function for the thread to run
  void* arg; // argument to the thread function
  void* result; // captured result of the thread function
  uint64_t started; // if non-zero then the thread has been started
  uint64_t ended; // if non-zero then the thread is done running
  int64_t refcount; // when reaches zero, delete this struct
#ifdef _WIN32
#else
  pthread_t thread; // it's only meaningful if 'started' is non-zero
#endif
} ThredData;

// Creates a thread w/ a pointer to the function it will run and an argument to pass to that function.
// The thread does not run until Thred_Start() is called.
Thred Thred_Create(void* (*func)(void*), void* arg)
{
  if (func == 0) { FATAL_ERROR("null func param"); }
  ThredData* data = malloc(sizeof(ThredData));
  if (data == 0) { FATAL_ERROR("malloc failed"); }
  data->arg = arg;
  data->func = func;
  data->result = 0;
  data->started = 0;
  data->refcount = 1;
  data->ended = 0;
  return data;
}

// Releases the resources allocated for observing the thread's completion.
// This has no impact on the running thread.
void Thred_Release(Thred thread)
{
  ThredData* data = thread;
  int64_t newRefCount = AtomicDecrement64s(&data->refcount);
  if (newRefCount < 0) { FATAL_ERROR("bad refcount"); }
  else if (newRefCount == 0)
  {
#ifdef _WIN32
#else
    if (data->started)
    {
      pthread_detach(data->thread);
    }
#endif
    free(data);
  }
}

static void* Thred_Proc(void* arg)
{
  ThredData* data = arg;
  void* result = data->func(data->arg);
  AtomicExchange64((uint64_t*)&data->result, (uint64_t)result);
  AtomicExchange64(&data->ended, 0xFFFF);

  // FUTURE: notify waiting listeners that the thread has exited

  Thred_Release(data);
  return result;
}

// Starts the given thread (only works once)
void Thred_Start(Thred thread)
{
  ThredData* data = thread;
  if (AtomicIncrement64s(&data->refcount) <= 1) { FATAL_ERROR("bad refcount"); }
  if (AtomicCompareExchange64(&data->started, 0xFFFF, 0) != 0) { FATAL_ERROR("can't start an already-started thread'"); }
  
#if _WIN32
#else
  if (0 != pthread_create(&data->thread, 0, Thred_Proc, data)) { FATAL_ERROR("pthread_create"); }
#endif
}

// Waits for the given thread to end (returns immediately if already ended)
// and returns its return value
void* Thred_Wait(Thred thread)
{
  ThredData* data = thread;
  if (AtomicRead64(&data->started) == 0) { FATAL_ERROR("can't wait; thread not yet started"); }

  // FUTURE: implement this via mutex notifications
  while (AtomicRead64(&data->ended) == 0)
  {
    usleep(1000);
  }
  return (void*)AtomicRead64((uint64_t*)&data->result);
}
