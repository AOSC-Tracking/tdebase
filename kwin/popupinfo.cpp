/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2002 Alexander Kellett <lypanov@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

//#define QT_CLEAN_NAMESPACE
#include "popupinfo.h"
#include "workspace.h"
#include "client.h"
#include <tqpainter.h>
#include <tqlabel.h>
#include <tqdrawutil.h>
#include <tqstyle.h>
#include <kglobal.h>
#include <fixx11h.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <tqapplication.h>
#include <tqdesktopwidget.h>
#include <kstringhandler.h>
#include <kglobalsettings.h>

// specify externals before namespace

namespace KWinInternal
{

PopupInfo::PopupInfo( Workspace* ws, const char *name )
    : TQWidget( 0, name ), workspace( ws )
    {
    m_infoString = "";
    m_shown = false;
    reset();
    reconfigure();
    connect(&m_delayedHideTimer, TQT_SIGNAL(timeout()), this, TQT_SLOT(hide()));

    TQFont f = font();
    f.setBold( TRUE );
    f.setPointSize( 14 );
    setFont( f );

    }

PopupInfo::~PopupInfo()
    {
    }


/*!
  Resets the popup info
 */
void PopupInfo::reset()
    {
    TQRect r = workspace->screenGeometry( workspace->activeScreen());

    int w = fontMetrics().width( m_infoString ) + 30;

    setGeometry( 
       (r.width()-w)/2 + r.x(), r.height()/2-fontMetrics().height()-10 + r.y(),
                 w,                       fontMetrics().height() + 20 );
    }


/*!
  Paints the popup info
 */
void PopupInfo::paintEvent( TQPaintEvent* )
    {
    TQPainter p( this );
    tqstyle().tqdrawPrimitive( TQStyle::PE_Panel, &p, TQRect( 0, 0, width(), height() ),
          colorGroup(), TQStyle::Style_Default );
    paintContents();
    }


/*!
  Paints the contents of the tab popup info box. 
  Used in paintEvent() and whenever the contents changes.
 */
void PopupInfo::paintContents()
    {
    TQPainter p( this );
    TQRect r( 6, 6, width()-12, height()-12 );

    p.fillRect( r, colorGroup().brush( TQColorGroup::Background ) );

    /*
    p.setPen(Qt::white);
    p.drawText( r, AlignCenter, m_infoString );
    p.setPen(Qt::black);
    r.moveBy( -1, -1 );
    p.drawText( r, AlignCenter, m_infoString );
    r.moveBy( -1, 0 );
    */
    p.drawText( r, AlignCenter, m_infoString );
    }

void PopupInfo::hide()
    {
    m_delayedHideTimer.stop();
    TQWidget::hide();
    TQApplication::syncX();
    XEvent otherEvent;
    while (XCheckTypedEvent (qt_xdisplay(), EnterNotify, &otherEvent ) )
        ;
    m_shown = false;
    }

void PopupInfo::reconfigure() 
    {
    KConfig * c(KGlobal::config());
    c->setGroup("PopupInfo");
    m_show = c->readBoolEntry("ShowPopup", false );
    m_delayTime = c->readNumEntry("PopupHideDelay", 350 );
    }

void PopupInfo::showInfo(TQString infoString)
    {
    if (m_show) 
        {
        m_infoString = infoString;
        reset();
        if (m_shown) 
            {
            paintContents();
            }
        else 
            {
            show();
            raise();
            m_shown = true;
            }
        m_delayedHideTimer.start(m_delayTime, true);
        }
    }

} // namespace

#include "popupinfo.moc"
