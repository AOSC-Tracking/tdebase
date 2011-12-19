/*
    KSysGuard, the KDE System Guard

	Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms version 2 of of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

	KSysGuard is currently maintained by Chris Schlaeger <cs@kde.org>.
	Please do not commit any changes without consulting me first. Thanks!

*/

#include <assert.h>

#include <tqtimer.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdialogbase.h>
#include <klistviewsearchline.h>

#include <ksgrd/SensorManager.h>

#include "ProcessController.moc"
#include "SignalIDs.h"

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqgroupbox.h>
#include <tqlayout.h>

#include <kapplication.h>
#include <kpushbutton.h>



ProcessController::ProcessController(TQWidget* parent, const char* name, const TQString &title, bool nf)
	: KSGRD::SensorDisplay(parent, name, title, nf)
{
	dict.setAutoDelete(true);
	dict.insert("Name", new TQString(i18n("Name")));
	dict.insert("PID", new TQString(i18n("PID")));
	dict.insert("PPID", new TQString(i18n("PPID")));
	dict.insert("UID", new TQString(i18n("UID")));
	dict.insert("GID", new TQString(i18n("GID")));
	dict.insert("Status", new TQString(i18n("Status")));
	dict.insert("User%", new TQString(i18n("User%")));
	dict.insert("System%", new TQString(i18n("System%")));
	dict.insert("Nice", new TQString(i18n("Nice")));
	dict.insert("VmSize", new TQString(i18n("VmSize")));
	dict.insert("VmRss", new TQString(i18n("VmRss")));
	dict.insert("Login", new TQString(i18n("Login")));
	dict.insert("Command", new TQString(i18n("Command")));

	// Setup the geometry management.
	gm = new TQVBoxLayout(this, 10);
	Q_CHECK_PTR(gm);
	gm->addSpacing(15);

	gmSearch = new TQHBoxLayout();
	Q_CHECK_PTR(gmSearch);
	gm->addLayout(gmSearch, 0);

	// Create the table that lists the processes.
	pList = new ProcessList(this, "pList");
	Q_CHECK_PTR(pList);
	pList->setShowSortIndicator(true);
	pListSearchLine = new KListViewSearchLineWidget(pList, this, "process_list_search_line");
	gmSearch->addWidget(pListSearchLine, 1);

	connect(pList, TQT_SIGNAL(killProcess(int, int)),
			this, TQT_SLOT(killProcess(int, int)));
	connect(pList, TQT_SIGNAL(reniceProcess(const TQValueList<int> &, int)),
			this, TQT_SLOT(reniceProcess(const TQValueList<int> &, int)));
	connect(pList, TQT_SIGNAL(listModified(bool)),
			this, TQT_SLOT(setModified(bool)));

	/* Create the combo box to configure the process filter. The
	 * cbFilter must be created prior to constructing pList as the
	 * pList constructor sets cbFilter to its start value. */
	cbFilter = new TQComboBox(this, "pList_cbFilter");
	Q_CHECK_PTR(cbFilter);
	gmSearch->addWidget(cbFilter,0);
	cbFilter->insertItem(i18n("All Processes"), 0);
	cbFilter->insertItem(i18n("System Processes"), 1);
	cbFilter->insertItem(i18n("User Processes"), 2);
	cbFilter->insertItem(i18n("Own Processes"), 3);
	cbFilter->setMinimumSize(cbFilter->sizeHint());
	// Create the check box to switch between tree view and list view.
	xbTreeView = new TQCheckBox(i18n("&Tree"), this, "xbTreeView");
	Q_CHECK_PTR(xbTreeView);
	xbTreeView->setMinimumSize(xbTreeView->sizeHint());
	connect(xbTreeView, TQT_SIGNAL(toggled(bool)),
			this, TQT_SLOT(setTreeView(bool)));


	/* When the both cbFilter and pList are constructed we can connect the
	 * missing link. */
	connect(cbFilter, TQT_SIGNAL(activated(int)),
			this, TQT_SLOT(filterModeChanged(int)));

	// Create the 'Refresh' button.
	bRefresh = new KPushButton( KGuiItem(  i18n( "&Refresh" ), "reload" ),
            this, "bRefresh" );
	Q_CHECK_PTR(bRefresh);
	bRefresh->setMinimumSize(bRefresh->sizeHint());
	connect(bRefresh, TQT_SIGNAL(clicked()), this, TQT_SLOT(updateList()));

	// Create the 'Kill' button.
	bKill = new KPushButton(i18n("&Kill"), this, "bKill");
	Q_CHECK_PTR(bKill);
	bKill->setMinimumSize(bKill->sizeHint());
	connect(bKill, TQT_SIGNAL(clicked()), this, TQT_SLOT(killProcess()));
	/* Disable the kill button until we know that the daemon supports the
	 * kill command. */
	bKill->setEnabled(false);
	killSupported = false;

	gm->addWidget(pList, 1);

	gm1 = new TQHBoxLayout();
	Q_CHECK_PTR(gm1);
	gm->addLayout(gm1, 0);
	gm1->addStretch();
	gm1->addWidget(xbTreeView);
	gm1->addStretch();
	gm1->addWidget(bRefresh);
	gm1->addStretch();
	gm1->addWidget(bKill);
	gm1->addStretch();
	gm->addSpacing(5);

	gm->activate();

	setPlotterWidget(pList);

	setMinimumSize(sizeHint());
	fixTabOrder(); 
}

void ProcessController::setSearchFocus() {
	//stupid search line widget.  See rant in fixTabOrder
	if(!pListSearchLine->searchLine())
		TQTimer::singleShot(100, this, TQT_SLOT(setSearchFocus()));
	else {
		pListSearchLine->searchLine()->setFocus();
	}
}
void ProcessController::fixTabOrder() {

	//Wow, I hate this search line widget so much.
	//It creates the searchline in a singleshot timer.  This makes it totally unpredictable when searchLine is actually valid.
	//So we set up singleshot timer and call ourselves over and over until it's ready.
	//
	//Did i mention I hate this?
	if(!pListSearchLine->searchLine())
		TQTimer::singleShot(100, this, TQT_SLOT(fixTabOrder()));
	else {
		setTabOrder(pListSearchLine->searchLine(), cbFilter);
		setTabOrder(cbFilter, pList);
		setTabOrder(pList, xbTreeView);
		setTabOrder(xbTreeView, bRefresh);
		setTabOrder(bRefresh, bKill);
	}
}

void
ProcessController::resizeEvent(TQResizeEvent* ev)
{
	if(frame())
		frame()->setGeometry(0, 0, width(), height());

	TQWidget::resizeEvent(ev);
}

bool
ProcessController::addSensor(const TQString& hostName,
							 const TQString& sensorName,
							const TQString& sensorType,
							 const TQString& title)
{
	if (sensorType != "table")
		return (false);

	registerSensor(new KSGRD::SensorProperties(hostName, sensorName, sensorType, title));
	/* This just triggers the first communication. The full set of
	 * requests are send whenever the sensor reconnects (detected in
	 * sensorError(). */

	sendRequest(hostName, "test kill", 4);

	if (title.isEmpty())
		setTitle(i18n("%1: Running Processes").arg(hostName));
	else
		setTitle(title);

	return (true);
}

void
ProcessController::updateList()
{
	sendRequest(sensors().at(0)->hostName(), "ps", 2);
}

void
ProcessController::killProcess(int pid, int sig)
{
	sendRequest(sensors().at(0)->hostName(),
				TQString("kill %1 %2" ).arg(pid).arg(sig), 3);

	if ( !timerOn() )
	    // give ksysguardd time to update its proccess list
	    TQTimer::singleShot(3000, this, TQT_SLOT(updateList()));
	else
	    updateList();
}

void
ProcessController::killProcess()
{
	const TQStringList& selectedAsStrings = pList->getSelectedAsStrings();
	if (selectedAsStrings.isEmpty())
	{
		KMessageBox::sorry(this,
						   i18n("You need to select a process first."));
		return;
	}
	else
	{
		TQString  msg = i18n("Do you want to kill the selected process?",
				"Do you want to kill the %n selected processes?",
				selectedAsStrings.count());

		KDialogBase *dlg = new KDialogBase (  i18n ("Kill Process"), 
						      KDialogBase::Yes | KDialogBase::Cancel,
						      KDialogBase::Yes, KDialogBase::Cancel, this->parentWidget(),
						      "killconfirmation",
			       			      true, true, KGuiItem(i18n("Kill")));

		bool dontAgain = false;
		
		int res = KMessageBox::createKMessageBox(dlg, TQMessageBox::Question,
			                                 msg, selectedAsStrings,
							 i18n("Do not ask again"), &dontAgain, 
							 KMessageBox::Notify);

		if (res != KDialogBase::Yes)
		{
			return;
		}
	}

	const TQValueList<int>& selectedPIds = pList->getSelectedPIds();

	// send kill signal to all seleted processes
	TQValueListConstIterator<int> it;
	for (it = selectedPIds.begin(); it != selectedPIds.end(); ++it)
		sendRequest(sensors().at(0)->hostName(), TQString("kill %1 %2" ).arg(*it)
					.arg(MENU_ID_SIGKILL), 3);

	if ( !timerOn())
		// give ksysguardd time to update its proccess list
		TQTimer::singleShot(3000, this, TQT_SLOT(updateList()));
	else
		updateList();
}

void
ProcessController::reniceProcess(const TQValueList<int> &pids, int niceValue)
{
	for( TQValueList<int>::ConstIterator it = pids.constBegin(), end = pids.constEnd(); it != end; ++it )
		sendRequest(sensors().at(0)->hostName(),
					TQString("setpriority %1 %2" ).arg(*it).arg(niceValue), 5);
	sendRequest(sensors().at(0)->hostName(), "ps", 2);  //update the display afterwards
}

void
ProcessController::answerReceived(int id, const TQString& answer)
{
	/* We received something, so the sensor is probably ok. */
	sensorError(id, false);

	switch (id)
	{
	case 1:
	{
		/* We have received the answer to a ps? command that contains
		 * the information about the table headers. */
		KSGRD::SensorTokenizer lines(answer, '\n');
		if (lines.count() != 2)
		{
			kdDebug (1215) << "ProcessController::answerReceived(1)"
				  "wrong number of lines [" <<  answer << "]" << endl;
			sensorError(id, true);
			return;
		}
		KSGRD::SensorTokenizer headers(lines[0], '\t');
		KSGRD::SensorTokenizer colTypes(lines[1], '\t');

		pList->removeColumns();
		for (unsigned int i = 0; i < headers.count(); i++)
		{
			TQString header;
			if (dict[headers[i]])
				header = *dict[headers[i]];
			else
				header = headers[i];
			pList->addColumn(header, colTypes[i]);
		}

		break;
	}
	case 2:
		/* We have received the answer to a ps command that contains a
		 * list of processes with various additional information. */
		pList->update(answer);
		pListSearchLine->searchLine()->updateSearch(); //re-apply the filter
		break;
	case 3:
	{
		// result of kill operation
		kdDebug(1215) << answer << endl;
		KSGRD::SensorTokenizer vals(answer, '\t');
		switch (vals[0].toInt())
		{
		case 0:	// successful kill operation
			break;
		case 1:	// unknown error
			KSGRD::SensorMgr->notify(
				i18n("Error while attempting to kill process %1.")
				.arg(vals[1]));
			break;
		case 2:
			KSGRD::SensorMgr->notify(
				i18n("Insufficient permissions to kill "
							 "process %1.").arg(vals[1]));
			break;
		case 3:
			KSGRD::SensorMgr->notify(
				i18n("Process %1 has already disappeared.")
				.arg(vals[1]));
			break;
		case 4:
			KSGRD::SensorMgr->notify(i18n("Invalid Signal."));
			break;
		}
		break;
	}
	case 4:
		killSupported = (answer.toInt() == 1);
		pList->setKillSupported(killSupported);
		bKill->setEnabled(killSupported);
		break;
	case 5:
	{
		// result of renice operation
		kdDebug(1215) << answer << endl;
		KSGRD::SensorTokenizer vals(answer, '\t');
		switch (vals[0].toInt())
		{
		case 0:	// successful renice operation
			break;
		case 1:	// unknown error
			KSGRD::SensorMgr->notify(
				i18n("Error while attempting to renice process %1.")
				.arg(vals[1]));
			break;
		case 2:
			KSGRD::SensorMgr->notify(
				i18n("Insufficient permissions to renice "
							 "process %1.").arg(vals[1]));
			break;
		case 3:
			KSGRD::SensorMgr->notify(
				i18n("Process %1 has already disappeared.")
				.arg(vals[1]));
			break;
		case 4:
			KSGRD::SensorMgr->notify(i18n("Invalid argument."));
			break;
		}
		break;
	}
	}
}

void
ProcessController::sensorError(int, bool err)
{
	if (err == sensors().at(0)->isOk())
	{
		if (!err)
		{
			/* Whenever the communication with the sensor has been
			 * (re-)established we need to requests the full set of
			 * properties again, since the back-end might be a new
			 * one. */
			sendRequest(sensors().at(0)->hostName(), "test kill", 4);
			sendRequest(sensors().at(0)->hostName(), "ps?", 1);
			sendRequest(sensors().at(0)->hostName(), "ps", 2);
		}

		/* This happens only when the sensorOk status needs to be changed. */
		sensors().at(0)->setIsOk( !err );
	}
	setSensorOk(sensors().at(0)->isOk());
}

bool
ProcessController::restoreSettings(TQDomElement& element)
{
	bool result = addSensor(element.attribute("hostName"),
							element.attribute("sensorName"), (element.attribute("sensorType").isEmpty() ? "table" : element.attribute("sensorType")),
							TQString::null);

	xbTreeView->setChecked(element.attribute("tree").toInt());
	setTreeView(element.attribute("tree").toInt());

	uint filter = element.attribute("filter").toUInt();
	cbFilter->setCurrentItem(filter);
	filterModeChanged(filter);

	uint col = element.attribute("sortColumn").toUInt();
	bool inc = element.attribute("incrOrder").toUInt();

	if (!pList->load(element))
		return (false);

	pList->setSortColumn(col, inc);

	SensorDisplay::restoreSettings(element);

	setModified(false);

	return (result);
}

bool
ProcessController::saveSettings(TQDomDocument& doc, TQDomElement& element, bool save)
{
	element.setAttribute("hostName", sensors().at(0)->hostName());
	element.setAttribute("sensorName", sensors().at(0)->name());
	element.setAttribute("sensorType", sensors().at(0)->type());
	element.setAttribute("tree", (uint) xbTreeView->isChecked());
	element.setAttribute("filter", cbFilter->currentItem());
	element.setAttribute("sortColumn", pList->getSortColumn());
	element.setAttribute("incrOrder", pList->getIncreasing());

	if (!pList->save(doc, element))
		return (false);

	SensorDisplay::saveSettings(doc, element);

	if (save)
		setModified(false);

	return (true);
}
