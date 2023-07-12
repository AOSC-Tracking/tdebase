/* This file is part of the KDE project
   Copyright (C) 2003 Aaron J. Seigo <aseigo@olympusproject.org>

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

#ifndef __tom_h_
#define __tom_h_

#include <tqpixmap.h>

#include <kpanelmenu.h>
#include <klibloader.h>

class TDEPopupMenu;
class TQPopupMenu;

typedef TQPtrList<TQPopupMenu> PopupMenuList;
typedef TQMap<int, KService::Ptr> TaskMap;

class TOM : public KPanelMenu
{
    Q_OBJECT

    public:
        TOM(TQWidget *parent = 0, const char *name = 0);
        ~TOM();

        // for the side image
        /*void setMinimumSize(const TQSize &);
        void setMaximumSize(const TQSize &);
        void setMinimumSize(int, int);
        void setMaximumSize(int, int);  */

    protected slots:
        void slotClear();
        void slotExec(int);
        //void configChanged();
        void initialize();
        void contextualizeRMBmenu(TDEPopupMenu* menu, int menuItem, TQPopupMenu* ctxMenu);
        //void paletteChanged();
        void clearRecentDocHistory();
        void runCommand();
        void runTask(int id);
        void initializeRecentDocs();
        void openRecentDocument(int id);
        void logout();

        /*
         * slots for the RMB menu on task group
         */
        void removeTask();

    protected:
        void reload();

        int  appendTaskGroup(TDEConfig& config, bool inSubMenu = true );
        void initializeRecentApps(TQPopupMenu* menu);
        //int  insertTOMTitle(TQPopupMenu* menu, const TQString &text, int id = -1, int index = -1);

        /*
         * this stuff should be shared w/the kmenu

        TQRect sideImageRect();
        TQMouseEvent translateMouseEvent( TQMouseEvent* e );
        void resizeEvent(TQResizeEvent *);
        void paintEvent(TQPaintEvent *);
        void mousePressEvent(TQMouseEvent *);
        void mouseReleaseEvent(TQMouseEvent *);
        void mouseMoveEvent(TQMouseEvent *);
        bool loadSidePixmap();

        TQPixmap m_sidePixmap;
        TQPixmap m_sideTilePixmap;*/
        PopupMenuList m_submenus;
        TQFont m_largerFont;
        int m_maxIndex;
        bool m_isImmutable;
        bool m_detailedTaskEntries;
        bool m_detailedNamesFirst;
        TaskMap m_tasks;
        TDEPopupMenu* m_recentDocsMenu;
        TQStringList m_recentDocURLs;
};

class TOMFactory : public KLibFactory
{
    public:
        TOMFactory(TQObject *parent = 0, const char *name = 0);

    protected:
        TQObject* createObject(TQObject *parent = 0, const char *name = 0,
                              const char *classname = "TQObject",
                              const TQStringList& args = TQStringList());
};


#endif
