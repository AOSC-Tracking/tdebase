/*
 *
 * Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "preview.h"

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <tqlabel.h>
#include <tqstyle.h>
#include <kiconloader.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <kdecorationfactory.h>
#include <kdecoration_plugins_p.h>

// FRAME the preview doesn't update to reflect the changes done in the kcm

KDecorationPreview::KDecorationPreview( TQWidget* parent, const char* name )
    :   TQWidget( parent, name )
    {
    options = new KDecorationPreviewOptions;

    bridge[Active]   = new KDecorationPreviewBridge( this, true );
    bridge[Inactive] = new KDecorationPreviewBridge( this, false );

    deco[Active] = deco[Inactive] = NULL;

    no_preview = new TQLabel( i18n( "No preview available.\n"
                                   "Most probably there\n"
                                   "was a problem loading the plugin." ), this );

    no_preview->setAlignment( AlignCenter );

    setMinimumSize( 100, 100 );
    no_preview->resize( size());
    }

KDecorationPreview::~KDecorationPreview()
    {
    for ( int i = 0; i < NumWindows; i++ )
        {
        delete deco[i];
        delete bridge[i];
	}
    delete options;
    }

bool KDecorationPreview::recreateDecoration( KDecorationPlugins* plugins )
    {
    for ( int i = 0; i < NumWindows; i++ )
        {
        delete deco[i];   // deletes also window
        deco[i] = plugins->createDecoration( bridge[i] );
        deco[i]->init();
        }

    if( deco[Active] == NULL || deco[Inactive] == NULL )
        {
        return false;
        }

    positionPreviews();
    deco[Inactive]->widget()->show();
    deco[Active]->widget()->show();

    return true;
    }

void KDecorationPreview::enablePreview()
    {
    no_preview->hide();
    }

void KDecorationPreview::disablePreview()
    {
    delete deco[Active];
    delete deco[Inactive];
    deco[Active] = deco[Inactive] = NULL;
    no_preview->show();
    }

void KDecorationPreview::resizeEvent( TQResizeEvent* e )
    {
    TQWidget::resizeEvent( e );
    positionPreviews();
    }

void KDecorationPreview::positionPreviews()
    {
    int titleBarHeight, leftBorder, rightBorder, xoffset,
        dummy1, dummy2, dummy3;
    TQRect geometry;
    TQSize size;

    no_preview->resize( this->size() );

    if ( !deco[Active] || !deco[Inactive] )
        return;

    // don't have more than one reference to the same dummy variable in one borders() call.
    deco[Active]->borders( dummy1, dummy2, titleBarHeight, dummy3 );
    deco[Inactive]->borders( leftBorder, rightBorder, dummy1, dummy2 );

    titleBarHeight = kMin( int( titleBarHeight * .9 ), 30 );
    xoffset = kMin( kMax( 10, TQApplication::reverseLayout()
			    ? leftBorder : rightBorder ), 30 );

    // Resize the active window
    size = TQSize( width() - xoffset, height() - titleBarHeight )
                .expandedTo( deco[Active]->minimumSize() );
    geometry = TQRect( TQPoint( 0, titleBarHeight ), size );
    deco[Active]->widget()->setGeometry( TQStyle::visualRect( geometry, this ) );

    // Resize the inactive window
    size = TQSize( width() - xoffset, height() - titleBarHeight )
                .expandedTo( deco[Inactive]->minimumSize() );
    geometry = TQRect( TQPoint( xoffset, 0 ), size );
    deco[Inactive]->widget()->setGeometry( TQStyle::visualRect( geometry, this ) );
    }

void KDecorationPreview::setPreviewMask( const TQRegion& reg, int mode, bool active )
    {
    TQWidget *widget = active ? deco[Active]->widget() : deco[Inactive]->widget();

    // FRAME duped from client.cpp
    if( mode == Unsorted )
        {
        XShapeCombineRegion( qt_xdisplay(), widget->winId(), ShapeBounding, 0, 0,
            reg.handle(), ShapeSet );
        }
    else
        {
        TQMemArray< TQRect > rects = reg.rects();
        XRectangle* xrects = new XRectangle[ rects.count() ];
        for( unsigned int i = 0;
             i < rects.count();
             ++i )
            {
            xrects[ i ].x = rects[ i ].x();
            xrects[ i ].y = rects[ i ].y();
            xrects[ i ].width = rects[ i ].width();
            xrects[ i ].height = rects[ i ].height();
            }
        XShapeCombineRectangles( qt_xdisplay(), widget->winId(), ShapeBounding, 0, 0,
	    xrects, rects.count(), ShapeSet, mode );
        delete[] xrects;
        }
    if( active )
        mask = reg; // keep shape of the active window for unobscuredRegion()
    }

TQRect KDecorationPreview::windowGeometry( bool active ) const
    {
    TQWidget *widget = active ? deco[Active]->widget() : deco[Inactive]->widget();
    return widget->geometry();
    }

void KDecorationPreview::setTempBorderSize(KDecorationPlugins* plugin, KDecorationDefines::BorderSize size)
    {
    options->setCustomBorderSize(size);
    if (plugin->factory()->reset(KDecorationDefines::SettingBorder) )
        {
        // can't handle the change, recreate decorations then
        recreateDecoration(plugin);
        }
    else
        {
        // handles the update, only update position...
        positionPreviews();
        }
    }

void KDecorationPreview::setTempButtons(KDecorationPlugins* plugin, bool customEnabled, const TQString &left, const TQString &right)
    {
    options->setCustomTitleButtonsEnabled(customEnabled);
    options->setCustomTitleButtons(left, right);
    if (plugin->factory()->reset(KDecorationDefines::SettingButtons) )
        {
        // can't handle the change, recreate decorations then
        recreateDecoration(plugin);
        }
    else
        {
        // handles the update, only update position...
        positionPreviews();
        }
    }

TQRegion KDecorationPreview::unobscuredRegion( bool active, const TQRegion& r ) const
    {
    if( active ) // this one is not obscured
        return r;
    else
        {
        // copied from KWin core's code
        TQRegion ret = r;
        TQRegion r2 = mask;
        if( r2.isEmpty())
            r2 = TQRegion( windowGeometry( true ));
        r2.translate( windowGeometry( true ).x() - windowGeometry( false ).x(),
            windowGeometry( true ).y() - windowGeometry( false ).y());
        ret -= r2;
        return ret;
        }
    }

KDecorationPreviewBridge::KDecorationPreviewBridge( KDecorationPreview* p, bool a )
    :   preview( p ), active( a )
    {
    }

TQWidget* KDecorationPreviewBridge::initialParentWidget() const
    {
    return preview;
    }

Qt::WFlags KDecorationPreviewBridge::initialWFlags() const
    {
    return 0;
    }

bool KDecorationPreviewBridge::isActive() const
    {
    return active;
    }

bool KDecorationPreviewBridge::isCloseable() const
    {
    return true;
    }

bool KDecorationPreviewBridge::isMaximizable() const
    {
    return true;
    }

KDecoration::MaximizeMode KDecorationPreviewBridge::maximizeMode() const
    {
    return KDecoration::MaximizeRestore;
    }

bool KDecorationPreviewBridge::isMinimizable() const
    {
    return true;
    }

bool KDecorationPreviewBridge::providesContextHelp() const
    {
    return true;
    }

int KDecorationPreviewBridge::desktop() const
    {
    return 1;
    }

bool KDecorationPreviewBridge::isModal() const
    {
    return false;
    }

bool KDecorationPreviewBridge::isShadeable() const
    {
    return true;
    }

bool KDecorationPreviewBridge::isShade() const
    {
    return false;
    }

bool KDecorationPreviewBridge::isSetShade() const
    {
    return false;
    }

bool KDecorationPreviewBridge::keepAbove() const
    {
    return false;
    }

bool KDecorationPreviewBridge::keepBelow() const
    {
    return false;
    }

bool KDecorationPreviewBridge::isMovable() const
    {
    return true;
    }

bool KDecorationPreviewBridge::isResizable() const
    {
    return true;
    }

NET::WindowType KDecorationPreviewBridge::windowType( unsigned long ) const
    {
    return NET::Normal;
    }

TQIconSet KDecorationPreviewBridge::icon() const
    {
    return TQIconSet( KGlobal::iconLoader()->loadIcon( "xapp", KIcon::NoGroup, 16 ),
        KGlobal::iconLoader()->loadIcon( "xapp", KIcon::NoGroup, 32 ));
    }

TQString KDecorationPreviewBridge::caption() const
    {
    return active ? i18n( "Active Window" ) : i18n( "Inactive Window" );
    }

void KDecorationPreviewBridge::processMousePressEvent( TQMouseEvent* )
    {
    }

void KDecorationPreviewBridge::showWindowMenu( const TQRect &)
    {
    }

void KDecorationPreviewBridge::showWindowMenu( TQPoint )
    {
    }

void KDecorationPreviewBridge::performWindowOperation( WindowOperation )
    {
    }

void KDecorationPreviewBridge::setMask( const TQRegion& reg, int mode )
    {
    preview->setPreviewMask( reg, mode, active );
    }

bool KDecorationPreviewBridge::isPreview() const
    {
    return true;
    }

TQRect KDecorationPreviewBridge::geometry() const
    {
    return preview->windowGeometry( active );
    }

TQRect KDecorationPreviewBridge::iconGeometry() const
    {
    return TQRect();
    }

TQRegion KDecorationPreviewBridge::unobscuredRegion( const TQRegion& r ) const
    {
    return preview->unobscuredRegion( active, r );
    }

TQWidget* KDecorationPreviewBridge::workspaceWidget() const
    {
    return preview;
    }

WId KDecorationPreviewBridge::windowId() const
    {
    return 0; // no decorated window
    }

void KDecorationPreviewBridge::closeWindow()
    {
    }

void KDecorationPreviewBridge::maximize( MaximizeMode )
    {
    }

void KDecorationPreviewBridge::minimize()
    {
    }

void KDecorationPreviewBridge::showContextHelp()
    {
    }

void KDecorationPreviewBridge::setDesktop( int )
    {
    }

void KDecorationPreviewBridge::titlebarDblClickOperation()
    {
    }

void KDecorationPreviewBridge::titlebarMouseWheelOperation( int )
    {
    }

void KDecorationPreviewBridge::setShade( bool )
    {
    }

void KDecorationPreviewBridge::setKeepAbove( bool )
    {
    }

void KDecorationPreviewBridge::setKeepBelow( bool )
    {
    }

int KDecorationPreviewBridge::currentDesktop() const
    {
    return 1;
    }

void KDecorationPreviewBridge::helperShowHide( bool )
    {
    }

void KDecorationPreviewBridge::grabXServer( bool )
    {
    }

KDecorationPreviewOptions::KDecorationPreviewOptions()
    {
    customBorderSize = BordersCount; // invalid
    customButtonsChanged = false; // invalid
    customButtons = true;
    customTitleButtonsLeft = TQString::null; // invalid
    customTitleButtonsRight = TQString::null; // invalid

    d = new KDecorationOptionsPrivate;
    d->defaultKWinSettings();
    updateSettings();
    }

KDecorationPreviewOptions::~KDecorationPreviewOptions()
    {
    delete d;
    }

unsigned long KDecorationPreviewOptions::updateSettings()
    {
    KConfig cfg( "kwinrc", true );
    unsigned long changed = 0;
    changed |= d->updateKWinSettings( &cfg );

    // set custom border size/buttons
    if (customBorderSize != BordersCount)
        d->border_size = customBorderSize;
    if (customButtonsChanged)
        d->custom_button_positions = customButtons;
    if (customButtons) {
        if (!customTitleButtonsLeft.isNull() )
            d->title_buttons_left = customTitleButtonsLeft;
        if (!customTitleButtonsRight.isNull() )
            d->title_buttons_right = customTitleButtonsRight;
    } else {
        d->title_buttons_left = "MS";
        d->title_buttons_right = "HIAX";
    }

    return changed;
    }

void KDecorationPreviewOptions::setCustomBorderSize(BorderSize size)
    {
    customBorderSize = size;

    updateSettings();
    }

void KDecorationPreviewOptions::setCustomTitleButtonsEnabled(bool enabled)
{
    customButtonsChanged = true;
    customButtons = enabled;

    updateSettings();
}

void KDecorationPreviewOptions::setCustomTitleButtons(const TQString &left, const TQString &right)
    {
    customTitleButtonsLeft = left;
    customTitleButtonsRight = right;

    updateSettings();
    }

bool KDecorationPreviewPlugins::provides( Requirement )
    {
    return false;
    }

#include "preview.moc"
