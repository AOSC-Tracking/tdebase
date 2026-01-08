/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

/*

 This file contains things relevant to direct user actions, such as
 responses to global keyboard shortcuts, or selecting actions
 from the window operations menu.

*/

#include <tqhbox.h>
#include <tqpushbutton.h>
#include <tqslider.h>
#include <tqtooltip.h>
#include <tqpopupmenu.h>
#include <tdeglobalsettings.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <tdeglobalaccel.h>
#include <tdeapplication.h>
#include <tqregexp.h>

#include "client.h"
#include "workspace.h"
#include <fixx11h.h>

#include "killwindow.h"
#include "tabbox.h"

namespace KWinInternal
{

//****************************************
// Workspace
//****************************************

TQPopupMenu* Workspace::makeTileMenu()
{
    TQPopupMenu *m = new TQPopupMenu;

    // Tile to side (the menu id matched the ActiveBorder index used for tiling)
    int id = m->insertItem( SmallIconSet("tile_left"), i18n("&Left"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 6 );
    id = m->insertItem( SmallIconSet("tile_right"), i18n("&Right"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 2 );
    id = m->insertItem( SmallIconSet("tile_top"), i18n("&Top"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 0 );
    id = m->insertItem( SmallIconSet("tile_bottom"), i18n("&Bottom"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 4 );

    // Tile to corner (the menu id matched the ActiveBorder index used for tiling)
    id = m->insertItem( SmallIconSet("tile_topleft"), i18n("Top &Left"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 7 );
    id = m->insertItem( SmallIconSet("tile_topright"), i18n("Top &Right"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 1 );
    id = m->insertItem( SmallIconSet("tile_bottomleft"), i18n("Bottom L&eft"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 5 );
    id = m->insertItem( SmallIconSet("tile_bottomright"), i18n("&Bottom R&ight"), this, TQ_SLOT( tileCurrentWindowToBorder(int) ) );
    m->setItemParameter( id, 3 );

    return m;
}

TQPopupMenu* Workspace::clientPopup()
    {
    if ( !popup )
        {
        popup = new TQPopupMenu;
        popup->setCheckable( true );
        popup->setFont(TDEGlobalSettings::menuFont());
        connect( popup, TQ_SIGNAL( aboutToShow() ), this, TQ_SLOT( clientPopupAboutToShow() ) );
        connect( popup, TQ_SIGNAL( activated(int) ), this, TQ_SLOT( clientPopupActivated(int) ) );
      
        advanced_popup = new TQPopupMenu( popup );
        advanced_popup->setCheckable( true );
        advanced_popup->setFont(TDEGlobalSettings::menuFont());
        connect( advanced_popup, TQ_SIGNAL( activated(int) ), this, TQ_SLOT( clientPopupActivated(int) ) );
        advanced_popup->insertItem( SmallIconSet( "go-up" ),
            i18n("Keep &Above Others")+'\t'+keys->shortcut("Window Above Other Windows").seq(0).toString(), Options::KeepAboveOp );
        advanced_popup->insertItem( SmallIconSet( "go-down" ),
            i18n("Keep &Below Others")+'\t'+keys->shortcut("Window Below Other Windows").seq(0).toString(), Options::KeepBelowOp );
        advanced_popup->insertItem( SmallIconSet( "view-fullscreen" ),
            i18n("&Fullscreen")+'\t'+keys->shortcut("Window Fullscreen").seq(0).toString(), Options::FullScreenOp );
        advanced_popup->insertItem( i18n("&No Border")+'\t'+keys->shortcut("Window No Border").seq(0).toString(), Options::NoBorderOp );
	advanced_popup->insertItem( i18n("Shad&ow"), Options::ShadowOp );
        advanced_popup->insertItem( SmallIconSet("key_bindings"),
            i18n("Window &Shortcut…")+'\t'+keys->shortcut("Setup Window Shortcut").seq(0).toString(), Options::SetupWindowShortcutOp );
        advanced_popup->insertSeparator();
        advanced_popup->insertItem( SmallIconSet( "suspend" ), i18n("&Suspend Application"), Options::SuspendWindowOp );
        advanced_popup->insertItem( SmallIconSet( "application-x-executable" ), i18n("&Resume Application"), Options::ResumeWindowOp );
        advanced_popup->insertSeparator();
        advanced_popup->insertItem( SmallIconSet( "wizard" ), i18n("&Special Window Settings…"), Options::WindowRulesOp );
        advanced_popup->insertItem( SmallIconSet( "wizard" ), i18n("&Special Application Settings…"), Options::ApplicationRulesOp );

        popup->insertItem(i18n("Ad&vanced"), advanced_popup );
        tile_popup_index = popup->insertItem(i18n("T&ile"), makeTileMenu());
        desk_popup_index = popup->count();
        
        if (options->useTranslucency){
            TQPopupMenu *trans_popup = new TQPopupMenu( popup );
            TQVBox *transBox = new TQVBox(trans_popup);
            transButton = new TQPushButton(transBox, "transButton");
            TQToolTip::add(transButton, i18n("Reset opacity to default value"));
            transSlider = new TQSlider(0, 100, 1, 100, TQt::Horizontal, transBox, "transSlider");
            TQToolTip::add(transSlider, i18n("Slide this to set the window's opacity"));
            connect(transButton, TQ_SIGNAL(clicked()), TQ_SLOT(resetClientOpacity()));
            connect(transButton, TQ_SIGNAL(clicked()), trans_popup, TQ_SLOT(hide()));
            connect(transSlider, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(setTransButtonText(int)));
            connect(transSlider, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(setPopupClientOpacity(int)));
//             connect(transSlider, TQ_SIGNAL(sliderReleased()), trans_popup, TQ_SLOT(hide()));
            trans_popup->insertItem(transBox);
            popup->insertItem(i18n("&Opacity"), trans_popup );
        }
        
        popup->insertItem( SmallIconSet( "move" ), i18n("&Move")+'\t'+keys->shortcut("Window Move").seq(0).toString(), Options::MoveOp );
        popup->insertItem( i18n("Re&size")+'\t'+keys->shortcut("Window Resize").seq(0).toString(), Options::ResizeOp );
        popup->insertItem( i18n("Mi&nimize")+'\t'+keys->shortcut("Window Minimize").seq(0).toString(), Options::MinimizeOp );
        popup->insertItem( i18n("Ma&ximize")+'\t'+keys->shortcut("Window Maximize").seq(0).toString(), Options::MaximizeOp );
        popup->insertItem( i18n("Sh&ade")+'\t'+keys->shortcut("Window Shade").seq(0).toString(), Options::ShadeOp );

        popup->insertSeparator();

        if (!TDEGlobal::config()->isImmutable() && 
            !tdeApp->authorizeControlModules(Workspace::configModules(true)).isEmpty())
            {
            popup->insertItem(SmallIconSet( "configure" ), i18n("Configur&e Window Behavior…"), this, TQ_SLOT( configureWM() ));
            popup->insertSeparator();
            }

        popup->insertItem( SmallIconSet( "window-close" ), i18n("&Close")+'\t'+keys->shortcut("Window Close").seq(0).toString(), Options::CloseOp );
        }
    return popup;
    }
    
//sets the transparency of the client to given value(given by slider)
void Workspace::setPopupClientOpacity(int value)
    {
    active_popup_client->setCustomOpacityFlag(true);
    int opacityPercent = 100 - value;
    active_popup_client->setOpacity(true, percentToUint(opacityPercent));
    }

void Workspace::setTransButtonText(int value)
    {
    value = 100 - value;
    if(value < 0)
        transButton->setText("  0 %");
    else if (value >= 100 )
        transButton->setText("100 %");
    else if(value < 10)
        transButton->setText("  "+TQString::number(value)+" %");
    else if(value < 100)
        transButton->setText(" "+TQString::number(value)+" %");
    }

void Workspace::resetClientOpacity()
    {
    active_popup_client->setCustomOpacityFlag(false);
    active_popup_client->updateOpacity();
    transSlider->setValue(100-active_popup_client->opacityPercentage());
    setTransButtonText(100-active_popup_client->opacityPercentage());
    }


/*!
  The client popup menu will become visible soon.

  Adjust the items according to the respective popup client.
 */
void Workspace::clientPopupAboutToShow()
    {
    if ( !active_popup_client || !popup )
        return;

    if ( numberOfDesktops() == 1 )
        {
        delete desk_popup;
        desk_popup = 0;
        }
    else
        {
        initDesktopPopup();
        }

    popup->setItemEnabled( Options::ResizeOp, active_popup_client->isResizable() );
    popup->setItemEnabled( Options::MoveOp, active_popup_client->isMovable() );
    popup->setItemEnabled( Options::MaximizeOp, active_popup_client->isMaximizable() );
    popup->setItemChecked( Options::MaximizeOp, active_popup_client->maximizeMode() == Client::MaximizeFull );
    // This should be checked also when hover unshaded
    popup->setItemChecked( Options::ShadeOp, active_popup_client->shadeMode() != ShadeNone );
    popup->setItemEnabled( Options::ShadeOp, active_popup_client->isShadeable());
    advanced_popup->setItemChecked( Options::KeepAboveOp, active_popup_client->keepAbove() );
    advanced_popup->setItemChecked( Options::KeepBelowOp, active_popup_client->keepBelow() );
    advanced_popup->setItemChecked( Options::FullScreenOp, active_popup_client->isFullScreen() );
    advanced_popup->setItemEnabled( Options::FullScreenOp, active_popup_client->userCanSetFullScreen() );
    advanced_popup->setItemEnabled( Options::SuspendWindowOp, active_popup_client->isSuspendable() );
    advanced_popup->setItemEnabled( Options::ResumeWindowOp, active_popup_client->isResumeable() );
    advanced_popup->setItemChecked( Options::NoBorderOp, active_popup_client->noBorder() );
    advanced_popup->setItemEnabled( Options::NoBorderOp, active_popup_client->userCanSetNoBorder() );
    advanced_popup->setItemEnabled( Options::ShadowOp, (options->shadowWindowType(active_popup_client->windowType()) && options->shadowEnabled(active_popup_client->isActive())) );
    advanced_popup->setItemChecked( Options::ShadowOp, active_popup_client->isShadowed() );
    
    popup->setItemEnabled( tile_popup_index, active_popup_client->isMovable() && active_popup_client->isResizable());

    popup->setItemEnabled( Options::MinimizeOp, active_popup_client->isMinimizable() );
    popup->setItemEnabled( Options::CloseOp, active_popup_client->isCloseable() );
    if (options->useTranslucency)
        {
        transSlider->setValue(100-active_popup_client->opacityPercentage());
        setTransButtonText(100-active_popup_client->opacityPercentage());
        }
    }


void Workspace::initDesktopPopup()
    {
    if (desk_popup)
        return;

    desk_popup = new TQPopupMenu( popup );
    desk_popup->setCheckable( true );
    desk_popup->setFont(TDEGlobalSettings::menuFont());
    connect( desk_popup, TQ_SIGNAL( activated(int) ),
             this, TQ_SLOT( slotSendToDesktop(int) ) );
    connect( desk_popup, TQ_SIGNAL( aboutToShow() ),
             this, TQ_SLOT( desktopPopupAboutToShow() ) );

    popup->insertItem(i18n("To &Desktop"), desk_popup, -1, desk_popup_index );
    }

/*!
  Adjusts the desktop popup to the current values and the location of
  the popup client.
 */
void Workspace::desktopPopupAboutToShow()
    {
    if ( !desk_popup )
        return;

    desk_popup->clear();
    desk_popup->insertItem( i18n("&All Desktops"), 0 );
    if ( active_popup_client && active_popup_client->isOnAllDesktops() )
        desk_popup->setItemChecked( 0, true );
    desk_popup->insertSeparator( -1 );
    int id;
    const int BASE = 10;
    for ( int i = 1; i <= numberOfDesktops(); i++ ) 
        {
        TQString basic_name("%1  %2");
        if (i<BASE)
            {
            basic_name.prepend('&');
            }
        id = desk_popup->insertItem(
                basic_name
                    .arg(i)
                    .arg( desktopName(i).replace( '&', "&&" )),
                i );
        if ( active_popup_client &&
             !active_popup_client->isOnAllDesktops() && active_popup_client->desktop()  == i )
            desk_popup->setItemChecked( id, true );
        }
    }

void Workspace::closeActivePopup()
    {
    if( active_popup )
        {
        active_popup->close();
        active_popup = NULL;
        active_popup_client = NULL;
        }
    }

/*!
  Create the global accel object \c keys.
 */
void Workspace::initShortcuts()
    {
    keys = new TDEGlobalAccel( this );
    // a separate TDEGlobalAccel is needed for the shortcut for disabling global shortcuts,
    // otherwise it would also disable itself
    disable_shortcuts_keys = new TDEGlobalAccel( this );
    disable_shortcuts_keys->disableBlocking( true );
#define IN_KWIN
#include "twinbindings.cpp"
    readShortcuts();
    }

void Workspace::readShortcuts()
    {
    keys->readSettings();
    disable_shortcuts_keys->readSettings();

    cutWalkThroughDesktops = keys->shortcut("Walk Through Desktops");
    cutWalkThroughDesktopsReverse = keys->shortcut("Walk Through Desktops (Reverse)");
    cutWalkThroughDesktopList = keys->shortcut("Walk Through Desktop List");
    cutWalkThroughDesktopListReverse = keys->shortcut("Walk Through Desktop List (Reverse)");
    cutWalkThroughWindows = keys->shortcut("Walk Through Windows");
    cutWalkThroughWindowsReverse = keys->shortcut("Walk Through Windows (Reverse)");
    cutWalkThroughApps = keys->shortcut("Walk Through Windows of Same Application");
    cutWalkThroughAppsReverse = keys->shortcut("Walk Through Windows of Same Application (Reverse)");

    keys->updateConnections();
    disable_shortcuts_keys->updateConnections();
    
    delete popup;
    popup = NULL; // so that it's recreated next time
    desk_popup = NULL;
    }


void Workspace::setupWindowShortcut( Client* c )
    {
    assert( client_keys_dialog == NULL );
    keys->suspend( true );
    disable_shortcuts_keys->suspend( true );
    client_keys->suspend( true );
    client_keys_dialog = new ShortcutDialog( c->shortcut());
    client_keys_client = c;
    connect( client_keys_dialog, TQ_SIGNAL( dialogDone( bool )), TQ_SLOT( setupWindowShortcutDone( bool )));
    TQRect r = clientArea( ScreenArea, c );
    TQSize size = client_keys_dialog->sizeHint();
    TQPoint pos = c->pos() + c->clientPos();
    if( pos.x() + size.width() >= r.right())
        pos.setX( r.right() - size.width());
    if( pos.y() + size.height() >= r.bottom())
        pos.setY( r.bottom() - size.height());
    client_keys_dialog->move( pos );
    client_keys_dialog->show();
    active_popup = client_keys_dialog;
    active_popup_client = c;
    }

void Workspace::setupWindowShortcutDone( bool ok )
    {
    keys->suspend( false );
    disable_shortcuts_keys->suspend( false );
    client_keys->suspend( false );
    if( ok )
        {
        client_keys_client->setShortcut( TDEShortcut( client_keys_dialog->shortcut()).toString());
        }
    closeActivePopup();
    delete client_keys_dialog;
    client_keys_dialog = NULL;
    client_keys_client = NULL;
    }

void Workspace::clientShortcutUpdated( Client* c )
    {
    TQString key = TQString::number( c->window());
    client_keys->remove( key );
    if( !c->shortcut().isNull())
        {
        client_keys->insert( key, key );
        client_keys->setShortcut( key, c->shortcut());
        client_keys->setSlot( key, c, TQ_SLOT( shortcutActivated()));
        client_keys->setActionEnabled( key, true );
        }
    client_keys->updateConnections();
    }

void Workspace::clientPopupActivated( int id )
    {
    WindowOperation op = static_cast< WindowOperation >( id );
    Client* c = active_popup_client ? active_popup_client : active_client;
    TQString type;
    switch( op )
        {
        case FullScreenOp:
            if( !c->isFullScreen() && c->userCanSetFullScreen())
                type = "fullscreenaltf3";
          break;
        case NoBorderOp:
            if( !c->noBorder() && c->userCanSetNoBorder())
                type = "noborderaltf3";
          break;
        default:
            break;
        };
    if( !type.isEmpty())
        helperDialog( type, c );
    performWindowOperation( c, op );
    }


void Workspace::performWindowOperation( Client* c, Options::WindowOperation op ) 
    {
    if ( !c )
        return;

    if (op == Options::MoveOp || op == Options::UnrestrictedMoveOp )
        TQCursor::setPos( c->geometry().center() );
    if (op == Options::ResizeOp || op == Options::UnrestrictedResizeOp )
        TQCursor::setPos( c->geometry().bottomRight());
    switch ( op ) 
        {
        case Options::MoveOp:
            c->performMouseCommand( Options::MouseMove, TQCursor::pos() );
            break;
        case Options::UnrestrictedMoveOp:
            c->performMouseCommand( Options::MouseUnrestrictedMove, TQCursor::pos() );
            break;
        case Options::ResizeOp:
            c->performMouseCommand( Options::MouseResize, TQCursor::pos() );
            break;
        case Options::UnrestrictedResizeOp:
            c->performMouseCommand( Options::MouseUnrestrictedResize, TQCursor::pos() );
            break;
        case Options::CloseOp:
            c->closeWindow();
            break;
        case Options::MaximizeOp:
            c->maximize( c->maximizeMode() == Client::MaximizeFull
                ? Client::MaximizeRestore : Client::MaximizeFull );
            break;
        case Options::HMaximizeOp:
            c->maximize( c->maximizeMode() ^ Client::MaximizeHorizontal );
            break;
        case Options::VMaximizeOp:
            c->maximize( c->maximizeMode() ^ Client::MaximizeVertical );
            break;
        case Options::RestoreOp:
            c->maximize( Client::MaximizeRestore );
            break;
        case Options::MinimizeOp:
            c->minimize();
            break;
        case Options::ShadeOp:
            c->performMouseCommand( Options::MouseShade, TQCursor::pos());
            break;
        case Options::ShadowOp:
            c->setShadowed( !c->isShadowed() );
            break;
        case Options::OnAllDesktopsOp:
            c->setOnAllDesktops( !c->isOnAllDesktops() );
            break;
        case Options::FullScreenOp:
            c->setFullScreen( !c->isFullScreen(), true );
            break;
        case Options::NoBorderOp:
            c->setUserNoBorder( !c->isUserNoBorder());
            break;
        case Options::KeepAboveOp:
            {
            StackingUpdatesBlocker blocker( this );
            bool was = c->keepAbove();
            c->setKeepAbove( !c->keepAbove() );
            if( was && !c->keepAbove())
                raiseClient( c );
            break;
            }
        case Options::KeepBelowOp:
            {
            StackingUpdatesBlocker blocker( this );
            bool was = c->keepBelow();
            c->setKeepBelow( !c->keepBelow() );
            if( was && !c->keepBelow())
                lowerClient( c );
            break;
            }
        case Options::OperationsOp:
            c->performMouseCommand( Options::MouseShade, TQCursor::pos());
            break;
        case Options::SuspendWindowOp:
            c->suspendWindow();
            break;
        case Options::ResumeWindowOp:
            c->resumeWindow();
            break;
        case Options::WindowRulesOp:
            editWindowRules( c, false );
            break;
        case Options::ApplicationRulesOp:
            editWindowRules( c, true );
            break;
        case Options::SetupWindowShortcutOp:
            setupWindowShortcut( c );
            break;
        case Options::LowerOp:
            lowerClient(c);
            break;
        case Options::NoOp:
            break;
        }
    }

/*!
  Performs a mouse command on this client (see options.h)
 */
bool Client::performMouseCommand( Options::MouseCommand command, TQPoint globalPos, bool handled )
    {
    bool replay = false;
    switch (command) 
        {
        case Options::MouseRaise:
            workspace()->raiseClient( this );
            break;
        case Options::MouseLower:
            workspace()->lowerClient( this );
            break;
        case Options::MouseShade :
            toggleShade();
            cancelShadeHover();
            break;
        case Options::MouseSetShade:
            setShade( ShadeNormal );
            cancelShadeHover();
            break;
        case Options::MouseUnsetShade:
            setShade( ShadeNone );
            cancelShadeHover();
            break;
        case Options::MouseOperationsMenu:
            if ( isActive() && options->clickRaise )
                autoRaise();
            workspace()->showWindowMenu( globalPos, this );
            break;
        case Options::MouseToggleRaiseAndLower:
            workspace()->raiseOrLowerClient( this );
            break;
        case Options::MouseActivateAndRaise:
            replay = isActive(); // for clickraise mode
            workspace()->takeActivity( this, ActivityFocus | ActivityRaise, handled && replay );
            workspace()->setActiveScreenMouse( globalPos );
            break;
        case Options::MouseActivateAndLower:
            workspace()->requestFocus( this );
            workspace()->lowerClient( this );
            workspace()->setActiveScreenMouse( globalPos );
            break;
        case Options::MouseActivate:
            replay = isActive(); // for clickraise mode
            workspace()->takeActivity( this, ActivityFocus, handled && replay );
            workspace()->setActiveScreenMouse( globalPos );
            break;
        case Options::MouseActivateRaiseAndPassClick:
            workspace()->takeActivity( this, ActivityFocus | ActivityRaise, handled );
            workspace()->setActiveScreenMouse( globalPos );
            replay = true;
            break;
        case Options::MouseActivateAndPassClick:
            workspace()->takeActivity( this, ActivityFocus, handled );
            workspace()->setActiveScreenMouse( globalPos );
            replay = true;
            break;
        case Options::MouseActivateRaiseAndMove:
        case Options::MouseActivateRaiseAndUnrestrictedMove:
            workspace()->raiseClient( this );
            workspace()->requestFocus( this );
            workspace()->setActiveScreenMouse( globalPos );
            if( options->moveMode == Options::Transparent && isMovable())
                move_faked_activity = workspace()->fakeRequestedActivity( this );
        // fallthrough
        case Options::MouseMove:
        case Options::MouseUnrestrictedMove:
            {
            if (!isMovable())
                break;
            if( moveResizeMode )
                finishMoveResize( false );
            mode = PositionCenter;
            buttonDown = true;
            moveOffset = TQPoint( globalPos.x() - x(), globalPos.y() - y()); // map from global
            invertedMoveOffset = rect().bottomRight() - moveOffset;
            unrestrictedMoveResize = ( command == Options::MouseActivateRaiseAndUnrestrictedMove
                                    || command == Options::MouseUnrestrictedMove );
            setCursor( mode );
            if( !startMoveResize())
                {
                buttonDown = false;
                setCursor( mode );
                }
            break;
            }
        case Options::MouseResize:
        case Options::MouseUnrestrictedResize:
            {
            if (!isResizable() || isShade())
                break;
            if( moveResizeMode )
                finishMoveResize( false );
            buttonDown = true;
            moveOffset = TQPoint( globalPos.x() - x(), globalPos.y() - y()); // map from global
            int x = moveOffset.x(), y = moveOffset.y();
            bool left = x < width() / 3;
            bool right = x >= 2 * width() / 3;
            bool top = y < height() / 3;
            bool bot = y >= 2 * height() / 3;
            if (top)
                mode = left ? PositionTopLeft : (right ? PositionTopRight : PositionTop);
            else if (bot)
                mode = left ? PositionBottomLeft : (right ? PositionBottomRight : PositionBottom);
            else
                mode = (x < width() / 2) ? PositionLeft : PositionRight;
            invertedMoveOffset = rect().bottomRight() - moveOffset;
            unrestrictedMoveResize = ( command == Options::MouseUnrestrictedResize );
            setCursor( mode );
            if( !startMoveResize())
                {
                buttonDown = false;
                setCursor( mode );
                }
            break;
            }
        case Options::MouseMaximize:
            maximize( Client::MaximizeFull );
            break;
        case Options::MouseRestore:
            maximize( Client::MaximizeRestore );
            break;
        case Options::MouseMinimize:
            minimize();
            break;
        case Options::MouseAbove:
            {
            StackingUpdatesBlocker blocker( workspace());
            if( keepBelow())
                setKeepBelow( false );
            else
                setKeepAbove( true );
            break;
            }
        case Options::MouseBelow:
            {
            StackingUpdatesBlocker blocker( workspace());
            if( keepAbove())
                setKeepAbove( false );
            else
                setKeepBelow( true );
            break;
            }
        case Options::MousePreviousDesktop:
            workspace()->windowToPreviousDesktop( this );
            break;
        case Options::MouseNextDesktop:
            workspace()->windowToNextDesktop( this );
            break;
        case Options::MouseOpacityMore:
            if (opacity_ < Opacity::Opaque)
                {
                if (opacity_ < Opacity::Opaque - Opacity::MouseStep)
                    {
                    setOpacity(true, opacity_ + Opacity::MouseStep);
                    custom_opacity = true;
                    }
                else
                    {
                    setOpacity(false, Opacity::Opaque);
                    custom_opacity = false;
                    }
                }
            break;
        case Options::MouseOpacityLess:
            if (opacity_ > 0)
                {
                setOpacity(true, (opacity_ > Opacity::MouseStep) ? opacity_ - Opacity::MouseStep : Opacity::Transparent);
                custom_opacity = true;
                }
            break;
        case Options::MouseNothing:
            replay = true;
            break;
        }
    return replay;
    }

void Workspace::showWindowMenuAt( unsigned long window, int x, int y )
    {
    Client *client;
    if ((client = findClient(WindowMatchPredicate((WId)window))))
        showWindowMenu( x, y, client );
    }

void Workspace::showWindowMenu( unsigned long window )
    {
    Client *client;
    if ((client = findClient(WindowMatchPredicate((WId)window))))
        {
        TQPoint pos = client->pos() + client->clientPos();
        showWindowMenu( pos, client );
        }
    }

void Workspace::tileCurrentWindowToBorder(int position)
{
    Client *c = active_popup_client ? active_popup_client : active_client;
    if (!c) return;

    c->tileToBorder((ActiveBorder)position);
}

void Workspace::tileWindowToBorder(unsigned long w1, int location) {
    if (location < ActiveTop || location >= ACTIVE_BORDER_COUNT) return;

    Client *c1 = findClient(WindowMatchPredicate((WId)w1));
    if (!c1) return;

    c1->tileToBorder((ActiveBorder)location);
}

void Workspace::tileTwoWindowsHorizontally(unsigned long w1, unsigned long w2) {
    if (w1 == w2) return;

    Client *c1 = findClient(WindowMatchPredicate((WId)w1));
    Client *c2 = findClient(WindowMatchPredicate((WId)w2));
    if (!c1 || !c2) return;

    c1->tileToBorder(ActiveTop);
    c2->tileToBorder(ActiveBottom);
}

void Workspace::tileTwoWindowsVertically(unsigned long w1, unsigned long w2) {
    if (w1 == w2) return;

    Client *c1 = findClient(WindowMatchPredicate((WId)w1));
    Client *c2 = findClient(WindowMatchPredicate((WId)w2));
    if (!c1 || !c2) return;

    c1->tileToBorder(ActiveLeft);
    c2->tileToBorder(ActiveRight);
}

void Workspace::tileFourWindowsInGrid(unsigned long w1, unsigned long w2, unsigned long w3, unsigned long w4) {
    if (w1 == w2 || w1 == w3 || w1 == w4 || w2 == w3 || w2 == w4 || w3 == w4)
        return;

    Client *c1 = findClient(WindowMatchPredicate((WId)w1));
    Client *c2 = findClient(WindowMatchPredicate((WId)w2));
    Client *c3 = findClient(WindowMatchPredicate((WId)w3));
    Client *c4 = findClient(WindowMatchPredicate((WId)w4));
    if (!c1 || !c2 || !c3 || !c4) return;

    c1->tileToBorder(ActiveTopLeft);
    c2->tileToBorder(ActiveTopRight);
    c3->tileToBorder(ActiveBottomLeft);
    c4->tileToBorder(ActiveBottomRight);
}

void Workspace::slotActivateAttentionWindow()
    {
    if( attention_chain.count() > 0 )
        activateClient( attention_chain.first());
    }

void Workspace::slotSwitchDesktopNext()
    {
    int d = currentDesktop() + 1;
     if ( d > numberOfDesktops() ) 
        {
        if ( options->rollOverDesktops ) 
            {
            d = 1;
            }
        else 
            {
            return;
            }
        }
    setCurrentDesktop(d);
    }

void Workspace::slotSwitchDesktopPrevious()
    {
    int d = currentDesktop() - 1;
    if ( d <= 0 ) 
        {
        if ( options->rollOverDesktops )
          d = numberOfDesktops();
      else
          return;
        }
    setCurrentDesktop(d);
    }

void Workspace::slotSwitchDesktopRight()
    {
    int desktop = desktopToRight( currentDesktop());
    if( desktop == currentDesktop())
        return;
    setCurrentDesktop( desktop );
    }

void Workspace::slotSwitchDesktopLeft()
    {
    int desktop = desktopToLeft( currentDesktop());
    if( desktop == currentDesktop())
        return;
    setCurrentDesktop( desktop );
    }

void Workspace::slotSwitchDesktopUp()
    {
    int desktop = desktopUp( currentDesktop());
    if( desktop == currentDesktop())
        return;
    setCurrentDesktop( desktop );
    }

void Workspace::slotSwitchDesktopDown()
    {
    int desktop = desktopDown( currentDesktop());
    if( desktop == currentDesktop())
        return;
    setCurrentDesktop( desktop );
    }

void Workspace::slotSwitchToDesktop( int i )
    {
    setCurrentDesktop( i );
    }


void Workspace::slotWindowToDesktop( int i )
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( i >= 1 && i <= numberOfDesktops() && c
        && !c->isDesktop()
        && !c->isDock()
        && !c->isTopMenu())
            sendClientToDesktop( c, i, true );
    }

void Workspace::slotSwitchToScreen( int i )
    {
    setCurrentScreen( i );
    }

void Workspace::slotSwitchToNextScreen()
    {
    slotSwitchToScreen(( activeScreen() + 1 ) % numScreens());
    }

void Workspace::slotWindowToScreen( int i )
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( i >= 0 && i <= numScreens() && c
        && !c->isDesktop()
        && !c->isDock()
        && !c->isTopMenu())
        {
        sendClientToScreen( c, i );
        }
    }

void Workspace::slotWindowToNextScreen()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c
        && !c->isDesktop()
        && !c->isDock()
        && !c->isTopMenu())
        {
        sendClientToScreen( c, ( c->screen() + 1 ) % numScreens());
        }
    }

/*!
  Maximizes the popup client
 */
void Workspace::slotWindowMaximize()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if ( c )
        performWindowOperation( c, Options::MaximizeOp );
    }

/*!
  Maximizes the popup client vertically
 */
void Workspace::slotWindowMaximizeVertical()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if ( c )
        performWindowOperation( c, Options::VMaximizeOp );
    }

/*!
  Maximizes the popup client horiozontally
 */
void Workspace::slotWindowMaximizeHorizontal()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if ( c )
        performWindowOperation( c, Options::HMaximizeOp );
    }


/*!
  Minimizes the popup client
 */
void Workspace::slotWindowMinimize()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    performWindowOperation( c, Options::MinimizeOp );
    }

/*!
  Shades/unshades the popup client respectively
 */
void Workspace::slotWindowShade()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    performWindowOperation( c, Options::ShadeOp );
    }

/*!
  Raises the popup client
 */
void Workspace::slotWindowRaise()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if ( c )
        raiseClient( c );
    }

/*!
  Lowers the popup client
 */
void Workspace::slotWindowLower()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if ( c )
        lowerClient( c );
    }

/*!
  Does a toggle-raise-and-lower on the popup client;
  */
void Workspace::slotWindowRaiseOrLower()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if  ( c )
        raiseOrLowerClient( c );
    }

void Workspace::slotWindowOnAllDesktops()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c )
        c->setOnAllDesktops( !c->isOnAllDesktops());
    }

void Workspace::slotWindowFullScreen()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c )
        performWindowOperation( c, Options::FullScreenOp );
    }

void Workspace::slotWindowNoBorder()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c )
        performWindowOperation( c, Options::NoBorderOp );
    }

void Workspace::slotWindowAbove()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c )
        performWindowOperation( c, Options::KeepAboveOp );
    }

void Workspace::slotWindowBelow()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c )
        performWindowOperation( c, Options::KeepBelowOp );
    }
void Workspace::slotSetupWindowShortcut()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    if( c )
        performWindowOperation( c, Options::SetupWindowShortcutOp );
    }

/*!
  Move window to next desktop
 */
void Workspace::slotWindowToNextDesktop()
    {
    windowToNextDesktop( active_popup_client ? active_popup_client : active_client );
    }
    
void Workspace::windowToNextDesktop( Client* c )
    {
    int d = currentDesktop() + 1;
    if ( d > numberOfDesktops() )
        d = 1;
    if (c && !c->isDesktop()
        && !c->isDock() && !c->isTopMenu())
        {
        setClientIsMoving( c );
        setCurrentDesktop( d );
        setClientIsMoving( NULL );
        }
    }

/*!
  Move window to previous desktop
 */
void Workspace::slotWindowToPreviousDesktop()
    {
    windowToPreviousDesktop( active_popup_client ? active_popup_client : active_client );
    }
    
void Workspace::windowToPreviousDesktop( Client* c )
    {
    int d = currentDesktop() - 1;
    if ( d <= 0 )
        d = numberOfDesktops();
    if (c && !c->isDesktop()
        && !c->isDock() && !c->isTopMenu())
        {
        setClientIsMoving( c );
        setCurrentDesktop( d );
        setClientIsMoving( NULL );
        }
    }

void Workspace::slotWindowToDesktopRight()
    {
    int d = desktopToRight( currentDesktop());
    if( d == currentDesktop())
        return;
    Client* c = active_popup_client ? active_popup_client : active_client;
    if (c && !c->isDesktop()
        && !c->isDock() && !c->isTopMenu())
        {
        setClientIsMoving( c );
        setCurrentDesktop( d );
        setClientIsMoving( NULL );
        }
    }

void Workspace::slotWindowToDesktopLeft()
    {
    int d = desktopToLeft( currentDesktop());
    if( d == currentDesktop())
        return;
    Client* c = active_popup_client ? active_popup_client : active_client;
    if (c && !c->isDesktop()
        && !c->isDock() && !c->isTopMenu())
        {
        setClientIsMoving( c );
        setCurrentDesktop( d );
        setClientIsMoving( NULL );
        }
    }

void Workspace::slotWindowToDesktopUp()
    {
    int d = desktopUp( currentDesktop());
    if( d == currentDesktop())
        return;
    Client* c = active_popup_client ? active_popup_client : active_client;
    if (c && !c->isDesktop()
        && !c->isDock() && !c->isTopMenu())
        {
        setClientIsMoving( c );
        setCurrentDesktop( d );
        setClientIsMoving( NULL );
        }
    }

void Workspace::slotWindowToDesktopDown()
    {
    int d = desktopDown( currentDesktop());
    if( d == currentDesktop())
        return;
    Client* c = active_popup_client ? active_popup_client : active_client;
    if (c && !c->isDesktop()
        && !c->isDock() && !c->isTopMenu())
        {
        setClientIsMoving( c );
        setCurrentDesktop( d );
        setClientIsMoving( NULL );
        }
    }


/*!
  Kill Window feature, similar to xkill
 */
void Workspace::slotKillWindow()
    {
    KillWindow kill( this );
    kill.start();
    }

/*!
  Suspend Window feature
 */
void Workspace::slotSuspendWindow()
    {
    active_popup_client->suspendWindow();
    }

/*!
  Resume Window feature
 */
void Workspace::slotResumeWindow()
    {
    active_popup_client->resumeWindow();
    }

/*!
  Sends the popup client to desktop \a desk

  Internal slot for the window operation menu
 */
void Workspace::slotSendToDesktop( int desk )
    {
    if ( !active_popup_client )
        return;
    if ( desk == 0 ) 
        { // the 'on_all_desktops' menu entry
        active_popup_client->setOnAllDesktops( !active_popup_client->isOnAllDesktops());
        return;
        }

    sendClientToDesktop( active_popup_client, desk, false );

    }

/*!
  Shows the window operations popup menu for the activeClient()
 */
void Workspace::slotWindowOperations()
    {
    if ( !active_client )
        return;
    TQPoint pos = active_client->pos() + active_client->clientPos();
    showWindowMenu( pos.x(), pos.y(), active_client );
    }

void Workspace::showWindowMenu( const TQRect &pos, Client* cl )
    {
    if (!tdeApp->authorizeTDEAction("twin_rmb"))
        return;
    if( !cl )
        return;
    if( active_popup_client != NULL ) // recursion
        return;
    if ( cl->isDesktop()
        || cl->isDock()
        || cl->isTopMenu()
        || cl->isModalSystemNotification())
        return;

    active_popup_client = cl;
    TQPopupMenu* p = clientPopup();
    active_popup = p;
    int x = pos.left();
    int y = pos.bottom();
    clientPopupAboutToShow(); // needed for sizeHint() to be correct :-/

    TQRect area = clientArea(ScreenArea, TQPoint(x, y), currentDesktop());
    TQSize hint = p->sizeHint();
    if (x < 0) x = area.right() - hint.width() + x;
    if (y < 0) y = area.bottom() - hint.height() + y;

    if (pos.bottom() == pos.top())
        p->exec( TQPoint( x, y ) );
    else
        {
        if (y + hint.height() < area.height())
            p->exec( TQPoint( x, y ) );
        else
            p->exec( TQPoint( x, pos.top() - hint.height() ) );
        }
    // active popup may be already changed (e.g. the window shortcut dialog)
    if( active_popup == p )
        closeActivePopup();
    }

/*!
  Closes the popup client
 */
void Workspace::slotWindowClose()
    {
    if ( tab_box->isVisible())
        return;
    Client* c = active_popup_client ? active_popup_client : active_client;
    performWindowOperation( c, Options::CloseOp );
    }

/*!
  Starts keyboard move mode for the popup client
 */
void Workspace::slotWindowMove()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    performWindowOperation( c, Options::UnrestrictedMoveOp );
    }

/*!
  Starts keyboard resize mode for the popup client
 */
void Workspace::slotWindowResize()
    {
    Client* c = active_popup_client ? active_popup_client : active_client;
    performWindowOperation( c, Options::UnrestrictedResizeOp );
    }

void Client::setShortcut( const TQString& _cut )
    {
    TQString cut = rules()->checkShortcut( _cut );
    if( cut.isEmpty())
        return setShortcutInternal( TDEShortcut());
// Format:
// base+(abcdef)<space>base+(abcdef)
// E.g. Alt+Ctrl+(ABCDEF) Win+X,Win+(ABCDEF)
    if( !cut.contains( '(' ) && !cut.contains( ')' ) && !cut.contains( ' ' ))
        {
        if( workspace()->shortcutAvailable( TDEShortcut( cut ), this ))
            setShortcutInternal( TDEShortcut( cut ));
        else
            setShortcutInternal( TDEShortcut());
        return;
        }
    TQValueList< TDEShortcut > keys;
    TQStringList groups = TQStringList::split( ' ', cut );
    for( TQStringList::ConstIterator it = groups.begin();
         it != groups.end();
         ++it )
        {
        TQRegExp reg( "(.*\\+)\\((.*)\\)" );
        if( reg.search( *it ) > -1 )
            {
            TQString base = reg.cap( 1 );
            TQString list = reg.cap( 2 );
            for( unsigned int i = 0;
                 i < list.length();
                 ++i )
                {
                TDEShortcut c( base + list[ i ] );
                if( !c.isNull())
                    keys.append( c );
                }
            }
        }
    for( TQValueList< TDEShortcut >::ConstIterator it = keys.begin();
         it != keys.end();
         ++it )
        {
        if( _shortcut == *it ) // current one is in the list
            return;
        }
    for( TQValueList< TDEShortcut >::ConstIterator it = keys.begin();
         it != keys.end();
         ++it )
        {
        if( workspace()->shortcutAvailable( *it, this ))
            {
            setShortcutInternal( *it );
            return;
            }
        }
    setShortcutInternal( TDEShortcut());
    }

void Client::setShortcutInternal( const TDEShortcut& cut )
    {
    if( _shortcut == cut )
        return;
    _shortcut = cut;
    updateCaption();
    workspace()->clientShortcutUpdated( this );
    }

bool Workspace::shortcutAvailable( const TDEShortcut& cut, Client* ignore ) const
    {
    // TODO check global shortcuts etc.
    for( ClientList::ConstIterator it = clients.begin();
         it != clients.end();
         ++it )
        {
        if( (*it) != ignore && (*it)->shortcut() == cut )
            return false;    
        }
    return true;
    }

} // namespace
