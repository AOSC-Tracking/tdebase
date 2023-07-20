/*
 *  toc.h - part of the TDE Help Center
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
#ifndef KHC_TOC_H
#define KHC_TOC_H

#include "navigatoritem.h"

#include <tqdom.h>
#include <tqlistview.h>
#include <tqobject.h>

class TDEProcess;

namespace KHC {

class TOC : public QObject
{
	TQ_OBJECT
	public:
		TOC( NavigatorItem *parentItem );

		TQString application() const { return m_application; }
		void setApplication( const TQString &application ) { m_application = application; }
	
	public slots:
		void build( const TQString &file );
		
	signals:
		void itemSelected( const TQString &url );

	private slots:
		void slotItemSelected( TQListViewItem *item );
		void meinprocExited( TDEProcess *meinproc );

	private:
		enum CacheStatus { NeedRebuild, CacheOk };

		CacheStatus cacheStatus() const;
		int sourceFileCTime() const;
		int cachedCTime() const;
		TQDomElement childElement( const TQDomElement &e, const TQString &name );
		void buildCache();
		void fillTree();

		TQString m_application;
		TQString m_cacheFile;
		TQString m_sourceFile;

		NavigatorItem *m_parentItem;
};

}

#endif // KHC_TOC_H
