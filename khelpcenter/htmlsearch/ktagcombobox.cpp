/*
 * ktagcombobox.cpp - A combobox with support for submenues, icons and tags
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

#define INCLUDE_MENUITEM_DEF 1
#include <tqpainter.h>
#include <tqpopupmenu.h>

#include <kdebug.h>

#include "ktagcombobox.h"
#include "ktagcombobox.moc"

KTagComboBox::~KTagComboBox ()
{
  delete popup;
  delete tags;
}

KTagComboBox::KTagComboBox (TQWidget * parent, const char *name)
  : TQComboBox(parent, name),
	popup(0),
	old_popup(0)
{
  tags = new TQStringList;

  clear();
}

void KTagComboBox::popupMenu()
{
   popup->popup( mapToGlobal( TQPoint(0,0) ), current );
}

void KTagComboBox::keyPressEvent( TQKeyEvent *e )
{
    int c;

    if ( ( e->key() == Key_F4 && e->state() == 0 ) || 
         ( e->key() == Key_Down && (e->state() & AltButton) ) ||
         ( e->key() == Key_Space ) ) {
        if ( count() ) { 
            popup->setActiveItem( current );
            popupMenu();
        }
        return;
    } else {
        e->ignore();
        return;
    }

    c = currentItem();
    emit highlighted( c );
    emit activated( c );
}

void KTagComboBox::mousePressEvent( TQMouseEvent * )
{
  popupMenu();
}

void KTagComboBox::internalActivate( int index )
{
  if (current == index) return;
  current = index;
  emit activated( index );
  repaint();
}

void KTagComboBox::internalHighlight( int index )
{
  emit highlighted( index );
}

void KTagComboBox::clear()
{
  tags->clear();

  delete old_popup;
  old_popup = popup;
  popup = new TQPopupMenu(this);
  connect( popup, TQT_SIGNAL(activated(int)),
                        TQT_SLOT(internalActivate(int)) );
  connect( popup, TQT_SIGNAL(highlighted(int)),
                        TQT_SLOT(internalHighlight(int)) );
}

int KTagComboBox::count() const
{
  return tags->count();
}

static inline void checkInsertPos(TQPopupMenu *popup, const TQString & str, int &index)
{
  if (index == -2) index = popup->count();
  if (index != -1) return;

  int a = 0;
  int b = popup->count();
  while (a <= b) {
    int w = (a + b) / 2;

    int id = popup->idAt(w);
    int j = str.compare(popup->text(id));

    if (j > 0)
      a = w + 1;
    else
      b = w - 1;
  }

  index = a; // it doesn't really matter ... a == b here.
}

static inline TQPopupMenu *checkInsertIndex(TQPopupMenu *popup, const TQStringList *tags, const TQString &submenu)
{
  int pos = tags->findIndex(submenu);

  TQPopupMenu *pi = 0;
  if (pos != -1)
  {
    TQMenuItem *p = popup->findItem(pos);
    pi = p?p->popup():0;
  }
  if (!pi) pi = popup;

  return pi;
}

void KTagComboBox::insertItem(const TQIconSet& icon, const TQString &text, const TQString &tag, const TQString &submenu, int index )
{
  TQPopupMenu *pi = checkInsertIndex(popup, tags, submenu);
  checkInsertPos(pi, text, index);
  pi->insertItem(icon, text, count(), index);
  tags->append(tag);
}

void KTagComboBox::insertItem(const TQString &text, const TQString &tag, const TQString &submenu, int index )
{
  TQPopupMenu *pi = checkInsertIndex(popup, tags, submenu);
  checkInsertPos(pi, text, index);
  pi->insertItem(text, count(), index);
  tags->append(tag);
}

void KTagComboBox::insertSeparator(const TQString &submenu, int index)
{
  TQPopupMenu *pi = checkInsertIndex(popup, tags, submenu);
  pi->insertSeparator(index);
  tags->append(TQString::null);
}

void KTagComboBox::insertSubmenu(const TQString &text, const TQString &tag, const TQString &submenu, int index)
{
  TQPopupMenu *pi = checkInsertIndex(popup, tags, submenu);
  TQPopupMenu *p = new TQPopupMenu(pi);
  checkInsertPos(pi, text, index);
  pi->insertItem(text, p, count(), index);
  tags->append(tag);
  connect( p, TQT_SIGNAL(activated(int)),
                        TQT_SLOT(internalActivate(int)) );
  connect( p, TQT_SIGNAL(highlighted(int)),
                        TQT_SLOT(internalHighlight(int)) );
}

void KTagComboBox::paintEvent( TQPaintEvent * ev)
{
  TQComboBox::paintEvent(ev);

  TQPainter p (this);

  // Text
  TQRect clip(2, 2, width() - 4, height() - 4);
#if 0
  if ( hasFocus() && style().guiStyle() != MotifStyle )
    p.setPen( colorGroup().highlightedText() );
#endif
  p.drawText(clip, AlignCenter | SingleLine, popup->text( current ));

  // Icon
  TQIconSet *icon = popup->iconSet( this->current );
  if (icon) {
    TQPixmap pm = icon->pixmap();
    p.drawPixmap( 4, (height()-pm.height())/2, pm );
  }
}

bool KTagComboBox::containsTag( const TQString &str ) const
{
  return tags-.contains(str) > 0;
}

TQString KTagComboBox::currentTag() const
{
  return *tags->at(currentItem());
}

TQString KTagComboBox::tag(int i) const
{
  if (i < 0 || i >= count())
  {
    kdDebug() << "KTagComboBox::tag(), unknown tag " << i << endl;
    return TQString::null;
  }
  return *tags->at(i);
}

int KTagComboBox::currentItem() const
{
  return current;
}

void KTagComboBox::setCurrentItem(int i)
{
  if (i < 0 || i >= count()) return;
  current = i;
  repaint();
}

void KTagComboBox::setCurrentItem(const TQString &code)
{
  int i = tags->findIndex(code);
  if (code.isNull())
    i = 0;
  if (i != -1)
    setCurrentItem(i);
}

void KTagComboBox::setFont( const TQFont &font )
{
  TQComboBox::setFont( font );
  popup->setFont( font );
}
