#include <iostream>
#include <gtest/gtest.h>

#include "Poco/ThreadPool.h"
// #include <core/ITick.h>
// #include <core/RealTick.h>

using namespace std;
extern bool g_ForceIPC;

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);

  ///Mixed mode sim-real time ticker
  // RealTick *theTicker = new RealTick(10, true, true);
  //theTicker->starting();          // Simulate the thread start, required by StateMach

  // Poco::ThreadPool::defaultPool().start(*theTicker);
  Poco::Thread::sleep(100);   // Make sure the ticker is started.
  Poco::Thread::sleep(100);   // Make sure the ticker is started.
  Poco::Thread::sleep(100);   // Make sure the ticker is started.
  Poco::Thread::sleep(100);   // Make sure the ticker is started.
  Poco::Thread::sleep(100);   // Make sure the ticker is started.
  Poco::Thread::sleep(100);   // Make sure the ticker is started.

  int res = RUN_ALL_TESTS();

  // ppcRunnable::tellThreadsToStop();              // Send signal to all runnable threads to stop themselves
  sleep(1);

  Poco::ThreadPool::defaultPool().joinAll();  // Wait for the runnable threads to stop.

  // delete theTicker;

  return res;
}


