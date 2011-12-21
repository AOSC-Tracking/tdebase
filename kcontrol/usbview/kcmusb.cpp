/***************************************************************************
 *   Copyright (C) 2001 by Matthias Hoelzer-Kluepfel <mhk@caldera.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <tqgroupbox.h>
#include <tqheader.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqsplitter.h>
#include <tqtextview.h>
#include <tqtimer.h>

#include <kaboutdata.h>
#include <kdialog.h>
#include <kgenericfactory.h>

#include "usbdevices.h"
#include "kcmusb.moc"

typedef KGenericFactory<USBViewer, TQWidget > USBFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_usb, USBFactory("kcmusb") )

USBViewer::USBViewer(TQWidget *parent, const char *name, const TQStringList &)
  : KCModule(USBFactory::instance(), parent, name)
{
  setButtons(Help);

  setQuickHelp( i18n("<h1>USB Devices</h1> This module allows you to see"
     " the devices attached to your USB bus(es)."));

  TQVBoxLayout *vbox = new TQVBoxLayout(this, 0, KDialog::spacingHint());
  TQGroupBox *gbox = new TQGroupBox(i18n("USB Devices"), this);
  gbox->setColumnLayout( 0, Qt::Horizontal );
  vbox->addWidget(gbox);

  TQVBoxLayout *vvbox = new TQVBoxLayout(gbox->layout(), KDialog::spacingHint());

  TQSplitter *splitter = new TQSplitter(gbox);
  vvbox->addWidget(splitter);

  _devices = new TQListView(splitter);
  _devices->addColumn(i18n("Device"));
  _devices->setRootIsDecorated(true);
  _devices->header()->hide();
  _devices->setMinimumWidth(200);
  _devices->setColumnWidthMode(0, TQListView::Maximum);

  TQValueList<int> sizes;
  sizes.append(200);
  splitter->setSizes(sizes);

  _details = new TQTextView(splitter);

  splitter->setResizeMode(_devices, TQSplitter::KeepSize);

  TQTimer *refreshTimer = new TQTimer(this);
  // 1 sec seems to be a good compromise between latency and polling load.
  refreshTimer->start(1000);

  connect(refreshTimer, TQT_SIGNAL(timeout()), TQT_SLOT(refresh()));
  connect(_devices, TQT_SIGNAL(selectionChanged(TQListViewItem*)),
	  this, TQT_SLOT(selectionChanged(TQListViewItem*)));

  KAboutData *about =
  new KAboutData(I18N_NOOP("kcmusb"), I18N_NOOP("KDE USB Viewer"),
                0, 0, KAboutData::License_GPL,
                I18N_NOOP("(c) 2001 Matthias Hoelzer-Kluepfel"));

  about->addAuthor("Matthias Hoelzer-Kluepfel", 0, "mhk@kde.org");
  about->addCredit("Leo Savernik", "Live Monitoring of USB Bus", "l.savernik@aon.at");
  setAboutData( about );

  load();
}

void USBViewer::load()
{
  _items.clear();
  _devices->clear();

  refresh();
}

static TQ_UINT32 key( USBDevice &dev )
{
  return dev.bus()*256 + dev.device();
}

static TQ_UINT32 key_parent( USBDevice &dev )
{
  return dev.bus()*256 + dev.parent();
}

static void delete_recursive( TQListViewItem *item, const TQIntDict<TQListViewItem> &new_items )
{
  if (!item)
	return;

  TQListViewItemIterator it( item );
  while ( it.current() ) {
        if (!new_items.find(it.current()->text(1).toUInt())) {
		delete_recursive( it.current()->firstChild(), new_items);
		delete it.current();
	}
	++it;
  }
}

void USBViewer::refresh()
{
  TQIntDict<TQListViewItem> new_items;

  if (!USBDevice::parse("/proc/bus/usb/devices"))
    USBDevice::parseSys("/sys/bus/usb/devices");

  int level = 0;
  bool found = true;

  while (found)
    {
      found = false;

      TQPtrListIterator<USBDevice> it(USBDevice::devices());
      for ( ; it.current(); ++it)
	if (it.current()->level() == level)
	  {
	    TQ_UINT32 k = key(*it.current());
	    if (level == 0)
	      {
		TQListViewItem *item = _items.find(k);
		if (!item) {
		    item = new TQListViewItem(_devices,
				it.current()->product(),
				TQString::number(k));
		}
		new_items.insert(k, item);
		found = true;
	      }
	    else
	      {
		TQListViewItem *parent = new_items.find(key_parent(*it.current()));
		if (parent)
		  {
		    TQListViewItem *item = _items.find(k);

		    if (!item) {
		        item = new TQListViewItem(parent,
				    it.current()->product(),
				    TQString::number(k) );
		    }
		    new_items.insert(k, item);
		    parent->setOpen(true);
		    found = true;
		  }
	      }
	  }

      ++level;
    }

    // recursive delete all items not in new_items
    delete_recursive( _devices->firstChild(), new_items );

    _items = new_items;

    if (!_devices->selectedItem())
        selectionChanged(_devices->firstChild());
}


void USBViewer::selectionChanged(TQListViewItem *item)
{
  if (item)
    {
      TQ_UINT32 busdev = item->text(1).toUInt();
      USBDevice *dev = USBDevice::find(busdev>>8, busdev&255);
      if (dev)
	{
	  _details->setText(dev->dump());
	  return;
	}
    }
  _details->clear();
}


