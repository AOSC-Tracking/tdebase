/* This file is part of the TDE project
   Copyright (C) 2015 Michele Calgaro <micheleDOTcalgaro__AT__yahooDOTit>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_SESSIONPANEL_H__
#define __KATE_SESSIONPANEL_H__

/*
  The kate session panel displays the available sessions (and their documents)
  in a treeview list and allows for quick switching among them.
  A toolbar on the top also provides quick access to actions needed
  to manage sessions.
*/

#include <tqvbox.h>
#include <tdetoolbar.h>
#include <tdelistview.h>
#include <tqframe.h>

class KateMainWindow;
class KateViewManager;
class OldKateSessionManager;
class TDEActionCollection;


class KateSessionPanelToolBarParent: public TQFrame
{
  Q_OBJECT

  public:
    KateSessionPanelToolBarParent(TQWidget *parent) : TQFrame(parent), m_tbar(0) {}
   ~KateSessionPanelToolBarParent() {}
    void setToolBar(TDEToolBar *tbar);

  protected:
    virtual void resizeEvent (TQResizeEvent*);

  private:
    TDEToolBar *m_tbar;
};



class KateSessionPanel : public TQVBox
{
  Q_OBJECT

  public:

    KateSessionPanel(KateMainWindow *mainWindow=0, KateViewManager *viewManager=0,
                     TQWidget *parent=0, const char *name=0);
    ~KateSessionPanel() {}

  public slots:
    void saveSession();
    void saveSessionAs();
    void renameSession();
    void deleteSession();
    void sessionActivate();
    void sessionToggleReadOnly();
    void sessionMoveUp();
    void sessionMoveDown();

  private:
    void setup_toolbar();

    KateMainWindow *m_mainWin;
    KateViewManager *m_viewManager;
    OldKateSessionManager *m_sessionManager;
    TDEActionCollection *m_actionCollection;

    TDEToolBar  *m_toolbar;
    TDEListView *m_listview;
};


#endif //__KATE_SESSIONPANEL_H__
// kate: space-indent on; indent-width 2; replace-tabs on;
