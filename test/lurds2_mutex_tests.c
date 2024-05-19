#include "lurds2_test.h"
#include "lurds2_mutex.c"
#include "lurds2_thred.c"
#include "lurds2_errors.c"
#include "lurds2_atomic.h"

typedef struct WaitToExitArg
{
  Mutex m;
  int64_t thread_is_waiting;
  int64_t thread_may_exit;
} WaitToExitArg;

static void* WaitToExitThreadProc(void* arg)
{
  printf("j\n");
  WaitToExitArg * a = arg;
  Mutex_Lock(a->m);
  printf("k\n");

  // change state and notify
  a->thread_is_waiting = 1;
  Mutex_NotifyAll(a->m);

  // wait for condition
  while (!a->thread_may_exit)
  {
    printf("k.blurp\n");
    Mutex_Wait(a->m);
  }
  printf("k.blorped\n");

  // change state and notify
  a->thread_is_waiting = 2;
  Mutex_NotifyAll(a->m);

  Mutex_Unlock(a->m);
  printf("n\n");
  return 0;
}

static void Create_Release()
{
  // I suppose this just proves no FATAL_ERRORS are raised
  Mutex m = Mutex_Create();
  Mutex_Release(m);
  ASSERT_THAT(1);
}

static void Create_Lock_Unlock_Release()
{
  // I suppose this just proves no FATAL_ERRORS are raised
  Mutex m = Mutex_Create();
  Mutex_Lock(m);
  Mutex_Unlock(m);
  Mutex_Release(m);
  ASSERT_THAT(1);
}

static void Create_Lock_Release_Unlock()
{
  // This is absurd, but it is technically supposed to work
  Mutex m = Mutex_Create();
  Mutex_Lock(m);
  Mutex_Release(m);
  Mutex_Unlock(m);
  ASSERT_THAT(1);
}

static void Lock_Notify_Wait()
{
  WaitToExitArg a;
  a.m = Mutex_Create();
  a.thread_is_waiting = 0;
  a.thread_may_exit = 0;

  Mutex_Lock(a.m);

  Thred t = Thred_Create(&WaitToExitThreadProc, &a);
  Thred_Start(t);
  Thred_Release(t);

  // wait for thread to be waiting for us to let it go
  printf("a\n");
  while (a.thread_is_waiting != 1)
  {
    printf("a.blurp\n");
    Mutex_Wait(a.m);
  }
  printf("a.blorped\n");
  ASSERT_THAT(a.thread_is_waiting == 1);

  // tell thread it can go
  a.thread_may_exit = 1;
  Mutex_NotifyAll(a.m);

  // wait for thread to tell us it's finishing
  printf("b\n");
  while (a.thread_is_waiting != 2)
  {
    Mutex_Wait(a.m);
  }
  ASSERT_THAT(a.thread_is_waiting == 2);

  printf("c\n");
  Mutex_Unlock(a.m);
  Mutex_Release(a.m);
}

int main(int argc, char** argv)
{
  Create_Release();
  Create_Lock_Unlock_Release();
  Create_Lock_Release_Unlock();
  Lock_Notify_Wait();

  printf("%d assertions were made\n", sTotalTestCount);
  return 0;
}
