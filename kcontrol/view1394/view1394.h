/*
 * view1394.h
 *
 *  Copyright (C) 2003 Alexander Neundorf <neundorf@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef VIEW1394_H_
#define VIEW1394_H_

#include <tdecmodule.h>

#include <tqmap.h>
#include <tqsocketnotifier.h>
#include <tqstring.h>
#include <tqtimer.h>
#include <tqvaluelist.h>
#include <tqlistview.h>

#include "view1394widget.h"

#include <libraw1394/raw1394.h>

class OuiDb
{
   public:
      OuiDb();
      TQString vendor(octlet_t guid);
   private:
      TQMap<TQString, TQString> m_vendorIds;
};

class View1394: public TDECModule
{
   TQ_OBJECT
   public:
      View1394(TQWidget *parent = 0L, const char *name = 0L);
      virtual ~View1394();

   public slots: // Public slots
      void rescanBus();
      void generateBusReset();

   private:
      View1394Widget *m_view;
      TQValueList<raw1394handle_t> m_handles;
      TQPtrList<TQSocketNotifier> m_notifiers;
      bool readConfigRom(raw1394handle_t handle, nodeid_t nodeid, quadlet_t& firstQuad, quadlet_t& cap, octlet_t& guid);
      bool m_insideRescanBus;
      TQTimer m_rescanTimer;
      OuiDb *m_ouiDb;
   private slots:
      void callRaw1394EventLoop(int fd);
};
#endif
