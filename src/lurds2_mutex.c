#include "lurds2_mutex.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <string.h>
#include <stdint.h>
#include "lurds2_errors.h"
#include "lurds2_atomic.h"

typedef struct MutexData {
  int64_t refcount;
#ifdef _WIN32
#else
  pthread_mutex_t mutex; // for locking
  pthread_cond_t cond; // for notify/wait
#endif
} MutexData;

#define PTHREAD_ERROR_CHECK(result, methodName) \
  if ((result) != 0) \
  { \
    DEBUG_SHOW_INTEGER3( methodName " returned ", result, \
      " - ", GetLinuxErrorCodeMessage(result)); \
    FATAL_ERROR(methodName); \
  }

// Creates a mutex with a refcount of 1.
Mutex Mutex_Create()
{
  MutexData* data = malloc(sizeof(MutexData));
  if (data == 0) FATAL_ERROR("out of memory for MutexData");
  data->refcount = 1;

#ifdef _WIN32
#else
  pthread_mutexattr_t mutexAttributes;
  memset(&mutexAttributes, 0, sizeof(mutexAttributes));
  int result = pthread_mutexattr_init(&mutexAttributes);
  PTHREAD_ERROR_CHECK(result, "pthread_mutexattr_init");

  result = pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE);
  PTHREAD_ERROR_CHECK(result, "pthread_mutexattr_settype");

  memset(&data->mutex, 0, sizeof(data->mutex));
  result = pthread_mutex_init(&data->mutex, &mutexAttributes);
  PTHREAD_ERROR_CHECK(result, "pthread_mutex_init");

  result = pthread_mutexattr_destroy(&mutexAttributes);
  PTHREAD_ERROR_CHECK(result, "pthread_mutexattr_destroy");

  memset(&data->cond, 0, sizeof(data->cond));
  result = pthread_cond_init(&data->cond, 0);
  PTHREAD_ERROR_CHECK(result, "pthread_cond_init");
#endif

  return data;
}

// Increments the refcount of a mutex (so that it can be co-owned by multiple actors).
Mutex Mutex_AddRef(Mutex mutex)
{
  MutexData* data = mutex;
  if (data == 0) FATAL_ERROR("null mutex");
  int64_t newValue = AtomicIncrement64s(&data->refcount);
  if (newValue <= 1) FATAL_ERROR("bad refcount");
  return mutex;
}

// Decrements the refcount of a mutex. When the refcount reaches zero, the mutex is freed.
void Mutex_Release(Mutex mutex)
{
  MutexData* data = mutex;
  if (data == 0) FATAL_ERROR("null mutex");
  int64_t newValue = AtomicDecrement64s(&data->refcount);
  if (newValue < 0) FATAL_ERROR("bad refcount");
  if (newValue == 0)
  {
#ifdef _WIN32
#else
    int result = pthread_cond_destroy(&data->cond);
    PTHREAD_ERROR_CHECK(result, "pthread_cond_destroy");

    result = pthread_mutex_destroy(&data->mutex);
    PTHREAD_ERROR_CHECK(result, "pthread_mutex_destroy");
#endif

    free(data);
  }
}

// Locks the mutex. If the mutex is already locked by another thread, this
// blocks until the other thread has unlocked the mutex. Reentrance is supported (if this method
// is called when the current thread has already locked the mutex, it locks it again).
// The current thread must call Mutex_Unlock() once for each call to Mutex_Lock().
// This call increments the refcount of the mutex.
void Mutex_Lock(Mutex mutex)
{
  Mutex_AddRef(mutex);
  MutexData* data = mutex;
#ifdef _WIN32
#else
  int result = pthread_mutex_lock(&data->mutex);
  PTHREAD_ERROR_CHECK(result, "pthread_mutex_lock");
#endif
}

// Unlocks the mutex so that another thread blocked on Mutex_Lock() can proceed.
// The calling thread must have previously called Mutex_Lock().
// This should be called once for each call to Mutex_Lock(), on the thread
// where Mutex_Lock() was originally called.
// This call decrements the refcount of the mutex.
void Mutex_Unlock(Mutex mutex)
{
  MutexData* data = mutex;
  if (data == 0) FATAL_ERROR("null mutex");
#ifdef _WIN32
#else
  int result = pthread_mutex_unlock(&data->mutex);
  PTHREAD_ERROR_CHECK(result, "pthread_mutex_unlock");
#endif
  Mutex_Release(mutex);
}

// Temporarily unlocks the mutex and waits for another thread to call Mutex_Notify().
// Then re-locks the mutex (blocking if necessary to acquire it); then this method returns.
// It is advised that the calling thread should lock the mutex before calling this method
// to avoid race conditions in thread wait and notification logic.
void Mutex_Wait(Mutex mutex)
{
  // The caller should have done this, but we're doing it again
  // to avoid possibility of undefined behavior and errors
  Mutex_Lock(mutex);

  MutexData* data = mutex;
#ifdef _WIN32
#else
  int result = pthread_cond_wait(&data->cond, &data->mutex);
  PTHREAD_ERROR_CHECK(result, "pthread_cond_wait");
#endif;

  Mutex_Unlock(mutex);
}

// Wakes other threads blocked in Mutex_Wait() and returns immediately.
// The calling thread need not hold the mutex locked when calling this method.
void Mutex_NotifyAll(Mutex mutex)
{
  MutexData* data = mutex;
  if (data == 0) FATAL_ERROR("null mutex");
#ifdef _WIN32
#else
  int result = pthread_cond_broadcast(&data->cond);
  PTHREAD_ERROR_CHECK(result, "pthread_cond_broadcast");
#endif
}
