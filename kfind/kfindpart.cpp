/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "kfind.h"
#include "kfindpart.h"
#include "kquery.h"

#include <kparts/genericfactory.h>
#include <kdebug.h>
#include <klocale.h>

#include <tqdir.h>
#include <kinstance.h>

class KonqDirPart;

typedef KParts::GenericFactory<KFindPart> KFindFactory;
K_EXPORT_COMPONENT_FACTORY( libkfindpart, KFindFactory )

KFindPart::KFindPart( TQWidget * parentWidget, const char *widgetName, 
	              TQObject *parent, const char *name ,
		      const TQStringList & /*args*/ )
    : KonqDirPart (parent, name )/*KParts::ReadOnlyPart*/
{
    setInstance( KFindFactory::instance() );

    setBrowserExtension( new KonqDirPartBrowserExtension( this ) );

    kdDebug() << "KFindPart::KFindPart " << this << endl;
    m_kfindWidget = new Kfind( parentWidget, widgetName );
    m_kfindWidget->setMaximumHeight(m_kfindWidget->minimumSizeHint().height());
    const KFileItem *item = ((KonqDirPart*)parent)->currentItem();
    kdDebug() << "Kfind: currentItem:  " << ( item ? item->url().path().local8Bit() : TQString("null") ) << endl;
    TQDir d;
  	if( item && d.exists( item->url().path() ))
	  	m_kfindWidget->setURL( item->url() );

    setWidget( m_kfindWidget );

    connect( m_kfindWidget, TQT_SIGNAL(started()),
             this, TQT_SLOT(slotStarted()) );
    connect( m_kfindWidget, TQT_SIGNAL(destroyMe()),
             this, TQT_SLOT(slotDestroyMe()) );
    connect(m_kfindWidget->dirlister,TQT_SIGNAL(deleteItem(KFileItem*)), this, TQT_SLOT(removeFile(KFileItem*)));
    connect(m_kfindWidget->dirlister,TQT_SIGNAL(newItems(const KFileItemList&)), this, TQT_SLOT(newFiles(const KFileItemList&)));
    //setXMLFile( "kfind.rc" );
    query = new KQuery(this);
    connect(query, TQT_SIGNAL(addFile(const KFileItem *, const TQString&)),
            TQT_SLOT(addFile(const KFileItem *, const TQString&)));
    connect(query, TQT_SIGNAL(result(int)),
            TQT_SLOT(slotResult(int)));

    m_kfindWidget->setQuery(query);
    m_bShowsResult = false;

    m_lstFileItems.setAutoDelete( true );
}

KFindPart::~KFindPart()
{
}

KAboutData *KFindPart::createAboutData()
{
    return new KAboutData( "kfindpart", I18N_NOOP( "Find Component" ), "1.0" );
}

bool KFindPart::doOpenURL( const KURL &url )
{
    m_kfindWidget->setURL( url );
    return true;
}

void KFindPart::slotStarted()
{
    kdDebug() << "KFindPart::slotStarted" << endl;
    m_bShowsResult = true;
    m_lstFileItems.clear(); // clear our internal list
    emit started();
    emit clear();
}

void KFindPart::addFile(const KFileItem *item, const TQString& /*matchingLine*/)
{
    // item is deleted by caller
    // we need to clone it
    KFileItem *clonedItem = new KFileItem(*item);
    m_lstFileItems.append( clonedItem );

    KFileItemList lstNewItems;
    lstNewItems.append(clonedItem);
    emit newItems(lstNewItems);
   
  /*
  win->insertItem(item);

  if (!isResultReported)
  {
    emit haveResults(true);
    isResultReported = true;
  }

  */
}

/* An item has been removed, so update konqueror's view */
void KFindPart::removeFile(KFileItem *item)
{
  KFileItem *iter;
  KFileItemList listiter;

  emit started();
  emit clear();

  m_lstFileItems.remove( item );  //not working ?

  for(iter=m_lstFileItems.first(); iter; iter=m_lstFileItems.next() ) {
    if(iter->url()!=item->url())
      listiter.append(iter);
  }
  
  emit newItems(listiter);
  emit finished();
}

void KFindPart::newFiles(const KFileItemList&)
{
  if(m_bShowsResult)
    return;
  emit started();
  emit clear();
  if (m_lstFileItems.count())
    emit newItems(m_lstFileItems);
  emit finished();
}

void KFindPart::slotResult(int errorCode)
{
  if (errorCode == 0)
    emit finished();
    //setStatusMsg(i18n("Ready."));
  else if (errorCode == KIO::ERR_USER_CANCELED)
    emit canceled();
    //setStatusMsg(i18n("Aborted."));
  else
    emit canceled(); // TODO ?
    //setStatusMsg(i18n("Error."));
  m_bShowsResult=false;
  m_kfindWidget->searchFinished();
}

void KFindPart::slotDestroyMe()
{
  m_kfindWidget->stopSearch();
  emit clear(); // this is necessary to clear the delayed-mimetypes items list
  m_lstFileItems.clear(); // clear our internal list
  emit findClosed();
}

void KFindPart::saveState( TQDataStream& stream )
{
  KonqDirPart::saveState(stream); 

  m_kfindWidget->saveState( &stream );
  //Now we'll save the search result
  KFileItem *fileitem=m_lstFileItems.first();
  stream << m_lstFileItems.count();
  while(fileitem!=NULL)
  {
        stream << *fileitem;
        fileitem=m_lstFileItems.next();
  }
}

void KFindPart::restoreState( TQDataStream& stream )
{
  KonqDirPart::restoreState(stream); 
  int nbitems;
  KURL itemUrl;

  m_kfindWidget->restoreState( &stream );

  stream >> nbitems;
  slotStarted();
  for(int i=0;i<nbitems;i++)
  {
    KFileItem* item = new KFileItem( KFileItem::Unknown, KFileItem::Unknown, KURL() );
    stream >> *item;
    m_lstFileItems.append(item);
  }
  if (nbitems)
    emit newItems(m_lstFileItems);

  emit finished();
}

#include "kfindpart.moc"
