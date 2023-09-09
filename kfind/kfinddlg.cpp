/***********************************************************************
 *
 *  Kfinddlg.cpp
 *
 **********************************************************************/

#include <tqlayout.h>
#include <tqpushbutton.h>

#include <tdelocale.h>
#include <tdeglobal.h>
#include <kguiitem.h>
#include <kstatusbar.h>
#include <tdemessagebox.h>
#include <kdebug.h>
#include <tdeaboutapplication.h>
#include <kstandarddirs.h>

#include "kftabdlg.h"
#include "kquery.h"
#include "kfwin.h"

#include "kfinddlg.h"
#include "kfinddlg.moc"

KfindDlg::KfindDlg(const KURL & url, TQWidget *parent, const char *name)
  : KDialogBase( Plain, TQString::null,
	User1 | User2 | Apply | Close | Help, Apply,
        parent, name, true, false,
	KGuiItem(i18n("Stop"), "process-stop"),
	KStdGuiItem::saveAs())
{
  TQWidget::setCaption( i18n("Find Files/Folders" ) );
  setButtonBoxOrientation(Qt::Vertical);

  enableButton(Apply, true); // Enable "Find"
  enableButton(User1, false); // Disable "Stop"
  enableButton(User2, false); // Disable "Save As..."

  setButtonApply(KGuiItem(i18n("&Find"), "edit-find"));

  isResultReported = false;

  TQFrame *frame = plainPage();

  // create tabwidget
  tabWidget = new KfindTabWidget( frame, "dialog");
  tabWidget->setURL( url );

  // prepare window for find results
  win = new KfindWindow(frame,"window");

  mStatusBar = new KStatusBar(frame);
  mStatusBar->insertFixedItem(i18n("AMiddleLengthText…"), 0, true);
  setStatusMsg(i18n("Ready."));
  mStatusBar->setItemAlignment(0, AlignLeft | AlignVCenter);
  mStatusBar->insertItem(TQString::null, 1, 1, true);
  mStatusBar->setItemAlignment(1, AlignLeft | AlignVCenter);

  TQVBoxLayout *vBox = new TQVBoxLayout(frame);
  vBox->addWidget(tabWidget, 0);
  vBox->addWidget(win, 1);
  vBox->addWidget(mStatusBar, 0);

  connect(this, TQT_SIGNAL(applyClicked()),
	  this, TQT_SLOT(startSearch()));
  connect(this, TQT_SIGNAL(user1Clicked()),
	  this, TQT_SLOT(stopSearch()));
  connect(this, TQT_SIGNAL(user2Clicked()),
	  win, TQT_SLOT(saveResults()));

  connect(win ,TQT_SIGNAL(resultSelected(bool)),
	  this,TQT_SIGNAL(resultSelected(bool)));

  query = new KQuery(TQT_TQOBJECT(frame));
  connect(query, TQT_SIGNAL(addFile(const KFileItem*,const TQString&)),
	  TQT_SLOT(addFile(const KFileItem*,const TQString&)));
  connect(query, TQT_SIGNAL(result(int)), TQT_SLOT(slotResult(int)));

  dirwatch=NULL;
}

KfindDlg::~KfindDlg()
{
   stopSearch();
}

void KfindDlg::closeEvent(TQCloseEvent *)
{
   stopSearch();
   slotClose();
}

void KfindDlg::setProgressMsg(const TQString &msg)
{
   mStatusBar->changeItem(msg, 1);
}

void KfindDlg::setStatusMsg(const TQString &msg)
{
   mStatusBar->changeItem(msg, 0);
}


void KfindDlg::startSearch()
{
  tabWidget->setQuery(query);

  isResultReported = false;

  // Reset count - use the same i18n as below
  setProgressMsg(i18n("one file found", "%n files found", 0));

  emit resultSelected(false);
  emit haveResults(false);

  enableButton(Apply, false); // Disable "Find"
  enableButton(User1, true); // Enable "Stop"
  enableButton(User2, false); // Disable "Save As..."

  if(dirwatch!=NULL)
    delete dirwatch;
  dirwatch=new KDirWatch();
  connect(dirwatch, TQT_SIGNAL(created(const TQString&)), this, TQT_SLOT(slotNewItems(const TQString&)));
  connect(dirwatch, TQT_SIGNAL(deleted(const TQString&)), this, TQT_SLOT(slotDeleteItem(const TQString&)));
  dirwatch->addDir(query->url().path(),true);

#if 0
  // waba: Watching for updates is disabled for now because even with FAM it causes too
  // much problems. See BR68220, BR77854, BR77846, BR79512 and BR85802
  // There are 3 problems:
  // 1) addDir() keeps looping on recursive symlinks
  // 2) addDir() scans all subdirectories, so it basically does the same as the process that
  // is started by KQuery but in-process, undoing the advantages of using a seperate find process
  // A solution could be to let KQuery emit all the directories it has searched in.
  // Either way, putting dirwatchers on a whole file system is probably just too much.
  // 3) FAM has a tendency to deadlock with so many files (See BR77854) This has hopefully
  // been fixed in KDirWatch, but that has not yet been confirmed.

  //Getting a list of all subdirs
  if(tabWidget->isSearchRecursive() && (dirwatch->internalMethod() == KDirWatch::FAM))
  {
    TQStringList subdirs=getAllSubdirs(query->url().path());
    for(TQStringList::Iterator it = subdirs.begin(); it != subdirs.end(); ++it)
      dirwatch->addDir(*it,true);
  }
#endif

  win->beginSearch(query->url());
  tabWidget->beginSearch();

  setStatusMsg(i18n("Searching…"));
  query->start();
}

void KfindDlg::stopSearch()
{
  query->kill();
}

void KfindDlg::newSearch()
{
  // WABA: Not used any longer?
  stopSearch();

  tabWidget->setDefaults();

  emit haveResults(false);
  emit resultSelected(false);

  setFocus();
}

void KfindDlg::slotResult(int errorCode)
{
  if (errorCode == 0)
    setStatusMsg(i18n("Ready."));
  else if (errorCode == TDEIO::ERR_USER_CANCELED)
    setStatusMsg(i18n("Aborted."));
  else if (errorCode == TDEIO::ERR_MALFORMED_URL)
  {
     setStatusMsg(i18n("Error."));
     KMessageBox::sorry( this, i18n("Please specify an absolute path in the \"Look in\" box."));
  }
  else if (errorCode == TDEIO::ERR_DOES_NOT_EXIST)
  {
     setStatusMsg(i18n("Error."));
     KMessageBox::sorry( this, i18n("Could not find the specified folder."));
  }
  else
  {
     kdDebug()<<"TDEIO error code: "<<errorCode<<endl;
     setStatusMsg(i18n("Error."));
  };

  enableButton(Apply, true); // Enable "Find"
  enableButton(User1, false); // Disable "Stop"
  enableButton(User2, true); // Enable "Save As..."

  win->endSearch();
  tabWidget->endSearch();
  setFocus();

}

void KfindDlg::addFile(const KFileItem* item, const TQString& matchingLine)
{
  win->insertItem(*item,matchingLine);

  if (!isResultReported)
  {
    emit haveResults(true);
    isResultReported = true;
  }

  int count = win->childCount();
  TQString str = i18n("one file found", "%n files found", count);
  setProgressMsg(str);
}

void KfindDlg::setFocus()
{
  tabWidget->setFocus();
}

void KfindDlg::copySelection()
{
  win->copySelection();
}

void  KfindDlg::about ()
{
  TDEAboutApplication dlg(this, "about", true);
  dlg.exec ();
}

void KfindDlg::slotDeleteItem(const TQString& file)
{
  kdDebug()<<TQString(TQString("Will remove one item: %1").arg(file))<<endl;
  TQListViewItem *iter;
  TQString iterwithpath;

  iter=win->firstChild();
  while( iter ) {
    iterwithpath=query->url().path(+1)+iter->text(1)+iter->text(0);

    if(iterwithpath==file)
    {
      win->takeItem(iter);
      break;
    }
    iter = iter->nextSibling();
  }
}

void KfindDlg::slotNewItems( const TQString& file )
{
  kdDebug()<<TQString("Will add this item")<<endl;
  TQStringList newfiles;
  TQListViewItem *checkiter;
  TQString checkiterwithpath;

  if(file.find(query->url().path(+1))==0)
  {
    kdDebug()<<TQString("Can be added, path OK")<<endl;
    checkiter=win->firstChild();
    while( checkiter ) {
      checkiterwithpath=query->url().path(+1)+checkiter->text(1)+checkiter->text(0);
      if(file==checkiterwithpath)
        return;
      checkiter = checkiter->nextSibling();
    }
    query->slotListEntries(TQStringList(file));
  }
}

TQStringList KfindDlg::getAllSubdirs(TQDir d)
{
  TQStringList dirs;
  TQStringList subdirs;

  d.setFilter( TQDir::Dirs );
  dirs = d.entryList();

  for(TQStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it)
  {
    if((*it==".")||(*it==".."))
      continue;
    subdirs.append(d.path()+"/"+*it);
    subdirs+=getAllSubdirs(d.path()+"/"+*it);
  }
  return subdirs;
}
