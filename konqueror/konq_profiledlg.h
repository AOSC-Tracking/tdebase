/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __konq_profiledlg_h__
#define __konq_profiledlg_h__

#include <kdialogbase.h>

#include <tqlistview.h>
#include <tqmap.h>

class KonqViewManager;
class TQListViewItem;
class TQGridLayout;
class TQCheckBox;
class TQLineEdit;
class KPushButton;
class TDEListView;

typedef TQMap<TQString, TQString> KonqProfileMap;

class KonqProfileItem : public TQListViewItem
{
public:
  KonqProfileItem( TDEListView *, const TQString & );
  ~KonqProfileItem() {}

  TQString m_profileName;
};

class KonqProfileDlg : public KDialogBase
{
  TQ_OBJECT
public:
  KonqProfileDlg( KonqViewManager *manager, const TQString &preselectProfile, TQWidget *parent = 0L );
  ~KonqProfileDlg();

  /**
   * Find, read and return all available profiles
   * @return a map with < name, full path >
   */
  static KonqProfileMap readAllProfiles();

protected slots:
  virtual void slotUser1(); // User1 is "Rename Profile" button
  virtual void slotUser2(); // User2 is "Delete Profile" button
  virtual void slotUser3(); // User3 is Save button
  void slotTextChanged( const TQString & );
  void slotSelectionChanged( TQListViewItem * item );

  void slotItemRenamed( TQListViewItem * );

private:
  void loadAllProfiles(const TQString & = TQString::null);
  KonqViewManager *m_pViewManager;

  KonqProfileMap m_mapEntries;

  TQLineEdit *m_pProfileNameLineEdit;

  TQCheckBox *m_cbSaveURLs;
  TQCheckBox *m_cbSaveSize;

  TDEListView *m_pListView;
};

#endif
