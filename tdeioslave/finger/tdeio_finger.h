
/***************************************************************************
                          tdeio_finger.h  -  description
                             -------------------
    begin                : Sun Aug 12 2000
    copyright            : (C) 2000 by Andreas Schlapbach
    email                : schlpbch@iam.unibe.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef __tdeio_finger_h__
#define __tdeio_finger_h__

#include <tqstring.h>
#include <tqcstring.h>

#include <kurl.h>
#include <kprocess.h>
#include <tdeio/global.h>
#include <tdeio/slavebase.h>

class FingerProtocol : public TQObject, public TDEIO::SlaveBase
{
  TQ_OBJECT

public:

  FingerProtocol(const TQCString &pool_socket, const TQCString &app_socket);
  virtual ~FingerProtocol();

  virtual void mimetype(const KURL& url);
  virtual void get(const KURL& url);

private slots:
  void       slotGetStdOutput(TDEProcess*, char*, int);

private:
  KURL                  *myURL;

  QString	        *myPerlPath;
  TQString               *myFingerPath;
  TQString               *myFingerPerlScript;
  TQString               *myFingerCSSFile;

  QString		*myStdStream;


  TDEProcess	        *myTDEProcess;

  void       getProgramPath();
  void       parseCommandLine(const KURL& url);
};


#endif
