/*
 *  This file is part of the KDE Help Center
 *
 *  Copyright (C) 2002 Frerich Raabe (raabe@kde.org)
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
#include "glossary.h"
#include "view.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>

#include <tqheader.h>

#include <sys/stat.h>

using namespace KHC;

class SectionItem : public KListViewItem
{
	public:
		SectionItem( TQListViewItem *parent, const TQString &text )
			: KListViewItem( parent, text )
		{
			setOpen( false );
		}
		
		virtual void setOpen( bool open )
		{
				KListViewItem::setOpen(open);
				
				setPixmap( 0, SmallIcon( TQString::fromLatin1( open ? "contents" : "contents2" ) ) );

		}
};

class EntryItem : public KListViewItem
{
	public:
		EntryItem( SectionItem *parent, const TQString &term, const TQString &id )
			: KListViewItem( parent, term ),
			m_id( id )
		{
		}

		TQString id() const { return m_id; }
	
	private:
		TQString m_id;
};

Glossary::Glossary( TQWidget *parent ) : KListView( parent )
{
	m_initialized = false;

	connect( this, TQT_SIGNAL( clicked( TQListViewItem * ) ),
	         this, TQT_SLOT( treeItemSelected( TQListViewItem * ) ) );
	connect( this, TQT_SIGNAL( returnPressed( TQListViewItem * ) ),
	         this, TQT_SLOT( treeItemSelected( TQListViewItem * ) ) );
	
	setFrameStyle( TQFrame::Panel | TQFrame::Sunken );
	addColumn( TQString::null );
	header()->hide();
	setAllColumnsShowFocus( true );
	setRootIsDecorated( true );

	m_byTopicItem = new KListViewItem( this, i18n( "By Topic" ) );
	m_byTopicItem->setPixmap( 0, SmallIcon( "help" ) );

	m_alphabItem = new KListViewItem( this, i18n( "Alphabetically" ) );
	m_alphabItem->setPixmap( 0, SmallIcon( "charset" ) );

	m_cacheFile = locateLocal( "cache", "help/glossary.xml" );

	m_sourceFile = View::View::langLookup( TQString::fromLatin1( "khelpcenter/glossary/index.docbook" ) );

	m_config = kapp->config();
	m_config->setGroup( "Glossary" );

}

void Glossary::show()
{
	if ( !m_initialized ) {
		if ( cacheStatus() == NeedRebuild )
			rebuildGlossaryCache();
		else
			buildGlossaryTree();
		m_initialized = true;
	}
	KListView::show();
}

Glossary::~Glossary()
{
	m_glossEntries.setAutoDelete( true );
	m_glossEntries.clear();
}

const GlossaryEntry &Glossary::entry( const TQString &id ) const
{
	return *m_glossEntries[ id ];
}

Glossary::CacheStatus Glossary::cacheStatus() const
{
	if ( !TQFile::exists( m_cacheFile ) ||
	     m_config->readPathEntry( "CachedGlossary" ) != m_sourceFile ||
	     m_config->readNumEntry( "CachedGlossaryTimestamp" ) != glossaryCTime() )
		return NeedRebuild;

	return CacheOk;
}

int Glossary::glossaryCTime() const
{
	struct stat stat_buf;
	stat( TQFile::encodeName( m_sourceFile ).data(), &stat_buf );

	return stat_buf.st_ctime;
}

void Glossary::rebuildGlossaryCache()
{
	KMainWindow *mainWindow = dynamic_cast<KMainWindow *>( kapp->mainWidget() );
	Q_ASSERT( mainWindow );
	mainWindow->statusBar()->message( i18n( "Rebuilding cache..." ) );

	KProcess *meinproc = new KProcess;
	connect( meinproc, TQT_SIGNAL( processExited( KProcess * ) ),
	         this, TQT_SLOT( meinprocExited( KProcess * ) ) );

	*meinproc << locate( "exe", TQString::fromLatin1( "meinproc" ) );
	*meinproc << TQString::fromLatin1( "--output" ) << m_cacheFile;
	*meinproc << TQString::fromLatin1( "--stylesheet" )
	          << locate( "data", TQString::fromLatin1( "khelpcenter/glossary.xslt" ) );
	*meinproc << m_sourceFile;

	meinproc->start( KProcess::NotifyOnExit );
}

void Glossary::meinprocExited( KProcess *meinproc )
{
	delete meinproc;

	if ( !TQFile::exists( m_cacheFile ) )
		return;

	m_config->writePathEntry( "CachedGlossary", m_sourceFile );
	m_config->writeEntry( "CachedGlossaryTimestamp", glossaryCTime() );
	m_config->sync();
	
	m_status = CacheOk;

	KMainWindow *mainWindow = dynamic_cast<KMainWindow *>( kapp->mainWidget() );
	Q_ASSERT( mainWindow );
	mainWindow->statusBar()->message( i18n( "Rebuilding cache... done." ), 2000 );

	buildGlossaryTree();
}

void Glossary::buildGlossaryTree()
{
	TQFile cacheFile(m_cacheFile);
	if ( !cacheFile.open( IO_ReadOnly ) )
		return;

	TQDomDocument doc;
	if ( !doc.setContent( &cacheFile ) )
		return;

	TQDomNodeList sectionNodes = doc.documentElement().elementsByTagName( TQString::fromLatin1( "section" ) );
	for ( unsigned int i = 0; i < sectionNodes.count(); i++ ) {
		TQDomElement sectionElement = sectionNodes.item( i ).toElement();
		TQString title = sectionElement.attribute( TQString::fromLatin1( "title" ) );
		SectionItem *topicSection = new SectionItem( m_byTopicItem, title );

		TQDomNodeList entryNodes = sectionElement.elementsByTagName( TQString::fromLatin1( "entry" ) );
		for ( unsigned int j = 0; j < entryNodes.count(); j++ ) {
			TQDomElement entryElement = entryNodes.item( j ).toElement();
			
			TQString entryId = entryElement.attribute( TQString::fromLatin1( "id" ) );
			if ( entryId.isNull() )
				continue;
				
			TQDomElement termElement = childElement( entryElement, TQString::fromLatin1( "term" ) );
			TQString term = termElement.text().simplifyWhiteSpace();

			EntryItem *entry = new EntryItem(topicSection, term, entryId );
            m_idDict.insert( entryId, entry );

			SectionItem *alphabSection = 0L;
			for ( TQListViewItemIterator it( m_alphabItem ); it.current(); it++ )
				if ( it.current()->text( 0 ) == term[ 0 ].upper() ) {
					alphabSection = static_cast<SectionItem *>( it.current() );
					break;
				}

			if ( !alphabSection )
				alphabSection = new SectionItem( m_alphabItem, term[ 0 ].upper() );

			new EntryItem( alphabSection, term, entryId );

			TQDomElement definitionElement = childElement( entryElement, TQString::fromLatin1( "definition" ) );
			TQString definition = definitionElement.text().simplifyWhiteSpace();

			GlossaryEntryXRef::List seeAlso;

			TQDomElement referencesElement = childElement( entryElement, TQString::fromLatin1( "references" ) );
			TQDomNodeList referenceNodes = referencesElement.elementsByTagName( TQString::fromLatin1( "reference" ) );
			if ( referenceNodes.count() > 0 )
				for ( unsigned int k = 0; k < referenceNodes.count(); k++ ) {
					TQDomElement referenceElement = referenceNodes.item( k ).toElement();

					TQString term = referenceElement.attribute( TQString::fromLatin1( "term" ) );
					TQString id = referenceElement.attribute( TQString::fromLatin1( "id" ) );
					
					seeAlso += GlossaryEntryXRef( term, id );
				}
			
			m_glossEntries.insert( entryId, new GlossaryEntry( term, definition, seeAlso ) );
		}
	}
}

void Glossary::treeItemSelected( TQListViewItem *item )
{
	if ( !item )
		return;

	if ( EntryItem *i = dynamic_cast<EntryItem *>( item ) )
		emit entrySelected( entry( i->id() ) );

	item->setOpen( !item->isOpen() );
}
	
TQDomElement Glossary::childElement( const TQDomElement &element, const TQString &name )
{
	TQDomElement e;
	for ( e = element.firstChild().toElement(); !e.isNull(); e = e.nextSibling().toElement() )
		if ( e.tagName() == name )
			break;
	return e;
}

TQString Glossary::entryToHtml( const GlossaryEntry &entry )
{
    TQFile htmlFile( locate("data", "khelpcenter/glossary.html.in" ) );
    if (!htmlFile.open(IO_ReadOnly))
      return TQString( "<html><head></head><body><h3>%1</h3>%2</body></html>" )
             .arg( i18n( "Error" ) )
             .arg( i18n( "Unable to show selected glossary entry: unable to open "
                          "file 'glossary.html.in'!" ) );

    TQString seeAlso;
    if (!entry.seeAlso().isEmpty()) {
        seeAlso = i18n("See also: ");
        GlossaryEntryXRef::List seeAlsos = entry.seeAlso();
        GlossaryEntryXRef::List::ConstIterator it = seeAlsos.begin();
        GlossaryEntryXRef::List::ConstIterator end = seeAlsos.end();
        for (; it != end; ++it) {
            seeAlso += TQString::fromLatin1("<a href=\"glossentry:");
            seeAlso += (*it).id();
            seeAlso += TQString::fromLatin1("\">") + (*it).term();
            seeAlso += TQString::fromLatin1("</a>, ");
        }
        seeAlso = seeAlso.left(seeAlso.length() - 2);
    }

    TQTextStream htmlStream(&htmlFile);
    return htmlStream.read()
           .arg( i18n( "KDE Glossary" ) )
           .arg( entry.term() )
           .arg( View::langLookup( "khelpcenter/konq.css" ) )
           .arg( View::langLookup( "khelpcenter/pointers.png" ) )
           .arg( View::langLookup( "khelpcenter/khelpcenter.png" ) )
           .arg( View::langLookup( "khelpcenter/lines.png" ) )
           .arg( entry.term() )
           .arg( entry.definition() )
           .arg( seeAlso)
           .arg( View::langLookup( "khelpcenter/kdelogo2.png" ) );
}

void Glossary::slotSelectGlossEntry( const TQString &id )
{
    EntryItem *newItem = m_idDict.find( id );
    if ( newItem == 0 )
        return;

    EntryItem *curItem = dynamic_cast<EntryItem *>( currentItem() );
    if ( curItem != 0 ) {
        if ( curItem->id() == id )
            return;
        curItem->parent()->setOpen( false );
    }

    setCurrentItem( newItem );
    ensureItemVisible( newItem );
}

#include "glossary.moc"
// vim:ts=4:sw=4:et
