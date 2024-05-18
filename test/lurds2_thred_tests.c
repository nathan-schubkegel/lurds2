#include "lurds2_test.h"
#include "lurds2_thred.c"
#include "lurds2_errors.c"
#include "lurds2_atomic.h"

static void* NeverRunThreadProc(void* arg)
{
  FATAL_ERROR("this function should never be run");
  return 0;
}

static void* ReturnWhenArgIsPositiveThreadProc(void* arg)
{
  // set the argument negative if it's zero
  // to communicate to the test function that started the thread
  // that the thread is running now
  AtomicCompareExchange64s(arg, -55, 0);

  // wait for the argument to become positive
  // (this is how the test function communicates it's
  //  ready for the thread to exit)
  while (AtomicRead64s(arg) <= 0)
  {
    usleep(1000);
  }

  // return the value found set by the test function,
  // but also change it again as a way to communicate "k got it"
  int64_t result = AtomicExchange64s((int64_t*)arg, -999);
  return (void*)result;
}

static void Create_Release()
{
  // I suppose this just proves no FATAL_ERRORS are raised
  Thred t = Thred_Create(&NeverRunThreadProc, 0);
  Thred_Release(t);
}

static void Create_Start_Release()
{
  int64_t arg = 0;
  Thred t = Thred_Create(&ReturnWhenArgIsPositiveThreadProc, &arg);
  Thred_Start(t);
  Thred_Release(t);

  // wait for thread to report that it has started
  while (AtomicRead64s(&arg) >= 0)
  {
    usleep(1000);
  }

  // tell thread to stop
  AtomicWrite64s(&arg, 100);

  // wait for thread to report that it's stopping
  while (AtomicRead64s(&arg) != -999)
  {
    usleep(1000);
  }
}

static void Create_Start_WaitBeforeRunning_Release()
{
  int64_t arg = 101; // tell thread to stop as soon as it starts
  Thred t = Thred_Create(&ReturnWhenArgIsPositiveThreadProc, &arg);
  Thred_Start(t);
  // it's not guaranteed that this call to Thred_Wait() occurs before the thread is running, but it's as close as we can get
  void* result = Thred_Wait(t);
  ASSERT_THAT((uint64_t)result == 101);
  ASSERT_THAT(AtomicRead64s(&arg) == -999);
  Thred_Release(t);
}

static void Create_Start_WaitAfterRunning_Release()
{
  int64_t arg = 0;
  Thred t = Thred_Create(&ReturnWhenArgIsPositiveThreadProc, &arg);
  Thred_Start(t);
  // wait for thread to report that it has started
  while (AtomicRead64s(&arg) >= 0)
  {
    usleep(1000);
  }
  // tell thread to stop
  AtomicWrite64s(&arg, 100);
  void* result = Thred_Wait(t);
  ASSERT_THAT((uint64_t)result == 100);
  ASSERT_THAT(AtomicRead64s(&arg) == -999);
  Thred_Release(t);
}

static void* Create_Start_WaitWhileRunning_Release_HelperThreadProc(void* arg)
{
  int64_t * w = arg;
  usleep(100000);
  AtomicWrite64s(w, 100);
  return 0;
}

static void Create_Start_WaitWhileRunning_Release()
{
  int64_t arg = 0;
  Thred t = Thred_Create(&ReturnWhenArgIsPositiveThreadProc, &arg);
  Thred_Start(t);
  // wait for thread to report that it has started
  while (AtomicRead64s(&arg) >= 0)
  {
    usleep(1000);
  }
  // fire up another thread to tell that thread to stop in 100ms
  Thred t2 = Thred_Create(&Create_Start_WaitWhileRunning_Release_HelperThreadProc, &arg);
  Thred_Start(t2);
  Thred_Release(t2);

  // wait now
  void* result = Thred_Wait(t);
  ASSERT_THAT((uint64_t)result == 100);
  ASSERT_THAT(AtomicRead64s(&arg) == -999);
  Thred_Release(t);
}

static void Create_Start_WaitAfterFinished_Release()
{
  int64_t arg = 102; // tell thread to stop as soon as it starts
  Thred t = Thred_Create(&ReturnWhenArgIsPositiveThreadProc, &arg);
  Thred_Start(t);
  usleep(100000); // surely enough time for the thread to completely start and stop
  ASSERT_THAT(AtomicRead64s(&arg) == -999);
  void* result = Thred_Wait(t);
  ASSERT_THAT((uint64_t)result == 102);
  ASSERT_THAT(AtomicRead64s(&arg) == -999);
  Thred_Release(t);
}

int main(int argc, char** argv)
{
  Create_Release();
  Create_Start_Release();
  Create_Start_WaitBeforeRunning_Release();
  Create_Start_WaitAfterRunning_Release();
  Create_Start_WaitWhileRunning_Release();
  Create_Start_WaitAfterFinished_Release();

  printf("%d assertions were made\n", sTotalTestCount);
  return 0;
}
