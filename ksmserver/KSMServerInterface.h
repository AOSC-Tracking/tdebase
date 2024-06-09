#ifndef KSMSERVER_INTERFACE_H
#define KSMSERVER_INTERFACE_H

#include <dcopobject.h>
#include <tqstringlist.h>

class KSMServerInterface : virtual public DCOPObject
{
  K_DCOP

k_dcop:
  virtual void logout(int, int, int ) = 0;
  virtual void logoutTimed( int, int, TQString ) = 0;
  virtual bool suspend(int) = 0;

  virtual TQStringList suspendOptions() = 0;

  virtual void restoreSessionInternal() = 0;
  virtual void restoreSessionDoneInternal() = 0;
  virtual TQStringList sessionList() = 0;

  virtual TQString currentSession() = 0;
  virtual void saveCurrentSession() = 0;
  virtual void saveCurrentSessionAs( TQString ) = 0;

  virtual void autoStart2() = 0;

  virtual void suspendStartup( TQCString ) = 0;
  virtual void resumeStartup( TQCString ) = 0;

  virtual void reconfigure() = 0;
};

#endif
