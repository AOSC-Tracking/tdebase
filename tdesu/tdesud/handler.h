/*
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 */

#ifndef __Handler_h_included__
#define __Handler_h_included__

#include <sys/types.h>

#include <tqcstring.h>
#include "secure.h"

/**
 * A ConnectionHandler handles a client. It is called from the main program
 * loop whenever there is data to read from a corresponding socket.
 * It keeps reading data until a newline is read. Then, a command is parsed
 * and executed.
 */

class ConnectionHandler: public SocketSecurity 
{

public:
    ConnectionHandler(int fd);
    ~ConnectionHandler();

    /** Handle incoming data. */
    int handle();

    /* Send back exit code. */
    void sendExitCode();

private:
    enum Results { Res_OK, Res_NO };

    int doCommand(TQCString buf);
    void respond(int ok, TQCString s=0);
    TQCString makeKey(int namspace, TQCString s1, TQCString s2=0, TQCString s3=0);

    int m_Fd, m_Timeout;
    int m_Priority, m_Scheduler;
    TQCString m_Buf, m_Pass, m_Host;
public:
    int m_exitCode;
    bool m_hasExitCode;
    bool m_needExitCode;
    pid_t m_pid;
};

#endif
