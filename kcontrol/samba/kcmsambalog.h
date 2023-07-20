/*
 * kcmsambalog.h
 *
 * Copyright (c) 2000 Alexander Neundorf <neundorf@kde.org>
*
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
#ifndef kcmsambalog_h_included
#define kcmsambalog_h_included
 
#include <tqlabel.h>
#include <tqcstring.h>
#include <tqcheckbox.h>
#include <tqpushbutton.h>
#include <tqlistview.h>
#include <tqevent.h>
#include <tqapplication.h>
#include <tdeconfig.h>

#include <kurlrequester.h>

#define LOGGROUPNAME "SambaLogFileSettings"

class LogView: public TQWidget
{
   TQ_OBJECT
   public:
      LogView(TQWidget *parent=0, TDEConfig *config=0, const char *name=0);
      virtual ~LogView() {};
      void saveSettings();
      void loadSettings();
   private:
      TDEConfig *configFile;
      int filesCount, connectionsCount;
      KURLRequester logFileName;
      TQLabel label;
      TQListView viewHistory;
      TQCheckBox showConnOpen, showConnClose, showFileOpen, showFileClose;
      TQPushButton updateButton;
   private slots:
      void updateList();
   signals:
      void contentsChanged(TQListView* list, int nrOfFiles, int nrOfConnections);
};

class QListViewItemX:public TQListViewItem
{
   public:
      //a faster constructor saves a lot time
      QListViewItemX( TQListView * parent,
                      const char *c0,     const char *c1 = 0,
                      const char *c2 = 0, const char *c3 = 0,
                      const char *c4 = 0, const char *c5 = 0,
                      const char *c6 = 0, const char *c7 = 0 )
         :TQListViewItem(parent)
      {
         setText( 0, c0 );
         setText( 1, c1 );
         setText( 2, c2 );
         setText( 3, c3 );
         if (c4==0) return;
         setText( 4, c4 );
         if (c5==0) return;
         setText( 5, c5 );
         if (c6==0) return;
         setText( 6, c6 );
         if (c7==0) return;
         setText( 7, c7 );
      };
};

#endif // main_included
