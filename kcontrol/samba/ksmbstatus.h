/*
 * ksmbstatus.h
 *
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
#ifndef ksmbstatus_h_included
#define ksmbstatus_h_included
 
#include <tqtimer.h>
#include <tqlabel.h>
#include <tdeprocess.h>
#include <tqcstring.h>
#include <tqlistview.h>
#include <tdeconfig.h>

#define SCREEN_XY_OFFSET 20

class TQListView;
class TQListViewItem;
class TDEProcess;

class NetMon : public TQWidget
{
TQ_OBJECT
public:
   NetMon(TQWidget *parent, TDEConfig * config=0, const char * name=0);
   void saveSettings() {};
   void loadSettings() {};
private:
   TDEConfig *configFile;
   TDEProcess *showmountProc;
   TQListView *list;
   TQLabel *version;
   TQTimer *timer;
   TQListViewItem *killrow;
   int rownumber;
   enum {header, connexions, locked_files, finished, nfs} readingpart;
   int lo[65536];
   int nrpid;
   void processNFSLine(char *bufline, int linelen);
   void processSambaLine(char *bufline, int linelen);

   TQCString strShare, strUser, strGroup, strMachine, strSince, strPid;
   int iUser, iGroup, iMachine, iPid;

private slots:
   void killShowmount();
   void update();
   void slotReceivedData(TDEProcess *proc, char *buffer, int buflen);
};

#endif // main_included
