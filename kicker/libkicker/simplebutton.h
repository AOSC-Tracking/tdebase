/* This file is part of the KDE project
   Copyright (C) 2003-2004 Nadeem Hasan <nhasan@kde.org>
   Copyright (C) 2004-2005 Aaron J. Seigo <aseigo@kde.org>

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

#ifndef SIMPLEBUTTON_H
#define SIMPLEBUTTON_H

#include <tqbutton.h>
#include <tqpixmap.h>

#include <kdemacros.h>

class KDE_EXPORT SimpleButton : public TQButton
{
    Q_OBJECT

    public:
        SimpleButton(TQWidget *parent, const char *name = 0, bool forceStandardCursor = FALSE);
        void setPixmap(const TQPixmap &pix);
        void setOrientation(Qt::Orientation orientaton);
        TQSize sizeHint() const;
        TQSize minimumSizeHint() const;

    protected:
        void drawButton( TQPainter *p );
        void drawButtonLabel( TQPainter *p );
        void generateIcons();

        void enterEvent( TQEvent *e );
        void leaveEvent( TQEvent *e );
        void resizeEvent( TQResizeEvent *e );

    protected slots:
        virtual void slotSettingsChanged( int category );
        virtual void slotIconChanged( int group );

    private:
        bool m_highlight;
        TQPixmap m_normalIcon;
        TQPixmap m_activeIcon;
        TQPixmap m_disabledIcon;
        Qt::Orientation m_orientation;
        bool m_forceStandardCursor;
        class SimpleButtonPrivate;
        SimpleButtonPrivate* d;
};

class KDE_EXPORT SimpleArrowButton: public SimpleButton
{
    Q_OBJECT
    
    public:
        SimpleArrowButton(TQWidget *parent = 0, Qt::ArrowType arrow = Qt::UpArrow, const char *name = 0, bool forceStandardCursor = FALSE);
        virtual ~SimpleArrowButton() {};
        TQSize sizeHint() const;
    
    protected:
        virtual void enterEvent( TQEvent *e );
        virtual void leaveEvent( TQEvent *e );
        virtual void drawButton(TQPainter *p);
        Qt::ArrowType arrowType() const;
    
    public slots:
        void setArrowType(Qt::ArrowType a);
    
    private:
        Qt::ArrowType _arrow;
        bool m_forceStandardCursor;
        bool _inside;
};


#endif // HIDEBUTTON_H

// vim:ts=4:sw=4:et
