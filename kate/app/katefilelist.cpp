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

//BEGIN Includes
#include "katefilelist.h"
#include "katefilelist.moc"

#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "katemainwindow.h"

#include <tqapplication.h>
#include <tqpainter.h>
#include <tqpopupmenu.h>
#include <tqheader.h>
#include <tqcolor.h>
#include <tqcheckbox.h>
#include <tqhbox.h>
#include <tqlayout.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqwhatsthis.h>

#include <kiconloader.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kpassivepopup.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kstringhandler.h>
#include <kcolorbutton.h>
#include <kdialog.h>
//END Includes

//BEGIN ToolTip
class ToolTip : public TQToolTip
{
  public:
    ToolTip( TQWidget *parent, KateFileList *lv )
      : TQToolTip( parent ),
    m_listView( lv )
    {
    }
    virtual ~ToolTip() {};

    void maybeTip( const TQPoint &pos )
    {
      TQListViewItem *i = m_listView->itemAt( pos );
      if ( ! i ) return;

      KateFileListItem *item = ((KateFileListItem*)i);
      if ( ! item ) return;

      tip( m_listView->itemRect( i ), m_listView->tooltip( item, 0 ) );

    }

  private:
    KateFileList *m_listView;
};

//END ToolTip

//BEGIN KateFileList
KateFileList::KateFileList (KateMainWindow *main,
                            KateViewManager *_viewManager,
                            TQWidget * parent, const char * name )
    :  KListView (parent, name)
    , m_sort( KateFileList::sortByID )
{
  m_main = main;
  m_tooltip = new ToolTip( viewport(), this );

  // default colors
  m_viewShade = TQColor( 51, 204, 255 );
  m_editShade = TQColor( 255, 102, 153 );
  m_enableBgShading = false;

  setFocusPolicy ( TQ_NoFocus  );

  viewManager = _viewManager;

  header()->hide();
  addColumn("Document Name");

  setSelectionMode( TQListView::Single );
  setSorting( 0, true );
  setShowToolTips( false );

  setupActions ();

  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
  {
    slotDocumentCreated (KateDocManager::self()->document(i));
    slotModChanged (KateDocManager::self()->document(i));
  }

  connect(KateDocManager::self(),TQT_SIGNAL(documentCreated(Kate::Document *)),
	  this,TQT_SLOT(slotDocumentCreated(Kate::Document *)));
  connect(KateDocManager::self(),TQT_SIGNAL(documentDeleted(uint)),
	  this,TQT_SLOT(slotDocumentDeleted(uint)));

  // don't Honour KDE single/double click setting, this files are already open,
  // no need for hassle of considering double-click
  connect(this,TQT_SIGNAL(selectionChanged(TQListViewItem *)),
	  this,TQT_SLOT(slotActivateView(TQListViewItem *)));
  connect(viewManager,TQT_SIGNAL(viewChanged()), this,TQT_SLOT(slotViewChanged()));
  connect(this,TQT_SIGNAL(contextMenuRequested( TQListViewItem *, const TQPoint &, int )),
	  this,TQT_SLOT(slotMenu ( TQListViewItem *, const TQPoint &, int )));
}

KateFileList::~KateFileList ()
{
  delete m_tooltip;
}

void KateFileList::setupActions ()
{
  windowNext = KStdAction::back(TQT_TQOBJECT(this), TQT_SLOT(slotPrevDocument()), m_main->actionCollection());
  windowPrev = KStdAction::forward(TQT_TQOBJECT(this), TQT_SLOT(slotNextDocument()), m_main->actionCollection());
  sortAction = new KSelectAction( i18n("Sort &By"), 0,
      m_main->actionCollection(), "filelist_sortby"  );
  listMoveFileUp = new KAction( i18n("Move File Up"), 0, m_main->actionCollection(), "filelist_move_up" );
  listMoveFileUp->setShortcut(KShortcut(CTRL + SHIFT + Key_Comma));
  listMoveFileDown = new KAction( i18n("Move File Down"), 0, m_main->actionCollection(), "filelist_move_down" );
  listMoveFileDown->setShortcut(KShortcut(CTRL + SHIFT + Key_Period));
  connect( listMoveFileUp, TQT_SIGNAL(activated()), TQT_TQOBJECT(this), TQT_SLOT(moveFileUp()) );
  connect( listMoveFileDown, TQT_SIGNAL(activated()), TQT_TQOBJECT(this), TQT_SLOT(moveFileDown()) );
  TQStringList l;
  l << i18n("Opening Order") << i18n("Document Name") << i18n("URL") << i18n("Manual Placement");
  sortAction->setItems( l );
  connect( sortAction, TQT_SIGNAL(activated(int)), TQT_TQOBJECT(this), TQT_SLOT(setSortType(int)) );
}

void KateFileList::updateActions ()
{
  windowNext->setEnabled(KateDocManager::self()->documents()  > 1);
  windowPrev->setEnabled(KateDocManager::self()->documents()  > 1);
}

void KateFileList::keyPressEvent(TQKeyEvent *e) {
  if ( ( e->key() == Key_Return ) || ( e->key() == Key_Enter ) )
  {
    e->accept();
    slotActivateView( currentItem() );
  }
  else
  {
    KListView::keyPressEvent(e);
  }
}

// Protect single mode selection: don't let them
// leftclick outside items.
// ### if we get to accept keyboard navigation, set focus before
// returning
void KateFileList::contentsMousePressEvent( TQMouseEvent *e )
{
  if ( ! itemAt( contentsToViewport( e->pos() ) ) )
  return;

  KListView::contentsMousePressEvent( e );
}

void KateFileList::resizeEvent( TQResizeEvent *e )
{
  KListView::resizeEvent( e );

  // ### We may want to actually calculate the widest field,
  // since it's not automatically scrinked. If I add support for
  // tree or marks, the changes of the required width will vary
  // a lot with opening/closing of files and display changes for
  // the mark branches.
  int w = viewport()->width();
  if ( columnWidth( 0 ) < w )
    setColumnWidth( 0, w );
}

void KateFileList::slotNextDocument()
{
  if ( ! currentItem() || childCount() == 0 )
    return;

  // ### more checking once more item types are added

  if ( currentItem()->nextSibling() )
    viewManager->activateView( ((KateFileListItem*)currentItem()->nextSibling())->documentNumber() );
  else
    viewManager->activateView( ((KateFileListItem *)firstChild())->documentNumber() );
}

void KateFileList::slotPrevDocument()
{
  if ( ! currentItem() || childCount() == 0 )
    return;

  // ### more checking once more item types are added

  if ( currentItem()->itemAbove() )
    viewManager->activateView( ((KateFileListItem*)currentItem()->itemAbove())->documentNumber() );
  else
    viewManager->activateView( ((KateFileListItem *)lastItem())->documentNumber() );
}

void KateFileList::slotDocumentCreated (Kate::Document *doc)
{
  new KateFileListItem( this, doc/*, doc->documentNumber()*/ );
  connect(doc,TQT_SIGNAL(modStateChanged(Kate::Document *)),this,TQT_SLOT(slotModChanged(Kate::Document *)));
  connect(doc,TQT_SIGNAL(nameChanged(Kate::Document *)),this,TQT_SLOT(slotNameChanged(Kate::Document *)));
  connect(doc,TQT_SIGNAL(modifiedOnDisc(Kate::Document *, bool, unsigned char)),this,TQT_SLOT(slotModifiedOnDisc(Kate::Document *, bool, unsigned char)));

  sort();
  updateActions ();
}

void KateFileList::slotDocumentDeleted (uint documentNumber)
{
  TQListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem *)item)->documentNumber() == documentNumber )
    {
//       m_viewHistory.removeRef( (KateFileListItem *)item );
//       m_editHistory.removeRef( (KateFileListItem *)item );

      removeItem( item );

      break;
    }
    item = item->nextSibling();
  }

  updateActions ();
}

void KateFileList::slotActivateView( TQListViewItem *item )
{
  if ( ! item || item->rtti() != RTTI_KateFileListItem )
    return;

  viewManager->activateView( ((KateFileListItem *)item)->documentNumber() );
}

void KateFileList::slotModChanged (Kate::Document *doc)
{
  if (!doc) return;

  TQListViewItem * item = firstChild();
  while( item )
  {
    if ( ((KateFileListItem *)item)->documentNumber() == doc->documentNumber() )
      break;

    item = item->nextSibling();
  }

  if ( ((KateFileListItem *)item)->document()->isModified() )
  {
    m_editHistory.removeRef( (KateFileListItem *)item );
    m_editHistory.prepend( (KateFileListItem *)item );

    for ( uint i=0; i <  m_editHistory.count(); i++ )
    {
      m_editHistory.at( i )->setEditHistPos( i+1 );
      repaintItem(  m_editHistory.at( i ) );
    }
  }
  else
    repaintItem( item );
}

void KateFileList::slotModifiedOnDisc (Kate::Document *doc, bool, unsigned char)
{
  slotModChanged( doc );
}

void KateFileList::slotNameChanged (Kate::Document *doc)
{
  if (!doc) return;

  // ### using nextSibling to *only* look at toplevel items.
  // child items could be marks for example
  TQListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem*)item)->document() == doc )
    {
      item->setText( 0, doc->docName() );
      repaintItem( item );
      break;
    }
    item = item->nextSibling();
  }
  updateSort();
}

void KateFileList::slotViewChanged ()
{
  if (!viewManager->activeView()) return;

  Kate::View *view = viewManager->activeView();
  uint dn = view->getDoc()->documentNumber();

  TQListViewItem * i = firstChild();
  while( i ) {
    if ( ((KateFileListItem *)i)->documentNumber() == dn )
    {
      break;
    }
    i = i->nextSibling();
  }

  if ( ! i )
    return;

  KateFileListItem *item = (KateFileListItem*)i;
  setCurrentItem( item );

  // ### During load of file lists, all the loaded views gets active.
  // Do something to avoid shading them -- maybe not creating views, just
  // open the documents???


//   int p = 0;
//   if (  m_viewHistory.count() )
//   {
//     int p =  m_viewHistory.findRef( item ); // only repaint items that needs it
//   }

  m_viewHistory.removeRef( item );
  m_viewHistory.prepend( item );

  for ( uint i=0; i <  m_viewHistory.count(); i++ )
  {
    m_viewHistory.at( i )->setViewHistPos( i+1 );
    repaintItem(  m_viewHistory.at( i ) );
  }

}

void KateFileList::slotMenu ( TQListViewItem *item, const TQPoint &p, int /*col*/ )
{
  if (!item)
    return;

  m_clickedMenuItem = item;
  if (m_clickedMenuItem->itemAbove()) {
    listMoveFileUp->setEnabled(true);
  }
  else {
    listMoveFileUp->setEnabled(false);
  }
  if (m_clickedMenuItem->itemBelow()) {
    listMoveFileDown->setEnabled(true);
  }
  else {
    listMoveFileDown->setEnabled(false);
  }

  TQPopupMenu *menu = (TQPopupMenu*) ((viewManager->mainWindow())->factory()->container("filelist_popup", viewManager->mainWindow()));

  if (menu) {
    menu->exec(p);
  }
}

TQString KateFileList::tooltip( TQListViewItem *item, int )
{
  KateFileListItem *i = ((KateFileListItem*)item);
  if ( ! i ) return TQString::null;

  TQString str;
  const KateDocumentInfo *info = KateDocManager::self()->documentInfo(i->document());

  if (info && info->modifiedOnDisc)
  {
    if (info->modifiedOnDiscReason == 1)
      str += i18n("<b>This file was changed (modified) on disk by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 2)
      str += i18n("<b>This file was changed (created) on disk by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 3)
      str += i18n("<b>This file was changed (deleted) on disk by another program.</b><br />");
  }

  str += i->document()->url().prettyURL();
  return str;
}


void KateFileList::setSortType (int s)
{
  m_sort = s;
  if (m_sort == KateFileList::sortManual) {
    setSorting( -1, true );
  }
  else {
    setSorting( 0, true );
    updateSort ();
  }
}

void KateFileList::moveFileUp()
{
  if (m_clickedMenuItem) {
    sortAction->setCurrentItem(KateFileList::sortManual);
    setSortType(KateFileList::sortManual);
    TQListViewItem* nitemabove = m_clickedMenuItem->itemAbove();
    if (nitemabove) {
      nitemabove = nitemabove->itemAbove();
      if (nitemabove) {
        m_clickedMenuItem->moveItem(nitemabove);
      }
      else {
        // Qt made this hard
        nitemabove = m_clickedMenuItem->itemAbove();
        nitemabove->moveItem(m_clickedMenuItem);
      }
    }
  }
}

void KateFileList::moveFileDown()
{
  if (m_clickedMenuItem) {
    sortAction->setCurrentItem(KateFileList::sortManual);
    setSortType(KateFileList::sortManual);
    TQListViewItem* nitemabove = m_clickedMenuItem->itemBelow();
    if (nitemabove) {
      m_clickedMenuItem->moveItem(nitemabove);
    }
  }
}

void KateFileList::updateSort ()
{
  sort ();
}

void KateFileList::readConfig( KConfig *config, const TQString &group )
{
  TQString oldgroup = config->group();
  config->setGroup( group );

  setSortType( config->readNumEntry( "Sort Type", sortByID ) );
  m_viewShade = config->readColorEntry( "View Shade", &m_viewShade );
  m_editShade = config->readColorEntry( "Edit Shade", &m_editShade );
  m_enableBgShading = config->readBoolEntry( "Shading Enabled", &m_enableBgShading );

  sortAction->setCurrentItem( sortType() );

  config->setGroup( oldgroup );
}

void KateFileList::writeConfig( KConfig *config, const TQString &group )
{
  TQString oldgroup = config->group();
  config->setGroup( group );

  config->writeEntry( "Sort Type", m_sort );
  config->writeEntry( "View Shade", m_viewShade );
  config->writeEntry( "Edit Shade", m_editShade );
  config->writeEntry( "Shading Enabled", m_enableBgShading );

  config->setGroup( oldgroup );
}

void KateFileList::takeItem( TQListViewItem *item )
{
  if ( item->rtti() == RTTI_KateFileListItem )
  {
    m_editHistory.removeRef( (KateFileListItem*)item );
    m_viewHistory.removeRef( (KateFileListItem*)item );
  }
  TQListView::takeItem( item );
}
//END KateFileList

//BEGIN KateFileListItem
KateFileListItem::KateFileListItem( TQListView* lv,
				    Kate::Document *_doc )
  : TQListViewItem( lv, _doc->docName() ),
    doc( _doc ),
    m_viewhistpos( 0 ),
    m_edithistpos( 0 ),
    m_docNumber( _doc->documentNumber() )
{
  // Move this document to the end of the list where it belongs
  TQListViewItem* lastitem = lv->lastItem();
  if (lastitem) {
    moveItem(lastitem);
  }
}

KateFileListItem::~KateFileListItem()
{
}

const TQPixmap *KateFileListItem::pixmap ( int column ) const
{
  if ( column == 0) {
    static TQPixmap noPm = SmallIcon ("null");
    static TQPixmap modPm = SmallIcon("modified");
    static TQPixmap discPm = SmallIcon("modonhd");
    static TQPixmap modmodPm = SmallIcon("modmod");

    const KateDocumentInfo *info = KateDocManager::self()->documentInfo(doc);

    if (info && info->modifiedOnDisc)
      return doc->isModified() ? &modmodPm : &discPm;
    else
      return doc->isModified() ? &modPm : &noPm;
  }

  return 0;
}

void KateFileListItem::paintCell( TQPainter *painter, const TQColorGroup & cg, int column, int width, int align )
{
  KateFileList *fl = (KateFileList*)listView();
  if ( ! fl ) return;

  if ( column == 0 )
  {
    TQColorGroup cgNew = cg;

    // replace the base color with a different shading if necessary...
    if ( fl->shadingEnabled() && m_viewhistpos > 1 )
    {
      TQColor b( cg.base() );

      TQColor shade = fl->viewShade();
      TQColor eshade = fl->editShade();
      int hc = fl->histCount();
      // If this file is in the edit history, blend in the eshade
      // color. The blend is weighted by the position in the editing history
      if ( fl->shadingEnabled() && m_edithistpos > 0 )
      {
        int ec = fl->editHistCount();
        int v = hc-m_viewhistpos;
        int e = ec-m_edithistpos+1;
        e = e*e;
        int n = QMAX(v + e, 1);
        shade.setRgb(
            ((shade.red()*v) + (eshade.red()*e))/n,
            ((shade.green()*v) + (eshade.green()*e))/n,
            ((shade.blue()*v) + (eshade.blue()*e))/n
                    );
      }
      // blend in the shade color.
      // max transperancy < .5, latest is most colored.
      float t = (0.5/hc)*(hc-m_viewhistpos+1);
      b.setRgb(
          (int)((b.red()*(1-t)) + (shade.red()*t)),
          (int)((b.green()*(1-t)) + (shade.green()*t)),
          (int)((b.blue()*(1-t)) + (shade.blue()*t))
              );

      cgNew.setColor(TQColorGroup::Base, b);
    }

    TQListViewItem::paintCell( painter, cgNew, column, width, align );
  }
  else
    TQListViewItem::paintCell( painter, cg, column, width, align );
}

int KateFileListItem::compare ( TQListViewItem * i, int col, bool ascending ) const
{
  if ( i->rtti() == RTTI_KateFileListItem )
  {
    switch( ((KateFileList*)listView())->sortType() )
    {
      case KateFileList::sortByID:
      {

        int d = (int)doc->documentNumber() - ((KateFileListItem*)i)->documentNumber();
        return ascending ? d : -d;
        break;
      }
      case KateFileList::sortByURL:
        return doc->url().prettyURL().compare( ((KateFileListItem*)i)->document()->url().prettyURL() );
        break;
      default:
        return TQListViewItem::compare( i, col, ascending );
    }
  }
  return 0;
}
//END KateFileListItem

//BEGIN KFLConfigPage
KFLConfigPage::KFLConfigPage( TQWidget* parent, const char *name, KateFileList *fl )
  :  Kate::ConfigPage( parent, name ),
    m_filelist( fl ),
    m_changed( false )
{
  TQVBoxLayout *lo1 = new TQVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  lo1->setSpacing( spacing );

  TQGroupBox *gb = new TQGroupBox( 1, Qt::Horizontal, i18n("Background Shading"), this );
  lo1->addWidget( gb );

  TQWidget *g = new TQWidget( gb );
  TQGridLayout *lo = new TQGridLayout( g, 2, 2 );
  lo->setSpacing( KDialog::spacingHint() );
  cbEnableShading = new TQCheckBox( i18n("&Enable background shading"), g );
  lo->addMultiCellWidget( cbEnableShading, 1, 1, 0, 1 );

  kcbViewShade = new KColorButton( g );
  lViewShade = new TQLabel( kcbViewShade, i18n("&Viewed documents' shade:"), g );
  lo->addWidget( lViewShade, 2, 0 );
  lo->addWidget( kcbViewShade, 2, 1 );

  kcbEditShade = new KColorButton( g );
  lEditShade = new TQLabel( kcbEditShade, i18n("&Modified documents' shade:"), g );
  lo->addWidget( lEditShade, 3, 0 );
  lo->addWidget( kcbEditShade, 3, 1 );

  // sorting
  TQHBox *hbSorting = new TQHBox( this );
  lo1->addWidget( hbSorting );
  lSort = new TQLabel( i18n("&Sort by:"), hbSorting );
  cmbSort = new TQComboBox( hbSorting );
  lSort->setBuddy( cmbSort );
  TQStringList l;
  l << i18n("Opening Order") << i18n("Document Name") << i18n("URL");
  cmbSort->insertStringList( l );

  lo1->insertStretch( -1, 10 );

  TQWhatsThis::add( cbEnableShading, i18n(
      "When background shading is enabled, documents that have been viewed "
      "or edited within the current session will have a shaded background. "
      "The most recent documents have the strongest shade.") );
  TQWhatsThis::add( kcbViewShade, i18n(
      "Set the color for shading viewed documents.") );
  TQWhatsThis::add( kcbEditShade, i18n(
      "Set the color for modified documents. This color is blended into "
      "the color for viewed files. The most recently edited documents get "
      "most of this color.") );

  TQWhatsThis::add( cmbSort, i18n(
      "Set the sorting method for the documents.") );

  reload();

  slotEnableChanged();
  connect( cbEnableShading, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(slotMyChanged()) );
  connect( cbEnableShading, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(slotEnableChanged()) );
  connect( kcbViewShade, TQT_SIGNAL(changed(const TQColor&)), this, TQT_SLOT(slotMyChanged()) );
  connect( kcbEditShade, TQT_SIGNAL(changed(const TQColor&)), this, TQT_SLOT(slotMyChanged()) );
  connect( cmbSort, TQT_SIGNAL(activated(int)), this, TQT_SLOT(slotMyChanged()) );
}

void KFLConfigPage::apply()
{
  if ( ! m_changed )
    return;
  m_changed = false;

  // Change settings in the filelist
  m_filelist->m_viewShade = kcbViewShade->color();
  m_filelist->m_editShade = kcbEditShade->color();
  m_filelist->m_enableBgShading = cbEnableShading->isChecked();
  m_filelist->setSortType( cmbSort->currentItem() );
  // repaint the affected items
  m_filelist->triggerUpdate();
}

void KFLConfigPage::reload()
{
  // read in from config file
  KConfig *config = kapp->config();
  config->setGroup( "Filelist" );
  cbEnableShading->setChecked( config->readBoolEntry("Shading Enabled", &m_filelist->m_enableBgShading ) );
  kcbViewShade->setColor( config->readColorEntry("View Shade", &m_filelist->m_viewShade ) );
  kcbEditShade->setColor( config->readColorEntry("Edit Shade", &m_filelist->m_editShade ) );
  cmbSort->setCurrentItem( m_filelist->sortType() );
  m_changed = false;
}

void KFLConfigPage::slotEnableChanged()
{
  kcbViewShade->setEnabled( cbEnableShading->isChecked() );
  kcbEditShade->setEnabled( cbEnableShading->isChecked() );
  lViewShade->setEnabled( cbEnableShading->isChecked() );
  lEditShade->setEnabled( cbEnableShading->isChecked() );
}

void KFLConfigPage::slotMyChanged()
{
  m_changed = true;
  slotChanged();
}

//END KFLConfigPage


// kate: space-indent on; indent-width 2; replace-tabs on;
