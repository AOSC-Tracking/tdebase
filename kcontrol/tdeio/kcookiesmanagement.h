/**
 * kcookiesmanagement.h - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 *
 * Contributors:
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
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

#ifndef __KCOOKIESMANAGEMENT_H
#define __KCOOKIESMANAGEMENT_H

#include <tqdict.h>
#include <tqstringlist.h>
#include <tqlistview.h>

#include <tdecmodule.h>


class DCOPClient;
class KCookiesManagementDlgUI;

struct CookieProp;

class CookieListViewItem : public TQListViewItem
{
public:
    CookieListViewItem(TQListView *parent, TQString dom);
    CookieListViewItem(TQListViewItem *parent, CookieProp *cookie);
    ~CookieListViewItem();

    TQString domain() const { return mDomain; }
    CookieProp* cookie() const { return mCookie; }
    CookieProp* leaveCookie();
    void setCookiesLoaded() { mCookiesLoaded = true; }
    bool cookiesLoaded() const { return mCookiesLoaded; }
    virtual TQString text(int f) const;

private:
    void init( CookieProp* cookie,
               TQString domain = TQString::null,
               bool cookieLoaded=false );
    CookieProp *mCookie;
    TQString mDomain;
    bool mCookiesLoaded;
};

class KCookiesManagement : public TDECModule
{
    TQ_OBJECT

public:
    KCookiesManagement(TQWidget *parent = 0 );
    ~KCookiesManagement();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual TQString quickHelp() const;

private slots:
    void deleteCookie();
    void deleteAllCookies();
    void getDomains();
    void getCookies(TQListViewItem*);
    void showCookieDetails(TQListViewItem*);
    void doPolicy();

private:
    void reset ();
    void deleteCookie(TQListViewItem*);
    bool cookieDetails(CookieProp *cookie);
    void clearCookieDetails();
    bool policyenabled();

private:
    bool m_bDeleteAll;

    TQWidget* mainWidget;
    KCookiesManagementDlgUI* dlg;

    TQStringList deletedDomains;
    typedef TQPtrList<CookieProp> CookiePropList;
    TQDict<CookiePropList> deletedCookies;
};

#endif // __KCOOKIESMANAGEMENT_H
