/*  This file is part of the TDE libraries
    Copyright (C) 2013 Timothy Pearson
    Based on kivdirectoryoverlay.h

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _KIVFREESPACEOVERLAY_H_
#define _KIVFREESPACEOVERLAY_H_

#include <tdefileitem.h>
#include <libkonq_export.h>

#include <tqdict.h>

class KDirLister;
class KFileIVI;

class LIBKONQ_EXPORT KIVFreeSpaceOverlay : public TQObject
{
    TQ_OBJECT
    
public:
    KIVFreeSpaceOverlay(KFileIVI* freespace);
    virtual ~KIVFreeSpaceOverlay();
    void start();

signals:
    void finished();

protected:
    virtual void timerEvent(TQTimerEvent *);

private slots:
    void slotDisplay();

private:
    KFileIVI* m_freespace;
};

#endif
