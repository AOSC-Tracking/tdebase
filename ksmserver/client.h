/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqsocketnotifier.h>
#include <tqptrlist.h>
#include <tqvaluelist.h>
#include <tqcstring.h>
#include <tqdict.h>
#include <tqptrqueue.h>
#include <tqptrdict.h>
#include <tdeapplication.h>
#include <tqtimer.h>
#include <tqdatetime.h>
#include <dcopobject.h>

#include "server2.h"

class KSMListener;
class KSMConnection;
class KSMClient
{
public:
    KSMClient( SmsConn );
    ~KSMClient();

    void registerClient( const char* previousId  = 0 );
    SmsConn connection() const { return smsConn; }

    void resetState();
    uint saveYourselfDone : 1;
    uint pendingInteraction : 1;
    uint waitForPhase2 : 1;
    uint wasPhase2 : 1;

    TQPtrList<SmProp> properties;
    SmProp* property( const char* name ) const;

    TQString program() const;
    TQStringList restartCommand() const;
    TQStringList discardCommand() const;
    int restartStyleHint() const;
    TQString userId() const;
    const char* clientId() { return id ? id : ""; }

    TQDateTime terminationRequestTimeStamp;

private:
    const char* id;
    SmsConn smsConn;
};

#endif
