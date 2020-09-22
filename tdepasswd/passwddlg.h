/*
 *
 * $Id$
 *
 * This file is part of the KDE project, module tdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#ifndef __PasswdDlg_h_Incluced__
#define __PasswdDlg_h_Incluced__

#include <kpassdlg.h>

class TDEpasswd1Dialog
    : public KPasswordDialog
{
    Q_OBJECT

public:
    TDEpasswd1Dialog();
    ~TDEpasswd1Dialog();

    static int getPassword(TQCString &password);

protected:
    bool checkPassword(const char *password);
};
    

class TDEpasswd2Dialog
    : public KPasswordDialog
{
    Q_OBJECT

public:
    TDEpasswd2Dialog(const char *oldpass, TQCString user);
    ~TDEpasswd2Dialog();

protected:
    bool checkPassword(const char *password);
    
private:
    const char *m_Pass;
    TQCString m_User;
};
    


#endif // __PasswdDlg_h_Incluced__
