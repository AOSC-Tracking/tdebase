/*
 * ktagcombobox.h - A combobox with support for submenues, icons and tags
 *
 * Copyright (c) 1999-2000 Hans Petter Bieker <bieker@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __KTAGCOMBOBOX_H__
#define __KTAGCOMBOBOX_H__

#include <tqcombobox.h>

class TQPopupMenu;

/*
 * This class should be just like qcombobox, but it should be possible
 * to have have a TQIconSet for each entry, and each entry should have a tag.
 *
 * It has also support for sub menues.
 */
class KTagComboBox : public QComboBox
{
  TQ_OBJECT

public:
  KTagComboBox(TQWidget *parent=0, const char *name=0);
  ~KTagComboBox();

  void insertItem(const TQIconSet& icon, const TQString &text, const TQString &tag, const TQString &submenu = TQString::null, int index=-1 );
  void insertItem(const TQString &text, const TQString &tag, const TQString &submenu = TQString::null, int index=-1 );
  void insertSeparator(const TQString &submenu = TQString::null, int index=-1 );
  void insertSubmenu(const TQString &text, const TQString &tag, const TQString &submenu = TQString::null, int index=-1);

  int count() const;
  void clear();

  /*
   * Tag of the selected item
   */
  TQString currentTag() const;
  TQString tag ( int i ) const;
  bool containsTag (const TQString &str ) const;

  /*
   * Set the current item
   */
  int currentItem() const;
  void setCurrentItem(int i);
  void setCurrentItem(const TQString &code);

  // widget stuff
  virtual void setFont( const TQFont & );

signals:
  void activated( int index );
  void highlighted( int index );

private slots:
  void internalActivate( int );
  void internalHighlight( int );

protected:
  void paintEvent( TQPaintEvent * );
  void mousePressEvent( TQMouseEvent * );
  void keyPressEvent( TQKeyEvent *e );
  void popupMenu();

private:
  // work space for the new class
  TQStringList *tags;  
  TQPopupMenu *popup, *old_popup;
  int current;
};

#endif
