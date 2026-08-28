/*
    This file is part of the KDE system
    Copyright (C)  1999,2000 Boloni Laszlo <lboloni@cpe.ucf.edu>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#ifndef KONSOLE_PART_H
#define KONSOLE_PART_H

#include <tdeparts/browserextension.h>
#include <tdeparts/factory.h>


#include <kdialogbase.h>

#include <kde_terminal_interface.h>

#include "schema.h"
#include "session.h"


class TQCheckBox;
class TQPushButton;
class TQSpinBox;

class KRootPixmap;
class TDEActionMenu;
class TDEInstance;
class TDEPopupMenu;
class TDESelectAction;
class TDEToggleAction;

namespace KParts { class GUIActivateEvent; }

namespace Konsole {

class konsoleBrowserExtension;

class konsoleFactory : public KParts::Factory
{
    TQ_OBJECT
public:
    konsoleFactory();
    virtual ~konsoleFactory();

    virtual KParts::Part* createPartObject(TQWidget *parentWidget = 0, const char *widgetName = 0,
                                     TQObject* parent = 0, const char* name = 0,
                                     const char* classname = "KParts::Part",
                                     const TQStringList &args = TQStringList());

    static TDEInstance *instance();

 private:
    static TDEInstance *s_instance;
    static TDEAboutData *s_aboutData;
};

//////////////////////////////////////////////////////////////////////

class konsolePart: public KParts::ReadOnlyPart, public TerminalInterface, public ExtTerminalInterface
{
    TQ_OBJECT
	public:
    konsolePart(TQWidget *parentWidget, const char *widgetName, TQObject * parent, const char *name,
                const char *classname = 0, const TQString &title = TQString::null);
    virtual ~konsolePart();

signals:
    void processExited( TDEProcess * );
    void receivedData( const TQString& s );
    void forkedChild();
 protected:
    virtual bool openURL( const KURL & url );
    virtual bool openFile() {return false;} // never used
    virtual bool closeURL() {return true;}
    virtual void guiActivateEvent( KParts::GUIActivateEvent * event );

 protected slots:
    void showShell();

    void doneSession(TESession*);
    void sessionDestroyed();
    void configureRequest(TEWidget*,int,int x,int y);
    void updateTitle(TESession*);
    void enableMasterModeConnections();

 private slots:
    void emitOpenURLRequest(const TQString &url);

    void readProperties();
    void saveProperties();
    void applyProperties();
    void setSettingsMenuEnabled( bool );

    void sendSignal(int n);
    void closeCurrentSession();

    void notifySize(int /*columns*/, int /*lines*/);

    void slotToggleFrame();
    void slotSelectScrollbar();
    void slotSelectFont();
    void schema_menu_check();
    void keytab_menu_activated(int item);
    void updateSchemaMenu();
    void setSchema(int n);
    void pixmap_menu_activated(int item);
    void schema_menu_activated(int item);
    void slotHistoryType();
    void slotSelectBell();
    void slotSelectLineSpacing();
    void slotBlinkingCursor();
    void slotToggleMetaAsAltMode();
    void slotUseKonsoleSettings();
    void slotWordSeps();
    void slotSetEncoding();
    void slotGetSessionSchema(TESession *session, TQString &schema);
    void slotSetSessionSchema(TESession *session, const TQString &schema);
    void slotSetSessionEncoding(TESession *session, const TQString &encoding);
    void slotUpdateSessionKeytab(TESession *session, const TQString &keytab);
    void biggerFont();
    void smallerFont();

    void autoShowShell();

 private:
    konsoleBrowserExtension *m_extension;
    KURL currentURL;

    void makeGUI();
    void applySettingsToGUI();

    void setSchema(ColorSchema* s);
    void updateKeytabMenu();
    void setEncoding(const TQString &encoding);

	  bool doOpenStream( const TQString& );
  	bool doWriteStream( const TQByteArray& );
  	bool doCloseStream();

    TQWidget* parentWidget;
    TEWidget* te;
    TESession* se;
    ColorSchemaList* colors;
    KRootPixmap* rootxpm;

    TDEActionCollection* actions;
    TDEActionCollection* settingsActions;

    TDEToggleAction* blinkingCursor;
    TDEToggleAction* showFrame;
    TDEToggleAction* metaAsAlt;
    TDEToggleAction* m_useKonsoleSettings;

    TDESelectAction* selectBell;
    TDESelectAction* selectLineSpacing;
    TDESelectAction* selectScrollbar;
    TDESelectAction* selectSetEncoding;

    TDEActionMenu* m_fontsizes;

    TDEPopupMenu* m_keytab;
    TDEPopupMenu* m_schema;
    TDEPopupMenu* m_signals;
    TDEPopupMenu* m_options;
    TDEPopupMenu* m_popupMenu;

    TQFont       defaultFont;

    TQString     pmPath;  // pixmap path
    TQString     s_title; // session title
    TQString     s_schema;
    TQString     s_tdeconfigSchema;
    TQString     s_word_seps;			// characters that are considered part of a word

    bool        b_framevis:1;
    bool        b_metaAsAlt:1;
    bool        b_histEnabled:1;
    bool        b_useKonsoleSettings:1;
    bool        b_autoDestroy:1;
    bool        b_autoStartShell:1;

    int         curr_schema; // current schema no
    int         n_bell;
    int         n_keytab;
    int         n_render;
    int         n_scroll;
    unsigned    m_histSize;
    bool        m_runningShell;
    bool        m_streamEnabled;
    int         n_encoding;

public:
      virtual bool setPtyFd(int);

    // these are the implementations for the TermEmuInterface
    // functions...
    void startProgram( const TQString& program,
                       const TQStrList& args );
    void newSession();
    void showShellInDir( const TQString& dir );
    void sendInput( const TQString& text );
    void setAutoDestroy( bool );
    void setAutoStartShell( bool );
};

//////////////////////////////////////////////////////////////////////

class HistoryTypeDialog : public KDialogBase
{
    TQ_OBJECT
public:
  HistoryTypeDialog(const HistoryType& histType,
                    unsigned int histSize,
                    TQWidget *parent);

public slots:
  void slotDefault();
  void slotSetUnlimited();
  void slotHistEnable(bool);

  unsigned int nbLines() const;
  bool isOn() const;

protected:
  TQCheckBox* m_btnEnable;
  TQSpinBox*  m_size;
  TQPushButton* m_setUnlimited;
};

//////////////////////////////////////////////////////////////////////

class konsoleBrowserExtension : public KParts::BrowserExtension
{
    TQ_OBJECT
	friend class konsolePart;
 public:
    konsoleBrowserExtension(konsolePart *parent);
    virtual ~konsoleBrowserExtension();

    void emitOpenURLRequest(const KURL &url);
};

}

#endif
