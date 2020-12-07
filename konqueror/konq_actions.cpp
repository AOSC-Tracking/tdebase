/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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

#include "konq_actions.h"

#include <assert.h>

#include <tdetoolbarbutton.h>
#include <kanimwidget.h>
#include <kdebug.h>
#include <kstringhandler.h>

#include <konq_pixmapprovider.h>
#include <kiconloader.h>
#include <tdepopupmenu.h>
#include <tdeapplication.h>

#include "konq_view.h"
#include "konq_settingsxt.h"

template class TQPtrList<KonqHistoryEntry>;

/////////////////

//static - used by KonqHistoryAction and KonqBidiHistoryAction
void KonqBidiHistoryAction::fillHistoryPopup( const TQPtrList<HistoryEntry> &history,
                                          TQPopupMenu * popup,
                                          bool onlyBack,
                                          bool onlyForward,
                                          bool checkCurrentItem,
                                          uint startPos )
{
  assert ( popup ); // kill me if this 0... :/

  //kdDebug(1202) << "fillHistoryPopup position: " << history.at() << endl;
  HistoryEntry * current = history.current();
  TQPtrListIterator<HistoryEntry> it( history );
  if (onlyBack || onlyForward)
  {
      it += history.at(); // Jump to current item
      if ( !onlyForward ) --it; else ++it; // And move off it
  } else if ( startPos )
      it += startPos; // Jump to specified start pos

  uint i = 0;
  while ( it.current() )
  {
      TQString text = it.current()->title;
      text = KStringHandler::cEmSqueeze(text, popup->fontMetrics(), 30); //CT: squeeze
      text.replace( "&", "&&" );
      if ( checkCurrentItem && it.current() == current )
      {
          int id = popup->insertItem( text ); // no pixmap if checked
          popup->setItemChecked( id, true );
      } else
          popup->insertItem( KonqPixmapProvider::self()->pixmapFor(
					    it.current()->url.url() ), text );
      if ( ++i > 10 )
          break;
      if ( !onlyForward ) --it; else ++it;
  }
  //kdDebug(1202) << "After fillHistoryPopup position: " << history.at() << endl;
}

///////////////////////////////

KonqBidiHistoryAction::KonqBidiHistoryAction ( const TQString & text, TQObject* parent, const char* name )
  : TDEAction( text, 0, parent, name )
{
  setShortcutConfigurable(false);
  m_firstIndex = 0;
  m_goMenu = 0L;
}

int KonqBidiHistoryAction::plug( TQWidget *widget, int index )
{
  if (kapp && !kapp->authorizeTDEAction(name()))
    return -1;

  // Go menu
  if ( widget->inherits(TQPOPUPMENU_OBJECT_NAME_STRING) )
  {
    m_goMenu = (TQPopupMenu*)widget;
    // Forward signal (to main view)
    connect( m_goMenu, TQT_SIGNAL( aboutToShow() ),
             this, TQT_SIGNAL( menuAboutToShow() ) );
    connect( m_goMenu, TQT_SIGNAL( activated( int ) ),
             this, TQT_SLOT( slotActivated( int ) ) );
    //kdDebug(1202) << "m_goMenu->count()=" << m_goMenu->count() << endl;
    // Store how many items the menu already contains.
    // This means, the KonqBidiHistoryAction has to be plugged LAST in a menu !
    m_firstIndex = m_goMenu->count();
    return m_goMenu->count(); // hmmm, what should this be ?
  }
  return TDEAction::plug( widget, index );
}

void KonqBidiHistoryAction::fillGoMenu( const TQPtrList<HistoryEntry> & history )
{
    if (history.isEmpty())
        return; // nothing to do

    //kdDebug(1202) << "fillGoMenu position: " << history.at() << endl;
    if ( m_firstIndex == 0 ) // should never happen since done in plug
        m_firstIndex = m_goMenu->count();
    else
    { // Clean up old history (from the end, to avoid shifts)
        for ( uint i = m_goMenu->count()-1 ; i >= m_firstIndex; i-- )
            m_goMenu->removeItemAt( i );
    }
    // TODO perhaps smarter algorithm (rename existing items, create new ones only if not enough) ?

    // Ok, we want to show 10 items in all, among which the current url...

    if ( history.count() <= 9 )
    {
        // First case: limited history in both directions -> show it all
        m_startPos = history.count() - 1; // Start right from the end
    } else
    // Second case: big history, in one or both directions
    {
        // Assume both directions first (in this case we place the current URL in the middle)
        m_startPos = history.at() + 4;

        // Forward not big enough ?
        if ( history.at() > (int)history.count() - 4 )
          m_startPos = history.count() - 1;
    }
    Q_ASSERT( m_startPos >= 0 && (uint)m_startPos < history.count() );
    if ( m_startPos < 0 || (uint)m_startPos >= history.count() )
    {
        kdWarning() << "m_startPos=" << m_startPos << " history.count()=" << history.count() << endl;
        return;
    }
    m_currentPos = history.at(); // for slotActivated
    KonqBidiHistoryAction::fillHistoryPopup( history, m_goMenu, false, false, true, m_startPos );
}

void KonqBidiHistoryAction::slotActivated( int id )
{
  // 1 for first item in the list, etc.
  int index = m_goMenu->indexOf(id) - m_firstIndex + 1;
  if ( index > 0 )
  {
      kdDebug(1202) << "Item clicked has index " << index << endl;
      // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
      int steps = ( m_startPos+1 ) - index - m_currentPos; // make a drawing to understand this :-)
      kdDebug(1202) << "Emit activated with steps = " << steps << endl;
      emit activated( steps );
  }
}

///////////////////////////////

KonqLogoAction::KonqLogoAction( const TQString& text, int accel, TQObject* parent, const char* name )
  : TDEAction( text, accel, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const TQString& text, int accel,
                               TQObject* receiver, const char* slot, TQObject* parent, const char* name )
  : TDEAction( text, accel, receiver, slot, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const TQString& text, const TQIconSet& pix, int accel, TQObject* parent, const char* name )
  : TDEAction( text, pix, accel, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const TQString& text, const TQIconSet& pix,int accel, TQObject* receiver, const char* slot, TQObject* parent, const char* name )
  : TDEAction( text, pix, accel, receiver, slot, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const TQStringList& icons, TQObject* receiver,
                                const char* slot, TQObject* parent,
                                const char* name )
    : TDEAction( 0L, 0, receiver, slot, parent, name ) // text missing !
{
  iconList = icons;
}

void KonqLogoAction::start()
{
  int len = containerCount();
  for ( int i = 0; i < len; i++ )
  {
    TQWidget *w = container( i );

    if ( w->inherits( "TDEToolBar" ) )
    {
      KAnimWidget *anim = ((TDEToolBar *)w)->animatedWidget( menuId( i ) );
      anim->start();
    }
  }
}

void KonqLogoAction::stop()
{
  int len = containerCount();
  for ( int i = 0; i < len; i++ )
  {
    TQWidget *w = container( i );

    if ( w->inherits( "TDEToolBar" ) )
    {
      KAnimWidget *anim = ((TDEToolBar *)w)->animatedWidget( menuId( i ) );
      anim->stop();
    }
  }
}

void KonqLogoAction::updateIcon(int id)
{
    TQWidget *w = container( id );

    if ( w->inherits( "TDEToolBar" ) )
    {
      KAnimWidget *anim = ((TDEToolBar *)w)->animatedWidget( menuId( id ) );
      anim->setIcons(icon());
    }
}



int KonqLogoAction::plug( TQWidget *widget, int index )
{
  if (kapp && !kapp->authorizeTDEAction(name()))
    return -1;

/*
  if ( widget->inherits( "TDEMainWindow" ) )
  {
    ((TDEMainWindow*)widget)->setIndicatorWidget(m_logoLabel);

    addContainer( widget, -1 );

    return containerCount() - 1;
  }
*/
  if ( widget->inherits( "TDEToolBar" ) )
  {
    TDEToolBar *bar = (TDEToolBar *)widget;

    int id_ = getToolButtonID();

    bar->insertAnimatedWidget( id_, this, TQT_SIGNAL(activated()), TQString("trinity"), index );
    bar->alignItemRight( id_ );

    addContainer( bar, id_ );

    connect( bar, TQT_SIGNAL( destroyed() ), this, TQT_SLOT( slotDestroyed() ) );

    return containerCount() - 1;
  }

  int containerId = TDEAction::plug( widget, index );

  return containerId;
}

///////////

KonqViewModeAction::KonqViewModeAction( const TQString &text, const TQString &icon,
                                        TQObject *parent, const char *name )
    : TDERadioAction( text, icon, 0, parent, name )
{
    m_menu = new TQPopupMenu;

    connect( m_menu, TQT_SIGNAL( aboutToShow() ),
             this, TQT_SLOT( slotPopupAboutToShow() ) );
    connect( m_menu, TQT_SIGNAL( activated( int ) ),
             this, TQT_SLOT( slotPopupActivated() ) );
    connect( m_menu, TQT_SIGNAL( aboutToHide() ),
             this, TQT_SLOT( slotPopupAboutToHide() ) );
}

KonqViewModeAction::~KonqViewModeAction()
{
    delete m_menu;
}

int KonqViewModeAction::plug( TQWidget *widget, int index )
{
    int res = TDERadioAction::plug( widget, index );

    if ( widget->inherits( "TDEToolBar" ) && (res != -1) )
    {
        TDEToolBar *toolBar = static_cast<TDEToolBar *>( widget );

        TDEToolBarButton *button = toolBar->getButton( itemId( res ) );

        if ( m_menu->count() > 1 )
            button->setDelayedPopup( m_menu, false );
    }

    return res;
}

void KonqViewModeAction::slotPopupAboutToShow()
{
    m_popupActivated = false;
}

void KonqViewModeAction::slotPopupActivated()
{
    m_popupActivated = true;
}

void KonqViewModeAction::slotPopupAboutToHide()
{
    if ( !m_popupActivated )
    {
        int i = 0;
        for (; i < containerCount(); ++i )
        {
            TQWidget *widget = container( i );
            if ( !widget->inherits( "TDEToolBar" ) )
                continue;

            TDEToolBar *tb = static_cast<TDEToolBar *>( widget );

            TDEToolBarButton *button = tb->getButton( itemId( i ) );

            button->setDown( isChecked() );
        }
    }
}


MostOftenList * KonqMostOftenURLSAction::s_mostEntries = 0L;
uint KonqMostOftenURLSAction::s_maxEntries = 0;

KonqMostOftenURLSAction::KonqMostOftenURLSAction( const TQString& text,
						  TQObject *parent,
						  const char *name )
    : TDEActionMenu( text, "goto", parent, name )
{
    setDelayed( false );

    connect( popupMenu(), TQT_SIGNAL( aboutToShow() ), TQT_SLOT( slotFillMenu() ));
    //connect( popupMenu(), TQT_SIGNAL( aboutToHide() ), TQT_SLOT( slotClearMenu() ));
    connect( popupMenu(), TQT_SIGNAL( activated( int ) ),
	     TQT_SLOT( slotActivated(int) ));
    // Need to do all this upfront for a correct initial state
    init();
}

KonqMostOftenURLSAction::~KonqMostOftenURLSAction()
{
}

void KonqMostOftenURLSAction::init()
{
    s_maxEntries = KonqSettings::numberofmostvisitedURLs();

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    setEnabled( !mgr->entries().isEmpty() && s_maxEntries > 0 );
}

void KonqMostOftenURLSAction::parseHistory() // only ever called once
{
    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    KonqHistoryIterator it( mgr->entries() );

    connect( mgr, TQT_SIGNAL( entryAdded( const KonqHistoryEntry * )),
             TQT_SLOT( slotEntryAdded( const KonqHistoryEntry * )));
    connect( mgr, TQT_SIGNAL( entryRemoved( const KonqHistoryEntry * )),
             TQT_SLOT( slotEntryRemoved( const KonqHistoryEntry * )));
    connect( mgr, TQT_SIGNAL( cleared() ), TQT_SLOT( slotHistoryCleared() ));

    s_mostEntries = new MostOftenList; // exit() will clean this up for now
    for ( uint i = 0; it.current() && i < s_maxEntries; i++ ) {
	s_mostEntries->append( it.current() );
	++it;
    }
    s_mostEntries->sort();

    while ( it.current() ) {
	KonqHistoryEntry *leastOften = s_mostEntries->first();
	KonqHistoryEntry *entry = it.current();
	if ( leastOften->numberOfTimesVisited < entry->numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    s_mostEntries->inSort( entry );
	}

	++it;
    }
}

void KonqMostOftenURLSAction::slotEntryAdded( const KonqHistoryEntry *entry )
{
    // if it's already present, remove it, and inSort it
    s_mostEntries->removeRef( entry );

    if ( s_mostEntries->count() >= s_maxEntries ) {
	KonqHistoryEntry *leastOften = s_mostEntries->first();
	if ( leastOften->numberOfTimesVisited < entry->numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    s_mostEntries->inSort( entry );
	}
    }

    else
	s_mostEntries->inSort( entry );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    s_mostEntries->removeRef( entry );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotHistoryCleared()
{
    s_mostEntries->clear();
    setEnabled( false );
}

void KonqMostOftenURLSAction::slotFillMenu()
{
    if ( !s_mostEntries ) // first time
	parseHistory();

    popupMenu()->clear();
    m_popupList.clear();

    int id = s_mostEntries->count() -1;
    KonqHistoryEntry *entry = s_mostEntries->at( id );
    while ( entry ) {
	// we take either title, typedURL or URL (in this order)
	TQString text = entry->title.isEmpty() ? (entry->typedURL.isEmpty() ?
						 entry->url.prettyURL() :
						 entry->typedURL) :
		       entry->title;

	popupMenu()->insertItem(
		    KonqPixmapProvider::self()->pixmapFor( entry->url.url() ),
		    text, id );
        // Keep a copy of the URLs being shown in the menu
        // This prevents crashes when another process tells us to remove an entry.
        m_popupList.prepend( entry->url );

	entry = (id > 0) ? s_mostEntries->at( --id ) : 0L;
    }
    setEnabled( !s_mostEntries->isEmpty() );
    Q_ASSERT( s_mostEntries->count() == m_popupList.count() );
}

#if 0
void KonqMostOftenURLSAction::slotClearMenu()
{
    // Warning this is called _before_ slotActivated, when activating a menu item.
    // So e.g. don't clear m_popupList here.
}
#endif

void KonqMostOftenURLSAction::slotActivated( int id )
{
    Q_ASSERT( !m_popupList.isEmpty() ); // can not happen
    Q_ASSERT( id < (int)m_popupList.count() );

    KURL url = m_popupList[ id ];
    if ( url.isValid() )
	emit activated( url );
    else
	kdWarning() << "Invalid url: " << url.prettyURL() << endl;
    m_popupList.clear();
}

// sort by numberOfTimesVisited (least often goes first)
int MostOftenList::compareItems( TQPtrCollection::Item item1,
				 TQPtrCollection::Item item2)
{
    KonqHistoryEntry *entry1 = static_cast<KonqHistoryEntry *>( item1 );
    KonqHistoryEntry *entry2 = static_cast<KonqHistoryEntry *>( item2 );

    if ( entry1->numberOfTimesVisited > entry2->numberOfTimesVisited )
	return 1;
    else if ( entry1->numberOfTimesVisited < entry2->numberOfTimesVisited )
	return -1;
    else
	return 0;
}

#include "konq_actions.moc"
