/*
 *  glossary.h - part of the TDE Help Center
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
#ifndef KHC_GLOSSARY_H
#define KHC_GLOSSARY_H

#include <tdelistview.h>

#include <tqdict.h>
#include <tqdom.h>
#include <tqstringlist.h>

class TDEConfig;
class TDEProcess;

class EntryItem;

namespace KHC {

class GlossaryEntryXRef
{
	friend TQDataStream &operator>>( TQDataStream &, GlossaryEntryXRef & );
	public:
		typedef TQValueList<GlossaryEntryXRef> List;

		GlossaryEntryXRef() {}
		GlossaryEntryXRef( const TQString &term, const TQString &id ) :
			m_term( term ),
			m_id( id )
		{
		}

		TQString term() const { return m_term; }
		TQString id() const { return m_id; }
	
	private:
		TQString m_term;
		TQString m_id;
};

inline TQDataStream &operator<<( TQDataStream &stream, const GlossaryEntryXRef &e )
{
	return stream << e.term() << e.id();
}

inline TQDataStream &operator>>( TQDataStream &stream, GlossaryEntryXRef &e )
{
	return stream >> e.m_term >> e.m_id;
}

class GlossaryEntry
{
	friend TQDataStream &operator>>( TQDataStream &, GlossaryEntry & );
	public:
		GlossaryEntry() {}
		GlossaryEntry( const TQString &term, const TQString &definition,
				const GlossaryEntryXRef::List &seeAlso ) :
			m_term( term ),
			m_definition( definition ),
			m_seeAlso( seeAlso )
			{
			}

		TQString term() const { return m_term; }
		TQString definition() const { return m_definition; }
		GlossaryEntryXRef::List seeAlso() const { return m_seeAlso; }
	
	private:
		TQString m_term;
		TQString m_definition;
		GlossaryEntryXRef::List m_seeAlso;
};

inline TQDataStream &operator<<( TQDataStream &stream, const GlossaryEntry &e )
{
	return stream << e.term() << e.definition() << e.seeAlso();
}

inline TQDataStream &operator>>( TQDataStream &stream, GlossaryEntry &e )
{
	return stream >> e.m_term >> e.m_definition >> e.m_seeAlso;
}

class Glossary : public TDEListView
{
	TQ_OBJECT
	public:
		Glossary( TQWidget *parent );
		virtual ~Glossary();

		const GlossaryEntry &entry( const TQString &id ) const;
 
    static TQString entryToHtml( const GlossaryEntry &entry );

    virtual void show();

	public slots:
		void slotSelectGlossEntry( const TQString &id );

	signals:
		void entrySelected( const GlossaryEntry &entry );
		
	private slots:
		void meinprocExited( TDEProcess *meinproc );
		void treeItemSelected( TQListViewItem *item );

	private:
		enum CacheStatus { NeedRebuild, CacheOk };

		CacheStatus cacheStatus() const;
		int glossaryCTime() const;
		void rebuildGlossaryCache();
		void buildGlossaryTree();
		TQDomElement childElement( const TQDomElement &e, const TQString &name );

		TDEConfig *m_config;
		TQListViewItem *m_byTopicItem;
		TQListViewItem *m_alphabItem;
		TQString m_sourceFile;
		TQString m_cacheFile;
		CacheStatus m_status;
		TQDict<GlossaryEntry> m_glossEntries;
    TQDict<EntryItem> m_idDict;
    bool m_initialized;
};

}

#endif // KHC_GLOSSARY_H
