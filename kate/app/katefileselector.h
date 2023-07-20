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

#ifndef __KATE_FILESELECTOR_H__
#define __KATE_FILESELECTOR_H__

#include "katemain.h"
#include "katedocmanager.h"
#include <kate/document.h>

#include <tqvbox.h>
#include <tdefile.h>
#include <kurl.h>
#include <tdetoolbar.h>
#include <tqframe.h>

class KateMainWindow;
class KateViewManager;
class TDEActionCollection;
class TDEActionSelector;
class KFileView;

/*
    The kate file selector presents a directory view, in which the default action is
    to open the activated file.
    Additinally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/

/* I think this fix for not moving toolbars is better */
class KateFileSelectorToolBar: public TDEToolBar
{
	TQ_OBJECT
public:
	KateFileSelectorToolBar(TQWidget *parent);
	virtual ~KateFileSelectorToolBar();

	 virtual void setMovingEnabled( bool b );
};

class KateFileSelectorToolBarParent: public TQFrame
{
	TQ_OBJECT
public:
	KateFileSelectorToolBarParent(TQWidget *parent);
	~KateFileSelectorToolBarParent();
	void setToolBar(KateFileSelectorToolBar *tb);
private:
	KateFileSelectorToolBar *m_tb;
protected:
	virtual void resizeEvent ( TQResizeEvent * );
};

class KateFileSelector : public TQVBox
{
  TQ_OBJECT

  friend class KFSConfigPage;

  public:
    /* When to sync to current document directory */
    enum AutoSyncEvent { DocumentChanged=1, GotVisible=2 };

    KateFileSelector( KateMainWindow *mainWindow=0, KateViewManager *viewManager=0,
                      TQWidget * parent = 0, const char * name = 0 );
    ~KateFileSelector();

    void readConfig( TDEConfig *, const TQString & );
    void writeConfig( TDEConfig *, const TQString & );
    void setupToolbar( TDEConfig * );
    void setView( KFile::FileView );
    KDirOperator *dirOperator(){ return dir; }
    TDEActionCollection *actionCollection() { return mActionCollection; };

  public slots:
    void slotFilterChange(const TQString&);
    void setDir(KURL);
    void setDir( const TQString& url ) { setDir( KURL( url ) ); };
    void kateViewChanged();
    void selectorViewChanged( KFileView * );

  private slots:
    void cmbPathActivated( const KURL& u );
    void cmbPathReturnPressed( const TQString& u );
    void dirUrlEntered( const KURL& u );
    void dirFinishedLoading();
    void setActiveDocumentDir();
    void btnFilterClick();

  protected:
    void focusInEvent( TQFocusEvent * );
    void showEvent( TQShowEvent * );
    bool eventFilter( TQObject *, TQEvent * );
    void initialDirChangeHack();

  private:
    class KateFileSelectorToolBar *toolbar;
    TDEActionCollection *mActionCollection;
    class KBookmarkHandler *bookmarkHandler;
    KURLComboBox *cmbPath;
    KDirOperator * dir;
    class TDEAction *acSyncDir;
    KHistoryCombo * filter;
    class TQToolButton *btnFilter;

    KateMainWindow *mainwin;
    KateViewManager *viewmanager;

    TQString lastFilter;
    int autoSyncEvents; // enabled autosync events
    TQString waitingUrl; // maybe display when we gets visible
    TQString waitingDir;
};

/*  TODO anders
    KFSFilterHelper
    A popup widget presenting a listbox with checkable items
    representing the mime types available in the current directory, and
    providing a name filter based on those.
*/

/*
    Config page for file selector.
    Allows for configuring the toolbar, the history length
    of the path and file filter combos, and how to handle
    user closed session.
*/
class KFSConfigPage : public Kate::ConfigPage {
  TQ_OBJECT
  public:
    KFSConfigPage( TQWidget* parent=0, const char *name=0, KateFileSelector *kfs=0);
    virtual ~KFSConfigPage() {};

    virtual void apply();
    virtual void reload();

  private slots:
    void slotMyChanged();

  private:
    void init();

    KateFileSelector *fileSelector;
    TDEActionSelector *acSel;
    class TQSpinBox *sbPathHistLength, *sbFilterHistLength;
    class TQCheckBox *cbSyncActive, *cbSyncShow;
    class TQCheckBox *cbSesLocation, *cbSesFilter;

    bool m_changed;
};


#endif //__KATE_FILESELECTOR_H__
