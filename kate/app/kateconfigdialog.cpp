/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include "kateconfigdialog.h"
#include "kateconfigdialog.moc"

#include "katemainwindow.h"

#include "kateconsole.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katefileselector.h"
#include "katefilelist.h"
#include "kateexternaltools.h"

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqspinbox.h>
#include <tqvbox.h>
#include <tqwhatsthis.h>
#include <tqcombobox.h>

#include <tdeinstance.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <tdeglobalaccel.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <tdelistbox.h>
#include <tdelocale.h>
#include <tdesimpleconfig.h>
#include <kstdaction.h>
#include <tdestandarddirs.h>
#include <twin.h>
#include <kseparator.h>

KateConfigDialog::KateConfigDialog ( KateMainWindow *parent, Kate::View *view )
 : KDialogBase ( KDialogBase::TreeList,
                 i18n("Configure"),
                 KDialogBase::Ok | KDialogBase::Apply|KDialogBase::Cancel | KDialogBase::Help,
                 KDialogBase::Ok,
                 parent,
                 "configdialog" )
{
  TDEConfig *config = KateApp::self()->config();

  KWin::setIcons( winId(), KateApp::self()->icon(), KateApp::self()->miniIcon() );

  actionButton( KDialogBase::Apply)->setEnabled( false );

  mainWindow = parent;

  setMinimumSize(600,400);

  v = view;

  pluginPages.setAutoDelete (false);
  editorPages.setAutoDelete (false);

  TQStringList path;

  setShowIconsInTreeList(true);

  path.clear();
  path << i18n("Application");
  setFolderIcon (path, SmallIcon("kate", TDEIcon::SizeSmall));

  path.clear();

  //BEGIN General page
  path << i18n("Application") << i18n("General");
  TQFrame* frGeneral = addPage(path, i18n("General Options"), BarIcon("go-home", TDEIcon::SizeSmall));

  TQVBoxLayout *lo = new TQVBoxLayout( frGeneral );
  lo->setSpacing(KDialog::spacingHint());
  config->setGroup("General");

  // GROUP with the one below: "Appearance"
  TQButtonGroup *bgStartup = new TQButtonGroup( 1, TQt::Horizontal, i18n("&Appearance"), frGeneral );
  lo->addWidget( bgStartup );

  // show full path in title
  config->setGroup("General");
  cb_fullPath = new TQCheckBox(i18n("&Show full path in title"), bgStartup);
  cb_fullPath->setChecked(mainWindow->viewManager()->getShowFullPath());
  TQWhatsThis::add(cb_fullPath, i18n("If this option is checked, the full document path will be shown in the window caption."));
  connect(cb_fullPath, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));

  // show session name in title
  cb_showSessionName = new TQCheckBox(i18n("Show s&ession name in title"), bgStartup);
  cb_showSessionName->setChecked(parent->showSessionName);
  TQWhatsThis::add(cb_showSessionName, i18n("If this option is checked, the session name will be shown in the window caption."));
  connect(cb_showSessionName, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));

  // sort filelist if desired
  cb_sortFiles = new TQCheckBox(bgStartup);
  cb_sortFiles->setText(i18n("Sort &files alphabetically in the file list"));
  cb_sortFiles->setChecked(parent->filelist->sortType() == KateFileList::sortByName);
  TQWhatsThis::add( cb_sortFiles, i18n(
        "If this is checked, the files in the file list will be sorted alphabetically.") );
  connect( cb_sortFiles, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotChanged() ) );

  // GROUP with the one below: "Behavior"
  bgStartup = new TQButtonGroup( 1, TQt::Horizontal, i18n("&Behavior"), frGeneral );
  lo->addWidget( bgStartup );

  // number of recent files
  TQHBox *hbNrf = new TQHBox( bgStartup );
  TQLabel *lNrf = new TQLabel( i18n("&Number of recent files:"), hbNrf );
  sb_numRecentFiles = new TQSpinBox( 0, 1000, 1, hbNrf );
  sb_numRecentFiles->setValue( mainWindow->fileOpenRecent->maxItems() );
  lNrf->setBuddy( sb_numRecentFiles );
  TQString numRecentFileHelpString ( i18n(
        "<qt>Sets the number of recent files remembered by Kate.<p><strong>NOTE: </strong>"
        "If you set this lower than the current value, the list will be truncated and "
        "some items forgotten.</qt>") );
  TQWhatsThis::add( lNrf, numRecentFileHelpString );
  TQWhatsThis::add( sb_numRecentFiles, numRecentFileHelpString );
  connect( sb_numRecentFiles, TQ_SIGNAL( valueChanged ( int ) ), this, TQ_SLOT( slotChanged() ) );

  // Use only one instance of kate (MDI) ?
  cb_useInstance = new TQCheckBox(bgStartup);
  cb_useInstance->setText(i18n("Always use the current instance of kate to open new files"));
  cb_useInstance->setChecked(parent->useInstance);
  TQWhatsThis::add( cb_useInstance, i18n(
        "When checked, all files opened from outside of Kate will only use the "
        "currently opened instance of Kate.") );
  connect( cb_useInstance, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotChanged() ) );

  // sync the konsole ?
  cb_syncKonsole = new TQCheckBox(bgStartup);
  cb_syncKonsole->setText(i18n("Sync &terminal emulator with active document"));
  cb_syncKonsole->setChecked(parent->syncKonsole);
  TQWhatsThis::add( cb_syncKonsole, i18n(
        "If this is checked, the built in Konsole will <code>cd</code> to the directory "
        "of the active document when started and whenever the active document changes, "
        "if the document is a local file.") );
  connect( cb_syncKonsole, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotChanged() ) );

  // modified files notification
  cb_modNotifications = new TQCheckBox(
      i18n("Wa&rn about files modified by foreign processes"), bgStartup );
  cb_modNotifications->setChecked( parent->modNotification );
  TQWhatsThis::add( cb_modNotifications, i18n(
      "If enabled, when Kate receives focus you will be asked what to do with "
      "files that have been modified on the hard disk. If not enabled, you will "
      "be asked what to do with a file that has been modified on the hard disk only "
      "when that file gains focus inside Kate.") );
  connect( cb_modNotifications, TQ_SIGNAL( toggled( bool ) ),
           this, TQ_SLOT( slotChanged() ) );

  // GROUP with the one below: "Meta-informations"
  bgStartup = new TQButtonGroup( 1, TQt::Horizontal, i18n("Meta-Information"), frGeneral );
  lo->addWidget( bgStartup );

  // save meta infos
  cb_saveMetaInfos = new TQCheckBox( bgStartup );
  cb_saveMetaInfos->setText(i18n("Keep &meta-information past sessions"));
  cb_saveMetaInfos->setChecked(KateDocManager::self()->getSaveMetaInfos());
  TQWhatsThis::add(cb_saveMetaInfos, i18n(
        "Check this if you want document configuration like for example "
        "bookmarks to be saved past editor sessions. The configuration will be "
        "restored if the document has not changed when reopened."));
  connect( cb_saveMetaInfos, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotChanged() ) );

  // meta infos days
  TQWidget *wDmf = new TQWidget( bgStartup );
  wDmf->setEnabled(KateDocManager::self()->getSaveMetaInfos());
  TQLayout *layDmf = new TQHBoxLayout(wDmf);
  TQLayout *layLabelDmf = new TQHBoxLayout(layDmf);
  layLabelDmf->addItem(new TQSpacerItem(20,1, TQSizePolicy::Fixed));
  TQLabel *lDmf = new TQLabel( i18n("&Delete unused meta-information after:"), wDmf);
  layLabelDmf->add(lDmf);
  sb_daysMetaInfos = new TQSpinBox( 0, 180, 1, wDmf );
  sb_daysMetaInfos->setSpecialValueText(i18n("(never)"));
  sb_daysMetaInfos->setSuffix(i18n(" day(s)"));
  sb_daysMetaInfos->setValue( KateDocManager::self()->getDaysMetaInfos() );
  layDmf->add(sb_daysMetaInfos);
  lDmf->setBuddy( sb_daysMetaInfos );
  connect( cb_saveMetaInfos, TQ_SIGNAL( toggled( bool ) ), wDmf, TQ_SLOT( setEnabled( bool ) ) );
  connect( sb_daysMetaInfos, TQ_SIGNAL( valueChanged ( int ) ), this, TQ_SLOT( slotChanged() ) );

  lo->addStretch(1); // :-] works correct without autoadd
  //END General page

  path.clear();

  //BEGIN Session page
  path << i18n("Application") << i18n("Sessions");
  TQFrame* frSessions = addPage(path, i18n("Session Management"), BarIcon("history", TDEIcon::SizeSmall));

  lo = new TQVBoxLayout( frSessions );
  lo->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Startup"
  bgStartup = new TQButtonGroup( 1, TQt::Horizontal, i18n("Elements of Sessions"), frSessions );
  lo->addWidget( bgStartup );

  // restore view  config
  cb_restoreVC = new TQCheckBox( bgStartup );
  cb_restoreVC->setText(i18n("Include &window configuration"));
  config->setGroup("General");
  cb_restoreVC->setChecked( config->readBoolEntry("Restore Window Configuration", true) );
  TQWhatsThis::add(cb_restoreVC, i18n(
        "Check this if you want all your views and frames restored each time you open Kate"));
  connect( cb_restoreVC, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotChanged() ) );

  TQRadioButton *rb1, *rb2, *rb3;

  sessions_start = new TQButtonGroup( 1, TQt::Horizontal, i18n("Behavior on Application Startup"), frSessions );
  lo->add (sessions_start);

  sessions_start->setRadioButtonExclusive( true );
  sessions_start->insert( rb1=new TQRadioButton( i18n("&Start new session"), sessions_start ), 0 );
  sessions_start->insert( rb2=new TQRadioButton( i18n("&Load last-used session"), sessions_start ), 1 );
  sessions_start->insert( rb3=new TQRadioButton( i18n("&Manually choose a session"), sessions_start ), 2 );

  config->setGroup("General");
  TQString sesStart (config->readEntry ("Startup Session", "manual"));
  if (sesStart == "new")
    sessions_start->setButton (0);
  else if (sesStart == "last")
    sessions_start->setButton (1);
  else
    sessions_start->setButton (2);

  connect(rb1, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));
  connect(rb2, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));
  connect(rb3, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));

  sessions_exit = new TQButtonGroup( 1, TQt::Horizontal, i18n("Behavior on Application Exit or Session Switch"), frSessions );
  lo->add (sessions_exit);

  sessions_exit->setRadioButtonExclusive( true );
  sessions_exit->insert( rb1=new TQRadioButton( i18n("&Do not save session"), sessions_exit ), 0 );
  sessions_exit->insert( rb2=new TQRadioButton( i18n("&Save session"), sessions_exit ), 1 );
  sessions_exit->insert( rb3=new TQRadioButton( i18n("&Ask user"), sessions_exit ), 2 );

  config->setGroup("General");
  TQString sesExit (config->readEntry ("Session Exit", "save"));
  if (sesExit == "discard")
    sessions_exit->setButton (0);
  else if (sesExit == "save")
    sessions_exit->setButton (1);
  else
    sessions_exit->setButton (2);

  connect(rb1, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));
  connect(rb2, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));
  connect(rb3, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotChanged()));

  lo->addStretch(1); // :-] works correct without autoadd
  //END Session page

  path.clear();

  // file selector page
  path << i18n("Application") << i18n("File Selector");

  TQVBox *page = addVBoxPage( path, i18n("File Selector Settings"),
                              BarIcon("document-open", TDEIcon::SizeSmall) );
  fileSelConfigPage = new KFSConfigPage( page, "file selector config page",
                                         mainWindow->fileselector );
  connect( fileSelConfigPage, TQ_SIGNAL( changed() ), this, TQ_SLOT( slotChanged() ) );
  path.clear();

  path << i18n("Application") << i18n("Document List");
  page = addVBoxPage( path, i18n("Document List Settings"),
  BarIcon("view_text", TDEIcon::SizeSmall) );
  filelistConfigPage = new KFLConfigPage( page, "file list config page",
					  mainWindow->filelist );
  connect( filelistConfigPage, TQ_SIGNAL( changed() ), this, TQ_SLOT( slotChanged() ) );
  path.clear();

  path << i18n("Application") << i18n("Plugins");
  /*TQVBox **/page=addVBoxPage(path,i18n("Plugin Manager"),
                          BarIcon("connect_established",TDEIcon::SizeSmall));
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, TQ_SIGNAL( changed() ), this, TQ_SLOT( slotChanged() ) );

  // Tools->External Tools menu
  path.clear();
  path << i18n("Application") << i18n("External Tools");
  page = addVBoxPage( path, i18n("External Tools"),
      BarIcon("configure", TDEIcon::SizeSmall) );
  configExternalToolsPage = new KateExternalToolsConfigWidget(page, "external tools config page");
  connect( configExternalToolsPage, TQ_SIGNAL(changed()), this, TQ_SLOT(slotChanged()) );

  // editor widgets from kwrite/kwdialog
  path.clear();
  path << i18n("Editor");
  setFolderIcon (path, SmallIcon("edit", TDEIcon::SizeSmall));

  for (uint i = 0; i < KTextEditor::configInterfaceExtension (v->document())->configPages (); i++)
  {
    path.clear();
    path << i18n("Editor") << KTextEditor::configInterfaceExtension (v->document())->configPageName (i);
    /*TQVBox **/page = addVBoxPage(path, KTextEditor::configInterfaceExtension (v->document())->configPageFullName (i),
                              KTextEditor::configInterfaceExtension (v->document())->configPagePixmap(i, TDEIcon::SizeSmall) );

    KTextEditor::ConfigPage *cPage = KTextEditor::configInterfaceExtension (v->document())->configPage(i, page);
    connect( cPage, TQ_SIGNAL( changed() ), this, TQ_SLOT( slotChanged() ) );
    editorPages.append (cPage);
  }

  KatePluginList &pluginList (KatePluginManager::self()->pluginList());
  for (unsigned int i=0; i < pluginList.size(); ++i)
  {
    if  ( pluginList[i].load
          && Kate::pluginConfigInterfaceExtension(pluginList[i].plugin) )
      addPluginPage (pluginList[i].plugin);
  }

  enableButtonSeparator(true);
  dataChanged = false;
  unfoldTreeList ();
}

KateConfigDialog::~KateConfigDialog()
{
}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigInterfaceExtension(plugin))
    return;

  for (uint i=0; i<Kate::pluginConfigInterfaceExtension(plugin)->configPages(); i++)
  {
    TQStringList path;
    path.clear();
    path << i18n("Application")<<i18n("Plugins") << Kate::pluginConfigInterfaceExtension(plugin)->configPageName(i);
    TQVBox *page=addVBoxPage(path, Kate::pluginConfigInterfaceExtension(plugin)->configPageFullName(i), Kate::pluginConfigInterfaceExtension(plugin)->configPagePixmap(i, TDEIcon::SizeSmall));

    PluginPageListItem *info=new PluginPageListItem;
    info->plugin = plugin;
    info->page = Kate::pluginConfigInterfaceExtension(plugin)->configPage (i, page);
    connect( info->page, TQ_SIGNAL( changed() ), this, TQ_SLOT( slotChanged() ) );
    pluginPages.append(info);
  }
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
   if (!Kate::pluginConfigInterfaceExtension(plugin))
    return;

  for (uint i=0; i<pluginPages.count(); i++)
  {
    if  ( pluginPages.at(i)->plugin == plugin )
    {
      TQWidget *w = pluginPages.at(i)->page->parentWidget();
      delete pluginPages.at(i)->page;
      delete w;
      pluginPages.remove(pluginPages.at(i));
      i--;
    }
  }
}

void KateConfigDialog::slotOk()
{
  slotApply();
  accept();
}

void KateConfigDialog::slotApply()
{
  TDEConfig *config = KateApp::self()->config();

  // if data changed apply the kate app stuff
  if( dataChanged )
  {
    config->setGroup("General");

    config->writeEntry("Restore Window Configuration", cb_restoreVC->isChecked());

    int bu = sessions_start->id (sessions_start->selected());

    if (bu == 0)
      config->writeEntry ("Startup Session", "new");
    else if (bu == 1)
      config->writeEntry ("Startup Session", "last");
    else
      config->writeEntry ("Startup Session", "manual");

    bu = sessions_exit->id (sessions_exit->selected());

    if (bu == 0)
      config->writeEntry ("Session Exit", "discard");
    else if (bu == 1)
      config->writeEntry ("Session Exit", "save");
    else
      config->writeEntry ("Session Exit", "ask");

    config->writeEntry("Save Meta Infos", cb_saveMetaInfos->isChecked());
    KateDocManager::self()->setSaveMetaInfos(cb_saveMetaInfos->isChecked());

    config->writeEntry("Days Meta Infos", sb_daysMetaInfos->value() );
    KateDocManager::self()->setDaysMetaInfos(sb_daysMetaInfos->value());

    config->writeEntry("Modified Notification", cb_modNotifications->isChecked());
    mainWindow->modNotification = cb_modNotifications->isChecked();

    mainWindow->showSessionName = cb_showSessionName->isChecked();
    mainWindow->syncKonsole = cb_syncKonsole->isChecked();
    mainWindow->useInstance = cb_useInstance->isChecked();
    mainWindow->filelist->setSortType(cb_sortFiles->isChecked() ? KateFileList::sortByName : KateFileList::sortByID);

    config->writeEntry( "Number of recent files", sb_numRecentFiles->value() );
    mainWindow->fileOpenRecent->setMaxItems( sb_numRecentFiles->value() );

    fileSelConfigPage->apply();

    filelistConfigPage->apply();

    configExternalToolsPage->apply();
    KateExternalToolsCommand::self()->reload();
    for (uint i=0; i < KateApp::self()->mainWindows(); i++)
    {
      KateMainWindow *win = KateApp::self()->mainWindow (i);
      win->externalTools->reload();
    }
    //mainWindow->externalTools->reload();

    mainWindow->viewManager()->setShowFullPath(cb_fullPath->isChecked());

    mainWindow->saveOptions ();

    // save plugin config !!
    KateApp::self()->pluginManager()->writeConfig ();
  }

  //
  // editor config ! (the apply() methode will check the changed state internally)
  //
  for (uint i=0; i<editorPages.count(); i++)
  {
    editorPages.at(i)->apply();
  }

  v->getDoc()->writeConfig(config);

  //
  // plugins config ! (the apply() methode SHOULD check the changed state internally)
  //
  for (uint i=0; i<pluginPages.count(); i++)
  {
    pluginPages.at(i)->page->apply();
  }

  config->sync();

  dataChanged = false;
  actionButton( KDialogBase::Apply)->setEnabled( false );
}

void KateConfigDialog::slotChanged()
{
  dataChanged = true;
  actionButton( KDialogBase::Apply)->setEnabled( true );
}
