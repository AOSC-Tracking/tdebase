/*
 *
 * $Id$
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#ifndef __Passwd_h_Included__
#define __Passwd_h_Included__

#include <tqcstring.h>
#include <tdesu/process.h>

/**
 * A C++ API to passwd.
 */

class PasswdProcess
    : public PtyProcess
{
public:
    PasswdProcess(TQCString user=0);
    ~PasswdProcess();

    enum Errors { PasswdNotFound=1, PasswordIncorrect, PasswordNotGood };

    int checkCurrent(const TQString &oldpass);
    int exec(const char *oldpass, const char *newpass, int check=0);

    TQCString error() { return m_Error; }

private:
    bool isPrompt(TQCString line, const char *word=0L);
    int ConversePasswd(const char *oldpass, const char *newpass,
	    int check);

    TQCString m_User, m_Error;
    bool bOtherUser;
};


#endif // __Passwd_h_Included__
