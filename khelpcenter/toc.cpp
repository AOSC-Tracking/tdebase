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

#include "toc.h"

#include "docentry.h"

#include <kiconloader.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <tqdir.h>
#include <tqfileinfo.h>

#include <sys/stat.h>

using namespace KHC;

class TOCItem : public NavigatorItem
{
	public:
		TOCItem( TOC *parent, TQListViewItem *parentItem, TQListViewItem *after, const TQString &text );

		const TOC *toc() const { return m_toc; }

	private:
		TOC *m_toc;
};

class TOCChapterItem : public TOCItem
{
	public:
		TOCChapterItem( TOC *toc, NavigatorItem *parent, TQListViewItem *after, const TQString &title, 
				const TQString &name );

		virtual TQString url();
			
	private:
		TQString m_name;
};

class TOCSectionItem : public TOCItem
{
	public:
		TOCSectionItem( TOC *toc, TOCChapterItem *parent, TQListViewItem *after, const TQString &title, 
				const TQString &name );

		virtual TQString url();
	
	private:
		TQString m_name;
};

TOC::TOC( NavigatorItem *parentItem )
{
	m_parentItem = parentItem;
}

void TOC::build( const TQString &file )
{
	TQFileInfo fileInfo( file );
	TQString fileName = fileInfo.absFilePath();
	const TQStringList resourceDirs = KGlobal::dirs()->resourceDirs( "html" );
	TQStringList::ConstIterator it = resourceDirs.begin();
	TQStringList::ConstIterator end = resourceDirs.end();
	for ( ; it != end; ++it ) {
		if ( fileName.startsWith( *it ) ) {
			fileName.remove( 0, ( *it ).length() );
			break;
		}
	}

	TQString cacheFile = fileName.replace( TQDir::separator(), "__" );
	m_cacheFile = locateLocal( "cache", "help/" + cacheFile );
	m_sourceFile = file;

	if ( cacheStatus() == NeedRebuild )
		buildCache();
	else
		fillTree();
}

TOC::CacheStatus TOC::cacheStatus() const
{
	if ( !TQFile::exists( m_cacheFile ) ||
	     sourceFileCTime() != cachedCTime() )
		return NeedRebuild;

	return CacheOk;
}

int TOC::sourceFileCTime() const
{
	struct stat stat_buf;
	stat( TQFile::encodeName( m_sourceFile ).data(), &stat_buf );

	return stat_buf.st_ctime;
}

int TOC::cachedCTime() const
{
	TQFile f( m_cacheFile );
	if ( !f.open( IO_ReadOnly ) )
		return 0;
	
	TQDomDocument doc;
	if ( !doc.setContent( &f ) )
		return 0;

	TQDomComment timestamp = doc.documentElement().lastChild().toComment();

	return timestamp.data().stripWhiteSpace().toInt();
}

void TOC::buildCache()
{
	KProcess *meinproc = new KProcess;
	connect( meinproc, TQT_SIGNAL( processExited( KProcess * ) ),
	         this, TQT_SLOT( meinprocExited( KProcess * ) ) );

	*meinproc << locate( "exe", "meinproc" );
	*meinproc << "--stylesheet" << locate( "data", "khelpcenter/table-of-contents.xslt" );
	*meinproc << "--output" << m_cacheFile;
	*meinproc << m_sourceFile;

	meinproc->start( KProcess::NotifyOnExit );
}

void TOC::meinprocExited( KProcess *meinproc )
{
	if ( !meinproc->normalExit() || meinproc->exitStatus() != 0 ) {
		delete meinproc;
		return;
	}

	delete meinproc;

	TQFile f( m_cacheFile );
	if ( !f.open( IO_ReadWrite ) )
		return;

	TQDomDocument doc;
	if ( !doc.setContent( &f ) )
		return;

	TQDomComment timestamp = doc.createComment( TQString::number( sourceFileCTime() ) );
	doc.documentElement().appendChild( timestamp );

	f.tqat( 0 );
	TQTextStream stream( &f );
	stream.setEncoding( TQTextStream::UnicodeUTF8 );
	stream << doc.toString();

	f.close();

	fillTree();
}

void TOC::fillTree()
{
	TQFile f( m_cacheFile );
	if ( !f.open( IO_ReadOnly ) )
		return;

	TQDomDocument doc;
	if ( !doc.setContent( &f ) )
		return;
	
	TOCChapterItem *chapItem = 0;
	TQDomNodeList chapters = doc.documentElement().elementsByTagName( "chapter" );
	for ( unsigned int chapterCount = 0; chapterCount < chapters.count(); chapterCount++ ) {
		TQDomElement chapElem = chapters.item( chapterCount ).toElement();
		TQDomElement chapTitleElem = childElement( chapElem, TQString::fromLatin1( "title" ) );
		TQString chapTitle = chapTitleElem.text().simplifyWhiteSpace();
		TQDomElement chapRefElem = childElement( chapElem, TQString::fromLatin1( "anchor" ) );
		TQString chapRef = chapRefElem.text().stripWhiteSpace();

		chapItem = new TOCChapterItem( this, m_parentItem, chapItem, chapTitle, chapRef );

		TOCSectionItem *sectItem = 0;
		TQDomNodeList sections = chapElem.elementsByTagName( "section" );
		for ( unsigned int sectCount = 0; sectCount < sections.count(); sectCount++ ) {
			TQDomElement sectElem = sections.item( sectCount ).toElement();
			TQDomElement sectTitleElem = childElement( sectElem, TQString::fromLatin1( "title" ) );
			TQString sectTitle = sectTitleElem.text().simplifyWhiteSpace();
			TQDomElement sectRefElem = childElement( sectElem, TQString::fromLatin1( "anchor" ) );
			TQString sectRef = sectRefElem.text().stripWhiteSpace();

			sectItem = new TOCSectionItem( this, chapItem, sectItem, sectTitle, sectRef );
		}
	}

  m_parentItem->setOpen( true );
}

TQDomElement TOC::childElement( const TQDomElement &element, const TQString &name )
{
	TQDomElement e;
	for ( e = element.firstChild().toElement(); !e.isNull(); e = e.nextSibling().toElement() )
		if ( e.tagName() == name )
			break;
	return e;
}

void TOC::slotItemSelected( TQListViewItem *item )
{
	TOCItem *tocItem;
	if ( ( tocItem = dynamic_cast<TOCItem *>( item ) ) )
		emit itemSelected( tocItem->entry()->url() );

	item->setOpen( !item->isOpen() );
}

TOCItem::TOCItem( TOC *toc, TQListViewItem *parentItem, TQListViewItem *after, const TQString &text )
	: NavigatorItem( new DocEntry( text ), parentItem, after )
{
        setAutoDeleteDocEntry( true );
	m_toc = toc;
}

TOCChapterItem::TOCChapterItem( TOC *toc, NavigatorItem *parent, TQListViewItem *after,
				const TQString &title, const TQString &name )
	: TOCItem( toc, parent, after, title ),
	m_name( name )
{
	setOpen( false );
	entry()->setUrl(url());
}

TQString TOCChapterItem::url()
{
	return "help:" + toc()->application() + "/" + m_name + ".html";
}

TOCSectionItem::TOCSectionItem( TOC *toc, TOCChapterItem *parent, TQListViewItem *after,
				const TQString &title, const TQString &name )
	: TOCItem( toc, parent, after, title ),
	m_name( name )
{
	setPixmap( 0, SmallIcon( "document" ) );
	entry()->setUrl(url());
}

TQString TOCSectionItem::url()
{
	if ( static_cast<TOCSectionItem *>( parent()->firstChild() ) == this )
		return static_cast<TOCChapterItem *>( parent() )->url() + "#" + m_name;
	
	return "help:" + toc()->application() + "/" + m_name + ".html";
}

#include "toc.moc"
// vim:ts=2:sw=2:et
