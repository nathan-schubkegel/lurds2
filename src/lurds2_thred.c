/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_thred.h"
#include "lurds2_mutex.h"
#include "lurds2_errors.h"
#include <stdint.h>

#ifdef _WIN32
#else
#include <pthread.h>
#include <unistd.h> // for usleep; FUTURE: remove when notifications by mutex are added
#endif

#define PTHREAD_ERROR_CHECK(result, methodName) \
  if ((result) != 0) \
  { \
    DEBUG_SHOW_INTEGER3( methodName " returned ", result, \
      " - ", GetLinuxErrorCodeMessage(result)); \
    FATAL_ERROR(methodName); \
  }

typedef struct ThredData
{
  void* (*func)(void*); // the function for the thread to run
  void* arg; // argument to the thread function
  void* result; // captured result of the thread function
  Mutex mutex;
  uint8_t started; // if non-zero then the thread has been started
  uint8_t ended; // if non-zero then the thread is done running
  uint8_t released;
#ifdef _WIN32
#else
  pthread_t thread; // it's only meaningful if 'started' is non-zero
#endif
} ThredData;

// Creates a thread w/ a pointer to the function it will run and an argument
// to pass to that function. The thread does not run until Thred_Start() is called.
// The caller must call Thred_Release() once for each call to Thred_Create().
Thred Thred_Create(void* (*func)(void*), void* arg)
{
  if (func == 0) { FATAL_ERROR("null func param"); }
  ThredData* data = malloc(sizeof(ThredData));
  if (data == 0) { FATAL_ERROR("malloc failed"); }
  memset(data, 0, sizeof(data));
  data->func = func;
  data->arg = arg;
  data->mutex = Mutex_Create();
  return data;
}

static void* Thred_Proc(void* arg);

// Starts the given thread (only works once).
void Thred_Start(Thred thread)
{
  ThredData* data = thread;
  if (data == 0) { FATAL_ERROR("null arg"); }

  Mutex_Lock(data->mutex);
  if (data->released) { FATAL_ERROR("already released"); }
  if (data->started) { FATAL_ERROR("already started"); }
  data->started = 0xFF;
#if _WIN32
#else
  int result = pthread_create(&data->thread, 0, Thred_Proc, data);
  PTHREAD_ERROR_CHECK(result, "pthread_create");
#endif
  Mutex_Unlock(data->mutex);
}

static void DestroyThreadResources(ThredData* data)
{
#ifdef _WIN32
#else
  int result = pthread_detach(data->thread);
  PTHREAD_ERROR_CHECK(result, "pthread_detach");
#endif
  Mutex_Unlock(data->mutex);
  free(data);
}

// Releases the resources allocated for observing the thread's completion.
// This has no impact on the execution of the thread.
void Thred_Release(Thred thread)
{
  ThredData* data = thread;
  if (data == 0) { FATAL_ERROR("null arg"); }

  Mutex_Lock(data->mutex);
  if (data->released) { FATAL_ERROR("already released"); }

  data->released = 0xFF;

  // If the thread is done running, or if the thread never ran,
  // then it's this method's job to release resources
  if ((data->started && data->ended) || !data->started)
  {
    DestroyThreadResources(data);
  }
  else // else, the thread will free its own resources when it exits
  {
    Mutex_Unlock(data->mutex);
  }
}

static void* Thred_Proc(void* arg)
{
  ThredData* data = arg;

  // run the thread's function
  void* result = data->func(data->arg);

  Mutex_Lock(data->mutex);
  data->result = result;
  data->ended = 0xFF;

  // notify waiting listeners that the thread has exited
  Mutex_NotifyAll(data->mutex);

  // if nobody's going to be observing the thread's completion, then release resources now
  if (data->released)
  {
    DestroyThreadResources(data);
  }
  else
  {
    Mutex_Unlock(data->mutex);
  }
  return result;
}

// Waits for the given thread to end (returns immediately if already ended)
// and returns its return value
void* Thred_Wait(Thred thread)
{
  ThredData* data = thread;
  if (data == 0) { FATAL_ERROR("null arg"); }

  Mutex_Lock(data->mutex);
  if (data->released) { FATAL_ERROR("already released"); }
  if (!data->started) { FATAL_ERROR("thread was never started"); }

  while (!data->ended)
  {
    Mutex_Wait(data->mutex);
  }

  void* result = data->result;
  Mutex_Unlock(data->mutex);
  return result;
}
