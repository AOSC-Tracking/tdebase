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

#include <tqpainter.h>
#include <tqstyle.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcompletionbox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <klineedit.h>
#include <konq_pixmapprovider.h>
#include <kstdaccel.h>
#include <kurldrag.h>
#include <konq_mainwindow.h>
#include <kstringhandler.h>

#include <dcopclient.h>

#include "konq_combo.h"

KConfig * KonqCombo::s_config = 0L;
const int KonqCombo::temporary = 0;

static TQString titleOfURL( const TQString& urlStr )
{
    KURL url = KURL::fromPathOrURL( urlStr );
    KonqHistoryList& historylist = const_cast<KonqHistoryList&>( KonqHistoryManager::kself()->entries() );
    KonqHistoryEntry *historyentry = historylist.findEntry( url );
    if ( !historyentry && !url.url().endsWith( "/" ) ) {
        url.setPath( url.path()+'/' );
        historyentry = historylist.findEntry( url );
    }
    return ( historyentry ? historyentry->title : TQString() );
}

class TQ_EXPORT KonqComboListBoxPixmap : public TQListBoxItem
{
public:
    KonqComboListBoxPixmap( const TQString& text );
    KonqComboListBoxPixmap( const TQPixmap &, const TQString& text, const TQString& title );

    const TQPixmap *pixmap() const { return &pm; }

    int height( const TQListBox * ) const;
    int width( const TQListBox * )  const;

    int rtti() const;
    static int RTTI;

    bool reuse( const TQString& newText );

protected:
    void paint( TQPainter * );

private:
    bool lookup_pending;
    TQPixmap pm;
    TQString title;
};

class KonqComboLineEdit : public KLineEdit
{
public:
    KonqComboLineEdit( TQWidget *parent=0, const char *name=0 );
    void setCompletedItems( const TQStringList& items );
};

class KonqComboCompletionBox : public KCompletionBox
{
public:
    KonqComboCompletionBox( TQWidget *parent, const char *name = 0 );
    void setItems( const TQStringList& items );
    void insertStringList( const TQStringList & list, int index = -1 );
};

KonqCombo::KonqCombo( TQWidget *parent, const char *name )
          : KHistoryCombo( parent, name ),
            m_returnPressed( false ), 
            m_permanent( false ),
            m_modifier( Qt::NoButton ),
	    m_pageSecurity( KonqMainWindow::NotCrypted )
{
    setInsertionPolicy( NoInsertion );
    setSizePolicy( TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Fixed ));

    Q_ASSERT( s_config );

    KConfigGroupSaver cs( s_config, "Location Bar" );
    setMaxCount( s_config->readNumEntry("Maximum of URLs in combo", 20 ));

    // We should also connect the completionBox' highlighted signal to
    // our setEditText() slot, because we're handling the signals ourselves.
    // But we're lazy and let KCompletionBox do this and simply switch off
    // handling of signals later.
    setHandleSignals( true );

    KonqComboLineEdit *edit = new KonqComboLineEdit( this, "combo lineedit" );
    edit->setHandleSignals( true );
    edit->setCompletionBox( new KonqComboCompletionBox( edit, "completion box" ) );
    setLineEdit( edit );

    completionBox()->setTabHandling( true );

    // Make the lineedit consume the Key_Enter event...
    setTrapReturnKey( true );

    connect( KonqHistoryManager::kself(), TQT_SIGNAL(cleared()), TQT_SLOT(slotCleared()) );
    connect( this, TQT_SIGNAL(cleared() ), TQT_SLOT(slotCleared()) );
    connect( this, TQT_SIGNAL(highlighted( int )), TQT_SLOT(slotSetIcon( int )) );
    connect( this, TQT_SIGNAL(activated( const TQString& )),
             TQT_SLOT(slotActivated( const TQString& )) );

    setHistoryEditorEnabled( true ); 
    connect( this, TQT_SIGNAL(removed( const TQString&) ), TQT_SLOT(slotRemoved( const TQString& )) );

    if ( !kapp->dcopClient()->isAttached() )
        kapp->dcopClient()->attach();
}

KonqCombo::~KonqCombo()
{
}

void KonqCombo::init( KCompletion *completion )
{
    setCompletionObject( completion, false ); //KonqMainWindow handles signals
    setAutoDeleteCompletionObject( false );
    setCompletionMode( completion->completionMode() );

    loadItems();
}

void KonqCombo::setURL( const TQString& url )
{
    //kdDebug(1202) << "KonqCombo::setURL: " << url << ", returnPressed ? " << m_returnPressed << endl;
    setTemporary( url );

    if ( m_returnPressed ) { // Really insert...
        m_returnPressed = false;      
        TQByteArray data;
        TQDataStream s( data, IO_WriteOnly );
        s << url << kapp->dcopClient()->defaultObject();
        kapp->dcopClient()->send( "konqueror*", "KonquerorIface",
                                  "addToCombo(TQString,TQCString)", data);
    }
    // important security consideration: always display the beginning
    // of the url rather than its end to prevent spoofing attempts.
    lineEdit()->setCursorPosition( 0 );
}

void KonqCombo::setTemporary( const TQString& text )
{
    setTemporary( text, KonqPixmapProvider::self()->pixmapFor(text) );
}

void KonqCombo::setTemporary( const TQString& url, const TQPixmap& pix )
{
    //kdDebug(1202) << "KonqCombo::setTemporary: " << url << ", temporary = " << temporary << endl;

    // Insert a temporary item when we don't have one yet
    if ( count() == 0 )
      insertItem( pix, url, temporary, titleOfURL( url ) );
    else
    {
        if (url != temporaryItem())
          applyPermanent();

        updateItem( pix, url, temporary, titleOfURL( url ) );
    }

    setCurrentItem( temporary );
}

void KonqCombo::removeDuplicates( int index )
{
    //kdDebug(1202) << "KonqCombo::removeDuplicates: Starting index =  " << index << endl;

    TQString url (temporaryItem());
    if (url.endsWith("/"))
      url.truncate(url.length()-1);

    // Remove all dupes, if available...
    for ( int i = index; i < count(); i++ ) 
    {
        TQString item (text(i));
        if (item.endsWith("/"))
          item.truncate(item.length()-1);

        if ( item == url )
            removeItem( i );
    }
    lineEdit()->setCursorPosition( 0 );
}

// called via DCOP in all instances
void KonqCombo::insertPermanent( const TQString& url )
{
    //kdDebug(1202) << "KonqCombo::insertPermanent: URL = " << url << endl;
    saveState();
    setTemporary( url );
    m_permanent = true;
    restoreState();
}

// called right before a new (different!) temporary item will be set. So we
// insert an item that was marked permanent properly at position 1.
void KonqCombo::applyPermanent()
{
    if ( m_permanent && !temporaryItem().isEmpty() ) {

        // Remove as many items as needed to honour maxCount()
        int index = count();
        while ( count() >= maxCount() )
            removeItem( --index );

        TQString url (temporaryItem());
        insertItem( KonqPixmapProvider::self()->pixmapFor( url ), url, 1, titleOfURL( url ) );
        //kdDebug(1202) << "KonqCombo::applyPermanent: " << url << endl;

        // Remove all duplicates starting from index = 2
        removeDuplicates( 2 );
        m_permanent = false;
    }
}

void KonqCombo::insertItem( const TQString &text, int index, const TQString& title )
{
    KonqComboListBoxPixmap* item = new KonqComboListBoxPixmap( 0, text, title );
    listBox()->insertItem( item, index );
}

void KonqCombo::insertItem( const TQPixmap &pixmap, const TQString& text, int index, const TQString& title )
{
    KonqComboListBoxPixmap* item = new KonqComboListBoxPixmap( pixmap, text, title );
    listBox()->insertItem( item, index );
}

void KonqCombo::updateItem( const TQPixmap& pix, const TQString& t, int index, const TQString& title )
{
    // No need to flicker
    if (text( index ) == t &&
        (pixmap(index) && pixmap(index)->serialNumber() == pix.serialNumber()))
        return;

    // kdDebug(1202) << "KonqCombo::updateItem: item='" << t << "', index='"
    //               << index << "'" << endl;

    // TQComboBox::changeItem() doesn't honour the pixmap when
    // using an editable combobox, so we just remove and insert    
    // ### use TQComboBox::changeItem(), once that finally works
    // Well lets try it now as it seems to work fine for me. We
    // can always revert :)
    KonqComboListBoxPixmap* item = new KonqComboListBoxPixmap( pix, t, title );
    listBox()->changeItem( item, index );

    /*
    setUpdatesEnabled( false );
    lineEdit()->setUpdatesEnabled( false );

    removeItem( index );
    insertItem( pix, t, index );

    setUpdatesEnabled( true );
    lineEdit()->setUpdatesEnabled( true );
    update();
    */
}

void KonqCombo::saveState()
{
    m_cursorPos = cursorPosition();
    m_currentText = currentText();
    m_currentIndex = currentItem();
}

void KonqCombo::restoreState()
{
    setTemporary( m_currentText );
    lineEdit()->setCursorPosition( m_cursorPos );
}

void KonqCombo::updatePixmaps()
{
    saveState();

    setUpdatesEnabled( false );
    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    for ( int i = 1; i < count(); i++ ) {
        updateItem( prov->pixmapFor( text( i ) ), text( i ), i, titleOfURL( text( i ) ) );
    }
    setUpdatesEnabled( true );
    tqrepaint();

    restoreState();
}

void KonqCombo::loadItems()
{
    clear();
    int i = 0;

    s_config->setGroup( "History" ); // delete the old 2.0.x completion
    s_config->writeEntry( "CompletionItems", "unused" );

    s_config->setGroup( "Location Bar" );
    TQStringList items = s_config->readPathListEntry( "ComboContents" );
    TQStringList::ConstIterator it = items.begin();
    TQString item;
    bool first = true;
    while ( it != items.end() ) {
        item = *it;
        if ( !item.isEmpty() ) { // only insert non-empty items
	    if( first ) {
                insertItem( KonqPixmapProvider::self()->pixmapFor( item, KIcon::SizeSmall ),
                            item, i++, titleOfURL( item ) );
	    }
            else
                // icons will be loaded on-demand
                insertItem( item, i++, titleOfURL( item ) );
            first = false;
        }
        ++it;
    }

    if ( count() > 0 )
        m_permanent = true; // we want the first loaded item to stay
}

void KonqCombo::slotSetIcon( int index )
{
    if( pixmap( index ) == NULL )
        // on-demand icon loading
        updateItem( KonqPixmapProvider::self()->pixmapFor( text( index ),
                    KIcon::SizeSmall ), text( index ), index, 
                    titleOfURL( text( index ) ) );
    update();
}

void KonqCombo::popup()
{
  for( int i = 0; i < count(); ++i )
    {
        if( pixmap( i ) == NULL || pixmap( i )->isNull() )
        {
            // on-demand icon loading
            updateItem( KonqPixmapProvider::self()->pixmapFor( text( i ),
                        KIcon::SizeSmall), text( i ), i, titleOfURL( text( i ) ) );
        }
    }
    KHistoryCombo::popup();
}

void KonqCombo::saveItems()
{
    TQStringList items;
    int i = m_permanent ? 0 : 1;

    for ( ; i < count(); i++ )
        items.append( text( i ) );

    s_config->setGroup( "Location Bar" );
    s_config->writePathEntry( "ComboContents", items );
    KonqPixmapProvider::self()->save( s_config, "ComboIconCache", items );

    s_config->sync();
}

void KonqCombo::clearTemporary( bool makeCurrent )
{
    applyPermanent();
    changeItem( TQString::null, temporary ); // ### default pixmap?
    if ( makeCurrent )
      setCurrentItem( temporary );
}

bool KonqCombo::eventFilter( TQObject *o, TQEvent *ev )
{
    // Handle Ctrl+Del/Backspace etc better than the Qt widget, which always
    // jumps to the next whitespace.
    TQLineEdit *edit = lineEdit();
    if ( TQT_BASE_OBJECT(o) == TQT_BASE_OBJECT(edit) ) {
        int type = ev->type();
        if ( type == TQEvent::KeyPress ) {
            TQKeyEvent *e = TQT_TQKEYEVENT( ev );

            if ( e->key() == Key_Return || e->key() == Key_Enter ) {
                m_modifier = e->state();
                return false;
            }

            if ( KKey( e ) == KKey( int( KStdAccel::deleteWordBack() ) ) ||
                 KKey( e ) == KKey( int( KStdAccel::deleteWordForward() ) ) ||
                 ((e->state() & ControlButton) &&
                   (e->key() == Key_Left || e->key() == Key_Right) ) ) {
                selectWord(e);
                e->accept();
                return true;
            }
        }

        else if ( type == TQEvent::MouseButtonDblClick ) {
            edit->selectAll();
            return true;
        }
    }
    return KComboBox::eventFilter( o, ev );
}

void KonqCombo::keyPressEvent( TQKeyEvent *e )
{
    KHistoryCombo::keyPressEvent( e );
    // we have to set it as temporary, otherwise we wouldn't get our nice
    // pixmap. Yes, TQComboBox still sucks.
    if ( KKey( e ) == KKey( int( KStdAccel::rotateUp() ) ) ||
         KKey( e ) == KKey( int( KStdAccel::rotateDown() ) ) )
         setTemporary( currentText() );
}

/* 
   Handle Ctrl+Cursor etc better than the Qt widget, which always
   jumps to the next whitespace. This code additionally jumps to
   the next [/#?:], which makes more sense for URLs. The list of 
   chars that will stop the cursor are '/', '.', '?', '#', ':'.
*/
void KonqCombo::selectWord(TQKeyEvent *e)
{
    TQLineEdit* edit = lineEdit();    
    TQString text = edit->text();
    int pos = edit->cursorPosition();
    int pos_old = pos;
    int count = 0;  

    // TODO: make these a parameter when in kdelibs/kdeui...
    TQValueList<TQChar> chars;
    chars << TQChar('/') << TQChar('.') << TQChar('?') << TQChar('#') << TQChar(':');    
    bool allow_space_break = true;

    if( e->key() == Key_Left || e->key() == Key_Backspace ) {
        do {
            pos--;
            count++;
            if( allow_space_break && text[pos].isSpace() && count > 1 )
                break;
        } while( pos >= 0 && (chars.findIndex(text[pos]) == -1 || count <= 1) );

        if( e->state() & ShiftButton ) {
                  edit->cursorForward(true, 1-count);
        } 
        else if(  e->key() == Key_Backspace ) {
            edit->cursorForward(false, 1-count);
            TQString text = edit->text();
            int pos_to_right = edit->text().length() - pos_old;
            TQString cut = text.left(edit->cursorPosition()) + text.right(pos_to_right);
            edit->setText(cut);
            edit->setCursorPosition(pos_old-count+1);
        } 
        else {
            edit->cursorForward(false, 1-count);
        }
     } 
     else if( e->key() == Key_Right || e->key() == Key_Delete ){
        do {
            pos++;
            count++;
                  if( allow_space_break && text[pos].isSpace() )
                      break;
        } while( pos < (int) text.length() && chars.findIndex(text[pos]) == -1 );

        if( e->state() & ShiftButton ) {
            edit->cursorForward(true, count+1);
        } 
        else if(  e->key() == Key_Delete ) {
            edit->cursorForward(false, -count-1);
            TQString text = edit->text();
            int pos_to_right = text.length() - pos - 1;
            TQString cut = text.left(pos_old) +
               (pos_to_right > 0 ? text.right(pos_to_right) : TQString::null );
            edit->setText(cut);
            edit->setCursorPosition(pos_old);
        } 
        else {
            edit->cursorForward(false, count+1);
        }
    }
}

void KonqCombo::slotCleared()
{
    TQByteArray data;
    TQDataStream s( data, IO_WriteOnly );
    s << kapp->dcopClient()->defaultObject();
    kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "comboCleared(TQCString)", data);
}

void KonqCombo::slotRemoved( const TQString& item )
{
    TQByteArray data;
    TQDataStream s( data, IO_WriteOnly );
    s << item << kapp->dcopClient()->defaultObject();
    kapp->dcopClient()->send( "konqueror*", "KonquerorIface",
                               "removeFromCombo(TQString,TQCString)", data);
}

void KonqCombo::removeURL( const TQString& url )
{
    setUpdatesEnabled( false );
    lineEdit()->setUpdatesEnabled( false );

    removeFromHistory( url );
    applyPermanent();
    setTemporary( currentText() );

    setUpdatesEnabled( true );
    lineEdit()->setUpdatesEnabled( true );
    update();
}

void KonqCombo::mousePressEvent( TQMouseEvent *e )
{
    m_dragStart = TQPoint(); // null QPoint

    if ( e->button() == Qt::LeftButton && pixmap( currentItem()) ) {
        // check if the pixmap was clicked
        int x = e->pos().x();
        int x0 = TQStyle::visualRect( tqstyle().querySubControlMetrics( TQStyle::CC_ComboBox, this, TQStyle::SC_ComboBoxEditField ), this ).x();

        if ( x > x0 + 2 && x < lineEdit()->x() ) {
            m_dragStart = e->pos();
            return; // don't call KComboBox::mousePressEvent!
        }
    }

    if ( e->button() == Qt::LeftButton && m_pageSecurity!=KonqMainWindow::NotCrypted ) {
        // check if the lock icon was clicked
        int x = e->pos().x();
        int x0 = TQStyle::visualRect( tqstyle().querySubControlMetrics( TQStyle::CC_ComboBox, this, TQStyle::SC_ComboBoxArrow ), this ).x();
        if ( x < x0 )
            emit showPageSecurity();

    }

    KComboBox::mousePressEvent( e );
}

void KonqCombo::mouseMoveEvent( TQMouseEvent *e )
{
    KComboBox::mouseMoveEvent( e );
    if ( m_dragStart.isNull() || currentText().isEmpty() )
        return;

    if ( e->state() & Qt::LeftButton &&
         (e->pos() - m_dragStart).manhattanLength() >
         KGlobalSettings::dndEventDelay() )
    {
        KURL url = KURL::fromPathOrURL( currentText() );
        if ( url.isValid() )
        {
            KURL::List list;
            list.append( url );
            KURLDrag *drag = new KURLDrag( list, this );
            TQPixmap pix = KonqPixmapProvider::self()->pixmapFor( currentText(),
                                                                 KIcon::SizeMedium );
            if ( !pix.isNull() )
                drag->setPixmap( pix );
            drag->dragCopy();
        }
    }
}

void KonqCombo::slotActivated( const TQString& text )
{
    //kdDebug(1202) << "KonqCombo::slotActivated: " << text << endl;
    applyPermanent();
    m_returnPressed = true;
    emit activated( text, m_modifier );
    m_modifier = Qt::NoButton;
}

void KonqCombo::setConfig( KConfig *kc )
{
    s_config = kc;
}

void KonqCombo::paintEvent( TQPaintEvent *pe )
{
    TQComboBox::paintEvent( pe );

    TQLineEdit *edit = lineEdit();
    TQRect re = tqstyle().querySubControlMetrics( TQStyle::CC_ComboBox, this, TQStyle::SC_ComboBoxEditField );
    re = TQStyle::visualRect(re, this);
    
    if ( m_pageSecurity!=KonqMainWindow::NotCrypted ) {
        TQColor color(245, 246, 190);
        bool useColor = hasSufficientContrast(color,edit->paletteForegroundColor());

        TQPainter p( this );
        p.setClipRect( re );

	TQPixmap pix = KonqPixmapProvider::self()->pixmapFor( currentText() );
	if ( useColor ) {
            p.fillRect( re.x(), re.y(), pix.width() + 4, re.height(), TQBrush( color ));
            p.drawPixmap( re.x() + 2, re.y() + ( re.height() - pix.height() ) / 2, pix );
	}

        TQRect r = edit->geometry();
        r.setRight( re.right() - pix.width() - 4 );
        if ( r != edit->geometry() )
            edit->setGeometry( r );

	if ( useColor)
	    edit->setPaletteBackgroundColor( color );

        pix = SmallIcon( m_pageSecurity==KonqMainWindow::Encrypted ? "encrypted" : "halfencrypted" );
        p.fillRect( re.right() - pix.width() - 3 , re.y(), pix.width() + 4, re.height(), 
		    TQBrush( useColor ? color : edit->paletteBackgroundColor() ));
        p.drawPixmap( re.right() - pix.width() -1 , re.y() + ( re.height() - pix.height() ) / 2, pix );
        p.setClipping( FALSE );
    }
    else {
        TQRect r = edit->geometry();
        r.setRight( re.right() );
        if ( r != edit->geometry() )
            edit->setGeometry( r );
        edit->setPaletteBackgroundColor( TQApplication::palette( edit ).color( TQPalette::Active, TQColorGroup::Base ) );
    }
}

void KonqCombo::setPageSecurity( int pageSecurity )
{
    m_pageSecurity = pageSecurity;
    tqrepaint();
}

bool KonqCombo::hasSufficientContrast(const TQColor &c1, const TQColor &c2)
{
    // Taken from khtml/misc/helper.cc
#define HUE_DISTANCE 40
#define CONTRAST_DISTANCE 10

    int h1, s1, v1, h2, s2, v2;
    int hdist = -CONTRAST_DISTANCE;
    c1.hsv(&h1,&s1,&v1);
    c2.hsv(&h2,&s2,&v2);
    if(h1!=-1 && h2!=-1) { // grey values have no hue
        hdist = kAbs(h1-h2);
        if (hdist > 180) hdist = 360-hdist;
        if (hdist < HUE_DISTANCE) {
            hdist -= HUE_DISTANCE;
            // see if they are high key or low key colours
            bool hk1 = h1>=45 && h1<=225;
            bool hk2 = h2>=45 && h2<=225;
            if (hk1 && hk2)
                hdist = (5*hdist)/3;
            else if (!hk1 && !hk2)
                hdist = (7*hdist)/4;
        }
        hdist = kMin(hdist, HUE_DISTANCE*2);
    }
    return hdist + (kAbs(s1-s2)*128)/(160+kMin(s1,s2)) + kAbs(v1-v2) > CONTRAST_DISTANCE;
}

///////////////////////////////////////////////////////////////////////////////

KonqComboListBoxPixmap::KonqComboListBoxPixmap( const TQString& text )
    : TQListBoxItem()
{
    setText( text );
    lookup_pending = true;
}

KonqComboListBoxPixmap::KonqComboListBoxPixmap( const TQPixmap & pix, const TQString& text, const TQString& _title )
  : TQListBoxItem()
{
    pm = pix;
    title = _title;
    setText( text );
    lookup_pending = false;
}

void KonqComboListBoxPixmap::paint( TQPainter *painter )
{
    if ( lookup_pending ) {
        title = titleOfURL( text() );
        if ( !title.isEmpty() )
            pm = KonqPixmapProvider::self()->pixmapFor( text(), KIcon::SizeSmall );
        else if ( text().find( "://" ) == -1 ) {
            title = titleOfURL( "http://"+text() );
            if ( !title.isEmpty() )
                pm = KonqPixmapProvider::self()->pixmapFor( "http://"+text(), KIcon::SizeSmall );
            else
                pm = KonqPixmapProvider::self()->pixmapFor( text(), KIcon::SizeSmall );
        }
        else
            pm = TQPixmap();
        lookup_pending = false;
    }

    int itemHeight = height( listBox() );
    int yPos, pmWidth = 0;
    const TQPixmap *pm = pixmap();

    if ( pm && ! pm->isNull() ) {
        yPos = ( itemHeight - pm->height() ) / 2;
        painter->drawPixmap( 3, yPos, *pm );
        pmWidth = pm->width() + 5;
    }

    int entryWidth = listBox()->width() - listBox()->tqstyle().pixelMetric( TQStyle::PM_ScrollBarExtent ) -
                     2 * listBox()->tqstyle().pixelMetric( TQStyle::PM_DefaultFrameWidth );
    int titleWidth = ( entryWidth / 3 ) - 1;
    int urlWidth = entryWidth - titleWidth - pmWidth - 2;

    if ( !text().isEmpty() ) {
        TQString squeezedText = KStringHandler::rPixelSqueeze( text(), listBox()->fontMetrics(), urlWidth );
        painter->drawText( pmWidth, 0, urlWidth + pmWidth, itemHeight, 
                           Qt::AlignLeft | Qt::AlignTop, squeezedText );

        //painter->setPen( KGlobalSettings::inactiveTextColor() );
        squeezedText = KStringHandler::rPixelSqueeze( title, listBox()->fontMetrics(), titleWidth );
        TQFont font = painter->font();
        font.setItalic( true );
        painter->setFont( font );
        painter->drawText( entryWidth - titleWidth, 0, titleWidth,
                           itemHeight, Qt::AlignLeft | Qt::AlignTop, squeezedText );
    }
}

int KonqComboListBoxPixmap::height( const TQListBox* lb ) const
{
    int h;
    if ( text().isEmpty() )
        h = pm.height();
    else
        h = QMAX( pm.height(), lb->fontMetrics().lineSpacing() + 2 );
    return QMAX( h, TQApplication::globalStrut().height() );
}

int KonqComboListBoxPixmap::width( const TQListBox* lb ) const
{
    if ( text().isEmpty() )
        return QMAX( pm.width() + 6, TQApplication::globalStrut().width() );
    return QMAX( pm.width() + lb->fontMetrics().width( text() ) + 6,
                 TQApplication::globalStrut().width() );
}

int KonqComboListBoxPixmap::RTTI = 1003;

int KonqComboListBoxPixmap::rtti() const
{
    return RTTI;
}

bool KonqComboListBoxPixmap::reuse( const TQString& newText )
{
    if ( text() == newText )
        return false;

    lookup_pending = true;
    setText( newText );
    return true;
}

///////////////////////////////////////////////////////////////////////////////

KonqComboLineEdit::KonqComboLineEdit( TQWidget *parent, const char *name )
                  :KLineEdit( parent, name ) {}

void KonqComboLineEdit::setCompletedItems( const TQStringList& items )
{
    TQString txt;
    KonqComboCompletionBox *completionbox = static_cast<KonqComboCompletionBox*>( completionBox() );

    if ( completionbox && completionbox->isVisible() )
        // The popup is visible already - do the matching on the initial string,
        // not on the currently selected one.
        txt = completionbox->cancelledText();
    else
        txt = text();

    if ( !items.isEmpty() && !(items.count() == 1 && txt == items.first()) ) {
        if ( !completionBox( false ) )
            setCompletionBox( new KonqComboCompletionBox( this, "completion box" ) );

        if ( completionbox->isVisible() ) {
            bool wasSelected = completionbox->isSelected( completionbox->currentItem() );
            const TQString currentSelection = completionbox->currentText();
            completionbox->setItems( items );
            TQListBoxItem* item = completionbox->findItem( currentSelection, TQt::ExactMatch );
            if( !item || !wasSelected )
            {
                wasSelected = false;
                item = completionbox->item( 0 );
            }
            if ( item ) {
                completionbox->blockSignals( true );
                completionbox->setCurrentItem( item );
                completionbox->setSelected( item, wasSelected );
                completionbox->blockSignals( false );
            }
        }
        else { // completion box not visible yet -> show it
            if ( !txt.isEmpty() )
                completionbox->setCancelledText( txt );
            completionbox->setItems( items );
            completionbox->popup();
        }

        if ( autoSuggest() ) {
            int index = items.first().find( txt );
            TQString newText = items.first().mid( index );
            setUserSelection( false );
            setCompletedText( newText, true );
        }
    }
    else
        if ( completionbox && completionbox->isVisible() )
            completionbox->hide();
}

///////////////////////////////////////////////////////////////////////////////

KonqComboCompletionBox::KonqComboCompletionBox( TQWidget *parent, const char *name )
                       :KCompletionBox( parent, name ) {}

void KonqComboCompletionBox::setItems( const TQStringList& items )
{
    bool block = signalsBlocked();
    blockSignals( true );

    TQListBoxItem* item = firstItem();
    if ( !item )
        insertStringList( items );
    else {
        //Keep track of whether we need to change anything,
        //so we can avoid a tqrepaint for identical updates,
        //to reduce flicker
        bool dirty = false;

        TQStringList::ConstIterator it = items.constBegin();
        const TQStringList::ConstIterator itEnd = items.constEnd();

        for ( ; it != itEnd; ++it) {
            if ( item ) {
                const bool changed = ((KonqComboListBoxPixmap*)item)->reuse( *it );
                dirty = dirty || changed;
                item = item->next();
            }
            else {
                dirty = true;
                //Inserting an item is a way of making this dirty
                insertItem( new KonqComboListBoxPixmap( *it ) );
            }
        }

        //If there is an unused item, mark as dirty -> less items now
        if ( item )
            dirty = true;

        TQListBoxItem* tmp = item;
        while ( (item = tmp ) ) {
            tmp = item->next();
            delete item;
        }

        if ( dirty )
            triggerUpdate( false );
    }

    if ( isVisible() && size().height() != sizeHint().height() )
        sizeAndPosition();

    blockSignals( block );

    // Trigger d->down_workaround = true within KCompletionBox
    TQStringList dummy;
    KCompletionBox::insertItems( dummy, 1 );
}

void KonqComboCompletionBox::insertStringList( const TQStringList & list, int index )
{
    if ( index < 0 )
        index = count();
    for ( TQStringList::ConstIterator it = list.begin(); it != list.end(); ++it )
        insertItem( new KonqComboListBoxPixmap( *it ), index++ );
}

#include "konq_combo.moc"
