/***********************************************************************
 *
 *  Kfwin.cpp
 *
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#include <tqtextstream.h>
#include <tqfileinfo.h>
#include <tqdir.h>
#include <tqclipboard.h>
#include <tqpixmap.h>
#include <tqdragobject.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kapplication.h>
#include <krun.h>
#include <kprocess.h>
#include <kpropertiesdialog.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kglobal.h>
#include <kpopupmenu.h>
#include <kio/netaccess.h>
#include <kurldrag.h>
#include <tqptrlist.h>
#include <kdebug.h>
#include <kiconloader.h>

#include "kfwin.h"

#include "kfwin.moc"

template class TQPtrList<KfFileLVI>;

// Permission strings
static const char* perm[4] = {
  I18N_NOOP( "Read-write" ),
  I18N_NOOP( "Read-only" ),
  I18N_NOOP( "Write-only" ),
  I18N_NOOP( "Inaccessible" ) };
#define RW 0
#define RO 1
#define WO 2
#define NA 3

KfFileLVI::KfFileLVI(KfindWindow* lv, const KFileItem &item, const TQString& matchingLine)
  : TQListViewItem(lv),
    fileitem(item)
{
  fileInfo = new TQFileInfo(item.url().path());

  TQString size = KGlobal::locale()->formatNumber(item.size(), 0);

  TQDateTime dt;
  dt.setTime_t(item.time(KIO::UDS_MODIFICATION_TIME));
  TQString date = KGlobal::locale()->formatDateTime(dt);

  int perm_index;
  if(fileInfo->isReadable())
    perm_index = fileInfo->isWritable() ? RW : RO;
  else
    perm_index = fileInfo->isWritable() ? WO : NA;

  // Fill the item with data
  setText(0, item.url().fileName(false));
  setText(1, lv->reducedDir(item.url().directory(false)));
  setText(2, size);
  setText(3, date);
  setText(4, i18n(perm[perm_index]));
  setText(5, matchingLine);

  // put the icon into the leftmost column
  setPixmap(0, item.pixmap(16));
}

KfFileLVI::~KfFileLVI()
{
  delete fileInfo;
}

TQString KfFileLVI::key(int column, bool) const
{
  switch (column) {
  case 2:
    // Returns size in bytes. Used for sorting
    return TQString().sprintf("%010d", fileInfo->size());
  case 3:
    // Returns time in secs from 1/1/1970. Used for sorting
    return TQString().sprintf("%010ld", fileitem.time(KIO::UDS_MODIFICATION_TIME));
  }

  return text(column);
}

KfindWindow::KfindWindow( TQWidget *parent, const char *name )
  : KListView( parent, name )
,m_baseDir("")
,m_menu(0)
{
  setSelectionMode( TQListView::Extended );
  setShowSortIndicator( TRUE );

  addColumn(i18n("Name"));
  addColumn(i18n("In Subfolder"));
  addColumn(i18n("Size"));
  setColumnAlignment(2, AlignRight);
  addColumn(i18n("Modified"));
  setColumnAlignment(3, AlignRight);
  addColumn(i18n("Permissions"));
  setColumnAlignment(4, AlignRight);

  addColumn(i18n("First Matching Line"));
  setColumnAlignment(5, AlignLeft);

  // Disable autoresize for all columns
  // Resizing is done by resetColumns() function
  for (int i = 0; i < 6; i++)
    setColumnWidthMode(i, Manual);

  resetColumns(true);

  connect( this, TQT_SIGNAL(selectionChanged()),
	   this, TQT_SLOT( selectionHasChanged() ));

  connect(this, TQT_SIGNAL(contextMenu(KListView *, TQListViewItem*,const TQPoint&)),
	  this, TQT_SLOT(slotContextMenu(KListView *,TQListViewItem*,const TQPoint&)));

  connect(this, TQT_SIGNAL(executed(TQListViewItem*)),
	  this, TQT_SLOT(slotExecute(TQListViewItem*)));
  setDragEnabled(true);

}


TQString KfindWindow::reducedDir(const TQString& fullDir)
{
   if (fullDir.find(m_baseDir)==0)
   {
      TQString tmp=fullDir.mid(m_baseDir.length());
      return tmp;
   };
   return fullDir;
}

void KfindWindow::beginSearch(const KURL& baseUrl)
{
  kdDebug()<<TQString(TQString("beginSearch in: %1").arg(baseUrl.path()))<<endl;
  m_baseDir=baseUrl.path(+1);
  haveSelection = false;
  clear();
}

void KfindWindow::endSearch()
{
}

void KfindWindow::insertItem(const KFileItem &item, const TQString& matchingLine)
{
  new KfFileLVI(this, item, matchingLine);
}

// copy to clipboard aka X11 selection
void KfindWindow::copySelection()
{
  TQDragObject *drag_obj = dragObject();

  if (drag_obj)
  {
    TQClipboard *cb = kapp->tqclipboard();
    cb->setData(drag_obj);
  }
}

void KfindWindow::saveResults()
{
  TQListViewItem *item;

  KFileDialog *dlg = new KFileDialog(TQString::null, TQString::null, this,
	"filedialog", true);
  dlg->setOperationMode (KFileDialog::Saving);

  dlg->setCaption(i18n("Save Results As"));

  TQStringList list;

  list << "text/plain" << "text/html";

  dlg->setOperationMode(KFileDialog::Saving);
  
  dlg->setMimeFilter(list, TQString("text/plain"));

  dlg->exec();

  KURL u = dlg->selectedURL();
  KMimeType::Ptr mimeType = dlg->currentFilterMimeType();
  delete dlg;

  if (!u.isValid() || !u.isLocalFile())
     return;

  TQString filename = u.path();

  TQFile file(filename);

  if ( !file.open(IO_WriteOnly) )
    KMessageBox::error(parentWidget(),
		       i18n("Unable to save results."));
  else {
    TQTextStream stream( &file );
    stream.setEncoding( TQTextStream::Locale );

    if ( mimeType->name() == "text/html") {
      stream << TQString::fromLatin1("<HTML><HEAD>\n"
				    "<!DOCTYPE %1>\n"
				    "<TITLE>%2</TITLE></HEAD>\n"
				    "<BODY><H1>%3</H1>"
				    "<DL><p>\n")
	.arg(i18n("KFind Results File"))
	.arg(i18n("KFind Results File"))
	.arg(i18n("KFind Results File"));

      item = firstChild();
      while(item != NULL)
	{
	  TQString path=((KfFileLVI*)item)->fileitem.url().url();
	  TQString pretty=((KfFileLVI*)item)->fileitem.url().htmlURL();
	  stream << TQString::fromLatin1("<DT><A HREF=\"") << path
		 << TQString::fromLatin1("\">") << pretty
		 << TQString::fromLatin1("</A>\n");

	  item = item->nextSibling();
	}
      stream << TQString::fromLatin1("</DL><P></BODY></HTML>\n");
    }
    else {
      item = firstChild();
      while(item != NULL)
      {
	TQString path=((KfFileLVI*)item)->fileitem.url().url();
	stream << path << endl;
	item = item->nextSibling();
      }
    }

    file.close();
    KMessageBox::information(parentWidget(),
			     i18n("Results were saved to file\n")+
			     filename);
  }
}

// This function is called when selection is changed (both selected/deselected)
// It notifies the parent about selection status and enables/disables menubar
void KfindWindow::selectionHasChanged()
{
  emit resultSelected(true);

  TQListViewItem *item = firstChild();
  while(item != 0L)
  {
    if(isSelected(item)) {
      emit resultSelected( true );
      haveSelection = true;
      return;
    }

    item = item->nextSibling();
  }

  haveSelection = false;
  emit resultSelected(false);
}

void KfindWindow::deleteFiles()
{
  TQString tmp = i18n("Do you really want to delete the selected file?",
                     "Do you really want to delete the %n selected files?",selectedItems().count());
  if (KMessageBox::warningContinueCancel(parentWidget(), tmp, "", KGuiItem( i18n("&Delete"), "editdelete")) == KMessageBox::Cancel)
    return;

  // Iterate on all selected elements
  TQPtrList<TQListViewItem> selected = selectedItems();
  for ( uint i = 0; i < selected.count(); i++ ) {
    KfFileLVI *item = (KfFileLVI *) selected.at(i);
    KFileItem file = item->fileitem;

    KIO::NetAccess::del(file.url(), this);
  }
  selected.setAutoDelete(true);
}

void KfindWindow::fileProperties()
{
  // This dialog must be modal because it parent dialog is modal as well.
  // Non-modal property dialog will hide behind the main window
  (void) new KPropertiesDialog( &((KfFileLVI *)currentItem())->fileitem, this,
				"propDialog", true);
}

void KfindWindow::openFolder()
{
  KFileItem fileitem = ((KfFileLVI *)currentItem())->fileitem;
  KURL url = fileitem.url();
  url.setFileName(TQString::null);

  (void) new KRun(url);
}

void KfindWindow::openBinding()
{
  ((KfFileLVI*)currentItem())->fileitem.run();
}

void KfindWindow::slotExecute(TQListViewItem* item)
{
   if (item==0)
      return;
  ((KfFileLVI*)item)->fileitem.run();
}

// Resizes KListView to occupy all visible space
void KfindWindow::resizeEvent(TQResizeEvent *e)
{
  KListView::resizeEvent(e);
  resetColumns(false);
  clipper()->repaint();
}

TQDragObject * KfindWindow::dragObject()
{
  KURL::List uris;
  TQPtrList<TQListViewItem> selected = selectedItems();

  // create a list of URIs from selection
  for ( uint i = 0; i < selected.count(); i++ )
  {
    KfFileLVI *item = (KfFileLVI *) selected.at( i );
    if (item)
    {
      uris.append( item->fileitem.url() );
    }
  }

  if ( uris.count() <= 0 )
     return 0;

  TQUriDrag *ud = new KURLDrag( uris, (TQWidget *) this, "kfind uridrag" );

  const TQPixmap *pix = currentItem()->pixmap(0);
  if ( pix && !pix->isNull() )
    ud->setPixmap( *pix );

  return ud;
}

void KfindWindow::resetColumns(bool init)
{
   TQFontMetrics fm = fontMetrics();
  if (init)
  {
    setColumnWidth(2, QMAX(fm.width(columnText(2)), fm.width("0000000")) + 15);
    TQString sampleDate =
      KGlobal::locale()->formatDateTime(TQDateTime::currentDateTime());
    setColumnWidth(3, QMAX(fm.width(columnText(3)), fm.width(sampleDate)) + 15);
    setColumnWidth(4, QMAX(fm.width(columnText(4)), fm.width(i18n(perm[RO]))) + 15);
    setColumnWidth(5, QMAX(fm.width(columnText(5)), fm.width("some text")) + 15);
  }

  int free_space = visibleWidth() -
    columnWidth(2) - columnWidth(3) - columnWidth(4) - columnWidth(5);

//  int name_w = QMIN((int)(free_space*0.5), 150);
//  int dir_w = free_space - name_w;
  int name_w = QMAX((int)(free_space*0.5), fm.width("some long filename"));
  int dir_w = name_w;

  setColumnWidth(0, name_w);
  setColumnWidth(1, dir_w);
}

void KfindWindow::slotContextMenu(KListView *,TQListViewItem *item,const TQPoint&p)
{
  if (!item) return;
  int count = selectedItems().count();

  if (count == 0)
  {
     return;
  };

  if (m_menu==0)
     m_menu = new KPopupMenu(this);
  else
     m_menu->clear();

  if (count == 1)
  {
     //menu = new KPopupMenu(item->text(0), this);
     m_menu->insertTitle(item->text(0));
     m_menu->insertItem(SmallIcon("fileopen"),i18n("Menu item", "Open"), this, TQT_SLOT(openBinding()));
     m_menu->insertItem(SmallIcon("window_new"),i18n("Open Folder"), this, TQT_SLOT(openFolder()));
     m_menu->insertSeparator();
     m_menu->insertItem(SmallIcon("editcopy"),i18n("Copy"), this, TQT_SLOT(copySelection()));
     m_menu->insertItem(SmallIcon("editdelete"),i18n("Delete"), this, TQT_SLOT(deleteFiles()));
     m_menu->insertSeparator();
     m_menu->insertItem(i18n("Open With..."), this, TQT_SLOT(slotOpenWith()));
     m_menu->insertSeparator();
     m_menu->insertItem(i18n("Properties"), this, TQT_SLOT(fileProperties()));
  }
  else
  {
     m_menu->insertTitle(i18n("Selected Files"));
     m_menu->insertItem(SmallIcon("editcopy"),i18n("Copy"), this, TQT_SLOT(copySelection()));
     m_menu->insertItem(SmallIcon("editdelete"),i18n("Delete"), this, TQT_SLOT(deleteFiles()));
  }
  m_menu->popup(p, 1);
}

void KfindWindow::slotOpenWith()
{
   KRun::displayOpenWithDialog( KURL::split(((KfFileLVI*)currentItem())->fileitem.url()) );
}
