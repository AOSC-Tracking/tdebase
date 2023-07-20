/*
    KSysGuard, the KDE System Guard
   
    Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>. Please do
    not commit any changes without consulting me first. Thanks!

*/

#ifndef KSG_WORKSPACE_H
#define KSG_WORKSPACE_H

#include <tqptrlist.h>
#include <tqtabwidget.h>

class TDEConfig;
class KURL;
class TQString;
class WorkSheet;

class Workspace : public TQTabWidget
{
  TQ_OBJECT

  public:
    Workspace( TQWidget* parent, const char* name = 0 );
    ~Workspace();

    void saveProperties( TDEConfig* );
    void readProperties( TDEConfig* );

    bool saveOnQuit();

    void showProcesses();

    WorkSheet *restoreWorkSheet( const TQString &fileName,
                           const TQString &newName = TQString::null );
    void deleteWorkSheet( const TQString &fileName );

  public slots:
    void newWorkSheet();
    void loadWorkSheet();
    void loadWorkSheet( const KURL& );
    void saveWorkSheet();
    void saveWorkSheet( WorkSheet *sheet );
    void saveWorkSheetAs();
    void saveWorkSheetAs( WorkSheet *sheet );
    void deleteWorkSheet();
    void removeAllWorkSheets();
    void cut();
    void copy();
    void paste();
    void configure();
    void updateCaption( TQWidget* );
    void updateSheetTitle( TQWidget* );
    void applyStyle();

  signals:
    void announceRecentURL( const KURL &url );
    void setCaption( const TQString &text, bool modified );

  private:
    TQPtrList<WorkSheet> mSheetList;

    // Directory that was used for the last load/save.
    TQString mWorkDir;
    bool mAutoSave;
};

#endif
