/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_VIEWMANAGER_H__
#define __KATE_VIEWMANAGER_H__

#include "katemain.h"
#include "../interfaces/viewmanager.h"

#include <kate/view.h>
#include <kate/document.h>
#include <tqguardedptr.h>

class KateMainWindow;
class KateViewSpaceContainer;

class TDEConfig;
class TDEAction;

class TQToolButton;

class KateViewManager : public TQObject
{
  TQ_OBJECT

  public:
    KateViewManager (KateMainWindow *parent);
    ~KateViewManager ();

    Kate::ViewManager *viewManager () const { return m_viewManager; };

    KateViewSpaceContainer *activeContainer () { return m_currentContainer; }

    TQPtrList<KateViewSpaceContainer> *containers() { return &m_viewSpaceContainerList; }

    void updateViewSpaceActions ();

  private:
    /**
     * create all actions needed for the view manager
     */
    void setupActions ();

  public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(TDEConfig *config,const TQString& group);

    /* restore it */
    void restoreViewConfiguration (TDEConfig *config,const TQString& group);

    uint openURL (const KURL &url, const TQString& encoding, bool activate = true, bool isTempFile=false);

  public slots:
    void openURL (const KURL &url);

  private:
    void removeViewSpace (KateViewSpace *viewspace);

    bool showFullPath;

  public:
    Kate::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

    void setViewActivationBlocked (bool block);

  public:
    void closeViews(uint documentNumber);
    KateMainWindow *mainWindow();

  private slots:
    void activateView ( Kate::View *view );
    void activateSpace ( Kate::View* v );

    void tabChanged(TQWidget*);

  public slots:
    bool getShowFullPath() const { return showFullPath; }

    void activateView ( uint documentNumber );
    void activateView ( int documentNumber ) { activateView((uint) documentNumber); };

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentClose ();

    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz ();
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert ();

    void slotNewTab();
    void slotCloseTab ();
    void activateNextTab ();
    void activatePrevTab ();

    void slotCloseCurrentViewSpace();

    void setActiveSpace ( KateViewSpace* vs );
    void setActiveView ( Kate::View* view );

    void setShowFullPath(bool enable);

    void activateNextView();
    void activatePrevView();

  protected:
    friend class KateViewSpaceContainer;

    TQGuardedPtr<Kate::View> guiMergedView;

  signals:
    void statusChanged (Kate::View *, int, int, int, bool, int, const TQString &);
    void statChanged ();
    void viewChanged ();

  private:
    Kate::ViewManager *m_viewManager;
    TQPtrList<KateViewSpaceContainer> m_viewSpaceContainerList;
    KateViewSpaceContainer *m_currentContainer;

    KateMainWindow *m_mainWindow;
    bool m_init;

    TQToolButton *m_closeTabButton;
    TDEAction *m_closeView;
    TDEAction *m_closeTab;
    TDEAction *m_activateNextTab;
    TDEAction *m_activatePrevTab;
    TDEAction *goNext;
    TDEAction *goPrev;
};

#endif
