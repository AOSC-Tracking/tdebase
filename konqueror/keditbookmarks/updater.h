/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

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

#ifndef __updater_h
#define __updater_h

#include <kbookmark.h>
#include <konq_faviconmgr.h>

#include <tdeparts/part.h>
#include <tdeparts/browserinterface.h>

class FavIconWebGrabber : public QObject
{
    TQ_OBJECT
public:
    FavIconWebGrabber(KParts::ReadOnlyPart *part, const KURL &url);
    ~FavIconWebGrabber() {}

protected slots:
    void slotMimetype(TDEIO::Job *job, const TQString &_type);
    void slotFinished(TDEIO::Job *job);

private:
    KParts::ReadOnlyPart *m_part;
    KURL m_url;
};

class FavIconBrowserInterface;

class FavIconUpdater : public KonqFavIconMgr 
{
    TQ_OBJECT

public:
    FavIconUpdater(TQObject *parent, const char *name);
    ~FavIconUpdater();
    void downloadIcon(const KBookmark &bk);
    void downloadIconActual(const KBookmark &bk);

    virtual void notifyChange(bool isHost, TQString hostOrURL, 
                              TQString iconName);

protected slots:
    void setIconURL(const KURL &iconURL);
    void slotCompleted();
    void timerDone();
    

signals:
    void done(bool succeeded);

private:
    KParts::ReadOnlyPart *m_part;
    FavIconBrowserInterface *m_browserIface;
    FavIconWebGrabber *m_webGrabber;
    TQTimer *m_timer;
    KBookmark m_bk;
};

class FavIconBrowserInterface : public KParts::BrowserInterface
{
    TQ_OBJECT
public:
    FavIconBrowserInterface(FavIconUpdater *view, const char *name)
        : KParts::BrowserInterface(view, name), m_view(view) {
        ;
    }
private:
    FavIconUpdater *m_view;
};

#endif

