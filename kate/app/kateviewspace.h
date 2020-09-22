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

#ifndef __KATE_VIEWSPACE_H__
#define __KATE_VIEWSPACE_H__

#include "katemain.h"

#include <kate/view.h>
#include <kate/document.h>

#include <tqptrlist.h>
#include <tqwidget.h>
#include <tqvbox.h>
#include <kstatusbar.h>

class KVSSBSep;

class TDEConfig;
class KSqueezedTextLabel;
class KateViewSpaceContainer;

class KateVSStatusBar : public KStatusBar
{
  Q_OBJECT

   public:
      KateVSStatusBar ( KateViewSpace *parent = 0L, const char *name = 0L );
      virtual ~KateVSStatusBar ();

   public slots:
      void setStatus( int r, int c, int ovr, bool block, int mod, const TQString &msg );
      void updateMod( bool );
      /**
       * changed the modified icon according to document state.
       * @since Kate 2.4
       */
      void modifiedChanged();

   protected:
      virtual bool eventFilter (TQObject*,TQEvent *);
      virtual void showMenu ();

   private:
      TQLabel* m_lineColLabel;
      TQLabel* m_modifiedLabel;
      TQLabel* m_insertModeLabel;
      TQLabel* m_selectModeLabel;
      KSqueezedTextLabel* m_fileNameLabel;
      TQPixmap m_modPm, m_modDiscPm, m_modmodPm, m_noPm;
      class KateViewSpace *m_viewSpace;
};

class KateViewSpace : public TQVBox
{
  friend class KateViewSpaceContainer;
  friend class KateVSStatusBar;

  Q_OBJECT

  public:
    KateViewSpace(KateViewSpaceContainer *, TQWidget* parent=0, const char* name=0);
    ~KateViewSpace();
    bool isActiveSpace();
    void setActive(bool b, bool showled=false);
    TQWidgetStack* stack;
    void addView(Kate::View* v, bool show=true);
    void removeView(Kate::View* v);
    bool showView(Kate::View* v);
    bool showView(uint docID);
    Kate::View* currentView();
    int viewCount() const { return mViewList.count(); }

    void saveConfig (TDEConfig* config, int myIndex,const TQString& viewConfGrp);
    void restoreConfig ( class KateViewSpaceContainer *viewMan, TDEConfig* config, const TQString &group );


  protected:
    /** reimplemented to catch TQEvent::PaletteChange,
    since we use a modified palette for the statusbar */
    bool event( TQEvent * );

  private:
    bool mIsActiveSpace;
    KateVSStatusBar* mStatusBar;
    TQLabel* l;
    TQPixmap i_active;
    TQPixmap i_empty;
    TQPtrList<Kate::View> mViewList;
    int mViewCount;
    KVSSBSep *sep;
    KateViewSpaceContainer *m_viewManager;
    TQString m_group;

  private slots:
    void slotStatusChanged (Kate::View *view, int r, int c, int ovr, bool block, int mod, const TQString &msg);

  public slots:
    void polish();
    void modifiedOnDisc(Kate::Document *, bool, unsigned char);
};

#endif
