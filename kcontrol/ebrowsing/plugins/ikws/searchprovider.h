/*
 * Copyright (c) 2000 Malte Starostik <malte@kde.org>
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


#ifndef __SEARCHPROVIDER_H___
#define __SEARCHPROVIDER_H___

#include <kservice.h>
#include <tqmap.h>

typedef TQMap<TQString, TQString> SearchCategoryMap;

class SearchProvider
{
public:
    SearchProvider() : m_dirty(false) {};
    SearchProvider(const KService::Ptr service);

    const TQString& desktopEntryName() const { return m_desktopEntryName; }
    const TQString& name() const { return m_name; }
    const TQString& query() const { return m_query; }
    const TQStringList& keys() const { return m_keys; }
    const TQString& charset() const { return m_charset; }
    const TQString& category() const { return m_category; }
    bool isDirty() const { return m_dirty; }

    void setName(const TQString &);
    void setQuery(const TQString &);
    void setKeys(const TQStringList &);
    void setCharset(const TQString &);
    void setCategory(const TQString &);

    static SearchProvider *findByDesktopName(const TQString &);
    static SearchProvider *findByKey(const TQString &);

    static const TQString& searchCategoryName(const TQString& category);
    static const TQString& searchCategoryKey(const TQString& displayName);

    static SearchCategoryMap searchCategories;

private:
    void initSearchCategories();

private:
    TQString m_desktopEntryName;
    TQString m_name;
    TQString m_query;
    TQStringList m_keys;
    TQString m_charset;
    TQString m_category;

    bool m_dirty;
};

#endif
