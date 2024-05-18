#include "lurds2_test.h"
#include "lurds2_atomic.h"
#include <limits.h>

static void AtomicIncrement64s_Tests()
{
  int64_t a = 0;
  int64_t b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == 1);
  ASSERT_THAT(a == 1);
  
  a = 1;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == 2);
  ASSERT_THAT(a == 2);
  
  a = -1;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);

  a = -5;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == -4);
  ASSERT_THAT(a == -4);

  a = INT64_MAX - 1000;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == INT64_MAX - 999);
  ASSERT_THAT(a == INT64_MAX - 999);

  a = INT64_MAX;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == INT64_MIN);
  ASSERT_THAT(a == INT64_MIN);
  
  a = INT64_MIN;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == INT64_MIN + 1);
  ASSERT_THAT(a == INT64_MIN + 1);
  
  a = INT64_MIN + 1000;
  b = AtomicIncrement64s(&a);
  ASSERT_THAT(b == INT64_MIN + 1001);
  ASSERT_THAT(a == INT64_MIN + 1001);
}

static void AtomicIncrement64_Tests()
{
  uint64_t a = 0;
  uint64_t b = AtomicIncrement64(&a);
  ASSERT_THAT(b == 1);
  ASSERT_THAT(a == 1);
  
  a = 1;
  b = AtomicIncrement64(&a);
  ASSERT_THAT(b == 2);
  ASSERT_THAT(a == 2);
  
  a = 1000;
  b = AtomicIncrement64(&a);
  ASSERT_THAT(b == 1001);
  ASSERT_THAT(a == 1001);

  a = UINT64_MAX - 1000;
  b = AtomicIncrement64(&a);
  ASSERT_THAT(b == UINT64_MAX - 999);
  ASSERT_THAT(a == UINT64_MAX - 999);
  
  a = UINT64_MAX - 1;
  b = AtomicIncrement64(&a);
  ASSERT_THAT(b == UINT64_MAX);
  ASSERT_THAT(a == UINT64_MAX);
  
  a = UINT64_MAX;
  b = AtomicIncrement64(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
}

static void AtomicIncrement32s_Tests()
{
  int32_t a = 0;
  int32_t b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == 1);
  ASSERT_THAT(a == 1);
  
  a = 1;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == 2);
  ASSERT_THAT(a == 2);
  
  a = -1;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);

  a = -5;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == -4);
  ASSERT_THAT(a == -4);

  a = INT32_MAX - 1000;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == INT32_MAX - 999);
  ASSERT_THAT(a == INT32_MAX - 999);

  a = INT32_MAX;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == INT32_MIN);
  ASSERT_THAT(a == INT32_MIN);
  
  a = INT32_MIN;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == INT32_MIN + 1);
  ASSERT_THAT(a == INT32_MIN + 1);
  
  a = INT32_MIN + 1000;
  b = AtomicIncrement32s(&a);
  ASSERT_THAT(b == INT32_MIN + 1001);
  ASSERT_THAT(a == INT32_MIN + 1001);
}

static void AtomicIncrement32_Tests()
{
  uint32_t a = 0;
  uint32_t b = AtomicIncrement32(&a);
  ASSERT_THAT(b == 1);
  ASSERT_THAT(a == 1);
  
  a = 1;
  b = AtomicIncrement32(&a);
  ASSERT_THAT(b == 2);
  ASSERT_THAT(a == 2);
  
  a = 1000;
  b = AtomicIncrement32(&a);
  ASSERT_THAT(b == 1001);
  ASSERT_THAT(a == 1001);

  a = UINT32_MAX - 1000;
  b = AtomicIncrement32(&a);
  ASSERT_THAT(b == UINT32_MAX - 999);
  ASSERT_THAT(a == UINT32_MAX - 999);
  
  a = UINT32_MAX - 1;
  b = AtomicIncrement32(&a);
  ASSERT_THAT(b == UINT32_MAX);
  ASSERT_THAT(a == UINT32_MAX);
  
  a = UINT32_MAX;
  b = AtomicIncrement32(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
}

static void AtomicDecrement64s_Tests()
{
  int64_t a = 0;
  int64_t b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == -1);
  ASSERT_THAT(a == -1);
  
  a = 1;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = -1;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == -2);
  ASSERT_THAT(a == -2);

  a = -5;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == -6);
  ASSERT_THAT(a == -6);
  
  a = 5;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == 4);
  ASSERT_THAT(a == 4);

  a = INT64_MAX - 1000;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == INT64_MAX - 1001);
  ASSERT_THAT(a == INT64_MAX - 1001);

  a = INT64_MAX;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == INT64_MAX - 1);
  ASSERT_THAT(a == INT64_MAX - 1);
  
  a = INT64_MIN;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == INT64_MAX);
  ASSERT_THAT(a == INT64_MAX);

  a = INT64_MIN + 1;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == INT64_MIN);
  ASSERT_THAT(a == INT64_MIN);

  a = INT64_MIN + 1000;
  b = AtomicDecrement64s(&a);
  ASSERT_THAT(b == INT64_MIN + 999);
  ASSERT_THAT(a == INT64_MIN + 999);
}

static void AtomicDecrement64_Tests()
{
  uint64_t a = 0;
  uint64_t b = AtomicDecrement64(&a);
  ASSERT_THAT(b == UINT64_MAX);
  ASSERT_THAT(a == UINT64_MAX);
  
  a = 1;
  b = AtomicDecrement64(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 1000;
  b = AtomicDecrement64(&a);
  ASSERT_THAT(b == 999);
  ASSERT_THAT(a == 999);

  a = UINT64_MAX - 1000;
  b = AtomicDecrement64(&a);
  ASSERT_THAT(b == UINT64_MAX - 1001);
  ASSERT_THAT(a == UINT64_MAX - 1001);
  
  a = UINT64_MAX - 1;
  b = AtomicDecrement64(&a);
  ASSERT_THAT(b == UINT64_MAX - 2);
  ASSERT_THAT(a == UINT64_MAX - 2);
  
  a = UINT64_MAX;
  b = AtomicDecrement64(&a);
  ASSERT_THAT(b == UINT64_MAX - 1);
  ASSERT_THAT(a == UINT64_MAX - 1);
}

static void AtomicDecrement32s_Tests()
{
  int32_t a = 0;
  int32_t b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == -1);
  ASSERT_THAT(a == -1);
  
  a = 1;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = -1;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == -2);
  ASSERT_THAT(a == -2);

  a = -5;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == -6);
  ASSERT_THAT(a == -6);
  
  a = 5;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == 4);
  ASSERT_THAT(a == 4);

  a = INT32_MAX - 1000;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == INT32_MAX - 1001);
  ASSERT_THAT(a == INT32_MAX - 1001);

  a = INT32_MAX;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == INT32_MAX - 1);
  ASSERT_THAT(a == INT32_MAX - 1);
  
  a = INT32_MIN;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == INT32_MAX);
  ASSERT_THAT(a == INT32_MAX);
  
  a = INT32_MIN + 1;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == INT32_MIN);
  ASSERT_THAT(a == INT32_MIN);
  
  a = INT32_MIN + 1000;
  b = AtomicDecrement32s(&a);
  ASSERT_THAT(b == INT32_MIN + 999);
  ASSERT_THAT(a == INT32_MIN + 999);
}

static void AtomicDecrement32_Tests()
{
  uint32_t a = 0;
  uint32_t b = AtomicDecrement32(&a);
  ASSERT_THAT(b == UINT32_MAX);
  ASSERT_THAT(a == UINT32_MAX);
  
  a = 1;
  b = AtomicDecrement32(&a);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 1000;
  b = AtomicDecrement32(&a);
  ASSERT_THAT(b == 999);
  ASSERT_THAT(a == 999);

  a = UINT32_MAX - 1000;
  b = AtomicDecrement32(&a);
  ASSERT_THAT(b == UINT32_MAX - 1001);
  ASSERT_THAT(a == UINT32_MAX - 1001);
  
  a = UINT32_MAX - 1;
  b = AtomicDecrement32(&a);
  ASSERT_THAT(b == UINT32_MAX - 2);
  ASSERT_THAT(a == UINT32_MAX - 2);
  
  a = UINT32_MAX;
  b = AtomicDecrement32(&a);
  ASSERT_THAT(b == UINT32_MAX - 1);
  ASSERT_THAT(a == UINT32_MAX - 1);
}

static void AtomicExchange64s_Tests()
{
  int64_t a = 0;
  int64_t b = AtomicExchange64s(&a, 11);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicExchange64s(&a, -55);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == -55);

  a = 0;
  b = AtomicExchange64s(&a, INT64_MIN);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT64_MIN);
  
  a = 0;
  b = AtomicExchange64s(&a, INT64_MAX);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT64_MAX);
  
  a = 55;
  b = AtomicExchange64s(&a, 0);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = -55;
  b = AtomicExchange64s(&a, 0);
  ASSERT_THAT(b == -55);
  ASSERT_THAT(a == 0);

  a = INT64_MIN;
  b = AtomicExchange64s(&a, 0);
  ASSERT_THAT(b == INT64_MIN);
  ASSERT_THAT(a == 0);
  
  a = INT64_MAX;
  b = AtomicExchange64s(&a, 0);
  ASSERT_THAT(b == INT64_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicExchange64_Tests()
{
  uint64_t a = 0;
  uint64_t b = AtomicExchange64(&a, 11);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicExchange64(&a, 0xAABBCCDDEE);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0xAABBCCDDEE);

  a = 0;
  b = AtomicExchange64(&a, UINT64_MAX);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == UINT64_MAX);
  
  a = 55;
  b = AtomicExchange64(&a, 0);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = 0xAABBCCDDEE;
  b = AtomicExchange64(&a, 0);
  ASSERT_THAT(b == 0xAABBCCDDEE);
  ASSERT_THAT(a == 0);
  
  a = UINT64_MAX;
  b = AtomicExchange64(&a, 0);
  ASSERT_THAT(b == UINT64_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicExchange32s_Tests()
{
  int32_t a = 0;
  int32_t b = AtomicExchange32s(&a, 11);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicExchange32s(&a, -55);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == -55);

  a = 0;
  b = AtomicExchange32s(&a, INT32_MIN);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT32_MIN);
  
  a = 0;
  b = AtomicExchange32s(&a, INT32_MAX);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT32_MAX);
  
  a = 55;
  b = AtomicExchange32s(&a, 0);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = -55;
  b = AtomicExchange32s(&a, 0);
  ASSERT_THAT(b == -55);
  ASSERT_THAT(a == 0);

  a = INT32_MIN;
  b = AtomicExchange32s(&a, 0);
  ASSERT_THAT(b == INT32_MIN);
  ASSERT_THAT(a == 0);
  
  a = INT32_MAX;
  b = AtomicExchange32s(&a, 0);
  ASSERT_THAT(b == INT32_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicExchange32_Tests()
{
  uint32_t a = 0;
  uint32_t b = AtomicExchange32(&a, 11);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicExchange32(&a, 0xBBCCDDEE);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0xBBCCDDEE);

  a = 0;
  b = AtomicExchange32(&a, UINT32_MAX);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == UINT32_MAX);
  
  a = 55;
  b = AtomicExchange32(&a, 0);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = 0xAABBCCDD;
  b = AtomicExchange32(&a, 0);
  ASSERT_THAT(b == 0xAABBCCDD);
  ASSERT_THAT(a == 0);
  
  a = UINT32_MAX;
  b = AtomicExchange32(&a, 0);
  ASSERT_THAT(b == UINT32_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicCompareExchange64s_Tests()
{
  int64_t a = 0;
  int64_t b = AtomicCompareExchange64s(&a, 11, 5);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64s(&a, 11, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicCompareExchange64s(&a, -55, -6);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64s(&a, -55, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == -55);

  a = 0;
  b = AtomicCompareExchange64s(&a, INT64_MIN, 33);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64s(&a, INT64_MIN, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT64_MIN);
  
  a = 0;
  b = AtomicCompareExchange64s(&a, INT64_MAX, 33);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64s(&a, INT64_MAX, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT64_MAX);
  
  a = 55;
  b = AtomicCompareExchange64s(&a, 0, 33);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 55);
  
  a = 55;
  b = AtomicCompareExchange64s(&a, 0, 55);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = -55;
  b = AtomicCompareExchange64s(&a, 0, -12345);
  ASSERT_THAT(b == -55);
  ASSERT_THAT(a == -55);
  
  a = -55;
  b = AtomicCompareExchange64s(&a, 0, -55);
  ASSERT_THAT(b == -55);
  ASSERT_THAT(a == 0);

  a = INT64_MIN;
  b = AtomicCompareExchange64s(&a, 0, 403);
  ASSERT_THAT(b == INT64_MIN);
  ASSERT_THAT(a == INT64_MIN);
  
  a = INT64_MIN;
  b = AtomicCompareExchange64s(&a, 0, INT64_MIN);
  ASSERT_THAT(b == INT64_MIN);
  ASSERT_THAT(a == 0);
  
  a = INT64_MAX;
  b = AtomicCompareExchange64s(&a, 0, INT32_MAX);
  ASSERT_THAT(b == INT64_MAX);
  ASSERT_THAT(a == INT64_MAX);
  
  a = INT64_MAX;
  b = AtomicCompareExchange64s(&a, 0, INT64_MAX);
  ASSERT_THAT(b == INT64_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicCompareExchange64_Tests()
{
  uint64_t a = 0;
  uint64_t b = AtomicCompareExchange64(&a, 11, UINT64_MAX);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64(&a, 11, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicCompareExchange64(&a, 0xAABBCCDDEE, 55);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64(&a, 0xAABBCCDDEE, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0xAABBCCDDEE);

  a = 0;
  b = AtomicCompareExchange64(&a, UINT64_MAX, 449302);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange64(&a, UINT64_MAX, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == UINT64_MAX);
  
  a = 55;
  b = AtomicCompareExchange64(&a, 0, 4930490);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 55);
  
  a = 55;
  b = AtomicCompareExchange64(&a, 0, 55);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = 0xAABBCCDDEE;
  b = AtomicCompareExchange64(&a, 0, 0);
  ASSERT_THAT(b == 0xAABBCCDDEE);
  ASSERT_THAT(a == 0xAABBCCDDEE);
  
  a = 0xAABBCCDDEE;
  b = AtomicCompareExchange64(&a, 0, 0xAABBCCDDEE);
  ASSERT_THAT(b == 0xAABBCCDDEE);
  ASSERT_THAT(a == 0);
  
  a = UINT64_MAX;
  b = AtomicCompareExchange64(&a, 0, 4243);
  ASSERT_THAT(b == UINT64_MAX);
  ASSERT_THAT(a == UINT64_MAX);
  
  a = UINT64_MAX;
  b = AtomicCompareExchange64(&a, 0, UINT64_MAX);
  ASSERT_THAT(b == UINT64_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicCompareExchange32s_Tests()
{
  int32_t a = 0;
  int32_t b = AtomicCompareExchange32s(&a, 11, 33);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32s(&a, 11, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicCompareExchange32s(&a, -55, 33);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32s(&a, -55, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == -55);

  a = 0;
  b = AtomicCompareExchange32s(&a, INT32_MIN, INT32_MAX);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32s(&a, INT32_MIN, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT32_MIN);
  
  a = 0;
  b = AtomicCompareExchange32s(&a, INT32_MAX, -3932);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32s(&a, INT32_MAX, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == INT32_MAX);
  
  a = 55;
  b = AtomicCompareExchange32s(&a, 0, -55);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 55);
  
  a = 55;
  b = AtomicCompareExchange32s(&a, 0, 55);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = -55;
  b = AtomicCompareExchange32s(&a, 0, -0x10000000 + a);
  ASSERT_THAT(b == -55);
  ASSERT_THAT(a == -55);
  
  a = -55;
  b = AtomicCompareExchange32s(&a, 0, -55);
  ASSERT_THAT(b == -55);
  ASSERT_THAT(a == 0);

  a = INT32_MIN;
  b = AtomicCompareExchange32s(&a, 0, 0);
  ASSERT_THAT(b == INT32_MIN);
  ASSERT_THAT(a == INT32_MIN);
  
  a = INT32_MIN;
  b = AtomicCompareExchange32s(&a, 0, INT32_MIN);
  ASSERT_THAT(b == INT32_MIN);
  ASSERT_THAT(a == 0);
  
  a = INT32_MAX;
  b = AtomicCompareExchange32s(&a, 0, -49032);
  ASSERT_THAT(b == INT32_MAX);
  ASSERT_THAT(a == INT32_MAX);
  
  a = INT32_MAX;
  b = AtomicCompareExchange32s(&a, 0, INT32_MAX);
  ASSERT_THAT(b == INT32_MAX);
  ASSERT_THAT(a == 0);
}

static void AtomicCompareExchange32_Tests()
{
  uint32_t a = 0;
  uint32_t b = AtomicCompareExchange32(&a, 11, 333);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32(&a, 11, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 11);
  
  a = 0;
  b = AtomicCompareExchange32(&a, 0xBBCCDDEE, 390);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32(&a, 0xBBCCDDEE, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0xBBCCDDEE);

  a = 0;
  b = AtomicCompareExchange32(&a, UINT32_MAX, 0x10000);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == 0);
  
  a = 0;
  b = AtomicCompareExchange32(&a, UINT32_MAX, 0);
  ASSERT_THAT(b == 0);
  ASSERT_THAT(a == UINT32_MAX);
  
  a = 55;
  b = AtomicCompareExchange32(&a, 0, 55 + 65536);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 55);
  
  a = 55;
  b = AtomicCompareExchange32(&a, 0, 55);
  ASSERT_THAT(b == 55);
  ASSERT_THAT(a == 0);
  
  a = 0xAABBCCDD;
  b = AtomicCompareExchange32(&a, 0, 1);
  ASSERT_THAT(b == 0xAABBCCDD);
  ASSERT_THAT(a == 0xAABBCCDD);
  
  a = 0xAABBCCDD;
  b = AtomicCompareExchange32(&a, 0, 0xAABBCCDD);
  ASSERT_THAT(b == 0xAABBCCDD);
  ASSERT_THAT(a == 0);
  
  a = UINT32_MAX;
  b = AtomicCompareExchange32(&a, 0, 0);
  ASSERT_THAT(b == UINT32_MAX);
  ASSERT_THAT(a == UINT32_MAX);
  
  a = UINT32_MAX;
  b = AtomicCompareExchange32(&a, 0, UINT32_MAX);
  ASSERT_THAT(b == UINT32_MAX);
  ASSERT_THAT(a == 0);
}

int main(int argc, char** argv)
{
  AtomicIncrement32s_Tests();
  AtomicIncrement32_Tests();
  AtomicIncrement64s_Tests();
  AtomicIncrement64_Tests();

  AtomicDecrement32s_Tests();
  AtomicDecrement32_Tests();
  AtomicDecrement64s_Tests();
  AtomicDecrement64_Tests();
  
  AtomicExchange64s_Tests();
  AtomicExchange64_Tests();
  AtomicExchange32s_Tests();
  AtomicExchange32_Tests();
  
  AtomicCompareExchange64s_Tests();
  AtomicCompareExchange64_Tests();
  AtomicCompareExchange32s_Tests();
  AtomicCompareExchange32_Tests();

  printf("%d assertions were made\n", sTotalTestCount);
  return 0;
}
