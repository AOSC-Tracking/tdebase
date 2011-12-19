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

#include "konsole_part.h"

#include <assert.h>

#include <tqfile.h>
#include <tqlayout.h>
#include <tqwmatrix.h>

#include <kaboutdata.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kfontdialog.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstdaction.h>
#include <tqlabel.h>
#include <kprocctrl.h>

#include <tqcheckbox.h>
#include <tqspinbox.h>
#include <tqpushbutton.h>
#include <kpopupmenu.h>
#include <krootpixmap.h>
#include <kconfig.h>
#include <kaction.h>

// We can't use the ARGB32 visual when embedded in another application
bool argb_visual = false;

K_EXPORT_COMPONENT_FACTORY( libkonsolepart, konsoleFactory )

/**
 * We need one static instance of the factory for our C 'main' function
 */
KInstance *konsoleFactory::s_instance = 0L;
KAboutData *konsoleFactory::s_aboutData = 0;

konsoleFactory::konsoleFactory()
{
}

konsoleFactory::~konsoleFactory()
{
  if (s_instance)
    delete s_instance;

  if ( s_aboutData )
    delete s_aboutData;

  s_instance = 0;
  s_aboutData = 0;
}

KParts::Part *konsoleFactory::createPartObject(TQWidget *parentWidget, const char *widgetName,
                                         TQObject *parent, const char *name, const char *classname,
                                         const TQStringList&)
{
//  kdDebug(1211) << "konsoleFactory::createPart parentWidget=" << parentWidget << " parent=" << parent << endl;
  KParts::Part *obj = new konsolePart(parentWidget, widgetName, parent, name, classname);
  return obj;
}

KInstance *konsoleFactory::instance()
{
  if ( !s_instance )
    {
      s_aboutData = new KAboutData("konsole", I18N_NOOP("Konsole"), "1.5");
      s_instance = new KInstance( s_aboutData );
    }
  return s_instance;
}

#define DEFAULT_HISTORY_SIZE 1000

konsolePart::konsolePart(TQWidget *_parentWidget, const char *widgetName, TQObject *parent, const char *name, const char *classname)
  : KParts::ReadOnlyPart(parent, name)
,te(0)
,se(0)
,colors(0)
,rootxpm(0)
,blinkingCursor(0)
,showFrame(0)
,m_useKonsoleSettings(0)
,selectBell(0)
,selectLineSpacing(0)
,selectScrollbar(0)
,m_keytab(0)
,m_schema(0)
,m_signals(0)
,m_options(0)
,m_popupMenu(0)
,b_useKonsoleSettings(false)
,b_autoDestroy(true)
,b_autoStartShell(true)
,m_histSize(DEFAULT_HISTORY_SIZE)
,m_runningShell( false )
{
  parentWidget=_parentWidget;
  setInstance(konsoleFactory::instance());

  m_extension = new konsoleBrowserExtension(this);

  // This is needed since only konsole.cpp does it
  // Without this -> crash on keypress... (David)
  KeyTrans::loadAll();

  m_streamEnabled = ( classname && strcmp( classname, "TerminalEmulator" ) == 0 );

  TQStrList eargs;


  const char* shell = getenv("SHELL");
  if (shell == NULL || *shell == '\0') shell = "/bin/sh";
  eargs.append(shell);
  te = new TEWidget(parentWidget,widgetName);
  te->setMinimumSize(150,70);    // allow resizing, cause resize in TEWidget

  setWidget(TQT_TQWIDGET(te));
  te->setFocus();
  connect( te,TQT_SIGNAL(configureRequest(TEWidget*,int,int,int)),
           this,TQT_SLOT(configureRequest(TEWidget*,int,int,int)) );

  colors = new ColorSchemaList();
  colors->checkSchemas();
  colors->sort();

  // Check to see which config file we use: konsolepartrc or konsolerc
  KConfig* config = new KConfig("konsolepartrc", true);
  config->setDesktopGroup();
  b_useKonsoleSettings = config->readBoolEntry("use_konsole_settings", false);
  delete config;

  readProperties();

  makeGUI();

  if (m_schema)
  {
     updateSchemaMenu();

     ColorSchema *sch=colors->find(s_schema);
     if (sch)
        curr_schema=sch->numb();
     else
        curr_schema = 0;

     for (uint i=0; i<m_schema->count(); i++)
        m_schema->setItemChecked(i,false);

     m_schema->setItemChecked(curr_schema,true);
  }

  // insert keymaps into menu
  if (m_keytab)
  {
     m_keytab->clear();

     TQStringList kt_titles;
     typedef TQMap<TQString,KeyTrans*> QStringKeyTransMap;
     QStringKeyTransMap kt_map;

     for (int i = 0; i < KeyTrans::count(); i++)
     {
        KeyTrans* ktr = KeyTrans::find(i);
        assert( ktr );
        TQString title=ktr->hdr().lower();
        kt_titles << title;
        kt_map[title] = ktr;
     }
     kt_titles.sort();
     for ( TQStringList::Iterator it = kt_titles.begin(); it != kt_titles.end(); ++it ) {
        KeyTrans* ktr = kt_map[*it];
        assert( ktr );
        TQString title=ktr->hdr();
        m_keytab->insertItem(title.replace('&',"&&"),ktr->numb());
     }
  }

  applySettingsToGUI();

  TQTimer::singleShot( 0, this, TQT_SLOT( autoShowShell() ) );
}

void konsolePart::autoShowShell()
{
    // possibly clear the screen?
    if (b_autoStartShell)
        showShell();
}

void konsolePart::setAutoDestroy( bool enabled )
{
    b_autoDestroy = enabled;
}

void konsolePart::setAutoStartShell( bool enabled )
{
    b_autoStartShell = enabled;
}

void konsolePart::doneSession(TESession*)
{
  // see doneSession in konsole.cpp
  if (se && b_autoDestroy)
  {
//    kdDebug(1211) << "doneSession - disconnecting done" << endl;
    disconnect( se,TQT_SIGNAL(done(TESession*)),
                this,TQT_SLOT(doneSession(TESession*)) );
    se->setConnect(false);
    //TQTimer::singleShot(100,se,TQT_SLOT(terminate()));
//    kdDebug(1211) << "se->terminate()" << endl;
    se->terminate();
  }
}

void konsolePart::sessionDestroyed()
{
//  kdDebug(1211) << "sessionDestroyed()" << endl;
  disconnect( se, TQT_SIGNAL( destroyed() ), this, TQT_SLOT( sessionDestroyed() ) );
  se = 0;
  if (b_autoDestroy)
      delete this;
}

void konsolePart::configureRequest(TEWidget*_te,int,int x,int y)
{
  if (m_popupMenu)
     m_popupMenu->popup(_te->mapToGlobal(TQPoint(x,y)));
}

konsolePart::~konsolePart()
{
//  kdDebug(1211) << "konsolePart::~konsolePart() this=" << this << endl;
  if ( se ) {
    setAutoDestroy(false);
    se->closeSession();

    // Wait a bit for all childs to clean themselves up.
    while(se && KProcessController::theKProcessController->waitForProcessExit(1))
        ;

    disconnect( se, TQT_SIGNAL( destroyed() ), this, TQT_SLOT( sessionDestroyed() ) );
//    kdDebug(1211) << "Deleting se session" << endl;
    delete se;
    se=0;
  }

  if (colors) delete colors;
  colors=0;

  //te is deleted by the framework
}

bool konsolePart::openURL( const KURL & url )
{
  //kdDebug(1211) << "konsolePart::openURL " << url.prettyURL() << endl;

  if (currentURL==url) {
    emit completed();
    return true;
  }

  m_url = url;
  emit setWindowCaption( url.prettyURL() );
//  kdDebug(1211) << "Set Window Caption to " << url.prettyURL() << "\n";
  emit started( 0 );

  if ( url.isLocalFile() ) {
      struct stat buff;
      stat( TQFile::encodeName( url.path() ), &buff );
      TQString text = ( S_ISDIR( buff.st_mode ) ? url.path() : url.directory() );
      showShellInDir( text );
  }

  emit completed();
  return true;
}

void konsolePart::emitOpenURLRequest(const TQString &cwd)
{
  KURL url;
  url.setPath(cwd);
  if (url==currentURL)
    return;
  currentURL=url;
  m_extension->emitOpenURLRequest(url);
}


void konsolePart::makeGUI()
{
  if (!kapp->authorizeKAction("konsole_rmb"))
     return;

  actions = new KActionCollection( (KMainWindow*)parentWidget );
  settingsActions = new KActionCollection( (KMainWindow*)parentWidget );

  // Send Signal Menu -------------------------------------------------------------
  if (kapp->authorizeKAction("send_signal"))
  {
     m_signals = new KPopupMenu((KMainWindow*)parentWidget);
     m_signals->insertItem( i18n( "&Suspend Task" )   + " (STOP)", SIGSTOP);
     m_signals->insertItem( i18n( "&Continue Task" )  + " (CONT)", SIGCONT);
     m_signals->insertItem( i18n( "&Hangup" )         + " (HUP)",   SIGHUP);
     m_signals->insertItem( i18n( "&Interrupt Task" ) + " (INT)",   SIGINT);
     m_signals->insertItem( i18n( "&Terminate Task" ) + " (TERM)", SIGTERM);
     m_signals->insertItem( i18n( "&Kill Task" )      + " (KILL)",  SIGKILL);
     m_signals->insertItem( i18n( "User Signal &1")   + " (USR1)", SIGUSR1);
     m_signals->insertItem( i18n( "User Signal &2")   + " (USR2)", SIGUSR2);
     connect(m_signals, TQT_SIGNAL(activated(int)), TQT_SLOT(sendSignal(int)));
  }

  // Settings Menu ----------------------------------------------------------------
  if (kapp->authorizeKAction("settings"))
  {
     m_options = new KPopupMenu((KMainWindow*)parentWidget);

     // Scrollbar
     selectScrollbar = new KSelectAction(i18n("Sc&rollbar"), 0, this,
                                      TQT_SLOT(slotSelectScrollbar()), settingsActions);

     TQStringList scrollitems;
     scrollitems << i18n("&Hide") << i18n("&Left") << i18n("&Right");
     selectScrollbar->setItems(scrollitems);
     selectScrollbar->plug(m_options);

     // Select Bell
     m_options->insertSeparator();
     selectBell = new KSelectAction(i18n("&Bell"), SmallIconSet( "bell"), 0 , this,
                                 TQT_SLOT(slotSelectBell()), settingsActions, "bell");

     TQStringList bellitems;
     bellitems << i18n("System &Bell")
            << i18n("System &Notification")
            << i18n("&Visible Bell")
            << i18n("N&one");
     selectBell->setItems(bellitems);
     selectBell->plug(m_options);

      m_fontsizes = new KActionMenu( i18n( "Font" ), SmallIconSet( "text" ), settingsActions, 0L );
      m_fontsizes->insert( new KAction( i18n( "&Enlarge Font" ), SmallIconSet( "viewmag+" ), 0, this, TQT_SLOT( biggerFont() ), settingsActions, "enlarge_font" ) );
      m_fontsizes->insert( new KAction( i18n( "&Shrink Font" ), SmallIconSet( "viewmag-" ), 0, this, TQT_SLOT( smallerFont() ), settingsActions, "shrink_font" ) );
      m_fontsizes->insert( new KAction( i18n( "Se&lect..." ), SmallIconSet( "font" ), 0, this, TQT_SLOT( slotSelectFont() ), settingsActions, "select_font" ) );
      m_fontsizes->plug(m_options);

      // encoding menu, start with default checked !
      selectSetEncoding = new KSelectAction( i18n( "&Encoding" ), SmallIconSet("charset" ), 0, this, TQT_SLOT(slotSetEncoding()), settingsActions, "set_encoding" );
      TQStringList list = KGlobal::charsets()->descriptiveEncodingNames();
      list.prepend( i18n( "Default" ) );
      selectSetEncoding->setItems(list);
      selectSetEncoding->setCurrentItem (0);
      selectSetEncoding->plug(m_options);

     // Keyboard Options Menu ---------------------------------------------------
     if (kapp->authorizeKAction("keyboard"))
     {
        m_keytab = new KPopupMenu((KMainWindow*)parentWidget);
        m_keytab->setCheckable(true);
        connect(m_keytab, TQT_SIGNAL(activated(int)), TQT_SLOT(keytab_menu_activated(int)));
        m_options->insertItem( SmallIconSet( "key_bindings" ), i18n( "&Keyboard" ), m_keytab );
     }

     // Schema Options Menu -----------------------------------------------------
     if (kapp->authorizeKAction("schema"))
     {
        m_schema = new KPopupMenu((KMainWindow*)parentWidget);
        m_schema->setCheckable(true);
        connect(m_schema, TQT_SIGNAL(activated(int)), TQT_SLOT(schema_menu_activated(int)));
        connect(m_schema, TQT_SIGNAL(aboutToShow()), TQT_SLOT(schema_menu_check()));
        m_options->insertItem( SmallIconSet( "colorize" ), i18n( "Sch&ema" ), m_schema);
     }


     KAction *historyType = new KAction(i18n("&History..."), "history", 0, this,
                                        TQT_SLOT(slotHistoryType()), settingsActions, "history");
     historyType->plug(m_options);
     m_options->insertSeparator();

     // Select line spacing
     selectLineSpacing = new KSelectAction(i18n("Li&ne Spacing"),
        SmallIconSet("leftjust"), 0, this,
        TQT_SLOT(slotSelectLineSpacing()), settingsActions );

     TQStringList lineSpacingList;
     lineSpacingList
       << i18n("&0")
       << i18n("&1")
       << i18n("&2")
       << i18n("&3")
       << i18n("&4")
       << i18n("&5")
       << i18n("&6")
       << i18n("&7")
       << i18n("&8");
     selectLineSpacing->setItems(lineSpacingList);
     selectLineSpacing->plug(m_options);

     // Blinking Cursor
     blinkingCursor = new KToggleAction (i18n("Blinking &Cursor"),
                                      0, this,TQT_SLOT(slotBlinkingCursor()), settingsActions);
     blinkingCursor->plug(m_options);

     // Frame on/off
     showFrame = new KToggleAction(i18n("Show Fr&ame"), 0,
                                this, TQT_SLOT(slotToggleFrame()), settingsActions);
     showFrame->setCheckedState(i18n("Hide Fr&ame"));
     showFrame->plug(m_options);

     // Word Connectors
     KAction *WordSeps = new KAction(i18n("Wor&d Connectors..."), 0, this,
                                  TQT_SLOT(slotWordSeps()), settingsActions);
     WordSeps->plug(m_options);

     // Use Konsole's Settings
     m_options->insertSeparator();
     m_useKonsoleSettings = new KToggleAction( i18n("&Use Konsole's Settings"),
                          0, this, TQT_SLOT(slotUseKonsoleSettings()), 0, "use_konsole_settings" );
     m_useKonsoleSettings->plug(m_options);

     // Save Settings
     m_options->insertSeparator();
     KAction *saveSettings = new KAction(i18n("&Save as Default"), "filesave", 0, this, 
                    TQT_SLOT(saveProperties()), actions, "save_default");
     saveSettings->plug(m_options);
     if (KGlobalSettings::insertTearOffHandle())
        m_options->insertTearOffHandle();
  }

  // Popup Menu -------------------------------------------------------------------
  m_popupMenu = new KPopupMenu((KMainWindow*)parentWidget);
  KAction* selectionEnd = new KAction(i18n("Set Selection End"), 0, TQT_TQOBJECT(te),
                               TQT_SLOT(setSelectionEnd()), actions, "selection_end");
  selectionEnd->plug(m_popupMenu);

  KAction *copyClipboard = new KAction(i18n("&Copy"), "editcopy", 0,
                                        TQT_TQOBJECT(te), TQT_SLOT(copyClipboard()), actions, "edit_copy");
  copyClipboard->plug(m_popupMenu);

  KAction *pasteClipboard = new KAction(i18n("&Paste"), "editpaste", 0,
                                        TQT_TQOBJECT(te), TQT_SLOT(pasteClipboard()), actions, "edit_paste");
  pasteClipboard->plug(m_popupMenu);

  if (m_signals)
  {
     m_popupMenu->insertItem(i18n("&Send Signal"), m_signals);
     m_popupMenu->insertSeparator();
  }

  if (m_options)
  {
     m_popupMenu->insertItem(i18n("S&ettings"), m_options);
     m_popupMenu->insertSeparator();
  }

  KAction *closeSession = new KAction(i18n("&Close Terminal Emulator"), "fileclose", 0, this,
                                      TQT_SLOT(closeCurrentSession()), actions, "close_session");
  closeSession->plug(m_popupMenu);
  if (KGlobalSettings::insertTearOffHandle())
    m_popupMenu->insertTearOffHandle();
}

void konsolePart::applySettingsToGUI()
{
  m_useKonsoleSettings->setChecked( b_useKonsoleSettings );
  setSettingsMenuEnabled( !b_useKonsoleSettings );

  applyProperties();

  if ( b_useKonsoleSettings )
    return; // Don't change Settings menu items

  if (showFrame)
     showFrame->setChecked( b_framevis );
  if (selectScrollbar)
     selectScrollbar->setCurrentItem(n_scroll);
  updateKeytabMenu();
  if (selectBell)
     selectBell->setCurrentItem(n_bell);
  if (selectLineSpacing)
     selectLineSpacing->setCurrentItem(te->lineSpacing());
  if (blinkingCursor)
     blinkingCursor->setChecked(te->blinkingCursor());
  if (m_schema)
     m_schema->setItemChecked(curr_schema,true);
  if (selectSetEncoding)
     selectSetEncoding->setCurrentItem(n_encoding);
}

void konsolePart::applyProperties()
{
   if ( !se ) return;

   if ( b_histEnabled && m_histSize )
      se->setHistory( HistoryTypeBuffer(m_histSize ) );
   else if ( b_histEnabled && !m_histSize )
      se->setHistory(HistoryTypeFile() );
   else
     se->setHistory( HistoryTypeNone() );
   se->setKeymapNo( n_keytab );

   // FIXME:  Move this somewhere else...
   KConfig* config = new KConfig("konsolerc",true);
   config->setGroup("UTMP");
   se->setAddToUtmp( config->readBoolEntry("AddToUtmp",true));
   delete config;

   se->widget()->setVTFont( defaultFont );
   se->setSchemaNo( curr_schema );
   slotSetEncoding();
}

void konsolePart::setSettingsMenuEnabled( bool enable )
{
   uint count = settingsActions->count();
   for ( uint i = 0; i < count; i++ )
   {
      settingsActions->action( i )->setEnabled( enable );
   }

   // FIXME: These are not in settingsActions.
   //  When disabled, the icons are not 'grey-ed' out.
   m_keytab->setEnabled( enable );
   m_schema->setEnabled( enable );
}

void konsolePart::readProperties()
{
  KConfig* config;

  if ( b_useKonsoleSettings )
    config = new KConfig( "konsolerc", true );
  else
    config = new KConfig( "konsolepartrc", true );

  config->setDesktopGroup();

  b_framevis = config->readBoolEntry("has frame",false);
  b_histEnabled = config->readBoolEntry("historyenabled",true);
  n_bell = TQMIN(config->readUnsignedNumEntry("bellmode",TEWidget::BELLSYSTEM),3);
  n_keytab=config->readNumEntry("keytab",0); // act. the keytab for this session
  n_scroll = TQMIN(config->readUnsignedNumEntry("scrollbar",TEWidget::SCRRIGHT),2);
  m_histSize = config->readNumEntry("history",DEFAULT_HISTORY_SIZE);
  s_word_seps= config->readEntry("wordseps",":@-./_~");

  n_encoding = config->readNumEntry("encoding",0);

  TQFont tmpFont = KGlobalSettings::fixedFont();
  defaultFont = config->readFontEntry("defaultfont", &tmpFont);

  TQString schema = config->readEntry("Schema");

  s_kconfigSchema=config->readEntry("schema");
  ColorSchema* sch = colors->find(schema.isEmpty() ? s_kconfigSchema : schema);
  if (!sch) {
    sch=(ColorSchema*)colors->tqat(0);  //the default one
  }
  if (sch->hasSchemaFileChanged()) sch->rereadSchemaFile();
  s_schema = sch->relPath();
  curr_schema = sch->numb();
  pmPath = sch->imagePath();
  te->setColorTable(sch->table()); //FIXME: set twice here to work around a bug

  if (sch->useTransparency()) {
    if (!argb_visual) {
      if (!rootxpm)
        rootxpm = new KRootPixmap(TQT_TQWIDGET(te));
      rootxpm->setFadeEffect(sch->tr_x(), TQColor(sch->tr_r(), sch->tr_g(), sch->tr_b()));
      rootxpm->start();
      rootxpm->tqrepaint(true);
    }
    else {
      te->setBlendColor(tqRgba(sch->tr_r(), sch->tr_g(), sch->tr_b(), int(sch->tr_x() * 255)));
      te->setErasePixmap( TQPixmap() ); // make sure any background pixmap is unset
    }
  }
  else {
    if (rootxpm) {
      rootxpm->stop();
      delete rootxpm;
      rootxpm=0;
    }
    pixmap_menu_activated(sch->tqalignment());
  }

  te->setBellMode(n_bell);
  te->setBlinkingCursor(config->readBoolEntry("BlinkingCursor",false));
  te->setFrameStyle( b_framevis?(TQFrame::WinPanel|TQFrame::Sunken):TQFrame::NoFrame );
  te->setLineSpacing( config->readUnsignedNumEntry( "LineSpacing", 0 ) );
  te->setScrollbarLocation(n_scroll);
  te->setWordCharacters(s_word_seps);

  delete config;

  config = new KConfig("konsolerc",true);
  config->setDesktopGroup();
  te->setTerminalSizeHint( config->readBoolEntry("TerminalSizeHint",true) );
  delete config;
}

void konsolePart::saveProperties()
{
  KConfig* config = new KConfig("konsolepartrc");
  config->setDesktopGroup();

  if ( b_useKonsoleSettings ) { // Don't save Settings if using konsolerc
    config->writeEntry("use_konsole_settings", m_useKonsoleSettings->isChecked());
  } else {
    config->writeEntry("bellmode",n_bell);
    config->writeEntry("BlinkingCursor", te->blinkingCursor());
    config->writeEntry("defaultfont", (se->widget())->getVTFont());
    config->writeEntry("history", se->history().getSize());
    config->writeEntry("historyenabled", b_histEnabled);
    config->writeEntry("keytab",n_keytab);
    config->writeEntry("has frame",b_framevis);
    config->writeEntry("LineSpacing", te->lineSpacing());
    config->writeEntry("schema",s_kconfigSchema);
    config->writeEntry("scrollbar",n_scroll);
    config->writeEntry("wordseps",s_word_seps);
    config->writeEntry("encoding",n_encoding);
    config->writeEntry("use_konsole_settings",m_useKonsoleSettings->isChecked());
  }

  config->sync();
  delete config;
}

void konsolePart::sendSignal(int sn)
{
  if (se) se->sendSignal(sn);
}

void konsolePart::closeCurrentSession()
{
  if ( se ) se->closeSession();
}

void konsolePart::slotToggleFrame()
{
  b_framevis = showFrame->isChecked();
  te->setFrameStyle( b_framevis?(TQFrame::WinPanel|TQFrame::Sunken):TQFrame::NoFrame);
}

void konsolePart::slotSelectScrollbar()
{
  if ( ! se ) return;
  n_scroll = selectScrollbar->currentItem();
  te->setScrollbarLocation(n_scroll);
}

void konsolePart::slotSelectFont() {
   if ( !se ) return;

   TQFont font = se->widget()->getVTFont();
   if ( KFontDialog::getFont( font, true ) != TQDialog::Accepted )
      return;

   se->widget()->setVTFont( font );
}

void konsolePart::biggerFont(void) {
    if ( !se ) return;

    TQFont f = te->getVTFont();
    f.setPointSize( f.pointSize() + 1 );
    te->setVTFont( f );
}

void konsolePart::smallerFont(void) {
    if ( !se ) return;

    TQFont f = te->getVTFont();
    if ( f.pointSize() < 6 ) return;      // A minimum size
    f.setPointSize( f.pointSize() - 1 );
    te->setVTFont( f );
}

void konsolePart::updateKeytabMenu()
{
  if ( se && m_keytab ) {
    m_keytab->setItemChecked(n_keytab,false);
    m_keytab->setItemChecked(se->keymapNo(),true);
    n_keytab = se->keymapNo();
  } else if ( m_keytab ) {    // no se yet, happens at startup
    m_keytab->setItemChecked(n_keytab, true);
  }
}

void konsolePart::keytab_menu_activated(int item)
{
  if ( ! se ) return;
  se->setKeymapNo(item);
  updateKeytabMenu();
}

void konsolePart::schema_menu_activated(int item)
{
  setSchema(item);
  s_kconfigSchema = s_schema; // This is the new default
}

void konsolePart::schema_menu_check()
{
  if (colors->checkSchemas()) {
    colors->sort();
    updateSchemaMenu();
  }
}

void konsolePart::updateSchemaMenu()
{
  if (!m_schema) return;

  m_schema->clear();
  for (int i = 0; i < (int) colors->count(); i++)  {
    ColorSchema* s = (ColorSchema*)colors->tqat(i);
    TQString title=s->title();
    m_schema->insertItem(title.replace('&',"&&"),s->numb(),0);
  }

  if (te && se) {
    m_schema->setItemChecked(se->schemaNo(),true);
  }
}

void konsolePart::setSchema(int numb)
{
  ColorSchema* s = colors->find(numb);
  if (!s) {
    kdWarning() << "No schema found. Using default." << endl;
    s=(ColorSchema*)colors->tqat(0);
  }
  if (s->numb() != numb)  {
    kdWarning() << "No schema with number " << numb << endl;
  }

  if (s->hasSchemaFileChanged()) {
    const_cast<ColorSchema *>(s)->rereadSchemaFile();
  }
  if (s) setSchema(s);
}

void konsolePart::setSchema(ColorSchema* s)
{
  if (!se) return;
  if (!s) return;

  if (m_schema) {
    m_schema->setItemChecked(curr_schema,false);
    m_schema->setItemChecked(s->numb(),true);
  }

  s_schema = s->relPath();
  curr_schema = s->numb();
  pmPath = s->imagePath();
  te->setColorTable(s->table()); //FIXME: set twice here to work around a bug

  if (s->useTransparency()) {
    if (!argb_visual) {
      if (!rootxpm)
        rootxpm = new KRootPixmap(TQT_TQWIDGET(te));
      rootxpm->setFadeEffect(s->tr_x(), TQColor(s->tr_r(), s->tr_g(), s->tr_b()));
      rootxpm->start();
      rootxpm->tqrepaint(true);
    }
    else {
      te->setBlendColor(tqRgba(s->tr_r(), s->tr_g(), s->tr_b(), int(s->tr_x() * 255)));
      te->setErasePixmap( TQPixmap() ); // make sure any background pixmap is unset
    }
  }
  else {
    if (rootxpm) {
      rootxpm->stop();
      delete rootxpm;
      rootxpm=0;
    }
    pixmap_menu_activated(s->tqalignment());
  }

  te->setColorTable(s->table());
  se->setSchemaNo(s->numb());
}

void konsolePart::notifySize(int /* columns */, int /* lines */)
{
  ColorSchema *sch=colors->find(s_schema);

  if (sch && sch->tqalignment() >= 3)
    pixmap_menu_activated(sch->tqalignment());
}

void konsolePart::pixmap_menu_activated(int item)
{
  if (item <= 1) pmPath = "";
  TQPixmap pm(pmPath);
  if (pm.isNull()) {
    pmPath = "";
    item = 1;
    te->setBackgroundColor(te->getDefaultBackColor());
    return;
  }
  // FIXME: respect scrollbar (instead of te->size)
  n_render= item;
  switch (item) {
    case 1: // none
    case 2: // tile
            te->setBackgroundPixmap(pm);
    break;
    case 3: // center
            { TQPixmap bgPixmap;
              bgPixmap.resize(te->size());
              bgPixmap.fill(te->getDefaultBackColor());
              bitBlt( &bgPixmap, ( te->size().width() - pm.width() ) / 2,
                                ( te->size().height() - pm.height() ) / 2,
                      &pm, 0, 0,
                      pm.width(), pm.height() );

              te->setBackgroundPixmap(bgPixmap);
            }
    break;
    case 4: // full
            {
              float sx = (float)te->size().width() / pm.width();
              float sy = (float)te->size().height() / pm.height();
              TQWMatrix matrix;
              matrix.scale( sx, sy );
              te->setBackgroundPixmap(pm.xForm( matrix ));
            }
    break;
    default: // oops
             n_render = 1;
  }
}

void konsolePart::slotHistoryType()
{
  if ( ! se ) return;
  HistoryTypeDialog dlg(se->history(), m_histSize, (KMainWindow*)parentWidget);
  if (dlg.exec()) {
    if (dlg.isOn()) {
      if (dlg.nbLines() > 0) {
        se->setHistory(HistoryTypeBuffer(dlg.nbLines()));
        m_histSize = dlg.nbLines();
        b_histEnabled = true;
      }
      else {
        se->setHistory(HistoryTypeFile());
        m_histSize = 0;
        b_histEnabled = true;
      }
    }
    else {
      se->setHistory(HistoryTypeNone());
      m_histSize = dlg.nbLines();
      b_histEnabled = false;
    }
  }
}

void konsolePart::slotSelectBell() {
  n_bell = selectBell->currentItem();
  te->setBellMode(n_bell);
}

void konsolePart::slotSetEncoding()
{
  if (!se) return;

  bool found;
  TQString enc = KGlobal::charsets()->encodingForName(selectSetEncoding->currentText());
  TQTextCodec * qtc = KGlobal::charsets()->codecForName(enc, found);
  if(!found)
  {
    kdDebug() << "Codec " << selectSetEncoding->currentText() << " not found!" << endl;
    qtc = TQTextCodec::codecForLocale();
  }

  n_encoding = selectSetEncoding->currentItem();
  se->setEncodingNo(selectSetEncoding->currentItem());
  se->getEmulation()->setCodec(qtc);
}

void konsolePart::slotSelectLineSpacing()
{
  te->setLineSpacing( selectLineSpacing->currentItem() );
}

void konsolePart::slotBlinkingCursor()
{
  te->setBlinkingCursor(blinkingCursor->isChecked());
}

void konsolePart::slotUseKonsoleSettings()
{
   b_useKonsoleSettings = m_useKonsoleSettings->isChecked();

   setSettingsMenuEnabled( !b_useKonsoleSettings );

   readProperties();

   applySettingsToGUI();
}

void konsolePart::slotWordSeps() {
  bool ok;

  TQString seps = KInputDialog::getText( i18n( "Word Connectors" ),
      i18n( "Characters other than alphanumerics considered part of a word when double clicking:" ), s_word_seps, &ok, parentWidget );
  if ( ok )
  {
    s_word_seps = seps;
    te->setWordCharacters(s_word_seps);
  }
}

void konsolePart::enableMasterModeConnections()
{
  if ( se ) se->setListenToKeyPress(true);
}

void konsolePart::updateTitle(TESession *)
{
  if ( se ) emit setWindowCaption( se->fullTitle() );
}

void konsolePart::guiActivateEvent( KParts::GUIActivateEvent * )
{
    // Don't let ReadOnlyPart::guiActivateEvent reset the window caption
}

bool konsolePart::doOpenStream( const TQString& )
{
	return m_streamEnabled;
}

bool konsolePart::doWriteStream( const TQByteArray& data )
{
	if ( m_streamEnabled )
	{
		TQString cmd = TQString::fromLocal8Bit( data.data(), data.size() );
		se->sendSession( cmd );
		return true;
	}
	return false;
}

bool konsolePart::doCloseStream()
{
	return m_streamEnabled;
}

//////////////////////////////////////////////////////////////////////

HistoryTypeDialog::HistoryTypeDialog(const HistoryType& histType,
                                     unsigned int histSize,
                                     TQWidget *parent)
  : KDialogBase(Plain, i18n("History Configuration"),
                Help | Default | Ok | Cancel, Ok,
                parent)
{
  TQFrame *mainFrame = plainPage();

  TQHBoxLayout *hb = new TQHBoxLayout(mainFrame);

  m_btnEnable    = new TQCheckBox(i18n("&Enable"), mainFrame);

  TQObject::connect(m_btnEnable, TQT_SIGNAL(toggled(bool)),
                   this,      TQT_SLOT(slotHistEnable(bool)));

  m_size = new TQSpinBox(0, 10 * 1000 * 1000, 100, mainFrame);
  m_size->setValue(histSize);
  m_size->setSpecialValueText(i18n("Unlimited (number of lines)", "Unlimited"));

  m_setUnlimited = new TQPushButton(i18n("&Set Unlimited"), mainFrame);
  connect( m_setUnlimited,TQT_SIGNAL(clicked()), this,TQT_SLOT(slotSetUnlimited()) );

  hb->addWidget(m_btnEnable);
  hb->addSpacing(10);
  hb->addWidget(new TQLabel(i18n("Number of lines:"), mainFrame));
  hb->addWidget(m_size);
  hb->addSpacing(10);
  hb->addWidget(m_setUnlimited);

  if ( ! histType.isOn()) {
    m_btnEnable->setChecked(false);
    slotHistEnable(false);
  } else {
    m_btnEnable->setChecked(true);
    m_size->setValue(histType.getSize());
    slotHistEnable(true);
  }
  setHelp("configure-history");
}

void HistoryTypeDialog::slotDefault()
{
  m_btnEnable->setChecked(true);
  m_size->setValue(DEFAULT_HISTORY_SIZE);
  slotHistEnable(true);
}

void HistoryTypeDialog::slotHistEnable(bool b)
{
  m_size->setEnabled(b);
  m_setUnlimited->setEnabled(b);
  if (b) m_size->setFocus();
}

void HistoryTypeDialog::slotSetUnlimited()
{
  m_size->setValue(0);
}

unsigned int HistoryTypeDialog::nbLines() const
{
  return m_size->value();
}

bool HistoryTypeDialog::isOn() const
{
  return m_btnEnable->isChecked();
}

konsoleBrowserExtension::konsoleBrowserExtension(konsolePart *parent)
  : KParts::BrowserExtension(parent, "konsoleBrowserExtension")
{
}

konsoleBrowserExtension::~konsoleBrowserExtension()
{
}

void konsoleBrowserExtension::emitOpenURLRequest(const KURL &url)
{
  emit openURLRequest(url);
}

const char* sensibleShell()
{
  const char* shell = getenv("SHELL");
  if (shell == NULL || *shell == '\0') shell = "/bin/sh";
  return shell;
}

void konsolePart::startProgram( const TQString& program,
                                const TQStrList& args )
{
//    kdDebug(1211) << "konsolePart::startProgram for " << program << endl;
    if ( !se )
        newSession();
    se->setProgram( program, args );
    se->run();
}

bool konsolePart::setPtyFd( int master_pty )
{
   kdDebug(1211) << "konsolePart::setPtyFd " << master_pty << endl;
   TEPty *pty = new TEPty();
   pty->setPtyFd(master_pty);
   if ( !se )
      newSession();
   se->setPty(pty);
}

void konsolePart::newSession()
{
  if ( se ) delete se;
  se = new TESession(te, "xterm", parentWidget->winId());
  connect( se,TQT_SIGNAL(done(TESession*)),
           this,TQT_SLOT(doneSession(TESession*)) );
  connect( se,TQT_SIGNAL(openURLRequest(const TQString &)),
           this,TQT_SLOT(emitOpenURLRequest(const TQString &)) );
  connect( se, TQT_SIGNAL( updateTitle(TESession*) ),
           this, TQT_SLOT( updateTitle(TESession*) ) );
  connect( se, TQT_SIGNAL(enableMasterModeConnections()),
           this, TQT_SLOT(enableMasterModeConnections()) );
  connect( se, TQT_SIGNAL( processExited(KProcess *) ),
           this, TQT_SIGNAL( processExited(KProcess *) ) );
  connect( se, TQT_SIGNAL( receivedData( const TQString& ) ),
           this, TQT_SIGNAL( receivedData( const TQString& ) ) );
  connect( se, TQT_SIGNAL( forkedChild() ),
           this, TQT_SIGNAL( forkedChild() ));

  // We ignore the following signals
  //connect( se, TQT_SIGNAL(renameSession(TESession*,const TQString&)),
  //         this, TQT_SLOT(slotRenameSession(TESession*, const TQString&)) );
  //connect( se->getEmulation(), TQT_SIGNAL(changeColumns(int)),
  //         this, TQT_SLOT(changeColumns(int)) );
  //connect( se, TQT_SIGNAL(disableMasterModeConnections()),
  //        this, TQT_SLOT(disableMasterModeConnections()) );

  applyProperties();

  se->setConnect(true);
  // se->run();
  connect( se, TQT_SIGNAL( destroyed() ), this, TQT_SLOT( sessionDestroyed() ) );
//  setFont( n_font ); // we do this here, to make TEWidget recalculate
                     // its geometry..
}

void konsolePart::showShellInDir( const TQString& dir )
{
  if ( ! m_runningShell )
  {
    const char* s = sensibleShell();
    TQStrList args;
    args.append( s );
    startProgram( s, args );
    m_runningShell = true;
  };

  if ( ! dir.isNull() )
  {
      TQString text = dir;
      KRun::shellQuote(text);
      text = TQString::fromLatin1("cd ") + text + '\n';
      te->emitText( text );
  };
}

void konsolePart::showShell()
{
    if ( ! se ) showShellInDir( TQString::null );
}

void konsolePart::sendInput( const TQString& text )
{
    te->emitText( text );
}

#include "konsole_part.moc"
