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

    static int getPassword(TQString &password);

protected:
    bool checkPassword(const TQString &password);
};
    

class TDEpasswd2Dialog
    : public KPasswordDialog
{
    Q_OBJECT

public:
    TDEpasswd2Dialog(const TQString &oldpass, const TQString &user);
    ~TDEpasswd2Dialog();

protected:
    bool checkPassword(const TQString &password);
    
private:
    TQString m_Pass;
    TQString m_User;
};
    


#endif // __PasswdDlg_h_Incluced__
