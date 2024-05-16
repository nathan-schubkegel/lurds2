/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_ATOMIC
#define LURDS2_ATOMIC

#include <stdint.h>

// Increments the given value atomically (no threading goofs)
// and returns the incremented value
extern __attribute__(regparm(1)) uint32_t AtomicIncrement32(uint32_t* value);
extern __attribute__(regparm(1)) int32_t AtomicIncrement32s(int32_t* value);
extern __attribute__(regparm(1)) uint64_t AtomicIncrement64(uint64_t* value);
extern __attribute__(regparm(1)) int64_t AtomicIncrement64s(int64_t* value);

// Decrements the given value atomically (no threading goofs)
// and returns the decremented value
extern __attribute__(regparm(1)) uint32_t AtomicDecrement32(uint32_t* value);
extern __attribute__(regparm(1)) int32_t AtomicDecrement32s(int32_t* value);
extern __attribute__(regparm(1)) uint64_t AtomicDecrement64(uint64_t* value);
extern __attribute__(regparm(1)) int64_t AtomicDecrement64s(int64_t* value);

#endif
