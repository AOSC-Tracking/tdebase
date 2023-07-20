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

#ifndef __KWRITE_MAIN_H__
#define __KWRITE_MAIN_H__

#include <tdetexteditor/view.h>
#include <tdetexteditor/document.h>

#include <tdeparts/mainwindow.h>

#include <kdialogbase.h>

namespace KTextEditor { class EditorChooser; }

class TDEAction;
class TDEToggleAction;
class TDESelectAction;
class TDERecentFilesAction;

class KWrite : public KParts::MainWindow
{
  TQ_OBJECT
  

  public:
    KWrite(KTextEditor::Document * = 0L);
    ~KWrite();

    void loadURL(const KURL &url);

    KTextEditor::View *view() const { return m_view; }

    static bool noWindows () { return winList.isEmpty(); }

  private:
    void setupActions();
    void setupStatusBar();

    bool queryClose();

    void dragEnterEvent( TQDragEnterEvent * );
    void dropEvent( TQDropEvent * );

  public slots:
    void slotNew();
    void slotFlush ();
    void slotOpen();
    void slotOpen( const KURL& url);
    void newView();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();
    void changeEditor();

  private slots:
    void slotNewToolbarConfig();

  public slots:
    void printNow();
    void printDlg();

    void newStatus(const TQString &msg);
    void newCaption();

    void slotDropEvent(TQDropEvent *);

    void slotEnableActions( bool enable );

    /**
     * adds a changed URL to the recent files
     */
    void slotFileNameChanged();

  //config file functions
  public:
    void readConfig (TDEConfig *);
    void writeConfig (TDEConfig *);

    void readConfig ();
    void writeConfig ();

  //session management
  public:
    void restore(TDEConfig *,int);
    static void restore();

  private:
    void readProperties(TDEConfig *);
    void saveProperties(TDEConfig *);
    void saveGlobalProperties(TDEConfig *);

  private:
    KTextEditor::View * m_view;

    TDERecentFilesAction * m_recentFiles;
    TDEToggleAction * m_paShowPath;
    TDEToggleAction * m_paShowStatusBar;

    TQString encoding;

    static TQPtrList<KTextEditor::Document> docList;
    static TQPtrList<KWrite> winList;
};

class KWriteEditorChooser: public KDialogBase
{
  TQ_OBJECT
  

  public:
    KWriteEditorChooser(TQWidget *parent);
    virtual ~KWriteEditorChooser();

  private:
    KTextEditor::EditorChooser *m_chooser;

  protected slots:
    void slotOk();
};

#endif
