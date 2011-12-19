/*
    KSysGuard, the KDE System Guard

	Copyright (c) 2001 Tobias Koenig <tokoe@kde.org>

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

*/

#include <stdio.h>
#include <sys/types.h>

#include <tqpushbutton.h>
#include <tqregexp.h>

#include <tqfile.h>
#include <tqlistbox.h>

#include <kfontdialog.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcolorbutton.h>

#include <ksgrd/StyleEngine.h>

#include "LogFile.moc"

LogFile::LogFile(TQWidget *parent, const char *name, const TQString& title)
	: KSGRD::SensorDisplay(parent, name, title)
{
	monitor = new TQListBox(this);
	Q_CHECK_PTR(monitor);

	setMinimumSize(50, 25);

	setPlotterWidget(monitor);

	setModified(false);
}

LogFile::~LogFile(void)
{
	sendRequest(sensors().tqat(0)->hostName(), TQString("logfile_unregister %1" ).arg(logFileID), 43);
}

bool
LogFile::addSensor(const TQString& hostName, const TQString& sensorName, const TQString& sensorType, const TQString& title)
{
	if (sensorType != "logfile")
		return (false);

	registerSensor(new KSGRD::SensorProperties(hostName, sensorName, sensorType, title));

	TQString sensorID = sensorName.right(sensorName.length() - (sensorName.findRev("/") + 1));

	sendRequest(sensors().tqat(0)->hostName(), TQString("logfile_register %1" ).arg(sensorID), 42);

	if (title.isEmpty())
		setTitle(sensors().tqat(0)->hostName() + ":" + sensorID);
	else
		setTitle(title);

	setModified(true);

	return (true);
}


void LogFile::configureSettings(void)
{
	TQColorGroup cgroup = monitor->colorGroup();

	lfs = new LogFileSettings(this);
	Q_CHECK_PTR(lfs);

	lfs->fgColor->setColor(cgroup.text());
	lfs->fgColor->setText(i18n("Foreground color:"));
	lfs->bgColor->setColor(cgroup.base());
	lfs->bgColor->setText(i18n("Background color:"));
	lfs->fontButton->setFont(monitor->font());
	lfs->ruleList->insertStringList(filterRules);
	lfs->title->setText(title());

	connect(lfs->okButton, TQT_SIGNAL(clicked()), lfs, TQT_SLOT(accept()));
	connect(lfs->applyButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(applySettings()));
	connect(lfs->cancelButton, TQT_SIGNAL(clicked()), lfs, TQT_SLOT(reject()));

	connect(lfs->fontButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(settingsFontSelection()));
	connect(lfs->addButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(settingsAddRule()));
	connect(lfs->deleteButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(settingsDeleteRule()));
	connect(lfs->changeButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(settingsChangeRule()));
	connect(lfs->ruleList, TQT_SIGNAL(selected(int)), this, TQT_SLOT(settingsRuleListSelected(int)));
	connect(lfs->ruleText, TQT_SIGNAL(returnPressed()), this, TQT_SLOT(settingsAddRule()));

	if (lfs->exec()) {
		applySettings();
	}

	delete lfs;
	lfs = 0;
}

void LogFile::settingsFontSelection()
{
	TQFont tmpFont = lfs->fontButton->font();

	if (KFontDialog::getFont(tmpFont) == KFontDialog::Accepted) {
		lfs->fontButton->setFont(tmpFont);
	}
}

void LogFile::settingsAddRule()
{
	if (!lfs->ruleText->text().isEmpty()) {
		lfs->ruleList->insertItem(lfs->ruleText->text(), -1);
		lfs->ruleText->setText("");
	}
}

void LogFile::settingsDeleteRule()
{
	lfs->ruleList->removeItem(lfs->ruleList->currentItem());
	lfs->ruleText->setText("");
}

void LogFile::settingsChangeRule()
{
	lfs->ruleList->changeItem(lfs->ruleText->text(), lfs->ruleList->currentItem());
	lfs->ruleText->setText("");
}

void LogFile::settingsRuleListSelected(int index)
{
	lfs->ruleText->setText(lfs->ruleList->text(index));
}

void LogFile::applySettings(void)
{
	TQColorGroup cgroup = monitor->colorGroup();

	cgroup.setColor(TQColorGroup::Text, lfs->fgColor->color());
	cgroup.setColor(TQColorGroup::Base, lfs->bgColor->color());
	monitor->setPalette(TQPalette(cgroup, cgroup, cgroup));
	monitor->setFont(lfs->fontButton->font());

	filterRules.clear();
	for (uint i = 0; i < lfs->ruleList->count(); i++)
		filterRules.append(lfs->ruleList->text(i));

	setTitle(lfs->title->text());

	setModified(true);
}

void
LogFile::applyStyle()
{
	TQColorGroup cgroup = monitor->colorGroup();

	cgroup.setColor(TQColorGroup::Text, KSGRD::Style->firstForegroundColor());
	cgroup.setColor(TQColorGroup::Base, KSGRD::Style->backgroundColor());
	monitor->setPalette(TQPalette(cgroup, cgroup, cgroup));

	setModified(true);
}

bool
LogFile::restoreSettings(TQDomElement& element)
{
	TQFont font;
	TQColorGroup cgroup = monitor->colorGroup();

	cgroup.setColor(TQColorGroup::Text, restoreColor(element, "textColor", Qt::green));
	cgroup.setColor(TQColorGroup::Base, restoreColor(element, "backgroundColor", Qt::black));
	monitor->setPalette(TQPalette(cgroup, cgroup, cgroup));

	addSensor(element.attribute("hostName"), element.attribute("sensorName"), (element.attribute("sensorType").isEmpty() ? "logfile" : element.attribute("sensorType")), element.attribute("title"));

	font.fromString( element.attribute( "font" ) );
	monitor->setFont(font);

	TQDomNodeList dnList = element.elementsByTagName("filter");
	for (uint i = 0; i < dnList.count(); i++) {
		TQDomElement element = dnList.item(i).toElement();
		filterRules.append(element.attribute("rule"));
	}

	SensorDisplay::restoreSettings(element);

	setModified(false);

	return true;
}

bool
LogFile::saveSettings(TQDomDocument& doc, TQDomElement& element, bool save)
{
	element.setAttribute("hostName", sensors().tqat(0)->hostName());
	element.setAttribute("sensorName", sensors().tqat(0)->name());
	element.setAttribute("sensorType", sensors().tqat(0)->type());

	element.setAttribute("font", monitor->font().toString());

	saveColor(element, "textColor", monitor->colorGroup().text());
	saveColor(element, "backgroundColor", monitor->colorGroup().base());

	for (TQStringList::Iterator it = filterRules.begin();
		 it != filterRules.end(); it++)
	{
		TQDomElement filter = doc.createElement("filter");
		filter.setAttribute("rule", (*it));
		element.appendChild(filter);
	}

	SensorDisplay::saveSettings(doc, element);

	if (save)
		setModified(false);

	return true;
}

void
LogFile::updateMonitor()
{
	sendRequest(sensors().tqat(0)->hostName(),
				TQString("%1 %2" ).arg(sensors().tqat(0)->name()).arg(logFileID), 19);
}

void
LogFile::answerReceived(int id, const TQString& answer)
{
	/* We received something, so the sensor is probably ok. */
	sensorError(id, false);

	switch (id)
	{
		case 19: {
			KSGRD::SensorTokenizer lines(answer, '\n');

			for (uint i = 0; i < lines.count(); i++) {
				if (monitor->count() == MAXLINES)
					monitor->removeItem(0);

				monitor->insertItem(lines[i], -1);

				for (TQStringList::Iterator it = filterRules.begin(); it != filterRules.end(); it++) {
					TQRegExp *expr = new TQRegExp((*it).latin1());
					if (expr->search(lines[i].latin1()) != -1) {
						KNotifyClient::event(winId(), "pattern_match", TQString("rule '%1' matched").arg((*it).latin1()));
					}
					delete expr;
				}
			}

      monitor->setCurrentItem( monitor->count() - 1 );
      monitor->ensureCurrentVisible();

			break;
		}

		case 42: {
			logFileID = answer.toULong();
			break;
		}
	}
}

void
LogFile::resizeEvent(TQResizeEvent*)
{
	frame()->setGeometry(0, 0, this->width(), this->height());
	monitor->setGeometry(10, 20, this->width() - 20, this->height() - 30);
}
