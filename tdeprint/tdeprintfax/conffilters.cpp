/*
 *   tdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "conffilters.h"
#include "filterdlg.h"

#include <tqpushbutton.h>
#include <tqlayout.h>
#include <tqtooltip.h>
#include <tqfile.h>
#include <tqtextstream.h>
#include <tqregexp.h>
#include <tqheader.h>

#include <tdelocale.h>
#include <tdelistview.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <tdemessagebox.h>

ConfFilters::ConfFilters(TQWidget *parent, const char *name)
: TQWidget(parent, name)
{
	m_filters = new TDEListView(this);
	m_filters->addColumn(i18n("Mime Type"));
	m_filters->addColumn(i18n("Command"));
	m_filters->setFrameStyle(TQFrame::WinPanel|TQFrame::Sunken);
	m_filters->setLineWidth(1);
	m_filters->setSorting(-1);
	m_filters->header()->setStretchEnabled(true, 1);
	connect(m_filters, TQ_SIGNAL(doubleClicked(TQListViewItem*)), TQ_SLOT(slotChange()));

	m_add = new TQPushButton(this);
	m_add->setPixmap(BarIcon("document-new"));
	m_remove = new TQPushButton(this);
	m_remove->setIconSet(BarIconSet("remove"));
	m_change = new TQPushButton(this);
	m_change->setIconSet(BarIconSet("filter"));
	m_up = new TQPushButton(this);
	m_up->setIconSet(BarIconSet("go-up"));
	m_down = new TQPushButton(this);
	m_down->setIconSet(BarIconSet("go-down"));
	connect(m_add, TQ_SIGNAL(clicked()), TQ_SLOT(slotAdd()));
	connect(m_change, TQ_SIGNAL(clicked()), TQ_SLOT(slotChange()));
	connect(m_remove, TQ_SIGNAL(clicked()), TQ_SLOT(slotRemove()));
	connect(m_up, TQ_SIGNAL(clicked()), TQ_SLOT(slotUp()));
	connect(m_down, TQ_SIGNAL(clicked()), TQ_SLOT(slotDown()));
	TQToolTip::add(m_add, i18n("Add filter"));
	TQToolTip::add(m_change, i18n("Modify filter"));
	TQToolTip::add(m_remove, i18n("Remove filter"));
	TQToolTip::add(m_up, i18n("Move filter up"));
	TQToolTip::add(m_down, i18n("Move filter down"));

	TQHBoxLayout	*l0 = new TQHBoxLayout(this, 10, 10);
	TQVBoxLayout	*l1 = new TQVBoxLayout(0, 0, 0);
	l0->addWidget(m_filters, 1);
	l0->addLayout(l1, 0);
	l1->addWidget(m_add);
	l1->addWidget(m_change);
	l1->addWidget(m_remove);
	l1->addSpacing(10);
	l1->addWidget(m_up);
	l1->addWidget(m_down);
	l1->addStretch(1);
	updateButton();
	connect(m_filters, TQ_SIGNAL(selectionChanged ()),TQ_SLOT(updateButton()));
}

void ConfFilters::load()
{
	TQFile	f(locate("data","tdeprintfax/faxfilters"));
	if (f.exists() && f.open(IO_ReadOnly))
	{
		TQTextStream	t(&f);
		TQString		line;
		int		p(-1);
		TQListViewItem	*item(0);
		while (!t.eof())
		{
			line = t.readLine().stripWhiteSpace();
			if ((p=line.find(TQRegExp("\\s"))) != -1)
			{
				TQString	mime(line.left(p)), cmd(line.right(line.length()-p-1).stripWhiteSpace());
				if (!mime.isEmpty() && !cmd.isEmpty())
					item = new TQListViewItem(m_filters, item, mime, cmd);
			}
		}
	}
}

void ConfFilters::save()
{
	TQListViewItem	*item = m_filters->firstChild();
	TQFile	f(locateLocal("data","tdeprintfax/faxfilters"));
	if (f.open(IO_WriteOnly))
	{
		TQTextStream	t(&f);
		while (item)
		{
			t << item->text(0) << ' ' << item->text(1) << endl;
			item = item->nextSibling();
		}
	}
}

void ConfFilters::slotAdd()
{
	TQString	mime, cmd;
	if (FilterDlg::doIt(this, &mime, &cmd))
		if (!mime.isEmpty() && !cmd.isEmpty())
		  {
		    new TQListViewItem(m_filters, m_filters->currentItem(), mime, cmd);
		    updateButton();
		  }
		else
			KMessageBox::error(this, i18n("Empty parameters."));
}

void ConfFilters::slotRemove()
{
	TQListViewItem	*item = m_filters->currentItem();
	if (item)
		delete item;
	updateButton();
}

void ConfFilters::slotChange()
{
	TQListViewItem	*item = m_filters->currentItem();
	if (item)
	{
		TQString	mime(item->text(0)), cmd(item->text(1));
		if (FilterDlg::doIt(this, &mime, &cmd))
		{
			item->setText(0, mime);
			item->setText(1, cmd);
		}
	}
}

void ConfFilters::slotUp()
{
	TQListViewItem	*item = m_filters->currentItem();
	if (item && item->itemAbove())
	{
		m_filters->moveItem(item, 0, item->itemAbove()->itemAbove());
		m_filters->setCurrentItem(item);
		updateButton();
	}
}

void ConfFilters::slotDown()
{
	TQListViewItem	*item = m_filters->currentItem();
	if (item && item->itemBelow())
	{
		m_filters->moveItem(item, 0, item->itemBelow());
		m_filters->setCurrentItem(item);
		updateButton();
	}
}

void ConfFilters::updateButton()
{
  TQListViewItem	*item = m_filters->currentItem();

  bool state=item && item->itemBelow();
  m_remove->setEnabled(item);
  m_down->setEnabled(state);
  state=item && item->itemAbove();
  m_up->setEnabled(state);
  m_change->setEnabled(item);
}

#include "conffilters.moc"
