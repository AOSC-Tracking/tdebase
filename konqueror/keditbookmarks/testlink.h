/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __testlink_h
#define __testlink_h

#include <tqobject.h>

#include <tdeio/job.h>
#include <kbookmark.h>

#include "listview.h"
#include "bookmarkiterator.h"

class TestLinkItrHolder : public BookmarkIteratorHolder {
public:
   static TestLinkItrHolder* self() { 
      if (!s_self) { s_self = new TestLinkItrHolder(); }; return s_self; 
   }
   void addAffectedBookmark( const TQString & address );
   void resetToValue(const TQString &url, const TQString &val);
   const TQString getMod(const TQString &url) const;
   const TQString getOldVisit(const TQString &url) const;
   void setMod(const TQString &url, const TQString &val);
   void setOldVisit(const TQString &url, const TQString &val);
   static TQString calcPaintStyle(const TQString &, KEBListViewItem::PaintStyle&, 
                                 const TQString &, const TQString &);
protected:
   virtual void doItrListChanged();
private:
   TestLinkItrHolder();
   static TestLinkItrHolder *s_self;
   TQMap<TQString, TQString> m_modify;
   TQMap<TQString, TQString> m_oldModify;
   TQString m_affectedBookmark;
};

class TestLinkItr : public BookmarkIterator
{
   TQ_OBJECT

public:
   TestLinkItr(TQValueList<KBookmark> bks);
   ~TestLinkItr();
   virtual TestLinkItrHolder* holder() const { return TestLinkItrHolder::self(); }

public slots:
   void slotJobResult(TDEIO::Job *job);
   void slotJobData(TDEIO::Job *job, const TQByteArray &data);

private:
   virtual void doAction();
   virtual bool isApplicable(const KBookmark &bk) const;

   TDEIO::TransferJob *m_job;
   bool m_errSet;
};

#endif
