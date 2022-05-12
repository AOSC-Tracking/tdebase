/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_listview.h"
#include "konq_textviewwidget.h"
#include "konq_treeviewwidget.h"
#include "konq_infolistviewwidget.h"
#include "konq_listviewsettings.h"

#include <tdeaction.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kinputdialog.h>
#include <tdelocale.h>
#include <konq_drag.h>
#include <kpropertiesdialog.h>
#include <kstdaction.h>
#include <kprotocolinfo.h>
#include <klineedit.h>
#include <kmimetype.h>

#include <tqapplication.h>
#include <tqclipboard.h>
#include <tqheader.h>
#include <tqregexp.h>

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <kinstance.h>

#include <konq_sort_constants.h>

KonqListViewFactory::KonqListViewFactory()
{
  s_instance = 0;
  s_defaultViewProps = 0;
}

KonqListViewFactory::~KonqListViewFactory()
{
  delete s_instance;
  delete s_defaultViewProps;

  s_instance = 0;
  s_defaultViewProps = 0;
}

KParts::Part* KonqListViewFactory::createPartObject( TQWidget *parentWidget, const char *, TQObject *parent, const char *name, const char*, const TQStringList &args )
{
  if( args.count() < 1 )
    kdWarning() << "KonqListView: Missing Parameter" << endl;

  KParts::Part *obj = new KonqListView( parentWidget, parent, name, args.first() );
  return obj;
}

TDEInstance *KonqListViewFactory::instance()
{
  if ( !s_instance )
    s_instance = new TDEInstance( "konqlistview" );
  return s_instance;
}

KonqPropsView *KonqListViewFactory::defaultViewProps()
{
  if ( !s_defaultViewProps )
    s_defaultViewProps = new KonqPropsView( instance(),0L );
    //s_defaultViewProps = KonqPropsView::defaultProps( instance() );

  return s_defaultViewProps;
}

TDEInstance *KonqListViewFactory::s_instance = 0;
KonqPropsView *KonqListViewFactory::s_defaultViewProps = 0;

K_EXPORT_COMPONENT_FACTORY( konq_listview, KonqListViewFactory )

ListViewBrowserExtension::ListViewBrowserExtension( KonqListView *listView )
 : KonqDirPartBrowserExtension( listView )
{
  m_listView = listView;
}

int ListViewBrowserExtension::xOffset()
{
  //kdDebug(1202) << k_funcinfo << endl;
  return m_listView->listViewWidget()->contentsX();
}

int ListViewBrowserExtension::yOffset()
{
  //kdDebug(1202) << k_funcinfo << endl;
  return m_listView->listViewWidget()->contentsY();
}

void ListViewBrowserExtension::updateActions()
{
  // This code is very related to KonqIconViewWidget::slotSelectionChanged
  int canCopy = 0;
  int canDel = 0;
  int canTrash = 0;
  bool bInTrash = false;
  KFileItemList lstItems = m_listView->selectedFileItems();

  for (KFileItem *item = lstItems.first(); item; item = lstItems.next())
  {
    canCopy++;
    KURL url = item->url();
    if ( url.directory(false) == TDEGlobalSettings::trashPath() )
      bInTrash = true;
    if (  KProtocolInfo::supportsDeleting(  url ) )
      canDel++;
    if ( !item->localPath().isEmpty() )
      canTrash++;
  }

  emit enableAction( "copy", canCopy > 0 );
  emit enableAction( "cut", canDel > 0 );
  emit enableAction( "trash", canDel > 0 && !bInTrash && canDel == canTrash );
  emit enableAction( "del", canDel > 0 );
  emit enableAction( "properties", lstItems.count() > 0 && KPropertiesDialog::canDisplay( lstItems ) );
  emit enableAction( "editMimeType", ( lstItems.count() == 1 ) );
  emit enableAction( "rename", ( m_listView->listViewWidget()->currentItem() != 0 ) && !bInTrash );
}

void ListViewBrowserExtension::copySelection( bool move )
{
  KonqDrag *urlData = new KonqDrag( m_listView->listViewWidget()->selectedUrls(false), m_listView->listViewWidget()->selectedUrls(true), move );
  TQApplication::clipboard()->setData( urlData );
}

void ListViewBrowserExtension::paste()
{
  KonqOperations::doPaste( m_listView->listViewWidget(), m_listView->url() );
}

void ListViewBrowserExtension::pasteTo( const KURL& url )
{
  KonqOperations::doPaste( m_listView->listViewWidget(), url );
}

void ListViewBrowserExtension::rename()
{
  TQListViewItem* item = m_listView->listViewWidget()->currentItem();
  Q_ASSERT ( item );

  // Update shurtcuts for the 'rename and move' actions. Must be done every time since the
  // shortcuts may have been changed by the user in the meanwhile
  const TDEShortcut moveNextSC=m_listView->m_paRenameMoveNext->shortcut();
  const TDEShortcut movePrevSC=m_listView->m_paRenameMovePrev->shortcut();
  m_listView->listViewWidget()->setRenameSettings(TDEListViewRenameSettings(
                                !(moveNextSC.isNull() && movePrevSC.isNull()), moveNextSC, movePrevSC));
  
  m_listView->listViewWidget()->rename( item, 0 );

  // Enhanced rename: Don't highlight the file extension.
  KLineEdit* le = m_listView->listViewWidget()->renameLineEdit();
  if ( le ) {
     const TQString txt = le->text();
     TQString pattern;
     KMimeType::diagnoseFileName( txt, pattern );
     if (!pattern.isEmpty() && pattern.at(0)=='*' && pattern.find('*',1)==-1)
         le->setSelection(0, txt.length()-pattern.stripWhiteSpace().length()+1);
     else
     {
         int lastDot = txt.findRev('.');
         if (lastDot > 0)
             le->setSelection(0, lastDot);
     }
  }
}

void ListViewBrowserExtension::trash()
{
  KonqOperations::del(m_listView->listViewWidget(),
                      KonqOperations::TRASH,
                      m_listView->listViewWidget()->selectedUrls( true ));
}

void ListViewBrowserExtension::reparseConfiguration()
{
  // m_pProps is a problem here (what is local, what is global ?)
  // but settings is easy :
  m_listView->listViewWidget()->initConfig();
}

void ListViewBrowserExtension::setSaveViewPropertiesLocally(bool value)
{
   m_listView->props()->setSaveViewPropertiesLocally( value );
}

void ListViewBrowserExtension::setNameFilter( const TQString &nameFilter )
{
  m_listView->setNameFilter( nameFilter );
}

void ListViewBrowserExtension::properties()
{
  (void) new KPropertiesDialog( m_listView->selectedFileItems() );
}

void ListViewBrowserExtension::editMimeType()
{
  KFileItemList items = m_listView->selectedFileItems();
  assert ( items.count() == 1 );
  KonqOperations::editMimeType( items.first()->mimetype() );
}

KonqListView::KonqListView( TQWidget *parentWidget, TQObject *parent, const char *name, const TQString& mode )
 : KonqDirPart( parent, name )
,m_headerTimer(0)
{
   setInstance( KonqListViewFactory::instance(), false );

   // Create a properties instance for this view
   // All the listview view modes inherit the same properties defaults...
   m_pProps = new KonqPropsView( KonqListViewFactory::instance(), KonqListViewFactory::defaultViewProps() );

   setBrowserExtension( new ListViewBrowserExtension( this ) );

   TQString xmlFile;

   if (mode=="TextView")
   {
      kdDebug(1202) << "Creating KonqTextViewWidget" << endl;
      xmlFile = "konq_textview.rc";
      m_pListView=new KonqTextViewWidget(this, parentWidget);
   }
   else if (mode=="MixedTree")
   {
      kdDebug(1202) << "Creating KonqTreeViewWidget" << endl;
      xmlFile = "konq_treeview.rc";
      m_pListView=new KonqTreeViewWidget(this,parentWidget);
   }
   else if (mode=="InfoListView")
   {
      kdDebug(1202) << "Creating KonqInfoListViewWidget" << endl;
      xmlFile = "konq_infolistview.rc";
      m_pListView=new KonqInfoListViewWidget(this,parentWidget);
   }
   else
   {
      kdDebug(1202) << "Creating KonqDetailedListViewWidget" << endl;
      xmlFile = "konq_detailedlistview.rc";
      m_pListView = new KonqBaseListViewWidget( this, parentWidget);
   }
   setWidget( m_pListView );
   setDirLister( m_pListView->m_dirLister );

   m_mimeTypeResolver = new KMimeTypeResolver<KonqBaseListViewItem,KonqListView>(this);

   setXMLFile( xmlFile );
   m_sortColumnIndexPrimary   = 0;
   m_sortColumnIndexAlternate = 1;

   setupActions();

   m_pListView->confColumns.resize( 11 );
   m_pListView->confColumns[0].setData(I18N_NOOP("MimeType"),"Type",TDEIO::UDS_MIME_TYPE,m_paShowMimeType);
   m_pListView->confColumns[1].setData(I18N_NOOP("Size"),"Size",TDEIO::UDS_SIZE,m_paShowSize);
   m_pListView->confColumns[2].setData(I18N_NOOP("Modified"),"Date",TDEIO::UDS_MODIFICATION_TIME,m_paShowTime);
   m_pListView->confColumns[3].setData(I18N_NOOP("Accessed"),"AccessDate",TDEIO::UDS_ACCESS_TIME,m_paShowAccessTime);
   m_pListView->confColumns[4].setData(I18N_NOOP("Created"),"CreationDate",TDEIO::UDS_CREATION_TIME,m_paShowCreateTime);
   m_pListView->confColumns[5].setData(I18N_NOOP("Permissions"),"Access",TDEIO::UDS_ACCESS,m_paShowPermissions);
   m_pListView->confColumns[6].setData(I18N_NOOP("Owner"),"Owner",TDEIO::UDS_USER,m_paShowOwner);
   m_pListView->confColumns[7].setData(I18N_NOOP("Group"),"Group",TDEIO::UDS_GROUP,m_paShowGroup);
   m_pListView->confColumns[8].setData(I18N_NOOP("Link"),"Link",TDEIO::UDS_LINK_DEST,m_paShowLinkDest);
   m_pListView->confColumns[9].setData(I18N_NOOP("URL"),"URL",TDEIO::UDS_URL,m_paShowURL);
   // Note: File Type is in fact the mimetype comment. We use UDS_FILE_TYPE but that's not what we show in fact :/
   m_pListView->confColumns[10].setData(I18N_NOOP("File Type"),"Type",TDEIO::UDS_FILE_TYPE,m_paShowType);

   connect( m_pListView, TQT_SIGNAL( selectionChanged() ),
            m_extension, TQT_SLOT( updateActions() ) );
   connect( m_pListView, TQT_SIGNAL( selectionChanged() ),
            this, TQT_SLOT( slotSelectionChanged() ) );

   connect( m_pListView, TQT_SIGNAL( currentChanged(TQListViewItem*) ),
            m_extension, TQT_SLOT( updateActions() ) );
   connect(m_pListView->header(),TQT_SIGNAL(indexChange(int,int,int)),this,TQT_SLOT(headerDragged(int,int,int)));
   connect(m_pListView->header(),TQT_SIGNAL(clicked(int)),this,TQT_SLOT(slotHeaderClicked(int)));
   connect(m_pListView->header(),TQT_SIGNAL(sizeChange(int,int,int)),TQT_SLOT(slotHeaderSizeChanged()));

   // signals from konqdirpart (for BC reasons)
   connect( this, TQT_SIGNAL( findOpened( KonqDirPart * ) ), TQT_SLOT( slotKFindOpened() ) );
   connect( this, TQT_SIGNAL( findClosed( KonqDirPart * ) ), TQT_SLOT( slotKFindClosed() ) );

   loadPlugins( this, this, instance() );
}

KonqListView::~KonqListView()
{
  delete m_mimeTypeResolver;
  delete m_pProps;
}

void KonqListView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
   KonqDirPart::guiActivateEvent(event );
   //ReadOnlyPart::guiActivateEvent(event );
   ((ListViewBrowserExtension*)m_extension)->updateActions();
}

bool KonqListView::doOpenURL( const KURL &url )
{
  KURL u( url );
  const TQString prettyURL = url.pathOrURL();
  emit setWindowCaption( prettyURL );
  return m_pListView->openURL( url );
}

bool KonqListView::doCloseURL()
{
  m_pListView->stop();
  m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
  return true;
}

void KonqListView::listingComplete()
{
  m_mimeTypeResolver->start( /*10*/ 0 );
}

void KonqListView::determineIcon( KonqBaseListViewItem * item )
{
  //int oldSerial = item->pixmap(0)->serialNumber();

  (void) item->item()->determineMimeType();

  //TQPixmap newIcon = item->item()->pixmap( m_parent->iconSize(),
  //                                        item->state() );
  //if ( oldSerial != newIcon.serialNumber() )
  //  item->setPixmap( 0, newIcon );

  if (item->item()->isMimeTypeKnown()) {
    item->mimetypeFound();
  }

  // We also have columns to update, not only the icon
  item->updateContents();
}

void KonqListView::saveState( TQDataStream &stream )
{
    //kdDebug(1202) << k_funcinfo << endl;
    KonqDirPart::saveState( stream );
    m_pListView->saveState( stream );
}

void KonqListView::restoreState( TQDataStream &stream )
{
    //kdDebug(1202) << k_funcinfo << endl;
    KonqDirPart::restoreState( stream );
    m_pListView->restoreState( stream );
}

void KonqListView::disableIcons( const KURL::List &lst )
{
    m_pListView->disableIcons( lst );
}

void KonqListView::slotSelect()
{
   bool ok;
   TQString pattern = KInputDialog::getText( TQString::null,
      i18n( "Select files:" ), "*", &ok, m_pListView );
   if ( !ok )
      return;

   TQRegExp re( pattern, true, true );

   m_pListView->blockSignals( true );

   for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
   {
      if ((m_pListView->automaticSelection()) && (it->isSelected())) {
         it->setSelected(FALSE);
         //the following line is to prevent that more than one item were selected
         //and now get deselected and automaticSelection() was true, this shouldn't happen
         //but who knows, aleXXX
         m_pListView->deactivateAutomaticSelection();
      }
      if ( it->isVisible() ) {
         if ( re.exactMatch( it->text(0) ) ) {
            it->setSelected(TRUE);
         }
      }
      else {
         it->setSelected(FALSE);
      }
   }
   m_pListView->blockSignals( false );
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotUnselect()
{
   bool ok;
   TQString pattern = KInputDialog::getText( TQString::null,
      i18n( "Unselect files:" ), "*", &ok, m_pListView );
   if ( !ok )
      return;

   TQRegExp re( pattern, TRUE, TRUE );

   m_pListView->blockSignals(TRUE);

   for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ ) {
      if ( it->isVisible() ) {
         if ( re.exactMatch( it->text(0) ) ) {
            it->setSelected(FALSE);
         }
      }
      else {
         it->setSelected(FALSE);
      }
   }

   m_pListView->blockSignals(FALSE);
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotSelectAll()
{
   m_pListView->selectAll(TRUE);
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
}

void KonqListView::slotUnselectAll()
{
    m_pListView->selectAll(FALSE);
   m_pListView->deactivateAutomaticSelection();
    emit m_pListView->selectionChanged();
}


void KonqListView::slotInvertSelection()
{
   if ((m_pListView->automaticSelection())
       && (m_pListView->currentItem()!=0)
       && (m_pListView->currentItem()->isSelected()))
      m_pListView->currentItem()->setSelected(FALSE);

   m_pListView->invertSelection();
    m_pListView->deactivateAutomaticSelection();
    emit m_pListView->selectionChanged();
    m_pListView->viewport()->update();
}

void KonqListView::newIconSize( int size )
{
    KonqDirPart::newIconSize( size );
    m_pListView->updateListContents();
}

void KonqListView::slotChangeHiddenFileMatcher()
{

  /* Temporarily
  */

  TDEIO::HiddenFileMatcher* matcher = TDEIO::HiddenFileMatcher::getInstance();

  // Someday we will do this and it will actually accomplish what we want:

  TQString updated_criteria = matcher->updateCriteria( "Define hidden for listview" );
  if ( matcher->setCriteria( updated_criteria ) ) {
    m_pProps->setHiddenFileSpec ( updated_criteria );
  }
  else {
    kdWarning()
      << "Failed to update critera - bad input? <"
      << updated_criteria                   << ">"
    << endl;
  }

  // Until the UI dialog is ready, we'll just do a demonstration

  TQString stored_criteria = m_pProps->hiddenFileSpec();
  TQString stored_criteria_saved = stored_criteria;

  TQString active_criteria = matcher->getCriteria();

  kdWarning()
    << "Hidden file match criteria before test updates:" << endl
    << "  Config: <" << stored_criteria << "> (assuming that this is a good match criteria)"           << endl
    << "  Active: <" << active_criteria << "> (should be same because of KonqListview initialization)" << endl
  ;

  // What happens with good update?

  updated_criteria = "R^good/criteria$"; // simulation of output of TBD UI dialog:
    // TQString updated_criteria = Get_HiddenFileCriteri_Dialog(stored_criteria);
  if ( matcher->setCriteria( updated_criteria ) ) {
    m_pProps->setHiddenFileSpec ( updated_criteria );
  }
  else {
    kdWarning()
      << "Failed to update critera - bad input? <"
      << updated_criteria                   << ">"
    << endl;
  }

  active_criteria = matcher->getCriteria();
  stored_criteria = m_pProps->hiddenFileSpec();
  kdWarning()
    << "Hidden file match criteria after test update 1:" << endl
    << "  Update: <" << updated_criteria << ">" << endl
    << "  Active: <" << active_criteria  << ">" << endl
    << "  Config: <" << stored_criteria  << ">" << endl
  ;

  // What happens with bad update?

  updated_criteria = "^bad/need/leading/W_w_R_r";
  if ( matcher->setCriteria( updated_criteria ) ) {
    m_pProps->setHiddenFileSpec ( updated_criteria );
  }
  else {
    kdWarning()
      << "Failed to update critera - bad input? <"
      << updated_criteria                   << ">"
    << endl;
  }

  active_criteria = matcher->getCriteria();
  stored_criteria = m_pProps->hiddenFileSpec();
  kdWarning()
    << "Hidden file match criteria after test update 2:" << endl
    << "  Update: <" << updated_criteria << ">" << endl
    << "  Active: <" << active_criteria  << ">" << endl
    << "  Config: <" << stored_criteria  << ">" << endl
  ;

  // Restore the original (and hopefully valid) match criteria

  updated_criteria = stored_criteria_saved;
  if ( matcher->setCriteria( updated_criteria ) ) {
    m_pProps->setHiddenFileSpec ( updated_criteria );
  }
  else {
    kdWarning()
      << "Failed to update critera - bad input? <"
      << updated_criteria                   << ">"
    << endl;
  }

  active_criteria = matcher->getCriteria();
  stored_criteria = m_pProps->hiddenFileSpec();
  kdWarning()
    << "Hidden file match criteria after restoring original criteria:" << endl
    << "  Update: <" << updated_criteria << ">" << endl
    << "  Active: <" << active_criteria  << ">" << endl
    << "  Config: <" << stored_criteria  << ">" << endl
  ;

  return;
}

void KonqListView::slotShowDot()
{
   m_pProps->setShowingDotFiles( m_paShowDot->isChecked() );
   m_pListView->m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );
   m_pListView->m_dirLister->emitChanges();
}

void KonqListView::slotCaseInsensitive()
{
   m_pProps->setCaseInsensitiveSort( m_paCaseInsensitive->isChecked() );
   m_pListView->sort();
}

void KonqListView::slotColumnToggled()
{
   kdDebug(1202) << "::slotColumnToggled" << endl;
   for (uint i=0; i<m_pListView->NumberOfAtoms; i++)
   {
      m_pListView->confColumns[i].displayThisOne=!m_pListView->confColumns[i].toggleThisOne
        || (m_pListView->confColumns[i].toggleThisOne->isChecked()&&m_pListView->confColumns[i].toggleThisOne->isEnabled());
      //this column has been enabled, the columns after it slide one column back
      if ((m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn==-1))
      {
         int maxColumn(0);
         for (uint j=0; j<m_pListView->NumberOfAtoms; j++)
            if ((m_pListView->confColumns[j].displayInColumn>maxColumn) && (m_pListView->confColumns[j].displayThisOne))
               maxColumn=m_pListView->confColumns[j].displayInColumn;
         m_pListView->confColumns[i].displayInColumn=maxColumn+1;
      }
      //this column has been disabled, the columns after it slide one column
      if ((!m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn!=-1))
      {
         for (uint j=0; j<m_pListView->NumberOfAtoms; j++)
            if (m_pListView->confColumns[j].displayInColumn>m_pListView->confColumns[i].displayInColumn)
               m_pListView->confColumns[j].displayInColumn--;
         m_pListView->confColumns[i].displayInColumn=-1;
      }
   }

   //create the new list contents
   m_pListView->createColumns();
   m_pListView->updateListContents();

   //save the config
   TQStringList lstColumns;
   int currentColumn(m_pListView->m_filenameColumn+1);
   for (int i=0; i<(int)m_pListView->NumberOfAtoms; i++)
   {
      kdDebug(1202)<<"checking: -"<<m_pListView->confColumns[i].name<<"-"<<endl;
      if ((m_pListView->confColumns[i].displayThisOne) && (currentColumn==m_pListView->confColumns[i].displayInColumn))
      {
          lstColumns.append(m_pListView->confColumns[i].name);
          kdDebug(1202)<<" adding"<<endl;
          currentColumn++;
          i=-1;
      }
   }
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setColumns( lstColumns );
   config.writeConfig();

   // Update column sizes
   slotHeaderSizeChanged();

   // Columns may have been rearranged, so do this to be safe:
   resetSortConfig();
}

void KonqListView::slotHeaderClicked(int sec)
{
   kdDebug(1202)<<"section: "<<sec<<" clicked"<<endl;
   int clickedColumn(-1);
   for (uint i=0; i<m_pListView->NumberOfAtoms; i++)
      if (m_pListView->confColumns[i].displayInColumn==sec) clickedColumn=i;
   kdDebug(1202)<<"atom index "<<clickedColumn<<endl;
   TQString nameOfSortColumn;
   //we clicked the file name column
   if (clickedColumn==-1)
      nameOfSortColumn="FileName";
   else
      nameOfSortColumn=m_pListView->confColumns[clickedColumn].desktopFileName;

   if (nameOfSortColumn!=m_pListView->sortedByColumn)
   {
      m_pListView->sortedByColumn=nameOfSortColumn;
      m_pListView->setAscending(TRUE);
   }
   else
   {
      m_pListView->setAscending(!m_pListView->ascending());
   }

   checkSortConfig() ;
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();

   if (sec == m_sortColumnIndexPrimary) {
      kdDebug(1202)<<"Changing sort order on primary sort column"<<endl;
      m_sortColumnOrderPrimary = m_pListView->ascending();
      config.setPrimarySortOrder(m_pListView->ascending());
   }
   else if (sec == m_sortColumnIndexAlternate) {
      kdDebug(1202)<<"Changing sort order on alternate sort column"<<endl;
      m_sortColumnOrderAlternate  = m_pListView->ascending();
      config.setAlternateSortOrder(m_pListView->ascending());
   }
   else if ( toggleColumnAlternate ) {
      kdDebug(1202)<<"Setting new alternate sort column"<<endl;
      m_sortColumnNameAlternate   = nameOfSortColumn;
      m_sortColumnIndexAlternate  = sec;
      m_sortColumnOrderAlternate  = true;
      config.setAlternateSortCol(nameOfSortColumn);
      config.setAlternateSortIndex(sec);
      config.setAlternateSortOrder(true);
      toggleColumnAlternate = false;
   }
   else {
      kdDebug(1202)<<"Setting new primary sort column"<<endl;
      m_sortColumnNamePrimary   = nameOfSortColumn;
      m_sortColumnIndexPrimary  = sec;
      m_sortColumnOrderPrimary  = true;
      config.setPrimarySortCol(nameOfSortColumn);
      config.setPrimarySortIndex(sec);
      config.setPrimarySortOrder(true);
      toggleColumnAlternate = true;
   }

   config.setSortBy( nameOfSortColumn );
   config.setSortOrder( m_pListView->ascending() );
   config.writeConfig();
}

void KonqListView::resetSortConfig()
{
   int defaultVisibleColumn;
   int columnNumber;

   defaultVisibleColumn = 0; // First visible column from left
   columnNumber = -1;
   for (uint i = 0; i < m_pListView->NumberOfAtoms; i++) {
      if (m_pListView->confColumns[i].displayInColumn == defaultVisibleColumn) {
        columnNumber = i;
      }
   }
   if (columnNumber == -1) {
      // This should not happen!
      kdDebug() << "We did not find columnNumber" <<endl;
      m_sortColumnIndexPrimary = 0;
      m_sortColumnNamePrimary  = "FileName";
   }
   else {
      m_sortColumnIndexPrimary = defaultVisibleColumn;
      m_sortColumnNamePrimary = m_pListView->confColumns[columnNumber].desktopFileName;
   }

   defaultVisibleColumn = 1 ;  // Second visible column from left
   columnNumber = -1;
   for (uint i = 0; i < m_pListView->NumberOfAtoms; i++) {
      if (m_pListView->confColumns[i].displayInColumn == defaultVisibleColumn) {
        columnNumber = i;
      }
   }
   if (columnNumber == -1) {
      // This should not happen!
      kdDebug() << "We did not find columnNumber" <<endl;
      m_sortColumnIndexAlternate = 0;
      m_sortColumnNameAlternate  = "FileName";
   }
   else {
      m_sortColumnIndexAlternate = defaultVisibleColumn;
      m_sortColumnNameAlternate=m_pListView->confColumns[columnNumber].desktopFileName;
   }

   m_sortColumnOrderPrimary = true ;
   m_sortColumnOrderAlternate = true ;

   kdDebug(1202) << "Initialized m_sortColumnIndexPrimary to " <<m_sortColumnIndexPrimary <<endl;
   kdDebug(1202) << "Initialized m_sortColumnNamePrimary to " <<m_sortColumnNamePrimary <<endl;
   kdDebug(1202) << "Initialized m_sortColumnIndexAlternate to " <<m_sortColumnIndexAlternate <<endl;
   kdDebug(1202) << "Initialized m_sortColumnNameAlternate to " <<m_sortColumnNameAlternate <<endl;

   KonqListViewSettings config( m_pListView->url().protocol() );
   config.setPrimarySortCol(m_sortColumnNamePrimary);
   config.setPrimarySortIndex(m_sortColumnIndexPrimary);
   config.setPrimarySortOrder(m_sortColumnOrderPrimary);
   config.setAlternateSortCol(m_sortColumnNameAlternate);
   config.setAlternateSortIndex(m_sortColumnIndexAlternate);
   config.setAlternateSortOrder(m_sortColumnOrderAlternate);
   config.writeConfig() ;
}

void KonqListView::checkSortConfig()
{
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();

   m_sortColumnIndexPrimary   = config.primarySortIndex();
   m_sortColumnNamePrimary    = config.primarySortCol();
   m_sortColumnOrderPrimary   = config.primarySortOrder();

   m_sortColumnIndexAlternate = config.alternateSortIndex();
   m_sortColumnNameAlternate  = config.alternateSortCol();
   m_sortColumnOrderAlternate = config.alternateSortOrder();

   if (m_sortColumnIndexPrimary >= 0 && m_sortColumnIndexAlternate >= 0)
     return ;

   resetSortConfig();
}

#define LV_SORT_CHANGE_COLUMN 1
#define LV_SORT_CHANGE_ORDER  2

void KonqListView::sortListView(uint which)
{
   TQString sortColumnNameCurrent = m_pListView->sortedByColumn;
   if (sortColumnNameCurrent == "" ) {
     sortColumnNameCurrent = "FileName" ;
     m_pListView->sortedByColumn = sortColumnNameCurrent ;
   }
   TQString sortColumnNameNext;
   int      sortColumnIndex;
   bool     sortOrder;

   checkSortConfig() ;
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();

   switch (which)
   {
      case LV_SORT_CHANGE_COLUMN:
         if (m_sortColumnNamePrimary == sortColumnNameCurrent)
         {
            sortColumnNameNext = m_sortColumnNameAlternate;
            sortColumnIndex = m_sortColumnIndexAlternate;
            sortOrder = m_sortColumnOrderAlternate;
            kdDebug(1202) << "Changing sort column to alternate"<<endl ;
         }
         else
         {
            sortColumnNameNext = m_sortColumnNamePrimary;
            sortColumnIndex = m_sortColumnIndexPrimary;
            sortOrder = m_sortColumnOrderPrimary;
            kdDebug(1202) << "Changing sort column to primary"<<endl ;
         }
         m_pListView->setSorting( sortColumnIndex, sortOrder );
         m_pListView->sortedByColumn = sortColumnNameNext;
         config.setSortBy( sortColumnNameNext );
         break;

     case LV_SORT_CHANGE_ORDER:
        sortOrder = !m_pListView->ascending();

        if ( m_sortColumnNamePrimary == sortColumnNameCurrent )
        {
           sortColumnIndex = m_sortColumnIndexPrimary;
           m_sortColumnOrderPrimary = sortOrder;
           config.setPrimarySortOrder( sortOrder );
           kdDebug(1202) << "Changing sort order of primary"<<endl ;
        }
        else
        {
           sortColumnIndex = m_sortColumnIndexAlternate;
           m_sortColumnOrderAlternate = sortOrder ;
           config.setAlternateSortOrder( sortOrder ) ;
           kdDebug(1202) << "Changing sort order of alternate"<<endl ;
        }
        m_pListView->setAscending(sortOrder) ;
        config.setSortOrder(sortOrder);
        m_pListView->setSorting( sortColumnIndex, sortOrder );
        break;

     default:
        // Do nothing in case of invalid call
        return;
   }

   m_pListView->sort();

   config.writeConfig();
}

void KonqListView::slotSortAlternate()
{
   KonqListView::sortListView(LV_SORT_CHANGE_COLUMN);
}

void KonqListView::slotSortReverse()
{
   KonqListView::sortListView(LV_SORT_CHANGE_ORDER);
}

void KonqListView::slotToggleDisplayDirectoriesFirst()
{
   m_pProps->setDirsFirst( m_paDisplayDirectoriesFirst->isChecked() );
   m_pListView->updateListContents();
   m_pListView->sort();
}

void KonqListView::slotToggleDisplayHiddenFirst()
{
   m_pProps->setHiddenFirst( m_paDisplayHiddenFirst->isChecked() );
   m_pListView->updateListContents();
   m_pListView->sort();
}

void KonqListView::headerDragged(int sec, int from, int to)
{
   kdDebug(1202)<<"section: "<<sec<<" fromIndex: "<<from<<" toIndex "<<to<<endl;
   //at this point the columns aren't moved yet, so I let the listview
   //rearrange the stuff and use a single shot timer
   TQTimer::singleShot(200,this,TQT_SLOT(slotSaveAfterHeaderDrag()));
}

const KFileItem * KonqListView::currentItem()
{
   if (m_pListView==0 || m_pListView->currentItem()==0)
      return 0L;
   return static_cast<KonqListViewItem *>(m_pListView->currentItem())->item();
}

void KonqListView::slotSaveAfterHeaderDrag()
{
   TQStringList lstColumns;

   for ( int i=0; i < m_pListView->columns(); i++ )
   {
      int section = m_pListView->header()->mapToSection( i );

      // look for section
      for ( uint j=0; j < m_pListView->NumberOfAtoms; j++ )
      {
         if ( m_pListView->confColumns[j].displayInColumn == section )
         {
            lstColumns.append( m_pListView->confColumns[j].name );
            break;
         }
      }
   }
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setColumns( lstColumns );
   config.writeConfig();

   // Update column sizes
   slotHeaderSizeChanged();

   // Columns were rearranged, so do this to be safe:
   resetSortConfig();
}

void KonqListView::slotSaveColumnWidths()
{
   TQValueList<int> lstColumnWidths;

   for ( int i=0; i < m_pListView->columns(); i++ )
   {
      int section = m_pListView->header()->mapToSection( i );

      // look for section
      for ( uint j=0; j < m_pListView->NumberOfAtoms; j++ )
      {
         // Save size only if the column is found
         if ( m_pListView->confColumns[j].displayInColumn == section )
         {
            m_pListView->confColumns[j].width = m_pListView->columnWidth(section);
            lstColumnWidths.append( m_pListView->confColumns[j].width );
            break;
         }
      }
   }
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setColumnWidths( lstColumnWidths );

   // size of current filename column
   config.setFileNameColumnWidth( m_pListView->columnWidth(0) );
   config.writeConfig();
}

void KonqListView::slotHeaderSizeChanged()
{
   if ( !m_headerTimer )
   {
      m_headerTimer = new TQTimer( this );
      connect( m_headerTimer, TQT_SIGNAL( timeout() ), this, TQT_SLOT( slotSaveColumnWidths() ) );
   }
   else
      m_headerTimer->stop();

   m_headerTimer->start( 250, true );
}

void KonqListView::slotKFindOpened()
{
    if ( m_pListView->m_dirLister )
        m_pListView->m_dirLister->setAutoUpdate( false );
}

void KonqListView::slotKFindClosed()
{
    if ( m_pListView->m_dirLister )
        m_pListView->m_dirLister->setAutoUpdate( true );
}

void KonqListView::setupActions()
{
   m_paShowTime=new TDEToggleAction(i18n("Show &Modification Time"), 0,this, TQT_SLOT(slotColumnToggled()), actionCollection(), "show_time" );
   m_paShowTime->setCheckedState(i18n("Hide &Modification Time"));
   m_paShowType=new TDEToggleAction(i18n("Show &File Type"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_type" );
   m_paShowType->setCheckedState(i18n("Hide &File Type"));
   m_paShowMimeType=new TDEToggleAction(i18n("Show MimeType"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_mimetype" );
   m_paShowMimeType->setCheckedState(i18n("Hide MimeType"));
   m_paShowAccessTime=new TDEToggleAction(i18n("Show &Access Time"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_access_time" );
   m_paShowAccessTime->setCheckedState(i18n("Hide &Access Time"));
   m_paShowCreateTime=new TDEToggleAction(i18n("Show &Creation Time"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_creation_time" );
   m_paShowCreateTime->setCheckedState(i18n("Hide &Creation Time"));
   m_paShowLinkDest=new TDEToggleAction(i18n("Show &Link Destination"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_link_dest" );
   m_paShowLinkDest->setCheckedState(i18n("Hide &Link Destination"));
   m_paShowSize=new TDEToggleAction(i18n("Show Filesize"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_size" );
   m_paShowSize->setCheckedState(i18n("Hide Filesize"));
   m_paShowOwner=new TDEToggleAction(i18n("Show Owner"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_owner" );
   m_paShowOwner->setCheckedState(i18n("Hide Owner"));
   m_paShowGroup=new TDEToggleAction(i18n("Show Group"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_group" );
   m_paShowGroup->setCheckedState(i18n("Hide Group"));
   m_paShowPermissions=new TDEToggleAction(i18n("Show Permissions"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_permissions" );
   m_paShowPermissions->setCheckedState(i18n("Hide Permissions"));
   m_paShowURL=new TDEToggleAction(i18n("Show URL"), 0, this, TQT_SLOT(slotColumnToggled()),actionCollection(), "show_url" );

   m_paSelect = new TDEAction( i18n( "Se&lect..." ), CTRL+Key_Plus, this, TQT_SLOT( slotSelect() ), actionCollection(), "select" );
  m_paUnselect = new TDEAction( i18n( "Unselect..." ), CTRL+Key_Minus, this, TQT_SLOT( slotUnselect() ), actionCollection(), "unselect" );
  m_paSelectAll = KStdAction::selectAll( this, TQT_SLOT( slotSelectAll() ), actionCollection(), "selectall" );
  m_paUnselectAll = new TDEAction( i18n( "Unselect All" ), CTRL+Key_U, this, TQT_SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
  m_paInvertSelection = new TDEAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk, this, TQT_SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

  // These 2 actions are 'fake' actions. They are defined so that the keyboard shortcuts
  // can be set from the 'Configure Shortcuts..." dialog.
  // The real actions are performed in the TDEListViewLineEdit::keyPressEvent() in tdeui
  m_paRenameMoveNext = new TDEAction(i18n( "&Rename and move to next item" ), Key_Tab,
                              NULL, NULL, actionCollection(), "renameMoveNext" );
  m_paRenameMoveNext->setWhatsThis(i18n("Pressing this button completes the current rename operation,"
                                  "moves to the next item and starts a new rename operation."));
  m_paRenameMoveNext->setToolTip( i18n("Complete rename operation and move the next item"));
  m_paRenameMoveNext->setEnabled(false);
  m_paRenameMovePrev = new TDEAction( i18n( "&Rename and move to previous item" ), SHIFT+Key_BackTab,
                              NULL, NULL, actionCollection(), "renameMovePrev" );
  m_paRenameMovePrev->setWhatsThis(i18n("Pressing this button completes the current rename operation,"
                                  "moves to the previous item and starts a new rename operation."));
  m_paRenameMovePrev->setToolTip( i18n("Complete rename operation and move the previous item"));
  m_paRenameMovePrev->setEnabled(false);

  m_paChangeHiddenFileMatcher = new TDEAction( i18n( "Change &Hidden File Matcher" ), CTRL+SHIFT+Key_H, this,
   TQT_SLOT( slotChangeHiddenFileMatcher() ), actionCollection(), "change_hidden_file_matcher" );
  /* Temporarily
  */
  // Initialize hidden file matcher criteria
  TDEIO::HiddenFileMatcher* matcher = TDEIO::HiddenFileMatcher::getInstance();
    // This is probably the 1st call that creates the central instance
  matcher->setCriteria( m_pProps->hiddenFileSpec() );
  /* Do we want or need to display current match criteria in menu???
    Sample from konq_iconview.cpp::KonqKfmIconView::KonqKfmIconView
    //enable menu item representing the saved sorting criterion
    TQString sortcrit = KonqIconViewFactory::defaultViewProps()->sortCriterion();
    TDERadioAction *sort_action = tqt_dynamic_cast<TDERadioAction *>(actionCollection()->action(sortcrit.latin1()));
    if(sort_action!=NULL) sort_action->activate();
  */

  m_paShowDot = new TDEToggleAction( i18n( "Show &Hidden Files" ), CTRL+Key_H, this, TQT_SLOT( slotShowDot() ), actionCollection(), "show_dot" );
//  m_paShowDot->setCheckedState(i18n("Hide &Hidden Files"));

  m_paCaseInsensitive = new TDEToggleAction(i18n("Case Insensitive Sort"), 0, this, TQT_SLOT(slotCaseInsensitive()),actionCollection(), "sort_caseinsensitive" );

  m_paSortAlternate = new TDEAction( i18n( "&Alternate Sort Order" ), CTRL+Key_S, this,
    TQT_SLOT( slotSortAlternate() ), actionCollection(), "alternate_sort_order");
  m_paSortAlternate->setToolTip( i18n( "Sort by PREVIOUS sort column" ) );
  m_paSortReverse = new TDEAction( i18n( "&Reverse Sort Order" ), CTRL+Key_R, this,
    TQT_SLOT( slotSortReverse() ), actionCollection(), "reverse_sort_order");
  m_paSortReverse->setToolTip( i18n( "Reverse sort order of current sort column" ) );

  /*
     Build View->Sort submenu interface to properties
     SortOrder, SortDirsFirst & SortHiddenFirst
  */
  TextSortOrder initialOrder = m_pProps->getSortOrder();

  m_paOrderUnicodeUnmodified = new TDEToggleAction( i18n( "&Unicode based" ), ALT+Key_1, this,
    TQT_SLOT( slotOrderUnicodeUnmodified() ), actionCollection(), "order_unicode_based" );
  m_paOrderUnicodeUnmodified->setToolTip( i18n( "Strict numeric Unicode based order" ) );
  m_paOrderUnicodeUnmodified->setChecked( initialOrder == UNICODE_UNMODIFIED );

  m_paOrderUnicodeCaseInsensitive = new TDEToggleAction( i18n( "Unicode based, &case insensitive" ), ALT+Key_2, this,
    TQT_SLOT( slotOrderUnicodeCaseInsensitive() ), actionCollection(), "order_unicode_based_case_insensitive" );
  m_paOrderUnicodeCaseInsensitive->setToolTip( i18n( "Unicode based order but with lower/upper case ASCII letters adjacent" ) );
  m_paOrderUnicodeCaseInsensitive->setChecked(initialOrder == UNICODE_CASEINSENSITIVE);

  m_paOrderLocale = new TDEToggleAction( i18n( "&Locale based" ), ALT+Key_3, this,
    TQT_SLOT( slotOrderLocale() ), actionCollection(), "order_locale_based" );
  m_paOrderLocale->setToolTip( i18n( "Locale based order, usually \"friendly\"" ) );
  m_paOrderLocale->setChecked(initialOrder == LOCALE_UNMODIFIED);

  // Convert above 3 checkbox menu items into single 3-way radio button menu item
  m_paOrderUnicodeUnmodified->setExclusiveGroup( "sortOrder" );
  m_paOrderUnicodeCaseInsensitive->setExclusiveGroup( "sortOrder" );
  m_paOrderLocale->setExclusiveGroup( "sortOrder" );

  m_paDisplayDirectoriesFirst = new TDEToggleAction( i18n("Group &Directories First"), CTRL+SHIFT+Key_F, this,
    TQT_SLOT(slotToggleDisplayDirectoriesFirst()), actionCollection(), "group_directories_first");
  m_paDisplayDirectoriesFirst->setToolTip( i18n( "Always display directories before non-directories" ) );
  m_paDisplayDirectoriesFirst->setChecked(m_pProps->isDirsFirst());

  m_paDisplayHiddenFirst = new TDEToggleAction( i18n("Group &Hidden First"), CTRL+SHIFT+Key_H, this,
    TQT_SLOT(slotToggleDisplayHiddenFirst()), actionCollection(), "group_hidden_first");
  m_paDisplayHiddenFirst->setToolTip( i18n( "Always display hidden files before non-hidden files" ) );
  m_paDisplayHiddenFirst->setChecked(m_pProps->isHiddenFirst());

  newIconSize( TDEIcon::SizeSmall /* default size */ );
}

void KonqListView::slotSelectionChanged()
{
  bool itemSelected = selectedFileItems().count()>0;
  m_paUnselect->setEnabled( itemSelected );
  m_paUnselectAll->setEnabled( itemSelected );
//  m_paInvertSelection->setEnabled( itemSelected );
}

void KonqListView::slotOrderUnicodeUnmodified()
{
  TextSortOrder sortOrder = UNICODE_UNMODIFIED ;
  kdDebug(1202) << "Setting name order = " << sortOrder << endl;
  m_pProps->setSortOrder( sortOrder );
  m_pListView->m_sortOrder = sortOrder;
  m_pListView->sort();
}

void KonqListView::slotOrderUnicodeCaseInsensitive()
{
  TextSortOrder sortOrder = UNICODE_CASEINSENSITIVE;
  kdDebug(1202) << "Setting name order = " << sortOrder << endl;
  m_pProps->setSortOrder( sortOrder );
  m_pListView->m_sortOrder = sortOrder;
  m_pListView->sort();
}

void KonqListView::slotOrderLocale()
{
  TextSortOrder sortOrder = LOCALE_UNMODIFIED;
  kdDebug(1202) << "Setting name order = " << sortOrder << endl;
  m_pProps->setSortOrder( sortOrder );
  m_pListView->m_sortOrder = sortOrder;
  m_pListView->sort();
}

#include "konq_listview.moc"

