/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers
                  2002-2003 Douglas Hanley <douglash@caltech.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/

#ifndef __konq_tabs_h__
#define __konq_tabs_h__

#include "konq_frame.h"

#include <ktabwidget.h>

class TQPixmap;
class TQPopupMenu;
class TQToolButton;

class KonqView;
class KonqViewManager;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainerBase;
class KonqFrameContainer;
class TDEConfig;
class KSeparator;
class KProgress;
class TDEAction;

class KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
  TQ_OBJECT
  friend class KonqFrame; //for emitting ctrlTabPressed() only, aleXXX

public:
  KonqFrameTabs(TQWidget* parent, KonqFrameContainerBase* parentContainer,
		KonqViewManager* viewManager, const char * name = 0);
  virtual ~KonqFrameTabs();

  virtual void listViews( ChildViewList *viewList );

  virtual void saveConfig( TDEConfig* config, const TQString &prefix, bool saveURLs,
			   KonqFrameBase* docContainer, int id = 0, int depth = 0 );
  virtual void copyHistory( KonqFrameBase *other );

  virtual void printFrameInfo( const TQString& spaces );

  TQPtrList<KonqFrameBase>* childFrameList() { return m_pChildFrameList; }

  virtual void setTitle( const TQString &title, TQWidget* sender );
  virtual void setTabIcon( const KURL &url, TQWidget* sender );

  virtual TQWidget* widget() { return this; }
  virtual TQCString frameType() { return TQCString("Tabs"); }

  void activateChild();

  /**
   * Call this after inserting a new frame into the splitter.
   */
  void insertChildFrame( KonqFrameBase * frame, int index = -1);

  /**
   * Call this before deleting one of our children.
   */
  void removeChildFrame( KonqFrameBase * frame );

  //inherited
  virtual void reparentFrame(TQWidget * parent,
                             const TQPoint & p, bool showIt=false );

  void moveTabBackward(int index);
  void moveTabForward(int index);


public slots:
  void slotCurrentChanged(TQWidget* newPage);
  void setAlwaysTabbedMode(bool enable);

signals:
  void ctrlTabPressed();
  void removeTabPopup();

protected:
  void refreshSubPopupMenuTab();
  void hideTabBar();

  TQPtrList<KonqFrameBase>* m_pChildFrameList;

private slots:
  void slotContextMenu( const TQPoint& );
  void slotContextMenu( TQWidget*, const TQPoint& );
  void slotCloseRequest( TQWidget* );
  void slotMovedTab( int, int );
  void slotMouseMiddleClick();
  void slotMouseMiddleClick( TQWidget* );

  void slotTestCanDecode(const TQDragMoveEvent *e, bool &accept /* result */);
  void slotReceivedDropEvent( TQDropEvent* );
  void slotInitiateDrag( TQWidget * );
  void slotReceivedDropEvent( TQWidget *, TQDropEvent * );
  void slotSubPopupMenuTabActivated( int );

private:
  KonqViewManager* m_pViewManager;
  TQPopupMenu* m_pPopupMenu;
  TQPopupMenu* m_pSubPopupMenuTab;
  TQToolButton* m_rightWidget;
  TQToolButton* m_leftWidget;
  bool m_permanentCloseButtons;
  bool m_alwaysTabBar;
  bool m_MouseMiddleClickClosesTab;
  int  m_closeOtherTabsId;
};

#endif
