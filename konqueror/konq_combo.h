/* This file is part of the KDE project
   Copyright (C) 2001 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_COMBO_H
#define KONQ_COMBO_H

#include <tqevent.h>

#include <kcombobox.h>
#include <konq_historymgr.h>

class TDECompletion;
class TDEConfig;

// we use KHistoryCombo _only_ for the up/down keyboard handling, otherwise
// KComboBox would do fine.
class KonqCombo : public KHistoryCombo
{
    TQ_OBJECT

public:
    KonqCombo( TQWidget *parent, const char *name );
    ~KonqCombo();

    // initializes with the completion object and calls loadItems()
    void init( TDECompletion * );

    // determines internally if it's temporary or final
    void setURL( const TQString& url );

    void setTemporary( const TQString& );
    void setTemporary( const TQString&, const TQPixmap& );
    void clearTemporary( bool makeCurrent = true );
    void removeURL( const TQString& url );

    void insertPermanent( const TQString& );

    void updatePixmaps();

    void loadItems();
    void saveItems();

    static void setConfig( TDEConfig * );

    virtual void popup();

    void setPageSecurity( int );

    void insertItem( const TQString &text, int index=-1, const TQString& title=TQString::null );
    void insertItem( const TQPixmap &pixmap, const TQString &text, int index=-1, const TQString& title=TQString::null );

protected:
    virtual void keyPressEvent( TQKeyEvent * );
    virtual bool eventFilter( TQObject *, TQEvent * );
    virtual void mousePressEvent( TQMouseEvent * );
    virtual void mouseMoveEvent( TQMouseEvent * );
    void paintEvent( TQPaintEvent * );
    void selectWord(TQKeyEvent *e);

signals:
    /** 
      Specialized signal that emits the state of the modifier
      keys along with the actual activated text.
     */    
    void activated( const TQString &, int );

    /**
      User has clicked on the security lock in the combobar
     */
    void showPageSecurity();

private slots:
    void slotCleared();
    void slotRemoved( const TQString& item );
    void slotSetIcon( int index );
    void slotActivated( const TQString& text );

private:
    void updateItem( const TQPixmap& pix, const TQString&, int index, const TQString& title );
    void saveState();
    void restoreState();
    void applyPermanent();
    TQString temporaryItem() const { return text( temporary ); }
    void removeDuplicates( int index );
    bool hasSufficientContrast(const TQColor &c1, const TQColor &c2);

    bool m_returnPressed;
    bool m_permanent;
    int m_cursorPos;
    int m_currentIndex;
    int m_modifier;
    TQString m_currentText;
    TQPoint m_dragStart;
    int m_pageSecurity;

    static TDEConfig *s_config;
    static const int temporary; // the index of our temporary item
};

#endif // KONQ_COMBO_H
