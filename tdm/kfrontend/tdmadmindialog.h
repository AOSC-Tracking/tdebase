    /*

    Shutdown dialog

    Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
    Copyright (C) 2000-2003 Oswald Buddenhagen <ossi@kde.org>


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    */


#ifndef TDMADMIN_H
#define TDMADMIN_H

#include "kgverify.h"

#include <tqradiobutton.h>

class LiloInfo;
class TQLabel;
class KPushButton;
class TQButtonGroup;
class TQComboBox;

class TDMAdmin : public FDialog, public KGVerifyHandler {
    TQ_OBJECT
    typedef FDialog inherited;

public:
    TDMAdmin( const TQString &user, TQWidget *_parent = 0 );
    ~TDMAdmin();

public slots:
    void accept();
    void slotWhenChanged();
    void slotActivatePlugMenu();

private:
    void bye_bye();
    
    KPushButton		*okButton, *cancelButton;
    KGStdVerify		*verify;
    TQString             curUser;

    static int		curPlugin;
    static PluginList	pluginList;

public: // from KGVerifyHandler
    virtual void verifyPluginChanged( int id );
    virtual void verifyOk();
    virtual void verifyFailed();
    virtual void verifyRetry();
    virtual void verifySetUser( const TQString &user );
};

#endif
