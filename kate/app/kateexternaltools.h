/*
   This file is part of the Kate text editor of the KDE project.
   It describes a "external tools" action for kate and provides a dialog
   page to configure that.

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

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#ifndef _KATE_EXTERNAL_TOOLS_H_
#define _KATE_EXTERNAL_TOOLS_H_

#include <tdeaction.h>
#include <kdialogbase.h>
#include <kate/document.h>
#include <kmacroexpander.h>
#include <tqpixmap.h>

/**
 * The external tools action
 * This action creates a menu, in which each item will launch a process
 * with the provided arguments, which may include the following macros:
 * - %URLS: the URLs of all open documents.
 * - %URL: The URL of the active document.
 * - %filedir: The directory of the current document, if that is a local file.
 * - %selection: The selection of the active document.
 * - %text: The text of the active document.
 * - %line: The line number of the cursor in the active view.
 * - %column: The column of the cursor in the active view.
 *
 * Each item has the following properties:
 * - Name: The friendly text for the menu
 * - Exec: The command to execute, including arguments.
 * - TryExec: the name of the executable, if not available, the
 *       item will not be displayed.
 * - MimeTypes: An optional list of mimetypes. The item will be disabled or
 *       hidden if the current file is not of one of the indicated types.
 *
 */
class KateExternalToolsMenuAction : public TDEActionMenu
{
  friend class KateExternalToolAction;

  TQ_OBJECT
  public:
    KateExternalToolsMenuAction( const TQString &text=TQString::null, TQObject *parent=0, const char* name=0, class KateMainWindow *mw=0 );
    ~KateExternalToolsMenuAction() {};

    /**
     * This will load all the confiured services.
     */
    void reload();

    class TDEActionCollection *actionCollection() { return m_actionCollection; }

  private slots:
    void slotDocumentChanged();

  private:
    class TDEActionCollection *m_actionCollection;
    class KateMainWindow *mainwindow; // for the actions to access view/doc managers
};

/**
 * This Action contains a KateExternalTool
 */
class KateExternalToolAction : public TDEAction, public KWordMacroExpander
{
  TQ_OBJECT
  public:
    KateExternalToolAction( TQObject *parent, const char *name, class KateExternalTool *t );
    ~KateExternalToolAction();
  protected:
    virtual bool expandMacro( const TQString &str, TQStringList &ret );

  private slots:
    void slotRun();

  public:
    class KateExternalTool *tool;
};

/**
 * This class defines a single external tool.
 */
class KateExternalTool
{
  public:
    KateExternalTool( const TQString &name=TQString::null,
                      const TQString &command=TQString::null,
                      const TQString &icon=TQString::null,
                      const TQString &tryexec=TQString::null,
                      const TQStringList &mimetypes=TQStringList(),
                      const TQString &acname=TQString::null,
                      const TQString &cmdname=TQString::null,
                      int save=0 );
    ~KateExternalTool() {};

    TQString name; ///< The name used in the menu.
    TQString command; ///< The command to execute.
    TQString icon; ///< the icon to use in the menu.
    TQString tryexec; ///< The name or path of the executable.
    TQStringList mimetypes; ///< Optional list of mimetypes for which this action is valid.
    bool hasexec; ///< This is set by the constructor by calling checkExec(), if a value is present.
    TQString acname; ///< The name for the action. This is generated first time the action is is created.
    TQString cmdname; ///< The name for the commandline.
    int save; ///< We can save documents prior to activating the tool command: 0 = nothing, 1 = current document, 2 = all documents.

    /**
     * @return true if mimetypes is empty, or the @p mimetype matches.
     */
    bool valid( const TQString &mimetype ) const;
    /**
     * @return true if "tryexec" exists and has the executable bit set, or is
     * empty.
     * This is run at least once, and the tool is disabled if it fails.
     */
    bool checkExec();

  private:
    TQString m_exec; ///< The fully qualified path of the executable.
};

/**
 * The config widget.
 * The config widget allows the user to view a list of services of the type
 * "Kate/ExternalTool" and add, remove or edit them.
 */
class KateExternalToolsConfigWidget : public Kate::ConfigPage
{
  TQ_OBJECT
  public:
    KateExternalToolsConfigWidget( TQWidget *parent, const char* name);
    virtual ~KateExternalToolsConfigWidget();

    virtual void apply();
    virtual void reload();
    virtual void reset() { reload(); } // sigh
    virtual void defaults() { reload(); } // double sigh

  private slots:
    void slotNew();
    void slotEdit();
    void slotRemove();
    void slotInsertSeparator();

    void slotMoveUp();
    void slotMoveDown();

    void slotSelectionChanged();

  private:
    TQPixmap blankIcon();

    TQStringList m_removed;

    class TDEListBox *lbTools;
    class TQPushButton *btnNew, *btnRemove, *btnEdit, *btnMoveUp, *btnMoveDwn;

    class TDEConfig *config;

    bool m_changed;
};

/**
 * A Singleton class for invoking external tools with the view command line
 */
 class KateExternalToolsCommand : public Kate::Command {
 public:
    KateExternalToolsCommand ();
    virtual ~KateExternalToolsCommand () {};
    static KateExternalToolsCommand *self();
    void reload();
  public:
    virtual TQStringList cmds ();
    virtual bool exec (Kate::View *view, const TQString &cmd, TQString &msg);
    virtual bool help (Kate::View *view, const TQString &cmd, TQString &msg);
  private:
    static KateExternalToolsCommand *s_self;
    TQStringList m_list;
    TQMap<TQString,TQString> m_map;
    bool m_inited;
 };

/**
 * A Dialog to edit a single KateExternalTool object
 */
class KateExternalToolServiceEditor : public KDialogBase
{
  TQ_OBJECT

  public:

    KateExternalToolServiceEditor( KateExternalTool *tool=0,
    				   TQWidget *parent=0, const char *name=0 );

    class TQLineEdit *leName, *leExecutable, *leMimetypes,*leCmdLine;
    class TQTextEdit *teCommand;
    class TDEIconButton *btnIcon;
    class TQComboBox *cmbSave;

  private slots:
    /**
     * Run when the OK button is clicked, to ensure critical values are provided
     */
    void slotOk();
    /**
     * show a mimetype chooser dialog
     */
    void showMTDlg();

  private:
    KateExternalTool *tool;
};
#endif //_KATE_EXTERNAL_TOOLS_H_
