/*

   This file is part of the KDE project, module kcmbackground.

   Copyright (C) 2002 Laurent Montel <montell@club-internet.fr>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2005 David Saxton <david@bluehaze.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef _BGMONITOR_H_
#define _BGMONITOR_H_

#include <tqlabel.h>
#include <tqvaluevector.h>
#include <tqwidget.h>

class BGMonitor;
class BGMonitorLabel;
class KPixmap;

/**
 * This class arranges and resizes a set of monitor images according to the
 * monitor geometries.
 */
class BGMonitorArrangement : public TQWidget
{
    TQ_OBJECT
public:
    BGMonitorArrangement(TQWidget *parent, const char *name=0L);

    /**
     * Splits up the pixmap according to monitor geometries and sets each
     * BGMonitor pixmap accordingly.
     */
    void setPixmap( const KPixmap & pm );
    TQSize combinedPreviewSize() const { return m_combinedPreviewSize; }
    TQSize maxPreviewSize() const { return m_maxPreviewSize; }
    unsigned numMonitors() const { return m_pBGMonitor.size(); }

    BGMonitor * monitor( unsigned screen ) const;

public slots:
    void updateArrangement();

signals:
    void imageDropped(const TQString &);
    void arrangementUpdated();

protected:
    virtual void resizeEvent( TQResizeEvent * );
    TQRect expandToPreview( TQRect r ) const;
    TQSize expandToPreview( TQSize s ) const;
    TQPoint expandToPreview( TQPoint p ) const;

    TQValueVector<BGMonitorLabel*> m_pBGMonitor;
    TQSize m_combinedPreviewSize;
    TQSize m_maxPreviewSize;
};

/**
 * Contains a BGMonitor.
 */
class BGMonitorLabel : public TQLabel
{
public:
    BGMonitorLabel(TQWidget *parent, const char *name=0L);

    BGMonitor * monitor() const { return m_pBGMonitor; }
    void updateMonitorGeometry();

    void setPreviewPosition( TQRect r ) { m_previewPosition = r; }
    TQRect previewPosition() const { return m_previewPosition; }

protected:
    virtual void resizeEvent( TQResizeEvent * );
    BGMonitor * m_pBGMonitor;
    TQRect m_previewPosition;
};


/**
 * This class handles drops on the preview monitor.
 */
class BGMonitor : public TQLabel
{
    TQ_OBJECT
public:
    BGMonitor(TQWidget *parent, const char *name=0L);

signals:
    void imageDropped(const TQString &);

protected:
    virtual void dropEvent(TQDropEvent *);
    virtual void dragEnterEvent(TQDragEnterEvent *);
};


#endif
