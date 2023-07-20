/**
 * kcookiespolicies.h - Cookies configuration
 *
 * Original Authors
 * Copyright (c) Waldo Bastian <bastian@kde.org>
 * Copyright (c) 1999 David Faure <faure@kde.org>
 *
 * Re-written by:
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
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

#ifndef __KCOOKIESPOLICIES_H
#define __KCOOKIESPOLICIES_H

#include <tqmap.h>
#include <tdecmodule.h>

#include "policydlg.h"

class TQListViewItem;

class DCOPClient;
class KCookiesPolicyDlgUI;

class KCookiesPolicies : public TDECModule
{
    TQ_OBJECT

public:
    KCookiesPolicies(TQWidget *parent = 0);
    ~KCookiesPolicies();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual TQString quickHelp() const;

    void addNewPolicy(const TQString& domain);

protected slots:
    void autoAcceptSessionCookies ( bool );
    void ignoreCookieExpirationDate ( bool );
    void cookiesEnabled( bool );
    void configChanged();

    void selectionChanged();
    void updateButtons();

    void deleteAllPressed();
    void deletePressed();
    void changePressed();
    void addPressed();

private:
    void updateDomainList(const TQStringList& list);
    bool handleDuplicate( const TQString& domain, int );
    void splitDomainAdvice (const TQString& configStr, TQString &domain,
                            KCookieAdvice::Value &advice);

private:
    int d_itemsSelected;
    bool d_configChanged;
    KCookiesPolicyDlgUI* dlg;
    TQMap<TQListViewItem*, const char*> m_pDomainPolicy;
};

#endif // __KCOOKIESPOLICIES_H
