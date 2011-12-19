/*****************************************************************

Copyright (c) 2001 Carsten Pfeiffer <pfeiffer@kde.org>
              2001 Matthias Elter   <elter@kde.org>
              2001 Martijn Klingens <mklingens@yahoo.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/


#include <tqlayout.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <tqpopupmenu.h>
#include <tqtoolbutton.h>
#include <tqstyle.h>
#include <tqtooltip.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <krun.h>
#include <kdebug.h>

#include <stdlib.h>
#include "lockout.h"

extern "C"
{
    KDE_EXPORT KPanelApplet* init(TQWidget *parent, const TQString& configFile)
    {
        KGlobal::locale()->insertCatalogue("lockout");
        return new Lockout(configFile, parent, "lockout");
    }
}

Lockout::Lockout( const TQString& configFile, TQWidget *parent, const char *name)
    : KPanelApplet( configFile, KPanelApplet::Normal, 0, parent, name ), bTransparent( false )
{
    KConfig *conf = config();
    conf->setGroup("lockout");

    //setFrameStyle(Panel | Sunken);
    setBackgroundOrigin( AncestorOrigin );

    if ( orientation() == Qt::Horizontal )
        layout = new TQBoxLayout( this, TQBoxLayout::TopToBottom );
    else
        layout = new TQBoxLayout( this, TQBoxLayout::LeftToRight );

    layout->setAutoAdd( true );
    layout->setMargin( 0 );
    layout->setSpacing( 0 );

    lockButton = new SimpleButton( this, "lock");
    logoutButton = new SimpleButton( this, "logout");

    TQToolTip::add( lockButton, i18n("Lock the session") );
    TQToolTip::add( logoutButton, i18n("Log out") );

    lockButton->setPixmap( SmallIcon( "lock" ));
    logoutButton->setPixmap( SmallIcon( "exit" ));

    bTransparent = conf->readBoolEntry( "Transparent", bTransparent );

    connect( lockButton, TQT_SIGNAL( clicked() ), TQT_SLOT( lock() ));
    connect( logoutButton, TQT_SIGNAL( clicked() ), TQT_SLOT( logout() ));

    lockButton->installEventFilter( this );
    logoutButton->installEventFilter( this );

    if (!kapp->authorize("lock_screen"))
       lockButton->hide();

    if (!kapp->authorize("logout"))
       logoutButton->hide();

    lockButton->setSizePolicy(TQSizePolicy(TQSizePolicy::MinimumExpanding, TQSizePolicy::MinimumExpanding));
    logoutButton->setSizePolicy(TQSizePolicy(TQSizePolicy::MinimumExpanding, TQSizePolicy::MinimumExpanding));

    if ( !kapp->dcopClient()->isAttached() )
        kapp->dcopClient()->attach();

    connect( kapp, TQT_SIGNAL( iconChanged(int) ), TQT_SLOT( slotIconChanged() ));
}

Lockout::~Lockout()
{
    KGlobal::locale()->removeCatalogue("lockout");
}

// the -2 is due to kicker allowing us a width/height of 42 and the buttons
// having a size of 44. So we rather cut those 2 pixels instead of changing
// direction and wasting a lot of space.
void Lockout::checkLayout( int height ) const
{
    TQSize s = minimumSizeHint();
    TQBoxLayout::Direction direction = layout->direction();

    if ( direction == TQBoxLayout::LeftToRight &&
         ( ( orientation() == Qt::Vertical   && s.width() - 2 >= height ) ||
           ( orientation() == Qt::Horizontal && s.width() - 2 < height ) ) ) {
        layout->setDirection( TQBoxLayout::TopToBottom );
    }
    else if ( direction == TQBoxLayout::TopToBottom &&
              ( ( orientation() == Qt::Vertical   && s.height() - 2 < height ) ||
                ( orientation() == Qt::Horizontal && s.height() - 2 >= height ) ) ) {
        layout->setDirection( TQBoxLayout::LeftToRight );
    }
}

int Lockout::widthForHeight( int height ) const
{
    checkLayout( height );
    return sizeHint().width();
}

int Lockout::heightForWidth( int width ) const
{
    checkLayout( width );
    return sizeHint().height();
}

void Lockout::lock()
{
    TQCString appname( "kdesktop" );
    int kicker_screen_number = qt_xscreen();
    if ( kicker_screen_number )
        appname.sprintf("kdesktop-screen-%d", kicker_screen_number);
    kapp->dcopClient()->send(appname, "KScreensaverIface", "lock()", TQString(""));
}

void Lockout::logout()
{
    kapp->requestShutDown();
}

void Lockout::mousePressEvent(TQMouseEvent* e)
{
    propagateMouseEvent(e);
}

void Lockout::mouseReleaseEvent(TQMouseEvent* e)
{
    propagateMouseEvent(e);
}

void Lockout::mouseDoubleClickEvent(TQMouseEvent* e)
{
    propagateMouseEvent(e);
}

void Lockout::mouseMoveEvent(TQMouseEvent* e)
{
    propagateMouseEvent(e);
}

void Lockout::propagateMouseEvent(TQMouseEvent* e)
{
    if ( !isTopLevel()  ) {
        TQMouseEvent me(e->type(), mapTo( topLevelWidget(), e->pos() ),
                       e->globalPos(), e->button(), e->state() );
        TQApplication::sendEvent( topLevelWidget(), &me );
    }
}

bool Lockout::eventFilter( TQObject *o, TQEvent *e )
{
    if (!kapp->authorizeKAction("kicker_rmb"))
        return false;     // Process event normally:

    if( e->type() == TQEvent::MouseButtonPress )
    {
        KConfig *conf = config();
        conf->setGroup("lockout");

        TQMouseEvent *me = TQT_TQMOUSEEVENT( e );
        if( me->button() == Qt::RightButton )
        {
            if( TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(lockButton) )
            {
                TQPopupMenu *popup = new TQPopupMenu();

                popup->insertItem( SmallIcon( "lock" ), i18n("Lock Session"),
                                   this, TQT_SLOT( lock() ) );
                popup->insertSeparator();
                
                i18n("&Transparent");
                //popup->insertItem( i18n( "&Transparent" ), 100 );
                popup->insertItem( SmallIcon( "configure" ),
                                   i18n( "&Configure Screen Saver..." ),
                                   this, TQT_SLOT( slotLockPrefs() ) );

                //popup->setItemChecked( 100, bTransparent );
                //popup->connectItem(100, this, TQT_SLOT( slotTransparent() ) );
                //if (conf->entryIsImmutable( "Transparent" ))
                //    popup->setItemEnabled( 100, false );
                popup->exec( me->globalPos() );
                delete popup;

                return true;
            }
            else if ( TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(logoutButton) )
            {
                TQPopupMenu *popup = new TQPopupMenu();

                popup->insertItem( SmallIcon( "exit" ), i18n("&Log Out..."),
                                   this, TQT_SLOT( logout() ) );
                popup->insertSeparator();
                //popup->insertItem( i18n( "&Transparent" ), 100 );
                popup->insertItem( SmallIcon( "configure" ),
                                   i18n( "&Configure Session Manager..." ),
                                   this, TQT_SLOT( slotLogoutPrefs() ) );

                //popup->setItemChecked( 100, bTransparent );
                //popup->connectItem(100, this, TQT_SLOT( slotTransparent() ) );
                //if (conf->entryIsImmutable( "Transparent" ))
                //    popup->setItemEnabled( 100, false );
                popup->exec( me->globalPos() );
                delete popup;

                return true;
            } // if o
        } // if me->button
    } // if e->type

    // Process event normally:
    return false;
}

void Lockout::slotLockPrefs()
{
    // Run the screensaver settings
    KRun::run( "kcmshell screensaver", KURL::List() );
}

void Lockout::slotTransparent()
{
    bTransparent = !bTransparent;

    KConfig* conf = config();
    conf->setGroup("lockout");
    conf->writeEntry( "Transparent", bTransparent );
    conf->sync();
}

void Lockout::slotLogoutPrefs()
{
    // Run the logout settings.
    KRun::run( "kcmshell kcmsmserver", KURL::List() );
}

void Lockout::slotIconChanged()
{
    lockButton->setPixmap( SmallIcon( "lock" ));
    logoutButton->setPixmap( SmallIcon( "exit" ));
}

#include "lockout.moc"
