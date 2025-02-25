/**
 *  Copyright (c) 2000 Antonio Larrosa <larrosa@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <config.h>

#include <stdlib.h>
#include <unistd.h>

#include <tqfileinfo.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>

#include <kdebug.h>
#include <tdeapplication.h>
#include <tdestandarddirs.h>
#include <kservice.h>
#include <tdelocale.h>
#include <ksimpleconfig.h>
#undef Unsorted
#include <kipc.h>

#include <tdelistview.h>
#include <kurlrequesterdlg.h>
#include <tdemessagebox.h>
#include <kprogress.h>
#include <kiconloader.h>

#include <tdeio/job.h>
#include <tdeio/netaccess.h>
#include <ktar.h>

#ifdef HAVE_LIBART
#include <ksvgiconengine.h>
#endif

#include "iconthemes.h"

IconThemesConfig::IconThemesConfig(TQWidget *parent, const char *name)
  : TDECModule(parent, name)
{
  TQVBoxLayout *topLayout = new TQVBoxLayout(this, KDialog::marginHint(),
                                           KDialog::spacingHint());

  TQFrame *m_preview=new TQFrame(this);
  m_preview->setMinimumHeight(50);

  TQHBoxLayout *lh2=new TQHBoxLayout( m_preview );
  m_previewExec=new TQLabel(m_preview);
  m_previewExec->setPixmap(DesktopIcon("application-x-executable"));
  m_previewFolder=new TQLabel(m_preview);
  m_previewFolder->setPixmap(DesktopIcon("folder"));
  m_previewDocument=new TQLabel(m_preview);
  m_previewDocument->setPixmap(DesktopIcon("text-x-generic"));

  lh2->addStretch(10);
  lh2->addWidget(m_previewExec);
  lh2->addStretch(1);
  lh2->addWidget(m_previewFolder);
  lh2->addStretch(1);
  lh2->addWidget(m_previewDocument);
  lh2->addStretch(10);


  m_iconThemes=new TDEListView(this,"IconThemeList");
  m_iconThemes->addColumn(i18n("Name"));
  m_iconThemes->addColumn(i18n("Description"));
  m_iconThemes->setAllColumnsShowFocus( true );
  m_iconThemes->setFullWidth(true);
  connect(m_iconThemes,TQ_SIGNAL(selectionChanged(TQListViewItem *)),
		TQ_SLOT(themeSelected(TQListViewItem *)));

  TQPushButton *installButton=new TQPushButton( i18n("Install New Theme..."),
	this, "InstallNewTheme");
  connect(installButton,TQ_SIGNAL(clicked()),TQ_SLOT(installNewTheme()));
  m_removeButton=new TQPushButton( i18n("Remove Theme"),
	this, "RemoveTheme");
  connect(m_removeButton,TQ_SIGNAL(clicked()),TQ_SLOT(removeSelectedTheme()));

  topLayout->addWidget(
	new TQLabel(i18n("Select the icon theme you want to use:"), this));
  topLayout->addWidget(m_preview);
  topLayout->addWidget(m_iconThemes);
  TQHBoxLayout *lg = new TQHBoxLayout(topLayout, KDialog::spacingHint());
  lg->addWidget(installButton);
  lg->addWidget(m_removeButton);

  loadThemes();

  load();

  m_iconThemes->setFocus();
}

IconThemesConfig::~IconThemesConfig()
{
}

TQListViewItem *IconThemesConfig::iconThemeItem(const TQString &name)
{
  TQListViewItem *item;
  for ( item=m_iconThemes->firstChild(); item ; item=item->nextSibling() )
    if (m_themeNames[item->text(0)]==name) return item;

  return 0L;
}

void IconThemesConfig::loadThemes()
{
  m_iconThemes->clear();
  m_themeNames.clear();
  TQStringList themelist(TDEIconTheme::list());
  TQString name;
  TQString tname;
  TQStringList::Iterator it;
  for (it=themelist.begin(); it != themelist.end(); ++it)
  {
    TDEIconTheme icontheme(*it);
    if (!icontheme.isValid()) kdDebug() << "notvalid\n";
    if (icontheme.isHidden()) continue;

    name=icontheme.name();
    tname=name;

 //  Just in case we have duplicated icon theme names on separate directories
    for (int i=2; m_themeNames.find(tname)!=m_themeNames.end() ; i++)
        tname=TQString("%1-%2").arg(name).arg(i);

    m_iconThemes->insertItem(new TQListViewItem(m_iconThemes,name,
		icontheme.description()));

    m_themeNames.insert(name,*it);

  }
}

void IconThemesConfig::installNewTheme()
{
  KURL themeURL = KURLRequesterDlg::getURL(TQString::null, this,
                                           i18n("Drag or Type Theme URL"));
  kdDebug() << themeURL.prettyURL() << endl;

  if (themeURL.url().isEmpty()) return;

  TQString themeTmpFile;
  // themeTmpFile contains the name of the downloaded file

  if (!TDEIO::NetAccess::download(themeURL, themeTmpFile, this)) {
    TQString sorryText;
    if (themeURL.isLocalFile())
       sorryText = i18n("Unable to find the icon theme archive %1.");
    else
       sorryText = i18n("Unable to download the icon theme archive;\n"
                        "please check that address %1 is correct.");
    KMessageBox::sorry(this, sorryText.arg(themeURL.prettyURL()));
    return;
  }

  TQStringList themesNames = findThemeDirs(themeTmpFile);
  if (themesNames.isEmpty()) {
    TQString invalidArch(i18n("The file is not a valid icon theme archive."));
    KMessageBox::error(this, invalidArch);

    TDEIO::NetAccess::removeTempFile(themeTmpFile);
    return;
  }

  if (!installThemes(themesNames, themeTmpFile)) {
    //FIXME: make me able to know what is wrong....
    // TQStringList instead of bool?
    TQString somethingWrong =
        i18n("A problem occurred during the installation process; "
             "however, most of the themes in the archive have been installed");
    KMessageBox::error(this, somethingWrong);
  }

  TDEIO::NetAccess::removeTempFile(themeTmpFile);

  TDEGlobal::instance()->newIconLoader();
  loadThemes();

  TQListViewItem *item=iconThemeItem(TDEIconTheme::current());
  m_iconThemes->setSelected(item, true);
  updateRemoveButton();
}

bool IconThemesConfig::installThemes(const TQStringList &themes, const TQString &archiveName)
{
  bool everythingOk = true;
  TQString localThemesDir(locateLocal("icon", "./"));

  KProgressDialog progressDiag(this, "themeinstallprogress",
                               i18n("Installing icon themes"),
                               TQString::null,
                               true);
  progressDiag.setAutoClose(true);
  progressDiag.progressBar()->setTotalSteps(themes.count());
  progressDiag.show();

  KTar archive(archiveName);
  archive.open(IO_ReadOnly);
  tdeApp->processEvents();

  const KArchiveDirectory* rootDir = archive.directory();

  KArchiveDirectory* currentTheme;
  for (TQStringList::ConstIterator it = themes.begin();
       it != themes.end();
       ++it) {
    progressDiag.setLabel(
        i18n("<qt>Installing <strong>%1</strong> theme</qt>")
        .arg(*it));
    tdeApp->processEvents();

    if (progressDiag.wasCancelled())
      break;

    currentTheme = dynamic_cast<KArchiveDirectory*>(
                     const_cast<KArchiveEntry*>(
                       rootDir->entry(*it)));
    if (currentTheme == NULL) {
      // we tell back that something went wrong, but try to install as much
      // as possible
      everythingOk = false;
      continue;
    }

    currentTheme->copyTo(localThemesDir + *it);
    progressDiag.progressBar()->advance(1);
  }

  archive.close();
  return everythingOk;
}

TQStringList IconThemesConfig::findThemeDirs(const TQString &archiveName)
{
  TQStringList foundThemes;

  KTar archive(archiveName);
  archive.open(IO_ReadOnly);
  const KArchiveDirectory* themeDir = archive.directory();

  KArchiveEntry* possibleDir = 0L;
  KArchiveDirectory* subDir = 0L;

  // iterate all the dirs looking for an index.theme or index.desktop file
  TQStringList entries = themeDir->entries();
  for (TQStringList::Iterator it = entries.begin();
       it != entries.end();
       ++it) {
    possibleDir = const_cast<KArchiveEntry*>(themeDir->entry(*it));
    if (possibleDir->isDirectory()) {
      subDir = dynamic_cast<KArchiveDirectory*>( possibleDir );
      if (subDir && (subDir->entry("index.theme") != NULL ||
                     subDir->entry("index.desktop") != NULL))
        foundThemes.append(subDir->name());
    }
  }

  archive.close();
  return foundThemes;
}

void IconThemesConfig::removeSelectedTheme()
{
  TQListViewItem *selected = m_iconThemes->selectedItem();
  if (!selected)
     return;

  TQString question=i18n("<qt>Are you sure you want to remove the "
        "<strong>%1</strong> icon theme?<br>"
        "<br>"
        "This will delete the files installed by this theme.</qt>").
	arg(selected->text(0));

  bool deletingCurrentTheme=(selected==iconThemeItem(TDEIconTheme::current()));

  int r=KMessageBox::warningContinueCancel(this,question,i18n("Confirmation"),KStdGuiItem::del());
  if (r!=KMessageBox::Continue) return;

  TDEIconTheme icontheme(m_themeNames[selected->text(0)]);

  // delete the index file before the async TDEIO::del so loadThemes() will
  // ignore that dir.
  unlink(TQFile::encodeName(icontheme.dir()+"/index.theme").data());
  unlink(TQFile::encodeName(icontheme.dir()+"/index.desktop").data());
  TDEIO::del(KURL( icontheme.dir() ));

  TDEGlobal::instance()->newIconLoader();

  loadThemes();

  TQListViewItem *item=0L;
  //Fallback to the default if we've deleted the current theme
  if (!deletingCurrentTheme)
     item=iconThemeItem(TDEIconTheme::current());
  if (!item)
     item=iconThemeItem(TDEIconTheme::defaultThemeName());

  m_iconThemes->setSelected(item, true);
  updateRemoveButton();

  if (deletingCurrentTheme) // Change the configuration
    save();
}

void IconThemesConfig::updateRemoveButton()
{
  TQListViewItem *selected = m_iconThemes->selectedItem();
  bool enabled = false;
  if (selected)
  {
    TDEIconTheme icontheme(m_themeNames[selected->text(0)]);
    TQFileInfo fi(icontheme.dir());
    enabled = fi.isWritable();
    // Don't let users remove the current theme.
    if(m_themeNames[selected->text(0)] == TDEIconTheme::current() || 
			 m_themeNames[selected->text(0)] == TDEIconTheme::defaultThemeName())
      enabled = false;
  }
  m_removeButton->setEnabled(enabled);
}

void IconThemesConfig::themeSelected(TQListViewItem *item)
{
#ifdef HAVE_LIBART
  KSVGIconEngine engine;
#endif 
  TQString dirName(m_themeNames[item->text(0)]);
  TDEIconTheme icontheme(dirName);
  if (!icontheme.isValid()) kdDebug() << "notvalid\n";

  updateRemoveButton();
  const int size = icontheme.defaultSize(TDEIcon::Desktop);

  TDEIcon icon=icontheme.iconPath("exec.png", size, TDEIcon::MatchBest);
  if (!icon.isValid()) {
#ifdef HAVE_LIBART
	  icon=icontheme.iconPath("exec.svg", size, TDEIcon::MatchBest);
	  if(engine.load(size, size, icon.path))
              m_previewExec->setPixmap(*engine.image());
          else {
              icon=icontheme.iconPath("exec.svgz", size, TDEIcon::MatchBest);
              if(engine.load(size, size, icon.path))
                  m_previewExec->setPixmap(*engine.image());
          }
#endif
  }
  else
          m_previewExec->setPixmap(TQPixmap(icon.path));

  icon=icontheme.iconPath("folder.png",size,TDEIcon::MatchBest);
  if (!icon.isValid()) {
#ifdef HAVE_LIBART
	  icon=icontheme.iconPath("folder.svg", size, TDEIcon::MatchBest);
	  if(engine.load(size, size, icon.path))
              m_previewFolder->setPixmap(*engine.image());
          else {
              icon=icontheme.iconPath("folder.svgz", size, TDEIcon::MatchBest);
              if(engine.load(size, size, icon.path))
                  m_previewFolder->setPixmap(*engine.image());
          }
#endif
  }
  else
  	  m_previewFolder->setPixmap(TQPixmap(icon.path));

  icon=icontheme.iconPath("txt.png",size,TDEIcon::MatchBest);
  if (!icon.isValid()) {
#ifdef HAVE_LIBART
	  icon=icontheme.iconPath("txt.svg", size, TDEIcon::MatchBest);
	  if(engine.load(size, size, icon.path))
              m_previewDocument->setPixmap(*engine.image());
          else {
              icon=icontheme.iconPath("txt.svgz", size, TDEIcon::MatchBest);
              if(engine.load(size, size, icon.path))
                  m_previewDocument->setPixmap(*engine.image());
          }
#endif
  }
  else  
	  m_previewDocument->setPixmap(TQPixmap(icon.path));
  
  emit changed(true);
  m_bChanged = true;
}

void IconThemesConfig::load()
{
  m_defaultTheme=iconThemeItem(TDEIconTheme::current());
  m_iconThemes->setSelected(m_defaultTheme, true);
  updateRemoveButton();

  emit changed(false);
  m_bChanged = false;
}

void IconThemesConfig::save()
{
  if (!m_bChanged)
     return;
  TQListViewItem *selected = m_iconThemes->selectedItem();
  if (!selected)
     return;

  KSimpleConfig *config = new KSimpleConfig("kdeglobals", false);
  config->setGroup("Icons");
  config->writeEntry("Theme", m_themeNames[selected->text(0)]);
  delete config;

  TDEIconTheme::reconfigure();
  emit changed(false);

  for (int i=0; i<TDEIcon::LastGroup; i++)
  {
    KIPC::sendMessageAll(KIPC::IconChanged, i);
  }

  KService::rebuildKSycoca(this);

  m_bChanged = false;
  m_removeButton->setEnabled(false);
}

void IconThemesConfig::defaults()
{
  if (m_iconThemes->currentItem()==m_defaultTheme) return;

  m_iconThemes->setSelected(m_defaultTheme, true);
  updateRemoveButton();

  emit changed(true);
  m_bChanged = true;
}

#include "iconthemes.moc"
