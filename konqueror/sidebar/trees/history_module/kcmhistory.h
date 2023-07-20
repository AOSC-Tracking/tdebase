/*
 *  kcmhistory.h
 *  Copyright (c) 2002 Stephan Binner <binner@kde.org>
 *
 *  based on kcmtaskbar.h
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */
#ifndef __kcmhistory_h__
#define __kcmhistory_h__

#include <tdecmodule.h>

class KonqHistoryManager;
class KonqSidebarHistorySettings;
class KonqSidebarHistoryDlg;

class HistorySidebarConfig : public TDECModule
{
  TQ_OBJECT

public:
  HistorySidebarConfig( TQWidget *parent=0, const char* name=0, const TQStringList &list=TQStringList() );

  void load();
  void save();
  void defaults();

  TQString quickHelp() const;

private slots:
  void configChanged();

  void slotGetFontNewer();
  void slotGetFontOlder();

  void slotExpireChanged( int );
  void slotNewerChanged( int );
  void slotOlderChanged( int );

  void slotClearHistory();

private:
  TQFont m_fontNewer;
  TQFont m_fontOlder;

  KonqSidebarHistoryDlg* dialog;
  KonqSidebarHistorySettings *m_settings;
  KonqHistoryManager *mgr;
};

#endif
