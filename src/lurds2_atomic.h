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
extern __attribute__(regparm(1)) uint32_t AtomicIncrement32(uint32_t* address);
extern __attribute__(regparm(1)) int32_t AtomicIncrement32s(int32_t* address);
extern __attribute__(regparm(1)) uint64_t AtomicIncrement64(uint64_t* address);
extern __attribute__(regparm(1)) int64_t AtomicIncrement64s(int64_t* address);

// Decrements the given value atomically (no threading goofs)
// and returns the decremented value
extern __attribute__(regparm(1)) uint32_t AtomicDecrement32(uint32_t* address);
extern __attribute__(regparm(1)) int32_t AtomicDecrement32s(int32_t* address);
extern __attribute__(regparm(1)) uint64_t AtomicDecrement64(uint64_t* address);
extern __attribute__(regparm(1)) int64_t AtomicDecrement64s(int64_t* address);

// Writes the given value at the given address atomically (no threading goofs)
// and returns the previous value
extern __attribute__(regparm(3)) uint32_t AtomicExchange32(uint32_t* address, uint32_t value);
extern __attribute__(regparm(3)) int32_t AtomicExchange32s(int32_t* address, int32_t value);
extern __attribute__(regparm(3)) uint64_t AtomicExchange64(uint64_t* address, uint64_t value);
extern __attribute__(regparm(3)) int64_t AtomicExchange64s(int64_t* address, int64_t value);

// Writes the given newValue at the given address atomically (no threading goofs)
// only if the current value at address is 'compare'. Returns the previous value.
extern __attribute__(regparm(3)) uint32_t AtomicCompareExchange32(uint32_t* address, uint32_t newValue, uint32_t compare);
extern __attribute__(regparm(3)) int32_t AtomicCompareExchange32s(int32_t* address, int32_t newValue, int32_t compare);
extern __attribute__(regparm(3)) uint64_t AtomicCompareExchange64(uint64_t* address, uint64_t newValue, uint64_t compare);
extern __attribute__(regparm(3)) int64_t AtomicCompareExchange64s(int64_t* address, int64_t newValue, int64_t compare);

// Reads the value at the given address atomically (ensuring an old CPU-cached value is not returned)
inline uint64_t AtomicRead64(uint64_t* address) { return AtomicCompareExchange64(address, 0, 0); }
inline int64_t AtomicRead64s(int64_t* address) { return AtomicCompareExchange64s(address, 0, 0); }
inline uint32_t AtomicRead32(uint32_t* address) { return AtomicCompareExchange32(address, 0, 0); }
inline int32_t AtomicRead32s(int32_t* address) { return AtomicCompareExchange32s(address, 0, 0); }

inline void AtomicWrite64(uint64_t* address, uint64_t value) { AtomicExchange64(address, value); }
inline void AtomicWrite64s(int64_t* address, int64_t value) { AtomicExchange64s(address, value); }
inline void AtomicWrite32(uint32_t* address, uint32_t value) { AtomicExchange32(address, value); }
inline void AtomicWrite32s(int32_t* address, int32_t value) { AtomicExchange32s(address, value); }

#endif
