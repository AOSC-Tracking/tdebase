/* This file is part of the KDE project

   Copyright (C) 1999-2002,2003 Dawit Alemayehu <adawit@kde.org>
   Copyright (C) 2000 Malte Starostik <starosti@zedat.fu-berlin.de>
   Copyright (C) 2003 Sven Leiber <s.leiber@web.de>

   Kdesu integration:
   Copyright (C) 2000 Geert Jansen <jansen@kde.org>

   Original authors:
   Copyright (C) 1997 Matthias Ettrich <ettrich@kde.org>
   Copyright (C) 1997 Torben Weis [ Added command completion ]
   Copyright (C) 1999 Preston Brown <pbrown@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef MINICLI_H
#define MINICLI_H

#include <tqstring.h>
#include <tqstringlist.h>

#include <kdialog.h>
#include <kservice.h>

#include <kurlcompletion.h>

class TQTimer;
class TQWidget;
class MinicliDlgUI;
class KURIFilterData;

class Minicli : public KDialog
{
  Q_OBJECT

public:
  Minicli( TQWidget *parent=0, const char *name=0 );
  virtual ~Minicli();

  void setCommand(const TQString& command);
  void reset();
  void clearHistory();
  
  virtual void show();
  virtual TQSize sizeHint() const;

public slots:
  void saveConfig();

protected slots:
  virtual void accept();
  virtual void reject();
  void updateAuthLabel();

protected:
  void loadConfig();
  bool needsKDEsu();
  virtual void keyPressEvent( TQKeyEvent* );
  virtual void fontChange( const TQFont & );

private slots:
  void slotAdvanced();
  void slotParseTimer();
  void slotPriority(int);
  void slotRealtime(bool);
  void slotAppcompleteToggled(bool);
  void slotAutocompleteToggled(bool);
  void slotAutohistoryToggled(bool);
  void slotTerminal(bool);
  void slotChangeUid(bool);
  void slotChangeScheduler(bool);
  void slotCmdChanged(const TQString&);
  void slotMatch( const TQString&);
  void slotEXEMatch( const TQString&);

private:
  void setIcon();
  int runCommand();
  void parseLine( bool final );
  TQString terminalCommand (const TQString&, const TQString&);
  TQString calculate(const TQString &exp);
  void notifyServiceStarted(KService::Ptr service);


  int m_iPriority;
  int m_iScheduler;

  TQString m_iconName;
  TQString m_prevIconName;
  TQStringList m_terminalAppList;
  TQStringList m_middleFilters;
  TQStringList m_finalFilters;

  TQTimer* m_parseTimer;
  TQWidget* m_FocusWidget;
  MinicliDlgUI* m_dlg;
  KURIFilterData* m_filterData;

  // Cached values
  TQString m_prevUser;
  TQString m_prevPass;
  bool m_prevChecked;
  bool m_prevCached;
  bool m_autoCheckedRunInTerm;

  // Autocomplete
  KURLCompletion *m_pURLCompletion;
  KURLCompletion *m_pEXECompletion;
  bool m_filesystemAutocomplete;
  bool m_systempathAutocomplete;
  bool m_histfilesystemAutocomplete;
  bool m_urlCompletionStarted;
  bool m_exeCompletionStarted;
};
#endif
