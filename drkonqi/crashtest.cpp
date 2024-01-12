// Let's crash.
#include <unistd.h>
#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <tqthread.h>
#include <tqtimer.h>
#include <kdebug.h>
#include <stdio.h>
#include <assert.h>

#include "crashtest.h"

void WorkerObject::run()
{
	while (1) {
		sleep(10000);
	}
}

#define SET_UP_WORKER(x, y)												\
	WorkerObject x;													\
	x.moveToThread(&y);												\
	TQTimer::singleShot(0, &x, TQ_SLOT(run()));

static TDECmdLineOptions options[] =
{
  { "+crash|malloc|div0|assert", "Type of crash.", 0 },
  TDECmdLineLastOption
};

enum CrashType { Crash, Malloc, Div0, Assert };

void do_crash()
{
  TDECmdLineArgs *args = 0;
  TQCString type = args->arg(0);
  printf("result = %s\n", type.data());
}

void do_malloc()
{
  delete (char*)0xdead;
}

void do_div0()
{
  volatile int a = 99;
  volatile int b = 10;
  volatile int c = a / ( b - 10 );
  printf("result = %d\n", c);
}

void do_assert()
{
  assert(false);
}

void level4(int t)
{
  if (t == Malloc)
    do_malloc();
  else if (t == Div0)
    do_div0();
  else if (t == Assert)
    do_assert();
  else
    do_crash();
}

void level3(int t)
{
  level4(t);
}

void level2(int t)
{
  level3(t);
}

void level1(int t)
{
  level2(t);
}

int main(int argc, char *argv[])
{
  TDEAboutData aboutData("crashtext", "Crash Test for DrKonqi",
                       "1.1",
                       "Crash Test for DrKonqi",
                       TDEAboutData::License_GPL,
                       "(c) 2000-2002 David Faure, Waldo Bastian");

  // Start 3 threads
  TQEventLoopThread workerthread0;
  TQEventLoopThread workerthread1;
  TQEventLoopThread workerthread2;
  SET_UP_WORKER(worker0, workerthread0)
  SET_UP_WORKER(worker1, workerthread1)
  SET_UP_WORKER(worker2, workerthread2)
  workerthread0.start();
  workerthread1.start();
  workerthread2.start();

  TDECmdLineArgs::init(argc, argv, &aboutData);
  TDECmdLineArgs::addCmdLineOptions(options);

  TDEApplication app(false, false);
  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
  TQCString type = args->count() ? args->arg(0) : "";
  int crashtype = Crash;
  if (type == "malloc")
    crashtype = Malloc;
  else if (type == "div0")
    crashtype = Div0;
  else if (type == "assert")
    crashtype = Assert;
  level1(crashtype);
  return app.exec();
}

#include "crashtest.moc"
