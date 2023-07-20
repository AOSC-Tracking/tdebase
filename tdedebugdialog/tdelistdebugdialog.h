/* This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

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

#ifndef TDELISTDEBUGDIALOG__H
#define TDELISTDEBUGDIALOG__H

#include "kabstractdebugdialog.h"
#include <tqcheckbox.h>
#include <tqptrlist.h>
#include <tqstringlist.h>

class TQVBox;
class KLineEdit;

/**
 * Control debug output of TDE applications
 * This dialog offers a reduced functionality compared to the full TDEDebugDialog
 * class, but allows to set debug output on or off to several areas much more easily.
 *
 * @author David Faure <faure@kde.org>
 */
class TDEListDebugDialog : public KAbstractDebugDialog
{
  TQ_OBJECT

public:
  TDEListDebugDialog( TQStringList areaList, TQWidget *parent=0, const char *name=0, bool modal=true );
  virtual ~TDEListDebugDialog() {}

  void activateArea( TQCString area, bool activate );

  virtual void save();

protected slots:
  void selectAll();
  void deSelectAll();

  void generateCheckBoxes( const TQString& filter );

private:
  void load();
  TQPtrList<TQCheckBox> boxes;
  TQStringList m_areaList;
  TQVBox *m_box;
  KLineEdit *m_incrSearch;
  TQMap<TQCString, int> m_changes;
};

#endif
