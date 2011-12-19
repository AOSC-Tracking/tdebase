
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqtimer.h>
#include <tqwhatsthis.h>
#include <tqwidgetstack.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kipc.h>
#include <klineedit.h>
#include <klistview.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "newtypedlg.h"
#include "filetypedetails.h"
#include "filegroupdetails.h"
#include "filetypesview.h"
#include <ksycoca.h>

FileTypesView::FileTypesView(TQWidget *p, const char *name)
  : KCModule(p, name)
{
  m_konqConfig = KSharedConfig::openConfig("konquerorrc", false, false);

  setQuickHelp( i18n("<h1>File Associations</h1>"
    " This module allows you to choose which applications are associated"
    " with a given type of file. File types are also referred to MIME types"
    " (MIME is an acronym which stands for \"Multipurpose Internet Mail"
    " Extensions\".)<p> A file association consists of the following:"
    " <ul><li>Rules for determining the MIME-type of a file, for example"
    " the filename pattern *.kwd, which means 'all files with names that end"
    " in .kwd', is associated with the MIME type \"x-kword\";</li>"
    " <li>A short description of the MIME-type, for example the description"
    " of the MIME type \"x-kword\" is simply 'KWord document';</li>"
    " <li>An icon to be used for displaying files of the given MIME-type,"
    " so that you can easily identify the type of file in, say, a Konqueror"
    " view (at least for the types you use often);</li>"
    " <li>A list of the applications which can be used to open files of the"
    " given MIME-type -- if more than one application can be used then the"
    " list is ordered by priority.</li></ul>"
    " You may be surprised to find that some MIME types have no associated"
    " filename patterns; in these cases, Konqueror is able to determine the"
    " MIME-type by directly examining the contents of the file."));

  setButtons(Help | Apply | Cancel | Ok);
  TQString wtstr;

  TQHBoxLayout *l = new TQHBoxLayout(this, 0, KDialog::marginHint());
  TQGridLayout *leftLayout = new TQGridLayout(0, 4, 3);
  leftLayout->setSpacing( KDialog::spacingHint() );
  leftLayout->setColStretch(1, 1);

  l->addLayout( TQT_TQLAYOUT(leftLayout) );

  TQLabel *patternFilterLBL = new TQLabel(i18n("F&ind filename pattern:"), this);
  leftLayout->addMultiCellWidget(patternFilterLBL, 0, 0, 0, 2);

  patternFilterLE = new KLineEdit(this);
  patternFilterLBL->setBuddy( patternFilterLE );
  leftLayout->addMultiCellWidget(patternFilterLE, 1, 1, 0, 2);

  connect(patternFilterLE, TQT_SIGNAL(textChanged(const TQString &)),
          this, TQT_SLOT(slotFilter(const TQString &)));

  wtstr = i18n("Enter a part of a filename pattern. Only file types with a "
               "matching file pattern will appear in the list.");

  TQWhatsThis::add( patternFilterLE, wtstr );
  TQWhatsThis::add( patternFilterLBL, wtstr );

  typesLV = new KListView(this);
  typesLV->setRootIsDecorated(true);
  typesLV->setFullWidth(true);

  typesLV->addColumn(i18n("Known Types"));
  leftLayout->addMultiCellWidget(typesLV, 2, 2, 0, 2);
  connect(typesLV, TQT_SIGNAL(selectionChanged(TQListViewItem *)),
          this, TQT_SLOT(updateDisplay(TQListViewItem *)));
  connect(typesLV, TQT_SIGNAL(doubleClicked(TQListViewItem *)),
          this, TQT_SLOT(slotDoubleClicked(TQListViewItem *)));

  TQWhatsThis::add( typesLV, i18n("Here you can see a hierarchical list of"
    " the file types which are known on your system. Click on the '+' sign"
    " to expand a category, or the '-' sign to collapse it. Select a file type"
    " (e.g. text/html for HTML files) to view/edit the information for that"
    " file type using the controls on the right.") );

  TQPushButton *addTypeB = new TQPushButton(i18n("Add..."), this);
  connect(addTypeB, TQT_SIGNAL(clicked()), TQT_SLOT(addType()));
  leftLayout->addWidget(addTypeB, 3, 0);

  TQWhatsThis::add( addTypeB, i18n("Click here to add a new file type.") );

  m_removeTypeB = new TQPushButton(i18n("&Remove"), this);
  connect(m_removeTypeB, TQT_SIGNAL(clicked()), TQT_SLOT(removeType()));
  leftLayout->addWidget(m_removeTypeB, 3, 2);
  m_removeTypeB->setEnabled(false);

  TQWhatsThis::add( m_removeTypeB, i18n("Click here to remove the selected file type.") );

  // For the right panel, prepare a widget stack
  m_widgetStack = new TQWidgetStack(this);

  l->addWidget( m_widgetStack );

  // File Type Details
  m_details = new FileTypeDetails( m_widgetStack );
  connect( m_details, TQT_SIGNAL( changed(bool) ),
           this, TQT_SLOT( setDirty(bool) ) );
  connect( m_details, TQT_SIGNAL( embedMajor(const TQString &, bool &) ),
           this, TQT_SLOT( slotEmbedMajor(const TQString &, bool &)));
  m_widgetStack->addWidget( m_details, 1 /*id*/ );

  // File Group Details
  m_groupDetails = new FileGroupDetails( m_widgetStack );
  connect( m_groupDetails, TQT_SIGNAL( changed(bool) ),
           this, TQT_SLOT( setDirty(bool) ) );
  m_widgetStack->addWidget( m_groupDetails, 2 /*id*/ );

  // Widget shown on startup
  m_emptyWidget = new TQLabel( i18n("Select a file type by name or by extension"), m_widgetStack);
  m_emptyWidget->setAlignment(AlignCenter);

  m_widgetStack->addWidget( m_emptyWidget, 3 /*id*/ );

  m_widgetStack->raiseWidget( m_emptyWidget );

  TQTimer::singleShot( 0, this, TQT_SLOT( init() ) ); // this takes some time

  connect( KSycoca::self(), TQT_SIGNAL( databaseChanged() ), TQT_SLOT( slotDatabaseChanged() ) );
}

FileTypesView::~FileTypesView()
{
}

void FileTypesView::setDirty(bool state)
{
  emit changed(state);
  m_dirty = state;
}

void FileTypesView::init()
{
  show();
  setEnabled( false );

  setCursor( KCursor::waitCursor() );
  readFileTypes();
  unsetCursor();

  setDirty(false);
  setEnabled( true );
}

// only call this method once on startup, then never again! Otherwise, newly
// added Filetypes will be lost.
void FileTypesView::readFileTypes()
{
    typesLV->clear();
    m_majorMap.clear();
    m_itemList.clear();
    TypesListItem::reset();

    TypesListItem *groupItem;
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    TQValueListIterator<KMimeType::Ptr> it2(mimetypes.begin());
    for (; it2 != mimetypes.end(); ++it2) {
	TQString mimetype = (*it2)->name();
	int index = mimetype.find("/");
	TQString maj = mimetype.left(index);
	TQString min = mimetype.right(mimetype.length() - index+1);

	TQMapIterator<TQString,TypesListItem*> mit = m_majorMap.find( maj );
	if ( mit == m_majorMap.end() ) {
	    groupItem = new TypesListItem( typesLV, maj );
	    m_majorMap.insert( maj, groupItem );
	}
	else
	    groupItem = mit.data();

	TypesListItem *item = new TypesListItem(groupItem, (*it2));
	m_itemList.append( item );
    }
    updateDisplay(0L);

}

void FileTypesView::slotEmbedMajor(const TQString &major, bool &embed)
{
    TypesListItem *groupItem;
    TQMapIterator<TQString,TypesListItem*> mit = m_majorMap.find( major );
    if ( mit == m_majorMap.end() )
        return;
        
    groupItem = mit.data();
    
    embed = (groupItem->autoEmbed() == 0);
}

void FileTypesView::slotFilter(const TQString & patternFilter)
{
    // one of the few ways to clear a listview without destroying the
    // listviewitems and without making TQListView crash.
    TQListViewItem *item;
    while ( (item = typesLV->firstChild()) ) {
	while ( item->firstChild() )
	    item->takeItem( item->firstChild() );

	typesLV->takeItem( item );
    }

    // insert all items and their group that match the filter
    TQPtrListIterator<TypesListItem> it( m_itemList );
    while ( it.current() ) {
	if ( patternFilter.isEmpty() ||
	     !((*it)->patterns().grep( patternFilter, false )).isEmpty() ) {

	    TypesListItem *group = m_majorMap[ (*it)->majorType() ];
	    // TQListView makes sure we don't insert a group-item more than once
	    typesLV->insertItem( group );
	    group->insertItem( *it );
	}
	++it;
    }
}

void FileTypesView::addType()
{
  TQStringList allGroups;
  TQMapIterator<TQString,TypesListItem*> it = m_majorMap.begin();
  while ( it != m_majorMap.end() ) {
      allGroups.append( it.key() );
      ++it;
  }

  NewTypeDialog m(allGroups, this);

  if (m.exec()) {
    TQListViewItemIterator it(typesLV);
    TQString loc = m.group() + "/" + m.text() + ".desktop";
    loc = locateLocal("mime", loc);
    KMimeType::Ptr mimetype = new KMimeType(loc,
                                            m.group() + "/" + m.text(),
                                            TQString(), TQString(),
                                            TQStringList());

    TypesListItem *group = m_majorMap[ m.group() ];
    if ( !group )
    {
       //group = new TypesListItem(
       //TODO ! (The combo in NewTypeDialog must be made editable again when that happens)
       Q_ASSERT(group);
    }

    // find out if our group has been filtered out -> insert if necessary
    TQListViewItem *item = typesLV->firstChild();
    bool insert = true;
    while ( item ) {
	if ( item == group ) {
	    insert = false;
	    break;
	}
	item = item->nextSibling();
    }
    if ( insert )
	typesLV->insertItem( group );

    TypesListItem *tli = new TypesListItem(group, mimetype, true);
    m_itemList.append( tli );

    group->setOpen(true);
    typesLV->setSelected(tli, true);

    setDirty(true);
  }
}

void FileTypesView::removeType()
{
  TypesListItem *current = (TypesListItem *) typesLV->currentItem();

  if ( !current )
      return;

  // Can't delete groups
  if ( current->isMeta() )
      return;
  // nor essential mimetypes
  if ( current->isEssential() )
      return;

  TQListViewItem *li = current->itemAbove();
  if (!li)
      li = current->itemBelow();
  if (!li)
      li = current->parent();

  removedList.append(current->name());
  current->parent()->takeItem(current);
  m_itemList.removeRef( current );
  setDirty(true);

  if ( li )
      typesLV->setSelected(li, true);
}

void FileTypesView::slotDoubleClicked(TQListViewItem *item)
{
  if ( !item ) return;
  item->setOpen( !item->isOpen() );
}

void FileTypesView::updateDisplay(TQListViewItem *item)
{
  if (!item)
  {
    m_widgetStack->raiseWidget( m_emptyWidget );
    m_removeTypeB->setEnabled(false);
    return;
  }

  bool wasDirty = m_dirty;

  TypesListItem *tlitem = (TypesListItem *) item;
  if (tlitem->isMeta()) // is a group
  {
    m_widgetStack->raiseWidget( m_groupDetails );
    m_groupDetails->setTypeItem( tlitem );
    m_removeTypeB->setEnabled(false);
  }
  else
  {
    m_widgetStack->raiseWidget( m_details );
    m_details->setTypeItem( tlitem );
    m_removeTypeB->setEnabled( !tlitem->isEssential() );
  }

  // Updating the display indirectly called change(true)
  if ( !wasDirty )
    setDirty(false);
}

bool FileTypesView::sync( TQValueList<TypesListItem *>& itemsModified )
{
  bool didIt = false;
  // first, remove those items which we are asked to remove.
  TQStringList::Iterator it(removedList.begin());
  TQString loc;

  for (; it != removedList.end(); ++it) {
    didIt = true;
    KMimeType::Ptr m_ptr = KMimeType::mimeType(*it);

    loc = m_ptr->desktopEntryPath();
    loc = locateLocal("mime", loc);

    KDesktopFile config(loc, false, "mime");
    config.writeEntry("Type", "MimeType");
    config.writeEntry("MimeType", m_ptr->name());
    config.writeEntry("Hidden", true);
  }

  // now go through all entries and sync those which are dirty.
  // don't use typesLV, it may be filtered
  TQMapIterator<TQString,TypesListItem*> it1 = m_majorMap.begin();
  while ( it1 != m_majorMap.end() ) {
    TypesListItem *tli = *it1;
    if (tli->isDirty()) {
      kdDebug() << "Entry " << tli->name() << " is dirty. Saving." << endl;
      tli->sync();
      itemsModified.append( tli );
      didIt = true;
    }
    ++it1;
  }
  TQPtrListIterator<TypesListItem> it2( m_itemList );
  while ( it2.current() ) {
    TypesListItem *tli = *it2;
    if (tli->isDirty()) {
      kdDebug() << "Entry " << tli->name() << " is dirty. Saving." << endl;
      tli->sync();
      itemsModified.append( tli );
      didIt = true;
    }
    ++it2;
  }

  m_konqConfig->sync();

  setDirty(false);
  return didIt;
}

void FileTypesView::load()
{
    readFileTypes();
}

void FileTypesView::save()
{
  m_itemsModified.clear();
  if (sync(m_itemsModified)) {
    // only rebuild if sync() was necessary
    KService::rebuildKSycoca(this);
    KIPC::sendMessageAll(KIPC::SettingsChanged);
  }
}

void FileTypesView::slotDatabaseChanged()
{
  if ( KSycoca::self()->isChanged( "mime" ) )
  {
    // ksycoca has new KMimeTypes objects for us, make sure to update
    // our 'copies' to be in sync with it. Not important for OK, but
    // important for Apply (how to differentiate those 2?).
    // See BR 35071.
    TQValueList<TypesListItem *>::Iterator it = m_itemsModified.begin();
    for( ; it != m_itemsModified.end(); ++it ) {
        TQString name = (*it)->name();
        if ( removedList.find( name ) == removedList.end() ) // if not deleted meanwhile
            (*it)->refresh();
    }
    m_itemsModified.clear();
  }
}

void FileTypesView::defaults()
{
}

#include "filetypesview.moc"

