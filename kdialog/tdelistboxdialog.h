//
//  Copyright (C) 1998 Matthias Hoelzer
//  email:  hoelzer@physik.uni-wuerzburg.de
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//


#ifndef _TDELISTBOXDIALOG_H_
#define _TDELISTBOXDIALOG_H_

#include <kdialogbase.h>

class TDEListBoxDialog : public KDialogBase
{
  TQ_OBJECT

public:

  TDEListBoxDialog(TQString text, TQWidget *parent=0);
  ~TDEListBoxDialog() {};

  TQListBox &getTable() { return *table; };

  void insertItem( const TQString& text );
  void setCurrentItem ( const TQString& text );
  int currentItem();

protected:

  TQListBox *table;
  TQLabel *label;

};


#endif
