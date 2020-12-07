/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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


#include "desktop.h"
#include "krootwm.h"
#include "bgmanager.h"
#include "bgsettings.h"
#include "startupid.h"
#include "kdiconview.h"
#include "minicli.h"
#include "kdesktopsettings.h"
#include "tdelaunchsettings.h"

#include <string.h>
#include <unistd.h>
#include <kcolordrag.h>
#include <kurldrag.h>
#include <stdlib.h>
#include <tdeio/job.h>
#include <tqfile.h>

#include <tqdir.h>
#include <tqevent.h>
#include <tqtooltip.h>

#include <netwm.h>
#include <dcopclient.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kimageio.h>
#include <kinputdialog.h>
#include <kipc.h>
#include <tdelocale.h>
#include <tdeio/netaccess.h>
#include <kprocess.h>
#include <tdesycoca.h>
#include <tdetempfile.h>
#include <tdemessagebox.h>
#include <kglobalaccel.h>
#include <twinmodule.h>
#include <krun.h>
#include <twin.h>
#include <tdeglobalsettings.h>
#include <tdepopupmenu.h>
#include <tdeapplication.h>
#include <kdirlister.h>
// Create the equivalent of TDEAccelBase::connectItem
// and then remove this include and fix reconnects in initRoot() -- ellis
//#include <tdeaccelbase.h>

extern int kdesktop_screen_number;
extern TQCString kdesktop_name, kicker_name, twin_name;

KRootWidget::KRootWidget() : TQObject()
{
     kapp->desktop()->installEventFilter(this);
     kapp->desktop()->setAcceptDrops( true );
}

bool KRootWidget::eventFilter ( TQObject *, TQEvent * e )
{
     if (e->type() == TQEvent::MouseButtonPress)
     {
       TQMouseEvent *me = TQT_TQMOUSEEVENT(e);
       KRootWm::self()->mousePressed( me->globalPos(), me->button() );
       return true;
     }
     else if (e->type() == TQEvent::Wheel)
     {
       TQWheelEvent *we = TQT_TQWHEELEVENT(e);
       emit wheelRolled(we->delta());
       return true;
     }
     else if ( e->type() == TQEvent::DragEnter )
     {
       TQDragEnterEvent* de = static_cast<TQDragEnterEvent*>( e );
       bool b = !TDEGlobal::config()->isImmutable() && !TDEGlobal::dirs()->isRestrictedResource( "wallpaper" );

       bool imageURL = false;
       if ( KURLDrag::canDecode( de ) )
       {
         KURL::List list;
         KURLDrag::decode( de, list );
         KURL url = list.first();
         KMimeType::Ptr mime = KMimeType::findByURL( url );
         if ( !KImageIO::type( url.path() ).isEmpty() ||
              KImageIO::isSupported( mime->name(), KImageIO::Reading ) || mime->is( "image/svg+xml" ) )
           imageURL = true;
       }

       b = b && ( KColorDrag::canDecode( de ) || TQImageDrag::canDecode( de ) || imageURL );
       de->accept( b );
       return true;
     }
     else if ( e->type() == TQEvent::Drop )
     {
       TQDropEvent* de = static_cast<TQDropEvent*>( e );
       if ( KColorDrag::canDecode( de ) ) 
         emit colorDropEvent( de );
       else if ( TQImageDrag::canDecode( de ) )
         emit imageDropEvent( de );
       else if ( KURLDrag::canDecode( de ) ) {
	 KURL::List list;
         KURLDrag::decode( de, list );
         KURL url = list.first();
         emit newWallpaper( url );
       }
       return true;
     }
     return false; // Don't filter.
}

// -----------------------------------------------------------------------------
#define DEFAULT_DELETEACTION 1

KDesktop::WheelDirection KDesktop::m_eWheelDirection = KDesktop::m_eDefaultWheelDirection;
const char* KDesktop::m_wheelDirectionStrings[2] = { "Forward", "Reverse" };

KDesktop::KDesktop( SaverEngine* saver, bool x_root_hack, bool wait_for_kded ) :
    TQWidget( 0L, "desktop", (WFlags)(WResizeNoErase | ( x_root_hack ? (WStyle_Customize | WStyle_NoBorder) : 0)) ),
    KDesktopIface(),
    // those two WStyle_ break kdesktop when the root-hack isn't used (no Dnd)
   startup_id( NULL ), m_waitForKicker(0)
{
  m_pSaver = saver;

  NETRootInfo i( tqt_xdisplay(), NET::Supported );
  m_wmSupport = i.isSupported( NET::WM2ShowingDesktop );

  m_bWaitForKded = wait_for_kded;
  m_miniCli = 0; // created on demand
  keys = 0; // created later
  TDEGlobal::locale()->insertCatalogue("kdesktop");
  TDEGlobal::locale()->insertCatalogue("libkonq"); // needed for apps using libkonq
  TDEGlobal::locale()->insertCatalogue("libdmctl");

  setCaption( "KDE Desktop");

  setAcceptDrops(true); // WStyle_Customize seems to disable that
  m_pKwinmodule = new KWinModule( TQT_TQOBJECT(this) );

  kapp->dcopClient()->setNotifications(true);
  kapp->dcopClient()->connectDCOPSignal(kicker_name, kicker_name, "desktopIconsAreaChanged(TQRect, int)",
                                        "KDesktopIface", "desktopIconsAreaChanged(TQRect, int)", false);

  // Dont repaint on configuration changes during construction
  m_bInit = true;

  // It's the child widget that gets the focus, not us
  setFocusPolicy( TQ_NoFocus );

  if ( x_root_hack )
  {
    // this is a ugly hack to make Dnd work
    // Matthias told me that it won't be necessary with twin
    // actually my first try with ICCCM (Dirk) :-)
    unsigned long data[2];
    data[0] = (unsigned long) 1;
    data[1] = (unsigned long) 0; // None; (Werner)
    Atom wm_state = XInternAtom(tqt_xdisplay(), "WM_STATE", False);
    XChangeProperty(tqt_xdisplay(), winId(), wm_state, wm_state, 32,
                    PropModeReplace, (unsigned char *)data, 2);

  }

  setGeometry( TQApplication::desktop()->geometry() );
  lower();

  connect( kapp, TQT_SIGNAL( shutDown() ),
           this, TQT_SLOT( slotShutdown() ) );

  connect(kapp, TQT_SIGNAL(settingsChanged(int)),
          this, TQT_SLOT(slotSettingsChanged(int)));
  kapp->addKipcEventMask(KIPC::SettingsChanged);

  kapp->addKipcEventMask(KIPC::IconChanged);
  connect(kapp, TQT_SIGNAL(iconChanged(int)), this, TQT_SLOT(slotIconChanged(int)));

  connect(KSycoca::self(), TQT_SIGNAL(databaseChanged()),
          this, TQT_SLOT(slotDatabaseChanged()));

  m_pIconView = 0;
  m_pRootWidget = 0;
  bgMgr = 0;
  initRoot();

  TQTimer::singleShot(0, this, TQT_SLOT( slotStart() ));

#if (TQT_VERSION-0 >= 0x030200) // XRANDR support
  connect( kapp->desktop(), TQT_SIGNAL( resized( int )), TQT_SLOT( desktopResized()));
#endif
}

void
KDesktop::initRoot()
{
  Display *dpy = tqt_xdisplay();
  Window root = RootWindow(dpy, kdesktop_screen_number);
  XDefineCursor(dpy, root, cursor().handle());
  
  m_bDesktopEnabled = KDesktopSettings::desktopEnabled();
  if ( !m_bDesktopEnabled && !m_pRootWidget )
  {
     hide();
     delete bgMgr;
     bgMgr = 0;
     if ( m_pIconView )
        m_pIconView->saveIconPositions();
     delete m_pIconView;
     m_pIconView = 0;

     { // trigger creation of QToolTipManager, it does XSelectInput() on the root window
     TQWidget w;
     TQToolTip::add( &w, "foo" );
     }
     // NOTE: If mouse clicks stop working again, it's most probably something doing XSelectInput()
     // on the root window after this, and setting it to some fixed value instead of adding its mask.
     XWindowAttributes attrs;
     XGetWindowAttributes(dpy, root, &attrs);
     XSelectInput(dpy, root, attrs.your_event_mask | ButtonPressMask);

     m_pRootWidget = new KRootWidget;
     connect(m_pRootWidget, TQT_SIGNAL(wheelRolled(int)), this, TQT_SLOT(slotSwitchDesktops(int)));
     connect(m_pRootWidget, TQT_SIGNAL(colorDropEvent(TQDropEvent*)), this, TQT_SLOT(handleColorDropEvent(TQDropEvent*)) );
     connect(m_pRootWidget, TQT_SIGNAL(imageDropEvent(TQDropEvent*)), this, TQT_SLOT(handleImageDropEvent(TQDropEvent*)) );
     connect(m_pRootWidget, TQT_SIGNAL(newWallpaper(const KURL&)), this, TQT_SLOT(slotNewWallpaper(const KURL&)) );

     // Geert Jansen: backgroundmanager belongs here
     // TODO tell KBackgroundManager if we change widget()
     bgMgr = new KBackgroundManager( m_pIconView, m_pKwinmodule );
     bgMgr->setExport(1);
     connect( bgMgr, TQT_SIGNAL( initDone()), TQT_SLOT( backgroundInitDone()));
     if (!m_bInit)
     {
        delete KRootWm::self();
        KRootWm* krootwm = new KRootWm( m_pSaver, this ); // handler for root menu (used by kdesktop on RMB click)
        keys->setSlot("Lock Session", krootwm, TQT_SLOT(slotLock()));
        keys->updateConnections();
     }
  }
  else if (m_bDesktopEnabled && !m_pIconView)
  {
     delete bgMgr;
     bgMgr = 0;
     delete m_pRootWidget;
     m_pRootWidget = 0;
     m_pIconView = new KDIconView( this, 0 );
     connect( m_pIconView, TQT_SIGNAL( imageDropEvent( TQDropEvent * ) ),
              this, TQT_SLOT( handleImageDropEvent( TQDropEvent * ) ) );
     connect( m_pIconView, TQT_SIGNAL( colorDropEvent( TQDropEvent * ) ),
              this, TQT_SLOT( handleColorDropEvent( TQDropEvent * ) ) );
     connect( m_pIconView, TQT_SIGNAL( newWallpaper( const KURL & ) ),
              this, TQT_SLOT( slotNewWallpaper( const KURL & ) ) );
     connect( m_pIconView, TQT_SIGNAL( wheelRolled( int ) ),
              this, TQT_SLOT( slotSwitchDesktops( int ) ) );

     // All the QScrollView/QWidget-specific stuff should go here, so that we can use
     // another qscrollview/widget instead of the iconview and use the same code
     m_pIconView->setVScrollBarMode( TQScrollView::AlwaysOff );
     m_pIconView->setHScrollBarMode( TQScrollView::AlwaysOff );
     m_pIconView->setDragAutoScroll( false );
     m_pIconView->setFrameStyle( TQFrame::NoFrame );
     m_pIconView->viewport()->setBackgroundMode( X11ParentRelative );
     m_pIconView->setFocusPolicy( TQ_StrongFocus );
     m_pIconView->viewport()->setFocusPolicy( TQ_StrongFocus );
     m_pIconView->setGeometry( geometry() );
     m_pIconView->show();

     // Geert Jansen: backgroundmanager belongs here
     // TODO tell KBackgroundManager if we change widget()
     bgMgr = new KBackgroundManager( m_pIconView, m_pKwinmodule );
     bgMgr->setExport(1);
     connect( bgMgr, TQT_SIGNAL( initDone()), TQT_SLOT( backgroundInitDone()));

     // make sure it is initialized before we first call updateWorkArea()
     m_pIconView->initConfig( m_bInit );

     // set the size of the area for desktop icons placement
     {
        TQByteArray data, result;
        TQDataStream arg(data, IO_WriteOnly);
        arg << kdesktop_screen_number;
        TQCString replyType;
        TQRect area;

        if ( kapp->dcopClient()->call(kicker_name, kicker_name, "desktopIconsArea(int)",
                                       data, replyType, result, false, 2000) )
        {
          TQDataStream res(result, IO_ReadOnly);
          res >> area;

          m_pIconView->updateWorkArea(area);
        }
        else
          if ( m_bInit )
          {
            // if we failed to get the information from kicker wait a little - probably
            // this is the KDE startup and kicker is simply not running yet
            m_waitForKicker = new TQTimer(this);
            connect(m_waitForKicker, TQT_SIGNAL(timeout()), this, TQT_SLOT(slotNoKicker()));
            m_waitForKicker->start(15000, true);
          }
          else  // we are not called from the ctor, so kicker should already run
          {
            area = twinModule()->workArea(twinModule()->currentDesktop());
            m_pIconView->updateWorkArea(area);
          }
     }

     if (!m_bInit)
     {
        m_pIconView->start();
        delete KRootWm::self();
        KRootWm* krootwm = new KRootWm( m_pSaver, this ); // handler for root menu (used by kdesktop on RMB click)
        keys->setSlot("Lock Session", krootwm, TQT_SLOT(slotLock()));
        keys->updateConnections();
     }
   } else {
     DCOPRef r( "ksmserver", "ksmserver" );
     r.send( "resumeStartup", TQCString( "kdesktop" ));
   }

   KWin::setType( winId(), NET::Desktop );
   KWin::setState( winId(), NET::SkipPager );
   KWin::setOnAllDesktops( winId(), true );
}

void KDesktop::slotNoKicker()
{
    kdDebug(1204) << "KDesktop::slotNoKicker ... kicker did not respond" << endl;
    // up till now, we got no desktopIconsArea from kicker - probably
    // it's not running, so use the area from KWinModule
    TQRect area = twinModule()->workArea(twinModule()->currentDesktop());
    m_pIconView->updateWorkArea(area);
}

void
KDesktop::backgroundInitDone()
{
    //kdDebug(1204) << "KDesktop::backgroundInitDone" << endl;
    // avoid flicker
    if (m_bDesktopEnabled)
    {
       const TQPixmap *bg = TQT_TQWIDGET(TQApplication::desktop()->screen())->backgroundPixmap();
       if ( bg )
          m_pIconView->setErasePixmap( *bg );

       show();
       kapp->sendPostedEvents();
    }

    DCOPRef r( "ksmserver", "ksmserver" );
    r.send( "resumeStartup", TQCString( "kdesktop" ));
}

void
KDesktop::slotStart()
{
  //kdDebug(1204) << "KDesktop::slotStart" << endl;
  if (!m_bInit) return;

  // In case we started without database
  KImageIO::registerFormats();

  initConfig();

//   if (m_bDesktopEnabled)
//   {
//      // We need to be visible in order to insert icons, even if the background isn't ready yet...

//      show();
//   }

  // Now we may react to configuration changes
  m_bInit = false;

  if (m_pIconView)
     m_pIconView->start();

  // Global keys
  keys = new TDEGlobalAccel( TQT_TQOBJECT(this) );
  (void) new KRootWm( m_pSaver, this );

#include "kdesktopbindings.cpp"

  keys->readSettings();
  keys->updateConnections();

  connect(kapp, TQT_SIGNAL(appearanceChanged()), TQT_SLOT(slotConfigure()));

  TQTimer::singleShot(300, this, TQT_SLOT( slotUpAndRunning() ));
}

void
KDesktop::runAutoStart()
{
     // now let's execute all the stuff in the autostart folder.
     // the stuff will actually be really executed when the event loop is
     // entered, since KRun internally uses a QTimer
     TQDir dir( TDEGlobalSettings::autostartPath() );
     TQStringList entries = dir.entryList( TQDir::Files );
     TQStringList::Iterator it = entries.begin();
     TQStringList::Iterator end = entries.end();
     for (; it != end; ++it )
     {
            // Don't execute backup files
            if ( (*it).right(1) != "~" && (*it).right(4) != ".bak" &&
                 ( (*it)[0] != '%' || (*it).right(1) != "%" ) &&
                 ( (*it)[0] != '#' || (*it).right(1) != "#" ) )
            {
                KURL url;
                url.setPath( dir.absPath() + '/' + (*it) );
                (void) new KRun( url, 0, true );
            }
     }
}

// -----------------------------------------------------------------------------

KDesktop::~KDesktop()
{
  delete m_miniCli;
  m_miniCli = 0; // see #120382
  delete bgMgr;
  bgMgr = 0;
  delete startup_id;
}

// -----------------------------------------------------------------------------

void KDesktop::initConfig()
{
    if (m_pIconView)
       m_pIconView->initConfig( m_bInit );

    if ( keys )
    {
        keys->readSettings();
        keys->updateConnections();
    }

    TDELaunchSettings::self()->readConfig();
    if( !TDELaunchSettings::busyCursor() )
    {
        delete startup_id;
        startup_id = NULL;
    }
    else
    {
        if( startup_id == NULL )
            startup_id = new StartupId;
        startup_id->configure();
    }

    set_vroot = KDesktopSettings::setVRoot();
    slotSetVRoot(); // start timer

    m_bWheelSwitchesWorkspace = KDesktopSettings::wheelSwitchesWorkspace();

    const char* forward_string = m_wheelDirectionStrings[Forward];
    m_eWheelDirection =
        (KDesktopSettings::wheelDirection() == forward_string) ? Forward : Reverse;
}

// -----------------------------------------------------------------------------

void KDesktop::slotExecuteCommand()
{
    // this function needs to be duplicated since it appears that one
    // cannot have a 'slot' be a DCOP method.  if this changes in the
    // future, then 'slotExecuteCommand' and 'popupExecuteCommand' can
    // merge into one slot.
    popupExecuteCommand();
}

/*
  Shows minicli
 */
void KDesktop::popupExecuteCommand()
{
  popupExecuteCommand("");
}

void KDesktop::popupExecuteCommand(const TQString& command)
{
  if (m_bInit)
      return;

  if (!kapp->authorize("run_command"))
      return;

  // Created on demand
  if ( !m_miniCli )
  {
      m_miniCli = new Minicli( this );
      m_miniCli->adjustSize(); // for the centering below
  }

  if (!command.isEmpty())
      m_miniCli->setCommand(command);

  // Move minicli to the current desktop
  NETWinInfo info( tqt_xdisplay(), m_miniCli->winId(), tqt_xrootwin(), NET::WMDesktop );
  int currentDesktop = twinModule()->currentDesktop();
  if ( info.desktop() != currentDesktop )
      info.setDesktop( currentDesktop );

  if ( m_miniCli->isVisible() ) {
      KWin::forceActiveWindow( m_miniCli->winId() );
  } else {
      NETRootInfo i( tqt_xdisplay(), NET::Supported );
      if( !i.isSupported( NET::WM2FullPlacement )) {
          TQRect rect = TDEGlobalSettings::desktopGeometry(TQCursor::pos());
          m_miniCli->move(rect.x() + (rect.width() - m_miniCli->width())/2,
                          rect.y() + (rect.height() - m_miniCli->height())/2);
      }
      m_miniCli->show(); // non-modal
  }
}

void KDesktop::slotSwitchUser()
{
     KRootWm::self()->slotSwitchUser();
}

void KDesktop::slotShowWindowList()
{
     KRootWm::self()->slotWindowList();
}

void KDesktop::slotShowTaskManager()
{
    //kdDebug(1204) << "Launching KSysGuard..." << endl;
    TDEProcess* p = new TDEProcess;
    TQ_CHECK_PTR(p);

    *p << "ksysguard";
    *p << "--showprocesses";

    p->start(TDEProcess::DontCare);

    delete p;
}

// -----------------------------------------------------------------------------

void KDesktop::rearrangeIcons()
{
    if (m_pIconView)
        m_pIconView->rearrangeIcons();
}

void KDesktop::lineupIcons()
{
    if (m_pIconView)
        m_pIconView->lineupIcons();
}

void KDesktop::selectAll()
{
    if (m_pIconView)
        m_pIconView->selectAll( true );
}

void KDesktop::unselectAll()
{
    if (m_pIconView)
        m_pIconView->selectAll( false );
}

TQStringList KDesktop::selectedURLs()
{
    if (m_pIconView)
        return m_pIconView->selectedURLs();
    return TQStringList();
}

void KDesktop::refreshIcons()
{
    if (m_pIconView)
        m_pIconView->refreshIcons();
}

void KDesktop::setShowDesktop( bool b )
{
    bool m_showingDesktop = showDesktopState();

    if (b == m_showingDesktop)
    {
        return;
    }

    if( m_wmSupport )
    {
        NETRootInfo i( tqt_xdisplay(), 0 );
        i.setShowingDesktop( b );
        return;
    }

    if (b)
    {
        m_activeWindow = twinModule()->activeWindow();
        m_iconifiedList.clear();

        const TQValueList<WId> windows = twinModule()->windows();
        for (TQValueList<WId>::ConstIterator it = windows.begin();
             it != windows.end();
             ++it)
        {
            WId w = *it;

            NETWinInfo info( tqt_xdisplay(), w, tqt_xrootwin(),
                             NET::XAWMState | NET::WMDesktop );

            if (info.mappingState() == NET::Visible &&
                (info.desktop() == NETWinInfo::OnAllDesktops ||
                 info.desktop() == (int)twinModule()->currentDesktop()))
            {
                m_iconifiedList.append( w );
            }
        }

        // find first, hide later, otherwise transients may get minimized
        // with the window they're transient for
        for (TQValueVector<WId>::Iterator it = m_iconifiedList.begin();
             it != m_iconifiedList.end();
             ++it)
        {
            KWin::iconifyWindow( *it, false );
        }

        // on desktop changes or when a window is deiconified, we abort the show desktop mode
        connect(twinModule(), TQT_SIGNAL(currentDesktopChanged(int)),
                TQT_SLOT(slotCurrentDesktopChanged(int)));
        connect(twinModule(), TQT_SIGNAL(windowChanged(WId,unsigned int)),
                TQT_SLOT(slotWindowChanged(WId,unsigned int)));
        connect(twinModule(), TQT_SIGNAL(windowAdded(WId)),
                TQT_SLOT(slotWindowAdded(WId)));
    }
    else
    {
        disconnect(twinModule(), TQT_SIGNAL(currentDesktopChanged(int)),
                   this, TQT_SLOT(slotCurrentDesktopChanged(int)));
        disconnect(twinModule(), TQT_SIGNAL(windowChanged(WId,unsigned int)),
                   this, TQT_SLOT(slotWindowChanged(WId,unsigned int)));
        disconnect(twinModule(), TQT_SIGNAL(windowAdded(WId)),
                   this, TQT_SLOT(slotWindowAdded(WId)));

        for (TQValueVector<WId>::ConstIterator it = m_iconifiedList.begin();
             it != m_iconifiedList.end();
             ++it)
        {
            KWin::deIconifyWindow(*it, false);
        }

        KWin::forceActiveWindow(m_activeWindow);
    }

    m_showingDesktop = b;
    emit desktopShown(m_showingDesktop);
}

void KDesktop::slotCurrentDesktopChanged(int)
{
    setShowDesktop( false );
}

void KDesktop::slotWindowAdded(WId w)
{
    bool m_showingDesktop = showDesktopState();

    if (!m_showingDesktop)
    {
        return;
    }

    NETWinInfo inf(tqt_xdisplay(), w, tqt_xrootwin(),
                   NET::XAWMState | NET::WMWindowType);
    NET::WindowType windowType = inf.windowType(NET::AllTypesMask);

    if ((windowType == NET::Normal || windowType == NET::Unknown) &&
        inf.mappingState() == NET::Visible)
    {
        TDEConfig twincfg( "twinrc", true ); // see in twin
        twincfg.setGroup( "Windows" );
        if( twincfg.readBoolEntry( "ShowDesktopIsMinimizeAll", false ))
        {
            m_iconifiedList.clear();
            m_showingDesktop = false;
            emit desktopShown(false);
        }
        else
        {
            m_activeWindow = w;
            setShowDesktop(false);
        }
    }
}

void KDesktop::slotWindowChanged(WId w, unsigned int dirty)
{
    bool m_showingDesktop = showDesktopState();

    if (!m_showingDesktop)
    {
        return;
    }

    if (dirty & NET::XAWMState)
    {
        NETWinInfo inf(tqt_xdisplay(), w, tqt_xrootwin(),
                       NET::XAWMState | NET::WMWindowType);
        NET::WindowType windowType = inf.windowType(NET::AllTypesMask);

        if ((windowType == NET::Normal || windowType == NET::Unknown) &&
            inf.mappingState() == NET::Visible)
        {
            // a window was deiconified, abort the show desktop mode.
            m_iconifiedList.clear();
            m_showingDesktop = false;
            emit desktopShown(false);
        }
    }
}

bool KDesktop::showDesktopState()
{
    return twinModule()->showingDesktop();
}

void KDesktop::toggleShowDesktop()
{
    setShowDesktop(!showDesktopState());
}

TDEActionCollection * KDesktop::actionCollection()
{
    if (!m_pIconView)
       return 0;
    return m_pIconView->actionCollection();
}

KURL KDesktop::url() const
{
    if (m_pIconView)
        return m_pIconView->url();
    return KURL();
}

// -----------------------------------------------------------------------------

void KDesktop::slotConfigure()
{
    configure();
}

void KDesktop::configure()
{
    // re-read configuration and apply it
    TDEGlobal::config()->reparseConfiguration();
    KDesktopSettings::self()->readConfig();

    // If we have done start() already, then re-configure.
    // Otherwise, start() will call initConfig anyway
    if (!m_bInit)
    {
       initRoot();
       initConfig();
       KRootWm::self()->initConfig();
    }

    if (keys)
    {
       keys->readSettings();
       keys->updateConnections();
    }
}

void KDesktop::slotSettingsChanged(int category)
{
    //kdDebug(1204) << "KDesktop::slotSettingsChanged" << endl;
    if (category == TDEApplication::SETTINGS_PATHS)
    {
        kdDebug(1204) << "KDesktop::slotSettingsChanged SETTINGS_PATHS" << endl;
        if (m_pIconView)
            m_pIconView->recheckDesktopURL();
    }
    else if (category == TDEApplication::SETTINGS_SHORTCUTS)
    {
        kdDebug(1204) << "KDesktop::slotSettingsChanged SETTINGS_SHORTCUTS" << endl;
        keys->readSettings();
        keys->updateConnections();
    }
}

void KDesktop::slotIconChanged(int group)
{
    if ( group == TDEIcon::Desktop )
    {
        kdDebug(1204) << "KDesktop::slotIconChanged" << endl;
        refresh();
    }
}

void KDesktop::slotDatabaseChanged()
{
    //kdDebug(1204) << "KDesktop::slotDatabaseChanged" << endl;
    if (m_bInit) // kded is done, now we can "start" for real
        slotStart();
    if (m_pIconView && KSycoca::isChanged("mimetypes"))
        m_pIconView->refreshMimeTypes();
}

void KDesktop::refresh()
{
  // George Staikos 3/14/01
  // This bit will just refresh the desktop and icons.  Now I have code
  // in KWin to do a complete refresh so this isn't really needed.
  // I'll leave it in here incase the plan is changed again
#if 0
  m_bNeedRepaint |= 1;
  updateWorkArea();
#endif
  kapp->dcopClient()->send( twin_name, "", "refresh()", TQString(""));
  refreshIcons();
}

// -----------------------------------------------------------------------------

void KDesktop::slotSetVRoot()
{
    if (!m_pIconView)
        return;

    if (KWin::windowInfo(winId()).mappingState() == NET::Withdrawn) {
        TQTimer::singleShot(100, this, TQT_SLOT(slotSetVRoot()));
        return;
    }

    unsigned long rw = RootWindowOfScreen(ScreenOfDisplay(tqt_xdisplay(), tqt_xscreen()));
    unsigned long vroot_data[1] = { m_pIconView->viewport()->winId() };
    static Atom vroot = XInternAtom(tqt_xdisplay(), "__SWM_VROOT", False);

    Window rootReturn, parentReturn, *children;
    unsigned int numChildren;
    Window top = winId();
    while (1) {
        /*int ret = */XQueryTree(tqt_xdisplay(), top , &rootReturn, &parentReturn,
                                 &children, &numChildren);
        if (children)
            XFree((char *)children);
        if (parentReturn == rw) {
            break;
        } else
            top = parentReturn;
    }
    if ( set_vroot )
        XChangeProperty(tqt_xdisplay(), top, vroot, XA_WINDOW, 32,
                        PropModeReplace, (unsigned char *)vroot_data, 1);
    else
        XDeleteProperty (tqt_xdisplay(), top, vroot);
}

// -----------------------------------------------------------------------------

void KDesktop::slotShutdown()
{
    if ( m_pIconView )
        m_pIconView->saveIconPositions();
    if ( m_miniCli )
        m_miniCli->saveConfig();
}

// don't hide when someone presses Alt-F4 on us
void KDesktop::closeEvent(TQCloseEvent *e)
{
    e->ignore();
}

void KDesktop::desktopIconsAreaChanged(const TQRect &area, int screen)
{
    // hurra! kicker is alive
    if ( m_waitForKicker ) m_waitForKicker->stop();

    // -2: all screens
    // -1: primary screen
    // else: screen number

    if (screen <= -2)
       screen = kdesktop_screen_number;
    else if (screen == -1)
       screen = kapp->desktop()->primaryScreen();

    // This is pretty broken, mixes Xinerama and non-Xinerama multihead
    // and generally doesn't seem to be required anyway => ignore screen.
    if ( /*(screen == kdesktop_screen_number) &&*/ m_pIconView ) {
        m_pIconView->updateWorkArea(area);
    }
}

void KDesktop::slotSwitchDesktops(int delta)
{
    if(m_bWheelSwitchesWorkspace && KWin::numberOfDesktops() > 1)
    {
      int newDesk, curDesk = KWin::currentDesktop();

      if( (delta < 0 && m_eWheelDirection == Forward) || (delta > 0 && m_eWheelDirection == Reverse) )
        newDesk = curDesk % KWin::numberOfDesktops() + 1;
      else
        newDesk = ( KWin::numberOfDesktops() + curDesk - 2 ) % KWin::numberOfDesktops() + 1;

      KWin::setCurrentDesktop( newDesk );
    }
}

void KDesktop::handleColorDropEvent(TQDropEvent * e)
{
    TDEPopupMenu popup;
    popup.insertItem(SmallIconSet("colors"),i18n("Set as Primary Background Color"), 1);
    popup.insertItem(SmallIconSet("colors"),i18n("Set as Secondary Background Color"), 2);
    int result = popup.exec(e->pos());

    TQColor c;
    KColorDrag::decode(e, c);
    switch (result) {
      case 1: bgMgr->setColor(c, true); break;
      case 2: bgMgr->setColor(c, false); break;
      default: return;
    }
    bgMgr->setWallpaper(0,0);
}

void KDesktop::handleImageDropEvent(TQDropEvent * e)
{
    TDEPopupMenu popup;
    if ( m_pIconView )
    popup.insertItem(SmallIconSet("document-save"),i18n("&Save to Desktop..."), 1);
    if ( ( m_pIconView && m_pIconView->maySetWallpaper() ) || m_pRootWidget )
       popup.insertItem(SmallIconSet("background"),i18n("Set as &Wallpaper"), 2);
    popup.insertSeparator();
    popup.insertItem(SmallIconSet("cancel"), i18n("&Cancel"));
    int result = popup.exec(e->pos());

    if (result == 1)
    {
        bool ok = true;
        TQString filename = KInputDialog::getText(TQString::null, i18n("Enter a name for the image below:"), TQString::null, &ok, m_pIconView);

        if (!ok)
        {
            return;
        }

        if (filename.isEmpty())
        {
            filename = i18n("image.png");
        }
        else if (filename.right(4).lower() != ".png")
        {
            filename += ".png";
        }

        TQImage i;
        TQImageDrag::decode(e, i);
        KTempFile tmpFile(TQString::null, filename);
        i.save(tmpFile.name(), "PNG");
        // We pass 0 as parent window because passing the desktop is not a good idea
        KURL src;
        src.setPath( tmpFile.name() );
        KURL dest( KDIconView::desktopURL() );
        dest.addPath( filename );
        TDEIO::NetAccess::copy( src, dest, 0 );
        tmpFile.unlink();
    }
    else if (result == 2)
    {
        TQImage i;
        TQImageDrag::decode(e, i);
        KTempFile tmpFile(TDEGlobal::dirs()->saveLocation("wallpaper"), ".png");
        i.save(tmpFile.name(), "PNG");
        kdDebug(1204) << "KDesktop::contentsDropEvent " << tmpFile.name() << endl;
        bgMgr->setWallpaper(tmpFile.name());
    }
}

void KDesktop::slotNewWallpaper(const KURL &url)
{
    // This is called when a file containing an image is dropped
    // (called by KonqOperations)
    if ( url.isLocalFile() )
        bgMgr->setWallpaper( url.path() );
    else
    {
        // Figure out extension
        TQString fileName = url.fileName();
        TQFileInfo fileInfo( fileName );
        TQString ext = fileInfo.extension();
        // Store tempfile in a place where it will still be available after a reboot
        KTempFile tmpFile( TDEGlobal::dirs()->saveLocation("wallpaper"), "." + ext );
        KURL localURL; localURL.setPath( tmpFile.name() );
        // We pass 0 as parent window because passing the desktop is not a good idea
        TDEIO::NetAccess::file_copy( url, localURL, -1, true /*overwrite*/ );
        bgMgr->setWallpaper( localURL.path() );
    }
}

// for dcop interface backward compatibility
void KDesktop::logout()
{
    logout( TDEApplication::ShutdownConfirmDefault,
            TDEApplication::ShutdownTypeNone );
}

void KDesktop::logout( TDEApplication::ShutdownConfirm confirm,
                       TDEApplication::ShutdownType sdtype )
{
    if( !kapp->requestShutDown( confirm, sdtype ) )
        // this i18n string is also in kicker/applets/run/runapplet
        KMessageBox::error( this, i18n("Could not log out properly.\nThe session manager cannot "
                                        "be contacted. You can try to force a shutdown by pressing "
                                        "Ctrl+Alt+Backspace; note, however, that your current session "
                                        "will not be saved with a forced shutdown." ) );
}

void KDesktop::slotLogout()
{
    logout( TDEApplication::ShutdownConfirmDefault,
            TDEApplication::ShutdownTypeDefault );
}

void KDesktop::slotLogoutNoCnf()
{
    logout( TDEApplication::ShutdownConfirmNo,
            TDEApplication::ShutdownTypeNone );
}

void KDesktop::slotHaltNoCnf()
{
    logout( TDEApplication::ShutdownConfirmNo,
            TDEApplication::ShutdownTypeHalt );
}

void KDesktop::slotRebootNoCnf()
{
    logout( TDEApplication::ShutdownConfirmNo,
            TDEApplication::ShutdownTypeReboot );
}

void KDesktop::setVRoot( bool enable )
{
    if ( enable == set_vroot )
        return;

    set_vroot = enable;
    kdDebug(1204) << "setVRoot " << enable << endl;
    KDesktopSettings::setSetVRoot( set_vroot );
    KDesktopSettings::writeConfig();
    slotSetVRoot();
}

void KDesktop::clearCommandHistory()
{
    if ( m_miniCli )
        m_miniCli->clearHistory();
}

void KDesktop::setIconsEnabled( bool enable )
{
    if ( enable == m_bDesktopEnabled )
        return;

    m_bDesktopEnabled = enable;
    kdDebug(1204) << "setIcons " << enable << endl;
    KDesktopSettings::setDesktopEnabled( m_bDesktopEnabled );
    KDesktopSettings::writeConfig();
    if (!enable) {
        delete m_pIconView;
        m_pIconView = 0;
    }
    configure();
}

void KDesktop::desktopResized()
{
    resize(kapp->desktop()->size());

    if ( m_pIconView )
    {
        // the sequence of actions is important:
        // remove all icons, resize desktop, tell kdiconview new iconsArea size
        // tell kdiconview to reget all icons
        m_pIconView->slotClear();
        m_pIconView->resize(kapp->desktop()->size());

        // get new desktopIconsArea from kicker
        TQByteArray data, result;
        TQDataStream arg(data, IO_WriteOnly);
        arg << kdesktop_screen_number;
        TQCString replyType;
        TQRect area;

        if ( kapp->dcopClient()->call(kicker_name, kicker_name, "desktopIconsArea(int)",
                                       data, replyType, result, false, 2000) )
        {
            TQDataStream res(result, IO_ReadOnly);
            res >> area;
        }
        else
        {
            area = twinModule()->workArea(twinModule()->currentDesktop());
        }

        m_pIconView->updateWorkArea(area);
        m_pIconView->startDirLister();
    }
}

void KDesktop::switchDesktops( int delta )
{
    bool old = m_bWheelSwitchesWorkspace;
    m_bWheelSwitchesWorkspace = true;
    slotSwitchDesktops(delta);
    m_bWheelSwitchesWorkspace = old;
}

bool KDesktop::event(TQEvent * e)
{
    if ( e->type() == TQEvent::WindowDeactivate)
    {
        if (m_pIconView)
            m_pIconView->clearSelection();
    }
    return TQWidget::event(e);
}

TQPoint KDesktop::findPlaceForIcon( int column, int row )
{
    if (m_pIconView)
        return m_pIconView->findPlaceForIcon(column, row);
    else
        return TQPoint(-1, -1);
}

void KDesktop::addIcon(const TQString & _url, int x, int y)
{
    addIcon( _url, TDEGlobalSettings::desktopPath(), x, y );
}

void KDesktop::addIcon(const TQString & _url, const TQString & _dest, int x, int y)
{
    TQString filename = _url.mid(_url.findRev('/') + 1);

    TQValueList<TDEIO::CopyInfo> files;
    TDEIO::CopyInfo i;
    i.uSource = KURL::fromPathOrURL( _url );
    i.uDest   = KURL::fromPathOrURL( _dest );
    i.uDest.addPath( filename );
    files.append(i);
    if (!TQFile::exists(i.uDest.prettyURL().replace("file://",TQString()))) { m_pIconView->slotAboutToCreate( TQPoint( x, y ), files );
    TDEIO::copy( i.uSource, i.uDest, false ); }

//    m_pIconView->addFuturePosition(filename, x, y);
    // tqDebug("addIcon %s %s %d %d", _url.latin1(), _dest.latin1(), x, y);
//    system(TQString("cp \"%1\" \"%2/%3\"").arg(KURL(_url).path()).arg(KURL(_dest).path()).arg(filename).latin1());
//    m_pIconView->update( _dest );
}

void KDesktop::removeIcon(const TQString &_url)
{
	if (_url.at(0) != '/') {
		tqDebug("removeIcon with relative path not supported for now");
		return;
	}
	unlink(KURL(_url).path().latin1());
	TQString dest = _url.left(_url.findRev('/') + 1);
        m_pIconView->update( dest );
}

#include "desktop.moc"
