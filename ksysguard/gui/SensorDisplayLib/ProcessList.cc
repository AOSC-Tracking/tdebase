/*
    KSysGuard, the KDE System Guard

    Copyright (C) 1997 Bernd Johannes Wuebben
                       <wuebben@math.cornell.edu>

    Copyright (C) 1998 Nicolas Leclercq <nicknet@planete.net>

    Copyright (c) 1999, 2000, 2001, 2002 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

	KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
	Please do not commit any changes without consulting me first. Thanks!

*/

#include <assert.h>
#include <config.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <tqbitmap.h>
#include <tqheader.h>
#include <tqimage.h>
#include <tqpopupmenu.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "ProcessController.h"
#include "ProcessList.h"
#include "ReniceDlg.h"
#include "SignalIDs.h"

#define NONE -1
#define INIT_PID 1

//extern const char* intKey(const char* text);
//extern const char* timeKey(const char* text);
//extern const char* floatKey(const char* text);

TQDict<TQString> ProcessList::aliases;

int ProcessLVI::compare( TQListViewItem *item, int col, bool ascending ) const
{
  int type = ((ProcessList*)listView())->columnType( col );

  if ( type == ProcessList::Int ) {
    int prev = (int)KGlobal::locale()->readNumber( key( col, ascending ) );
    int next = (int)KGlobal::locale()->readNumber( item->key( col, ascending ) );
    if ( prev < next )
      return -1;
    else if ( prev == next )
      return 0;
    else
      return 1;
  }

  if ( type == ProcessList::Float ) {
    double prev = KGlobal::locale()->readNumber( key( col, ascending ) );
    double next = KGlobal::locale()->readNumber( item->key( col, ascending ) );
    if ( prev < next )
      return -1;
    else
      return 1;
  }

  if ( type == ProcessList::Time ) {
    int hourPrev, hourNext, minutesPrev, minutesNext;
    sscanf( key( col, ascending ).latin1(), "%d:%d", &hourPrev, &minutesPrev );
    sscanf( item->key( col, ascending ).latin1(), "%d:%d", &hourNext, &minutesNext );
    int prev = hourPrev * 60 + minutesPrev;
    int next = hourNext * 60 + minutesNext;
    if ( prev < next )
      return -1;
    else if ( prev == next )
      return 0;
    else
      return 1;
  }

  return key( col, ascending ).localeAwareCompare( item->key( col, ascending ) );
}

ProcessList::ProcessList(TQWidget *parent, const char* name)
	: KListView(parent, name)
{
	iconCache.setAutoDelete(true);

	columnDict.setAutoDelete(true);
	columnDict.insert("running",
					  new TQString(i18n("process status", "running")));
	columnDict.insert("sleeping",
					  new TQString(i18n("process status", "sleeping")));
	columnDict.insert("disk sleep",
					  new TQString(i18n("process status", "disk sleep")));
	columnDict.insert("zombie", new TQString(i18n("process status", "zombie")));
	columnDict.insert("stopped",
					  new TQString(i18n("process status", "stopped")));
	columnDict.insert("paging", new TQString(i18n("process status", "paging")));
	columnDict.insert("idle", new TQString(i18n("process status", "idle")));

	if (aliases.isEmpty())
	{
#ifdef Q_OS_LINUX
		aliases.insert("init", new TQString("penguin"));
#else
		aliases.insert("init", new TQString("system"));
#endif
		/* kernel stuff */
		aliases.insert("bdflush", new TQString("kernel"));
		aliases.insert("dhcpcd", new TQString("kernel"));
		aliases.insert("kapm-idled", new TQString("kernel"));
		aliases.insert("keventd", new TQString("kernel"));
		aliases.insert("khubd", new TQString("kernel"));
		aliases.insert("klogd", new TQString("kernel"));
		aliases.insert("kreclaimd", new TQString("kernel"));
		aliases.insert("kreiserfsd", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU0", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU1", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU2", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU3", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU4", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU5", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU6", new TQString("kernel"));
		aliases.insert("ksoftirqd_CPU7", new TQString("kernel"));
		aliases.insert("kswapd", new TQString("kernel"));
		aliases.insert("kupdated", new TQString("kernel"));
		aliases.insert("mdrecoveryd", new TQString("kernel"));
		aliases.insert("scsi_eh_0", new TQString("kernel"));
		aliases.insert("scsi_eh_1", new TQString("kernel"));
		aliases.insert("scsi_eh_2", new TQString("kernel"));
		aliases.insert("scsi_eh_3", new TQString("kernel"));
		aliases.insert("scsi_eh_4", new TQString("kernel"));
		aliases.insert("scsi_eh_5", new TQString("kernel"));
		aliases.insert("scsi_eh_6", new TQString("kernel"));
		aliases.insert("scsi_eh_7", new TQString("kernel"));
		/* daemon and other service providers */
		aliases.insert("artsd", new TQString("daemon"));
		aliases.insert("atd", new TQString("daemon"));
		aliases.insert("automount", new TQString("daemon"));
		aliases.insert("cardmgr", new TQString("daemon"));
		aliases.insert("cron", new TQString("daemon"));
		aliases.insert("cupsd", new TQString("daemon"));
		aliases.insert("in.identd", new TQString("daemon"));
		aliases.insert("lpd", new TQString("daemon"));
		aliases.insert("mingetty", new TQString("daemon"));
		aliases.insert("nscd", new TQString("daemon"));
		aliases.insert("portmap", new TQString("daemon"));
		aliases.insert("rpc.statd", new TQString("daemon"));
		aliases.insert("rpciod", new TQString("daemon"));
		aliases.insert("sendmail", new TQString("daemon"));
		aliases.insert("sshd", new TQString("daemon"));
		aliases.insert("syslogd", new TQString("daemon"));
		aliases.insert("usbmgr", new TQString("daemon"));
		aliases.insert("wwwoffled", new TQString("daemon"));
		aliases.insert("xntpd", new TQString("daemon"));
		aliases.insert("ypbind", new TQString("daemon"));
		/* kde applications */
		aliases.insert("appletproxy", new TQString("kdeapp"));
		aliases.insert("dcopserver", new TQString("kdeapp"));
		aliases.insert("kcookiejar", new TQString("kdeapp"));
		aliases.insert("kde", new TQString("kdeapp"));
		aliases.insert("kded", new TQString("kdeapp"));
		aliases.insert("kdeinit", new TQString("kdeapp"));
		aliases.insert("kdesktop", new TQString("kdeapp"));
		aliases.insert("kdesud", new TQString("kdeapp"));
		aliases.insert("kdm", new TQString("kdeapp"));
		aliases.insert("khotkeys", new TQString("kdeapp"));
		aliases.insert("kio_file", new TQString("kdeapp"));
		aliases.insert("kio_uiserver", new TQString("kdeapp"));
		aliases.insert("klauncher", new TQString("kdeapp"));
		aliases.insert("ksmserver", new TQString("kdeapp"));
		aliases.insert("kwrapper", new TQString("kdeapp"));
		aliases.insert("kwrited", new TQString("kdeapp"));
		aliases.insert("kxmlrpcd", new TQString("kdeapp"));
		aliases.insert("startkde", new TQString("kdeapp"));
		/* other processes */
		aliases.insert("bash", new TQString("shell"));
		aliases.insert("cat", new TQString("tools"));
		aliases.insert("egrep", new TQString("tools"));
		aliases.insert("emacs", new TQString("wordprocessing"));
		aliases.insert("fgrep", new TQString("tools"));
		aliases.insert("find", new TQString("tools"));
		aliases.insert("grep", new TQString("tools"));
		aliases.insert("ksh", new TQString("shell"));
		aliases.insert("screen", new TQString("openterm"));
		aliases.insert("sh", new TQString("shell"));
		aliases.insert("sort", new TQString("tools"));
		aliases.insert("ssh", new TQString("shell"));
		aliases.insert("su", new TQString("tools"));
		aliases.insert("tcsh", new TQString("shell"));
		aliases.insert("tee", new TQString("tools"));
		aliases.insert("vi", new TQString("wordprocessing"));
	}

	/* The filter mode is controlled by a combo box of the parent. If
	 * the mode is changed we get a signal. */
	connect(parent, TQT_SIGNAL(setFilterMode(int)),
			this, TQT_SLOT(setFilterMode(int)));

	/* We need to catch this signal to show various popup menues. */
	connect(this,
			TQT_SIGNAL(rightButtonPressed(TQListViewItem*, const TQPoint&, int)),
			this,
			TQT_SLOT(handleRMBPressed(TQListViewItem*, const TQPoint&, int)));

	/* Since Qt does not tell us the sorting details we have to do our
	 * own bookkeping, so we can save and restore the sorting
	 * settings. */
	connect(header(), TQT_SIGNAL(clicked(int)), this, TQT_SLOT(sortingChanged(int)));

	treeViewEnabled = false;
	openAll = true;

	filterMode = FILTER_ALL;

	sortColumn = 1;
	increasing = false;

	// Elements in the process list may only live in this list.
	pl.setAutoDelete(true);

	setItemMargin(2);
	setAllColumnsShowFocus(true);
	setTreeStepSize(17);
	setSorting(sortColumn, increasing);
	setSelectionMode(TQListView::Extended);

	// Create popup menu for RMB clicks on table header
	headerPM = new TQPopupMenu();
	headerPM->insertItem(i18n("Remove Column"), HEADER_REMOVE);
	headerPM->insertItem(i18n("Add Column"), HEADER_ADD);
	headerPM->insertItem(i18n("Help on Column"), HEADER_HELP);

	connect(header(), TQT_SIGNAL(sizeChange(int, int, int)),
			this, TQT_SLOT(sizeChanged(int, int, int)));
	connect(header(), TQT_SIGNAL(indexChange(int, int, int)),
			this, TQT_SLOT(indexChanged(int, int, int)));

	killSupported = false;
	setModified(false);
}

ProcessList::~ProcessList()
{
	delete(headerPM);
}

const TQValueList<int>&
ProcessList::getSelectedPIds()
{
	selectedPIds.clear();
	// iterate through all selected visible items of the listview
	TQListViewItemIterator it(this, TQListViewItemIterator::Visible | TQListViewItemIterator::Selected );
	for ( ; it.current(); ++it )
		selectedPIds.append(it.current()->text(1).toInt());

	return (selectedPIds);
}

const TQStringList&
ProcessList::getSelectedAsStrings()
{
	selectedAsStrings.clear();
	// iterate through all selected visible items of the listview
	TQListViewItemIterator it(this, TQListViewItemIterator::Visible | TQListViewItemIterator::Selected );
	TQString spaces;
	for ( ; it.current(); ++it ) {
		spaces.fill(TQChar(' '), 7 - it.current()->text(1).length());
		selectedAsStrings.append("(PID: " + it.current()->text(1) + ")" + spaces + " " + it.current()->text(0));
	}	

	return (selectedAsStrings);
}
bool
ProcessList::update(const TQString& list)
{
	/* Disable painting to avoid flickering effects,
	 * especially when in tree view mode.
	 * Ditto for the scrollbar. */
	setUpdatesEnabled(false);
	viewport()->setUpdatesEnabled(false);

	pl.clear();

	// Convert ps answer in a list of tokenized lines
	KSGRD::SensorTokenizer procs(list, '\n');
	for (unsigned int i = 0; i < procs.count(); i++)
	{
		KSGRD::SensorPSLine* line = new KSGRD::SensorPSLine(procs[i]);
		if (line->count() != (uint) columns())
		{
#if 0
			// This is needed for debugging only.
			kdDebug(1215) << list << endl;
			TQString l;
			for (uint j = 0; j < line->count(); j++)
				l += (*line)[j] + "|";
			kdDebug(1215) << "Incomplete ps line:" << l << endl;
#endif
			return (false);
		}
		else
			pl.append(line);
	}

	int currItemPos = itemPos(currentItem());
	int vpos = verticalScrollBar()->value();
	int hpos = horizontalScrollBar()->value();

	updateMetaInfo();

	clear();

	if (treeViewEnabled)
		buildTree();
	else
		buildList();

	TQListViewItemIterator it( this );
	while ( it.current() ) {
		if ( itemPos( it.current() ) == currItemPos ) {
			setCurrentItem( it.current() );
			break;
		}
		++it;
	}

	verticalScrollBar()->setValue(vpos);
	horizontalScrollBar()->setValue(hpos);

	// Re-enable painting, and force an update.
	setUpdatesEnabled(true);
	viewport()->setUpdatesEnabled(true);

	triggerUpdate();

	return (true);
}

void
ProcessList::setTreeView(bool tv)
{
	if (treeViewEnabled = tv)
	{
		savedWidth[0] = columnWidth(0);
		openAll = true;
	}
	else
	{
		/* In tree view the first column is wider than in list view mode.
		 * So we shrink it to 1 pixel. The next update will resize it again
		 * appropriately. */
		setColumnWidth(0, savedWidth[0]);
	}
	/* In tree view mode borders are added to the icons. So we have to clear
	 * the cache when we change the tree view mode. */
	iconCache.clear();
}

bool
ProcessList::load(TQDomElement& el)
{
	TQDomNodeList dnList = el.elementsByTagName("column");
	for (uint i = 0; i < dnList.count(); ++i)
	{
		TQDomElement lel = dnList.item(i).toElement();
		if (savedWidth.count() <= i)
			savedWidth.append(lel.attribute("savedWidth").toInt());
		else
			savedWidth[i] = lel.attribute("savedWidth").toInt();
		if (currentWidth.count() <= i)
			currentWidth.append(lel.attribute("currentWidth").toInt());
		else
			currentWidth[i] = lel.attribute("currentWidth").toInt();
		if (index.count() <= i)
			index.append(lel.attribute("index").toInt());
		else
			index[i] = lel.attribute("index").toInt();
	}

	setModified(false);

	return (true);
}

bool
ProcessList::save(TQDomDocument& doc, TQDomElement& display)
{
	for (int i = 0; i < columns(); ++i)
	{
		TQDomElement col = doc.createElement("column");
		display.appendChild(col);
		col.setAttribute("currentWidth", columnWidth(i));
		col.setAttribute("savedWidth", savedWidth[i]);
		col.setAttribute("index", header()->mapToIndex(i));
	}

	setModified(false);

	return (true);
}

void
ProcessList::sortingChanged(int col)
{
	if (col == sortColumn)
		increasing = !increasing;
	else
	{
		sortColumn = col;
		increasing = true;
	}
	setSorting(sortColumn, increasing);
	setModified(true);
}

int ProcessList::columnType( uint pos ) const
{
  if ( pos >= mColumnTypes.count() )
    return 0;

  if ( mColumnTypes[ pos ] == "d" || mColumnTypes[ pos ] == "D" )
    return Int;
  else if ( mColumnTypes[ pos ] == "f" || mColumnTypes[ pos ] == "F" )
    return Float;
  else if ( mColumnTypes[ pos ] == "t" )
    return Time;
  else
    return Text;
}

bool
ProcessList::matchesFilter(KSGRD::SensorPSLine* p) const
{
	// This mechanism is likely to change in the future!

	switch (filterMode)
	{
	case FILTER_ALL:
		return (true);

	case FILTER_SYSTEM:
		return (p->uid() < 100 ? true : false);

	case FILTER_USER:
		return (p->uid() >= 100 ? true : false);

	case FILTER_OWN:
	default:
		return (p->uid() == (long) getuid() ? true : false);
	}
}

void
ProcessList::buildList()
{
	/* Get the first process in the list, check whether it matches the
	 * filter and append it to TQListView widget if so. */
	while (!pl.isEmpty())
	{
		KSGRD::SensorPSLine* p = pl.first();

		if (matchesFilter(p))
		{
			ProcessLVI* pli = new ProcessLVI(this);

			addProcess(p, pli);

			if (selectedPIds.findIndex(p->pid()) != -1)
				pli->setSelected(true);
		}
		pl.removeFirst();
	}
}

void
ProcessList::buildTree()
{
	// remove all leaves that do not match the filter
	deleteLeaves();

	KSGRD::SensorPSLine* ps = pl.first();

	while (ps)
	{
		if (ps->pid() == INIT_PID)
		{
			// insert root item into the tree widget
			ProcessLVI* pli = new ProcessLVI(this);
			addProcess(ps, pli);

			// remove the process from the process list, ps is now invalid
			int pid = ps->pid();
			pl.remove();

			if (selectedPIds.findIndex(pid) != -1)
				pli->setSelected(true);

			// insert all child processes of current process
			extendTree(&pl, pli, pid);
			break;
		}
		else
			ps = pl.next();
	}
}

void
ProcessList::deleteLeaves(void)
{
	for ( ; ; )
	{
		unsigned int i;
		for (i = 0; i < pl.count() &&
				 (!isLeafProcess(pl.at(i)->pid()) ||
				  matchesFilter(pl.at(i))); i++)
			;
		if (i == pl.count())
			return;

		pl.remove(i);
	}
}

bool
ProcessList::isLeafProcess(int pid)
{
	for (unsigned int i = 0; i < pl.count(); i++)
		if (pl.at(i)->ppid() == pid)
			return (false);

	return (true);
}

void
ProcessList::extendTree(TQPtrList<KSGRD::SensorPSLine>* pl, ProcessLVI* parent, int ppid)
{
	KSGRD::SensorPSLine* ps;

	// start at top list
	ps = pl->first();

	while (ps)
	{
		// look for a child process of the current parent
		if (ps->ppid() == ppid)
		{
			ProcessLVI* pli = new ProcessLVI(parent);

			addProcess(ps, pli);

			if (selectedPIds.findIndex(ps->pid()) != -1)
				pli->setSelected(true);

			if (ps->ppid() != INIT_PID && closedSubTrees.findIndex(ps->ppid()) != -1)
				parent->setOpen(false);
			else
				parent->setOpen(true);

			// remove the process from the process list, ps is now invalid
			int pid = ps->pid();
			pl->remove();

			// now look for the childs of the inserted process
			extendTree(pl, pli, pid);

			/* Since buildTree can remove processes from the list we
			 * can't find a "current" process. So we start searching
			 * at the top again. It's no endless loops since this
			 * branch is only entered when there are children of the
			 * current parent in the list. When we have removed them
			 * all the while loop will exit. */
			ps = pl->first();
		}
		else
			ps = pl->next();
	}
}
void
ProcessList::addProcess(KSGRD::SensorPSLine* p, ProcessLVI* pli)
{
	TQString name = p->name();
	if (aliases[name])
		name = *aliases[name];

	/* Get icon from icon list that might be appropriate for a process
	 * with this name. */
	TQPixmap pix;
	if (!iconCache[name])
	{
		pix = KGlobal::iconLoader()->loadIcon(name, KIcon::Small,
							  KIcon::SizeSmall, KIcon::DefaultState,
							  0L, true);
		if (pix.isNull() || !pix.mask())
			pix = KGlobal::iconLoader()->loadIcon("unknownapp", KIcon::User,
								  KIcon::SizeSmall);

		if (pix.width() != 16 || pix.height() != 16)
		{
			/* I guess this isn't needed too often. The KIconLoader should
			 * scale the pixmaps already appropriately. Since I got a bug
			 * report claiming that it doesn't work with GNOME apps I've
			 * added this safeguard. */
			TQImage img;
			img = pix;
			img.smoothScale(16, 16);
			pix = img;
		}
		/* We copy the icon into a 24x16 pixmap to add a 4 pixel margin on
		 * the left and right side. In tree view mode we use the original
		 * icon. */
		TQPixmap icon(24, 16, pix.depth());
		if (!treeViewEnabled)
		{
			icon.fill();
			bitBlt(&icon, 4, 0, &pix, 0, 0, pix.width(), pix.height());
			TQBitmap mask(24, 16, true);
			bitBlt(&mask, 4, 0, pix.mask(), 0, 0, pix.width(), pix.height());
			icon.setMask(mask);
			pix = icon;
		}
		iconCache.insert(name, new TQPixmap(pix));
	}
	else
		pix = *(iconCache[name]);

	// icon + process name
	pli->setPixmap(0, pix);
	pli->setText(0, p->name());

	// insert remaining field into table
	for (unsigned int col = 1; col < p->count(); col++)
	{
		if (mColumnTypes[col] == "S" && columnDict[(*p)[col]])
			pli->setText(col, *columnDict[(*p)[col]]);
		else if ( mColumnTypes[col] == "f" )
      pli->setText( col, KGlobal::locale()->formatNumber( (*p)[col].toFloat() ) );
    else if ( mColumnTypes[col] == "D" )
      pli->setText( col, KGlobal::locale()->formatNumber( (*p)[col].toInt(), 0 ) );
    else
			pli->setText(col, (*p)[col]);
	}
}

void
ProcessList::updateMetaInfo(void)
{
	selectedPIds.clear();
	closedSubTrees.clear();

	TQListViewItemIterator it(this);

	// iterate through all items of the listview
	for ( ; it.current(); ++it )
	{
		if (it.current()->isSelected() && it.current()->isVisible())
			selectedPIds.append(it.current()->text(1).toInt());
		if (treeViewEnabled && !it.current()->isOpen())
			closedSubTrees.append(it.current()->text(1).toInt());
	}

	/* In list view mode all list items are set to closed by TQListView.
	 * If the tree view is now selected, all item will be closed. This is
	 * annoying. So we use the openAll flag to force all trees to open when
	 * the treeViewEnbled flag was set to true. */
	if (openAll)
	{
		if (treeViewEnabled)
			closedSubTrees.clear();
		openAll = false;
	}
}

void
ProcessList::removeColumns(void)
{
	for (int i = columns() - 1; i >= 0; --i)
		removeColumn(i);
}

void
ProcessList::addColumn(const TQString& label, const TQString& type)
{
	TQListView::addColumn(label);
	uint col = columns() - 1;
	if (type == "s" || type == "S")
		setColumnAlignment(col, AlignLeft);
	else if (type == "d" || type == "D")
		setColumnAlignment(col, AlignRight);
	else if (type == "t")
		setColumnAlignment(col, AlignRight);
	else if (type == "f")
		setColumnAlignment(col, AlignRight);
	else
	{
		kdDebug(1215) << "Unknown type " << type << " of column " << label
				  << " in ProcessList!" << endl;
		return;
	}

	mColumnTypes.append(type);

	/* Just use some sensible default values as initial setting. */
	TQFontMetrics fm = fontMetrics();
	setColumnWidth(col, fm.width(label) + 10);

	if (currentWidth.count() - 1 == col)
	{
		/* Table has been loaded from file. We can restore the settings
		 * when the last column has been added. */
		for (uint i = 0; i < col; ++i)
		{
			/* In case the language has been changed the column width
			 * might need to be increased. */
			if (currentWidth[i] == 0)
			{
				if (fm.width(header()->label(i)) + 10 > savedWidth[i])
					savedWidth[i] = fm.width(header()->label(i)) + 10;
				setColumnWidth(i, 0);
			}
			else
			{
				if (fm.width(header()->label(i)) + 10 > currentWidth[i])
					setColumnWidth(i, fm.width(header()->label(i)) + 10);
				else
					setColumnWidth(i, currentWidth[i]);
			}
			setColumnWidthMode(i, currentWidth[i] == 0 ?
							   TQListView::Manual : TQListView::Maximum);
			header()->moveSection(i, index[i]);
		}
		setSorting(sortColumn, increasing);
	}
}

void
ProcessList::handleRMBPressed(TQListViewItem* lvi, const TQPoint& p, int col)
{
	if (!lvi)
		return;

  lvi->setSelected( true );

	/* lvi is only valid until the next time we hit the main event
	 * loop. So we need to save the information we need after calling
	 * processPM->exec(). */
	int currentPId = lvi->text(1).toInt();

	int currentNiceValue = 0;
	for (int i = 0; i < columns(); ++i)
		if (TQString::compare(header()->label(i), i18n("Nice")) == 0)
			currentNiceValue = lvi->text(i).toInt();

	TQPopupMenu processPM;
  if (columnWidth(col) != 0)
  	processPM.insertItem(i18n("Hide Column"), 5);
	TQPopupMenu* hiddenPM = new TQPopupMenu(&processPM);
	for (int i = 0; i < columns(); ++i)
		if (columnWidth(i) == 0)
			hiddenPM->insertItem(header()->label(i), i + 100);
	if(columns())
	  processPM.insertItem(i18n("Show Column"), hiddenPM);

	processPM.insertSeparator();

	processPM.insertItem(i18n("Select All Processes"), 1);
	processPM.insertItem(i18n("Unselect All Processes"), 2);

	TQPopupMenu* signalPM = new TQPopupMenu(&processPM);
	if (killSupported && lvi->isSelected())
	{
		processPM.insertSeparator();
		processPM.insertItem(i18n("Select All Child Processes"), 3);
		processPM.insertItem(i18n("Unselect All Child Processes"), 4);

		signalPM->insertItem(i18n("SIGABRT"), MENU_ID_SIGABRT);
		signalPM->insertItem(i18n("SIGALRM"), MENU_ID_SIGALRM);
		signalPM->insertItem(i18n("SIGCHLD"), MENU_ID_SIGCHLD);
		signalPM->insertItem(i18n("SIGCONT"), MENU_ID_SIGCONT);
		signalPM->insertItem(i18n("SIGFPE"), MENU_ID_SIGFPE);
		signalPM->insertItem(i18n("SIGHUP"), MENU_ID_SIGHUP);
		signalPM->insertItem(i18n("SIGILL"), MENU_ID_SIGILL);
		signalPM->insertItem(i18n("SIGINT"), MENU_ID_SIGINT);
		signalPM->insertItem(i18n("SIGKILL"), MENU_ID_SIGKILL);
		signalPM->insertItem(i18n("SIGPIPE"), MENU_ID_SIGPIPE);
		signalPM->insertItem(i18n("SIGQUIT"), MENU_ID_SIGQUIT);
		signalPM->insertItem(i18n("SIGSEGV"), MENU_ID_SIGSEGV);
		signalPM->insertItem(i18n("SIGSTOP"), MENU_ID_SIGSTOP);
		signalPM->insertItem(i18n("SIGTERM"), MENU_ID_SIGTERM);
		signalPM->insertItem(i18n("SIGTSTP"), MENU_ID_SIGTSTP);
		signalPM->insertItem(i18n("SIGTTIN"), MENU_ID_SIGTTIN);
		signalPM->insertItem(i18n("SIGTTOU"), MENU_ID_SIGTTOU);
		signalPM->insertItem(i18n("SIGUSR1"), MENU_ID_SIGUSR1);
		signalPM->insertItem(i18n("SIGUSR2"), MENU_ID_SIGUSR2);

		processPM.insertSeparator();
		processPM.insertItem(i18n("Send Signal"), signalPM);
	}

	/* differ between killSupported and reniceSupported in a future
	 * version. */
	if (killSupported && lvi->isSelected())
	{
		processPM.insertSeparator();
		processPM.insertItem(i18n("Renice Process..."), 300);
	}

	int id;
	switch (id = processPM.exec(p))
	{
	case -1:
		break;
	case 1:
	case 2:
		selectAllItems(id & 1);
		break;
	case 3:
	case 4:
		selectAllChilds(currentPId, id & 1);
		break;
	case 5:
		setColumnWidthMode(col, TQListView::Manual);
		savedWidth[col] = columnWidth(col);
		setColumnWidth(col, 0);
		setModified(true);
		break;
	case 300:
		{
		ReniceDlg reniceDlg(this, "reniceDlg", currentNiceValue, currentPId);

		int reniceVal;
		if ((reniceVal = reniceDlg.exec()) != 40) {
			emit reniceProcess(selectedPIds, reniceVal);
		}
		}
		break;
	default:
		/* IDs < 100 are used for signals. */
		if (id < 100)
		{
			/* we go through list to get all task also
			   when update interval is paused */
			selectedPIds.clear();
			TQListViewItemIterator it(this, TQListViewItemIterator::Visible | TQListViewItemIterator::Selected);

			// iterate through all selected visible items of the listview
			for ( ; it.current(); ++it )
			{
				selectedPIds.append(it.current()->text(1).toInt());
			}

			TQString msg = i18n("Do you really want to send signal %1 to the selected process?", 
					"Do you really want to send signal %1 to the %n selected processes?",
					selectedPIds.count())
				.arg(signalPM->text(id));
			int answ;
			switch(answ = KMessageBox::questionYesNo(this, msg, TQString::null, i18n("Send"), KStdGuiItem::cancel()))
			{
			case KMessageBox::Yes:
			{
				TQValueList<int>::Iterator it;
				for (it = selectedPIds.begin(); it != selectedPIds.end(); ++it)
					emit (killProcess(*it, id));
				break;
			}
			default:
				break;
			}
		}
		else
		{
			/* IDs >= 100 are used for hidden columns. */
			int col = id - 100;
			setColumnWidthMode(col, TQListView::Maximum);
			setColumnWidth(col, savedWidth[col]);
			setModified(true);
		}
	}
}

void
ProcessList::selectAllItems(bool select)
{
	selectedPIds.clear();

	TQListViewItemIterator it(this, TQListViewItemIterator::Visible);

	// iterate through all items of the listview
	for ( ; it.current(); ++it )
	{
		it.current()->setSelected(select);
		repaintItem(it.current());
		if (select)
			selectedPIds.append(it.current()->text(1).toInt());
	}
}

void
ProcessList::selectAllChilds(int pid, bool select)
{
	TQListViewItemIterator it(this, TQListViewItemIterator::Visible );

	// iterate through all items of the listview
	for ( ; it.current(); ++it )
	{
		// Check if PPID matches the pid (current is a child of pid)
		if (it.current()->text(2).toInt() == pid)
		{
			int currPId = it.current()->text(1).toInt();
			it.current()->setSelected(select);
			repaintItem(it.current());
			if (select)
				selectedPIds.append(currPId);
			else
				selectedPIds.remove(currPId);
			selectAllChilds(currPId, select);
		}
	}
}

#include "ProcessList.moc"
