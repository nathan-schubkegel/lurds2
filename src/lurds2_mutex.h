/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_MUTEX
#define LURDS2_MUTEX

// A mutex can be used to ensure only one thread at a time executes some code.
// It can also be used to block threads until notified, and deliver such notification.
typedef void* Mutex;

// Creates a mutex with a refcount of 1.
Mutex Mutex_Create();

// Increments the refcount of a mutex (so that it can be co-owned by multiple actors).
Mutex Mutex_AddRef(Mutex mutex);

// Decrements the refcount of a mutex. When the refcount reaches zero, the mutex is freed.
void Mutex_Release(Mutex mutex);

// Locks the mutex. If the mutex is already locked by another thread, this
// blocks until the other thread has unlocked the mutex. Reentrance is supported (if this method
// is called when the current thread has already locked the mutex, it locks it again).
// The current thread must call Mutex_Unlock() once for each call to Mutex_Lock().
// This call increments the refcount of the mutex.
void Mutex_Lock(Mutex mutex);

// Unlocks the mutex so that another thread blocked on Mutex_Lock() can proceed.
// The calling thread must have previously called Mutex_Lock().
// This should be called once for each call to Mutex_Lock(), on the thread
// where Mutex_Lock() was originally called.
// This call decrements the refcount of the mutex.
void Mutex_Unlock(Mutex mutex);

// Temporarily unlocks the mutex and waits for another thread to call Mutex_Notify().
// Then re-locks the mutex (blocking if necessary to acquire it); then this method returns.
// It is advised that the calling thread should lock the mutex before calling this method
// to avoid race conditions in thread wait and notification logic.
void Mutex_Wait(Mutex mutex);

// Wakes other threads blocked in Mutex_Wait() and returns immediately.
// The calling thread need not hold the mutex locked when calling this method.
void Mutex_NotifyAll(Mutex mutex);

#endif
