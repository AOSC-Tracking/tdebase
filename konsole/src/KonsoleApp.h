/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1996 by Matthias Ettrich <ettrich@kde.org>
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/
/* The material contained in here more or less directly orginates from    */
/* kvt, which is copyright (c) 1996 by Matthias Ettrich <ettrich@kde.org> */
/*                                                                        */

#ifndef KONSOLEAPP_H
#define KONSOLEAPP_H


#include <tdemainwindow.h>
#include <kdialogbase.h>
#include <tdesimpleconfig.h>
#include <keditcl.h>

#include <twinmodule.h>

#include <tqstrlist.h>
#include <tqintdict.h>
#include <tqptrdict.h>
#include <tqsignalmapper.h>

#include "TEPty.h"
#include "TEWidget.h"
#include "TEmuVt102.h"
#include "session.h"
#include "schema.h"
#include "konsolebookmarkmenu.h"
#include "konsolebookmarkhandler.h"

#include "konsoleiface.h"

#define KONSOLE_VERSION "1.6.6"

class TQCheckBox;
class TQLabel;
class TQSpinBox;
class TQToolButton;

class KRootPixmap;
class KTabWidget;
class KURIFilterData;
class TDEAction;
class TDEPopupMenu;
class TDERadioAction;
class TDESelectAction;
class TDEToggleAction;

namespace Konsole
{

class KonsoleFind;

class KonsoleApp : public TDEMainWindow, virtual public KonsoleIface
{
    TQ_OBJECT

    friend class KonsoleSessionManaged;
public:

  KonsoleApp(const char * name, int histon, bool menubaron, bool tabbaron,
      bool frameon, bool scrollbaron,
      TQCString type = 0, bool b_inRestore = false, const int wanted_tabbar = 0,
      const TQString &workdir=TQString::null);

  ~KonsoleApp();
  void setColLin(int columns, int lines);
  void setAutoClose(bool on);
  void initFullScreen();
  void initSessionFont(int fontNo);
  void initSessionFont(TQFont f);
  void initSessionKeyTab(const TQString &keyTab);
  void initMonitorActivity(bool on);
  void initMonitorSilence(bool on);
  void initMasterMode(bool on);
  void initTabColor(TQColor color);
  void initHistory(int lines, bool enable);
  void newSession(const TQString &program, const TQStrList &args, const TQString &term, const TQString &icon, const TQString &title, const TQString &cwd);
  void setSchema(const TQString & path);
  void setEncoding(int);
  void setSessionTitle(TQString&, TESession* = 0);
  void setSessionEncoding(const TQString&, TESession* = 0);

  void enableFullScripting(bool b);
  void enableFixedSize(bool b);

  void setDefaultSession(const TQString &filename);
  void showTipOnStart();

  // Additional functions for DCOP
  int sessionCount() { return sessions.count(); }

  TQString currentSession();
  TQString newSession(const TQString &type);
  TQString sessionId(const int position);

  void activateSession(const TQString& sessionId);
  void feedAllSessions(const TQString &text);
  void sendAllSessions(const TQString &text);

  KURL baseURL() const;

  virtual bool processDynamic(const TQCString &fun, const TQByteArray &data, TQCString& replyType, TQByteArray &replyData);
  virtual QCStringList functionsDynamic();

  void callReadPropertiesInternal(TDEConfig *config, int number) { readPropertiesInternal(config,number); }

  enum TabPosition { TabNone, TabTop, TabBottom };
  enum TabViewModes { ShowIconAndText = 0, ShowTextOnly = 1, ShowIconOnly = 2 };

public slots:
  void activateSession(int position);
  void activateSession(TQWidget*);
  void slotUpdateSessionConfig(TESession *session);
  void slotResizeSession(TESession*, TQSize);
  void slotSetSessionEncoding(TESession *session, const TQString &encoding);
  void slotGetSessionSchema(TESession *session, TQString &schema);
  void slotSetSessionSchema(TESession *session, const TQString &schema);

  void makeGUI();
  TQString newSession();

protected:

 bool queryClose();
 void saveProperties(TDEConfig* config);
 void readProperties(TDEConfig* config);

private slots:
  void configureRequest(TEWidget*,int,int,int);
  void activateSession();
  void activateSession(TESession*);
  void closeCurrentSession();
  void confirmCloseCurrentSession(TESession* _se=0);
  void doneSession(TESession*);
  void slotCouldNotClose();
  void toggleFullScreen();
  bool fullScreen();
  void setFullScreen(bool on);
  void schema_menu_activated(int item);
  void pixmap_menu_activated(int item, TEWidget* tewidget=0);
  void keytab_menu_activated(int item);
  void schema_menu_check();
  void attachSession(TESession*);
  void slotDetachSession();
  void bookmarks_menu_check();
  void newSession(int kind);
  void newSessionTabbar(int kind);
  void updateSchemaMenu();
  void updateKeytabMenu();
  void updateRMBMenu();

  void changeTabTextColor(TESession*, int);
  void changeColumns(int);
  void changeColLin(int columns, int lines);
  void notifySessionState(TESession* session,int state);
  void notifySize(int columns, int lines);
  void updateTitle(TESession* _se=0);
  void prevSession();
  void nextSession();
  void activateMenu();
  void slotMovedTab(int,int);
  void moveSessionLeft();
  void moveSessionRight();
  void allowPrevNext();
  void setSchema(int n, TEWidget* tewidget=0);   // no slot necessary?
  void sendSignal(int n);
  void slotClearTerminal();
  void slotResetClearTerminal();
  void slotSelectTabbar();
  void slotToggleMenubar();
  void slotRenameSession();
  void slotRenameSession(TESession* ses, const TQString &name);
  void slotToggleMonitor();
  void slotToggleMasterMode();
  void slotClearAllSessionHistories();
  void slotHistoryType();
  void slotClearHistory();
  void slotFindHistory();
  void slotSaveHistory();
  void slotSelectBell();
  void slotSelectSize();
  void slotSelectFont();
  void slotInstallBitmapFonts();
  void slotSelectScrollbar();
  void loadScreenSessions();
  void updateFullScreen(bool on);

  void slotOpenSelection();
  void slotOpenURI(int n);

  void slotSaveSettings();
  void slotSaveSessionsProfile();
  void slotConfigureNotifications();
  void slotConfigureKeys();
  void slotConfigure();
  void reparseConfiguration();

  void disableMasterModeConnections();
  void enableMasterModeConnections();
  void enterURL( const TQString&, const TQString& );
  void newSession( const TQString&, const TQString& );

  void slotFind();
  void slotFindDone();
  void slotFindNext();
  void slotFindPrevious();

  void showTip();

  void slotSetSelectionEnd() { te->setSelectionEnd(); }
  void slotCopyClipboard() { te->copyClipboard(); }
  void slotSelectAll() { if (se) se->getEmulation()->selectAll(b_selectAllHistory); }
  void slotPasteClipboard() { te->pasteClipboard(); }
  void slotPasteSelection() { te->pasteSelection(); }

  void listSessions();
  void switchToSession();

  void biggerFont();
  void smallerFont();

  void slotZModemDetected(TESession *session);
  void slotZModemUpload();

  void slotPrint();

  void toggleBidi();

  void slotTabContextMenu(TQWidget*, const TQPoint &);
  void slotTabDetachSession();
  void slotTabRenameSession();
  void slotTabSelectColor();
  void slotTabResetColor();
  void slotTabCloseSession();
  void slotTabToggleMonitor();
  void slotTabToggleMasterMode();
  void slotTabbarContextMenu(const TQPoint &);
  void slotTabSetViewOptions(int);
  void slotTabbarToggleDynamicHide();
  void slotToggleAutoResizeTabs();
  void slotFontChanged();

  void slotSetEncoding();
private:
  TDESimpleConfig *defaultSession();
  TQString newSession(TDESimpleConfig *co, TQString pgm = TQString::null, const TQStrList &args = TQStrList(),
                     const TQString &_term = TQString::null, const TQString &_icon = TQString::null,
                     const TQString &_title = TQString::null, const TQString &_cwd = TQString::null);
  void readProperties(TDEConfig *config, const TQString &schema, bool globalConfigOnly);
  void applySettingsToGUI();
  void makeTabWidget();
  void makeBasicGUI();
  void runSession(TESession* s);
  void addSession(TESession* s);
  void detachSession(TESession* _se=0);
  void setColorPixmaps();
  void renameSession(TESession* ses);

  void setSchema(ColorSchema* s, TEWidget* tewidget=0);
  void setMasterMode(bool _state, TESession* _se=0);

  void buildSessionMenus();
  void addSessionCommand(const TQString & path);
  void loadSessionCommands();
  void createSessionMenus();
  void addScreenSession(const TQString & path, const TQString & socket);
  void resetScreenSessions();
  void checkBitmapFonts();

  void initTEWidget(TEWidget* new_te, TEWidget* default_te);

  void createSessionTab(TEWidget *widget, const TQIconSet& iconSet,
                        const TQString &text, int index = -1);
  TQIconSet iconSetForSession(TESession *session) const;

  bool eventFilter( TQObject *o, TQEvent *e );

  void setMenuAcceleratos();

  TQPtrList<TEWidget> activeTEs();

  TQPtrDict<TESession> action2session;
  TQPtrDict<TDERadioAction> session2action;
  TQPtrList<TESession> sessions;

  TQIntDict<TDESimpleConfig> no2command;

  TDESimpleConfig* m_defaultSession;
  TQString m_defaultSessionFilename;

  KTabWidget* tabwidget;
  TEWidget*      te;     // the visible TEWidget, either sole one or one of many
  TESession*     se;
  TESession*     se_previous;
  TESession*     m_initialSession;
  ColorSchemaList* colors;
  TQString        s_encodingName;

  TQPtrDict<KRootPixmap> rootxpms;
  TWinModule*    tWinModule;

  KMenuBar*   menubar;
  KStatusBar* statusbar;

  // Text strings for main menu entries
  TQString       m_session_string;
  TQString       m_edit_string;
  TQString       m_view_string;
  TQString       m_bookmarks_string;
  TQString       m_options_string;
  TQString       m_help_string;

  // Main menu entry ids
  int           m_session_id;
  int           m_edit_id;
  int           m_view_id;
  int           m_bookmarks_id;
  int           m_options_id;
  int           m_help_id;
  
  TDEPopupMenu* m_session;
  TDEPopupMenu* m_edit;
  TDEPopupMenu* m_view;
  TDEPopupMenu* m_bookmarks;
  TDEPopupMenu* m_bookmarksSession;
  TDEPopupMenu* m_options;
  TDEPopupMenu* m_schema;
  TDEPopupMenu* m_keytab;
  TDEPopupMenu* m_tabbarSessionsCommands;
  TDEPopupMenu* m_signals;
  TDEPopupMenu* m_help;
  TDEPopupMenu* m_rightButton;
  TDEPopupMenu* m_sessionList;
  TDEPopupMenu* m_tabPopupMenu;
  TDEPopupMenu* m_tabPopupTabsMenu;
  TDEPopupMenu* m_tabbarPopupMenu;
  TDEPopupMenu* m_openSelection;

  TDEAction *m_zmodemUpload;
  TDEToggleAction *monitorActivity, *m_tabMonitorActivity;
  TDEToggleAction *monitorSilence, *m_tabMonitorSilence;
  TDEToggleAction *masterMode, *m_tabMasterMode;
  TDEToggleAction *showMenubar;
  TDEToggleAction *m_fullscreen;

  TDESelectAction *selectSize;
  TDESelectAction *selectScrollbar;
  TDESelectAction *selectTabbar;
  TDESelectAction *selectBell;
  TDESelectAction *selectSetEncoding;

  TDEAction       *m_clearHistory;
  TDEAction       *m_findHistory;
  TDEAction       *m_findNext;
  TDEAction       *m_findPrevious;
  TDEAction       *m_saveHistory;
  TDEAction       *m_detachSession;
  TDEAction       *m_moveSessionLeft;
  TDEAction       *m_moveSessionRight;

  TDEAction       *m_copyClipboard;
  TDEAction       *m_selectAll;
  TDEAction       *m_pasteClipboard;
  TDEAction       *m_pasteSelection;
  TDEAction       *m_clearTerminal;
  TDEAction       *m_resetClearTerminal;
  TDEAction       *m_clearAllSessionHistories;
  TDEAction       *m_renameSession;
  TDEAction       *m_saveProfile;
  TDEAction       *m_closeSession;
  TDEAction       *m_print;
  TDEAction       *m_quit;

  TDEActionCollection *m_shortcuts;

  KonsoleBookmarkHandler *bookmarkHandler;
  KonsoleBookmarkHandler *bookmarkHandlerSession;

  KonsoleFind* m_finddialog;
  bool         m_find_first;
  bool         m_find_found;
  TQString      m_find_pattern;
  TQString  selectedURL;

  int cmd_serial;
  int cmd_first_screen;
  int         n_keytab;
  int         n_defaultKeytab;
  int         n_scroll;
  int         n_tabbar;
  int         n_bell;
  int         n_render;
  int         curr_schema; // current schema no
  int         wallpaperSource;
  int         sessionIdCounter;
  int         monitorSilenceSeconds;

  TQString     s_schema;
  TQString     s_tdeconfigSchema;
  TQString     s_word_seps;			// characters that are considered part of a word
  TQString     pmPath; // pixmap path
  TQString     dropText;
  TQFont       defaultFont;
  TQSize       defaultSize;

  TQRect       _saveGeometry;

  TQTimer      m_closeTimeout;

  TabViewModes m_tabViewMode;
  bool        b_dynamicTabHide;
  bool        b_autoResizeTabs;
  bool        b_installBitmapFonts;

  bool        b_framevis:1;
  bool        b_metaAsAlt:1;
  bool        b_realTransparency:1;
  bool        b_fullscreen:1;
  bool        m_menuCreated:1;
  bool        b_warnQuit:1;
  bool        isRestored:1;
  bool        b_allowResize:1; // Whether application may resize
  bool        b_fixedSize:1; // Whether user may resize
  bool        b_addToUtmp:1;
  bool        b_xonXoff:1;
  bool        b_bidiEnabled:1;

  bool        b_histEnabled:1;
  bool        b_fullScripting:1;
  bool        b_showstartuptip:1;
  bool        b_sessionShortcutsEnabled:1;
  bool        b_sessionShortcutsMapped:1;
  bool        b_matchTabWinTitle:1;
  bool        b_mouseWheelScroll:1;
  bool        b_menuAccelerators:1;
  bool        b_selectAllHistory:1;

  unsigned int m_histSize;
  int m_separator_id;

  TESession*  m_contextMenuSession;

  TQToolButton* m_newSessionButton;
  TQToolButton* m_removeSessionButton;
  TQPoint      m_newSessionButtonMousePressPos;

  TQSignalMapper* sessionNumberMapper;
  TQStringList    sl_sessionShortCuts;
  TQString  s_workDir;

  TQColor    m_tabColor;
  KURIFilterData* m_filterData;
};

class HistoryTypeDialog : public KDialogBase
{
    TQ_OBJECT
public:
  HistoryTypeDialog(const HistoryType& histType,
                    unsigned int histSize,
                    TQWidget *parent);

public slots:

  void slotHistEnable(bool);
  void slotDefault();
  void slotSetUnlimited();

  unsigned int nbLines() const;
  bool isOn() const;

protected:
  TQLabel*        m_label;
  TQSpinBox*      m_size;
  TQCheckBox*     m_btnEnable;
  TQPushButton*   m_setUnlimited;
};

class SizeDialog : public KDialogBase
{
    TQ_OBJECT
public:
  SizeDialog(unsigned int const columns,
             unsigned int const lines,
             TQWidget *parent);

public slots:
  void slotDefault();

  unsigned int columns() const;
  unsigned int lines() const;

protected:
  TQSpinBox*  m_columns;
  TQSpinBox*  m_lines;
};

class KonsoleFind : public KEdFind
{
    TQ_OBJECT
public:
  KonsoleFind( TQWidget *parent = 0, const char *name=0, bool modal=true );
  bool reg_exp() const;

private slots:
  void slotEditRegExp();

private:
  TQCheckBox*    m_asRegExp;
  TQDialog*      m_editorDialog;
  TQPushButton*  m_editRegExp;
};

}

#endif
