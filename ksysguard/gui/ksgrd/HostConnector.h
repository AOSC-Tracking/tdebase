/*
    KSysGuard, the KDE System Guard

    Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef KSG_HOSTCONNECTOR_H
#define KSG_HOSTCONNECTOR_H

#include <kdialogbase.h>

class KComboBox;

class TQLabel;
class TQRadioButton;
class TQSpinBox;

class HostConnector : public KDialogBase
{
  TQ_OBJECT

  public:
    HostConnector( TQWidget *parent, const char *name = 0 );
    ~HostConnector();

    void setHostNames( const TQStringList &list );
    TQStringList hostNames() const;

    void setCommands( const TQStringList &list );
    TQStringList commands() const;

    void setCurrentHostName( const TQString &hostName );

    TQString currentHostName() const;
    TQString currentCommand() const;
    int port() const;

    bool useSsh() const;
    bool useRsh() const;
    bool useDaemon() const;
    bool useCustom() const;

  protected slots:
    virtual void slotHelp();
    void slotHostNameChanged( const TQString &_text );
  private:
    KComboBox *mCommands;
    KComboBox *mHostNames;

    TQLabel *mHostNameLabel;

    TQRadioButton *mUseSsh;
    TQRadioButton *mUseRsh;
    TQRadioButton *mUseDaemon;
    TQRadioButton *mUseCustom;

    TQSpinBox *mPort;
};

#endif
