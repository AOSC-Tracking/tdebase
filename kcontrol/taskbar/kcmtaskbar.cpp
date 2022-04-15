/*
 *  Copyright (c) 2000 Kurt Granroth <granroth@kde.org>
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlayout.h>
#include <tqtimer.h>
#include <tqvaluelist.h>
#include <tqfile.h>
#include <tqlabel.h>
#include <tqbuttongroup.h>
#include <tqtabwidget.h>

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <twin.h>
#include <kcolorbutton.h>
#include <kstandarddirs.h>

#define protected public
#include "kcmtaskbarui.h"
#undef protected

#include "taskbarsettings.h"

#include "kcmtaskbar.h"
#include "kcmtaskbar.moc"

#define GLOBAL_TASKBAR_CONFIG_FILE_NAME "ktaskbarrc"

typedef KGenericFactory<TaskbarConfig, TQWidget > TaskBarFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_taskbar, TaskBarFactory("kcmtaskbar") )

TaskbarAppearance::TaskbarAppearance(TQString name,
                                     bool drawButtons,
                                     bool haloText,
                                     bool showButtonOnHover,
                                     TaskBarSettings* settingsObject)
    : m_name(name),
      m_drawButtons(drawButtons),
      m_haloText(haloText),
      m_showButtonOnHover(showButtonOnHover),
      m_settingsObject(NULL)
{
    m_settingsObject = settingsObject;
    if (m_settingsObject)
    {
        m_settingsObject->readConfig();
    }
}

TaskbarAppearance::TaskbarAppearance()
    : m_drawButtons(false),
      m_haloText(false),
      m_showButtonOnHover(true),
      m_settingsObject(NULL)
{
}

bool TaskbarAppearance::matchesSettings() const
{
    return m_settingsObject->drawButtons() == m_drawButtons &&
           m_settingsObject->haloText() == m_haloText &&
           m_settingsObject->showButtonOnHover() == m_showButtonOnHover;
}

void TaskbarAppearance::alterSettings() const
{
    m_settingsObject->setDrawButtons(m_drawButtons);
    m_settingsObject->setHaloText(m_haloText);
    m_settingsObject->setShowButtonOnHover(m_showButtonOnHover);
}

// These are the strings that are actually stored in the config file.
const TQStringList& TaskbarConfig::actionList()
{
    static TQStringList list(
            TQStringList() << I18N_NOOP("Show Task List") << I18N_NOOP("Show Operations Menu")
            << I18N_NOOP("Activate, Raise or Minimize Task")
            << I18N_NOOP("Activate Task") << I18N_NOOP("Raise Task")
            << I18N_NOOP("Lower Task") << I18N_NOOP("Minimize Task")
            << I18N_NOOP("Move To Current Desktop")
            << I18N_NOOP("Close Task") );
    return list;
}

// Get a translated version of the above string list.
TQStringList TaskbarConfig::i18nActionList()
{
   TQStringList i18nList;
   for( TQStringList::ConstIterator it = actionList().begin(); it != actionList().end(); ++it ) {
      i18nList << i18n((*it).latin1());
   }
   return i18nList;
}

// These are the strings that are actually stored in the config file.
const TQStringList& TaskbarConfig::groupModeList()
{
    static TQStringList list(
            TQStringList() << I18N_NOOP("Never") << I18N_NOOP("When Taskbar Full")
            << I18N_NOOP("Always"));
    return list;
}

// Get a translated version of the above string list.
TQStringList TaskbarConfig::i18nGroupModeList()
{
   TQStringList i18nList;
   for( TQStringList::ConstIterator it = groupModeList().begin(); it != groupModeList().end(); ++it ) {
      i18nList << i18n((*it).latin1());
   }
   return i18nList;
}

// These are the strings that are actually stored in the config file.
const TQStringList& TaskbarConfig::showTaskStatesList()
{
    static TQStringList list(
            TQStringList() << I18N_NOOP("Any") << I18N_NOOP("Only Stopped")
            << I18N_NOOP("Only Running"));
    return list;
}

// Get a translated version of the above string list.
TQStringList TaskbarConfig::i18nShowTaskStatesList()
{
   TQStringList i18nList;
   for( TQStringList::ConstIterator it = showTaskStatesList().begin(); it != showTaskStatesList().end(); ++it ) {
      i18nList << i18n((*it).latin1());
   }
   return i18nList;
}

// These are the strings that are actually stored in the config file.
const TQStringList& TaskbarConfig::displayIconsNText()
{
    static TQStringList list(TQStringList()
                << I18N_NOOP("Icons and Text")
                << I18N_NOOP("Text only")
                << I18N_NOOP("Icons only"));
    return list;
}

// Get a translated version of the above string list.
TQStringList TaskbarConfig::i18ndisplayIconsNText()
{
   TQStringList i18nList;
   for (TQStringList::ConstIterator it = displayIconsNText().begin(); it != displayIconsNText().end(); ++it)
   {
     i18nList << i18n((*it).latin1());
   }
   return i18nList;
}

TaskbarConfig::TaskbarConfig(TQWidget *parent, const char* name, const TQStringList& args)
  : TDECModule(TaskBarFactory::instance(), parent, name),
    m_settingsObject(NULL)
{
    TQVBoxLayout *layout = new TQVBoxLayout(this, 0, KDialog::spacingHint());
    m_widget = new TaskbarConfigUI(this);
    layout->addWidget(m_widget);

    m_configFileName = GLOBAL_TASKBAR_CONFIG_FILE_NAME;
    if (args.count() > 0)
    {
        m_configFileName = args[0];
        m_isGlobalConfig = false;
    }
    else
    {
        m_isGlobalConfig = true;
    }
    connect(m_widget->globalConfigReload, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotReloadConfigurationFromGlobals()));
    connect(m_widget->globalConfigEdit, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotEditGlobalConfiguration()));
    connect(m_widget->kcfg_UseGlobalSettings, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));
    connect(m_widget->kcfg_SortByApp, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockouts()));

    TQFile configFile(locateLocal("config", m_configFileName));
    if (!configFile.exists())
    {
        TDEConfig globalConfig(GLOBAL_TASKBAR_CONFIG_FILE_NAME, TRUE, TRUE);
        TDEConfig localConfig(m_configFileName);
        globalConfig.copyTo(m_configFileName, &localConfig);
        localConfig.sync();
    }

    m_settingsObject = new TaskBarSettings(TDESharedConfig::openConfig(m_configFileName));
    m_settingsObject->readConfig();

    // TODO: Load these from .desktop files?
    m_appearances.append(TaskbarAppearance(i18n("Elegant"), false, false, true, m_settingsObject));
    m_appearances.append(TaskbarAppearance(i18n("Classic"), true, false, true, m_settingsObject));
    m_appearances.append(TaskbarAppearance(i18n("For Transparency"), false, true, true, m_settingsObject));

    for (TaskbarAppearance::List::const_iterator it = m_appearances.constBegin();
         it != m_appearances.constEnd();
         ++it)
    {
        m_widget->appearance->insertItem((*it).name());
    }
    m_widget->appearance->insertItem(i18n("Custom"));

    connect(m_widget->appearance, TQT_SIGNAL(activated(int)),
            this, TQT_SLOT(appearanceChanged(int)));
    addConfig(m_settingsObject, m_widget);

    setQuickHelp(i18n("<h1>Taskbar</h1> You can configure the taskbar here."
                " This includes options such as whether or not the taskbar should show all"
                " windows at once or only those on the current desktop."
                " You can also configure whether or not the Window List button will be displayed."));

    TQStringList list = i18nActionList();
    m_widget->kcfg_LeftButtonAction->insertStringList(list);
    m_widget->kcfg_MiddleButtonAction->insertStringList(list);
    m_widget->kcfg_RightButtonAction->insertStringList(list);
    m_widget->kcfg_DisplayIconsNText->insertStringList(i18ndisplayIconsNText());
    m_widget->kcfg_GroupTasks->insertStringList(i18nGroupModeList());
    m_widget->kcfg_ShowTaskStates->insertStringList(i18nShowTaskStatesList());

    connect(m_widget->kcfg_GroupTasks, TQT_SIGNAL(activated(int)),
            this, TQT_SLOT(slotUpdateComboBox()));
    connect(m_widget->kcfg_UseCustomColors, TQT_SIGNAL(stateChanged(int)), this, TQT_SLOT(slotUpdateCustomColors()));

    slotUpdateCustomColors();
    updateAppearanceCombo();

    if (KWin::numberOfDesktops() < 2)
    {
        m_widget->kcfg_ShowAllWindows->hide();
        m_widget->kcfg_SortByDesktop->hide();
        // m_widget->spacer2->changeSize(0, 0);
    }

    if (!TQApplication::desktop()->isVirtualDesktop() ||
        TQApplication::desktop()->numScreens() == 1) // No Ximerama
    {
        m_widget->showAllScreens->hide();
    }
    else
    {
        m_widget->showAllScreens->show();
    }
    connect( m_widget->showAllScreens, TQT_SIGNAL( stateChanged( int )), TQT_SLOT( changed()));

    TDEAboutData *about = new TDEAboutData(I18N_NOOP("kcmtaskbar"),
                                       I18N_NOOP("TDE Taskbar Control Module"),
                                       0, 0, TDEAboutData::License_GPL,
                                       I18N_NOOP("(c) 2000 - 2001 Matthias Elter"));

    about->addAuthor("Matthias Elter", 0, "elter@kde.org");
    about->addCredit("Stefan Nikolaus", I18N_NOOP("TDEConfigXT conversion"),
                     "stefan.nikolaus@kdemail.net");
    setAboutData(about);

    load();
    processLockouts();

    TQTimer::singleShot(0, this, TQT_SLOT(notChanged()));
}

TaskbarConfig::~TaskbarConfig()
{
    if (m_settingsObject)
    {
        delete m_settingsObject;
    }
}

void TaskbarConfig::slotEditGlobalConfiguration() {
	TQByteArray data;
	kapp->dcopClient()->send("kicker", "kicker", "reshowTaskBarConfig()", data);
}

void TaskbarConfig::processLockouts()
{
	m_configFileName = GLOBAL_TASKBAR_CONFIG_FILE_NAME;
	if (m_isGlobalConfig)
	{
		m_widget->globalConfigWarning->show();
		m_widget->localConfigWarning->hide();
		m_widget->globalConfigReload->hide();
		m_widget->globalConfigEdit->hide();
		m_widget->kcfg_UseGlobalSettings->hide();
	}
	else {
		m_widget->globalConfigWarning->hide();
		m_widget->localConfigWarning->show();
		m_widget->kcfg_UseGlobalSettings->show();
		if (m_widget->kcfg_UseGlobalSettings->isChecked()) {
			m_widget->tabs->hide();
			m_widget->globalConfigReload->hide();
			m_widget->globalConfigEdit->show();
		}
		else {
			m_widget->tabs->show();
			// FIXME
			// Disable this feature until a method can be found to force the TDECModule to reload its settings from disk after the global settings have been copied!
			//m_widget->globalConfigReload->show();
			m_widget->globalConfigReload->hide();
			m_widget->globalConfigEdit->hide();
		}
	}

	m_widget->kcfg_AllowDragAndDropReArrange->setEnabled(!m_widget->kcfg_SortByApp->isChecked());
}

void TaskbarConfig::slotReloadConfigurationFromGlobals()
{
    TDEConfig globalConfig(GLOBAL_TASKBAR_CONFIG_FILE_NAME, TRUE, TRUE);
    TDEConfig localConfig(m_configFileName);
    globalConfig.copyTo(m_configFileName, &localConfig);
    localConfig.sync();
    m_settingsObject->readConfig();
    load();
    m_widget->kcfg_UseGlobalSettings->setChecked(false);
}

void TaskbarConfig::slotUpdateCustomColors()
{
    m_widget->kcfg_ActiveTaskTextColor->setEnabled(m_widget->kcfg_UseCustomColors->isChecked());
    m_widget->activeTaskTextColorLabel->setEnabled(m_widget->kcfg_UseCustomColors->isChecked());

    m_widget->kcfg_InactiveTaskTextColor->setEnabled(m_widget->kcfg_UseCustomColors->isChecked());
    m_widget->inactiveTaskTextColorLabel->setEnabled(m_widget->kcfg_UseCustomColors->isChecked());

    m_widget->kcfg_TaskBackgroundColor->setEnabled(m_widget->kcfg_UseCustomColors->isChecked());
    m_widget->taskBackgroundColorLabel->setEnabled(m_widget->kcfg_UseCustomColors->isChecked());
}

void TaskbarConfig::slotUpdateComboBox()
{
    int pos = TaskBarSettings::ActivateRaiseOrMinimize;
    // If grouping is enabled, call "Activate, Raise or Iconify something else,
    // though the config key used is the same.
    if(m_widget->kcfg_GroupTasks->currentItem() != TaskBarSettings::GroupNever)
    {
        m_widget->kcfg_LeftButtonAction->changeItem(i18n("Cycle Through Windows"), pos);
        m_widget->kcfg_MiddleButtonAction->changeItem(i18n("Cycle Through Windows"), pos);
        m_widget->kcfg_RightButtonAction->changeItem(i18n("Cycle Through Windows"), pos);
    }
    else
    {
        TQString action = i18nActionList()[pos];
        m_widget->kcfg_LeftButtonAction->changeItem(action,pos);
        m_widget->kcfg_MiddleButtonAction->changeItem(action,pos);
        m_widget->kcfg_RightButtonAction->changeItem(action,pos);
    }
}

void TaskbarConfig::updateAppearanceCombo()
{
    unsigned int i = 0;
    for (TaskbarAppearance::List::const_iterator it = m_appearances.constBegin();
         it != m_appearances.constEnd();
         ++it, ++i)
    {
        if ((*it).matchesSettings())
        {
            break;
        }
    }

    if (i < m_appearances.count())
    {
        m_widget->appearance->setCurrentItem(i);
        m_widget->customAppearance->setEnabled(false);
        return;
    }

    else if (m_widget->appearance->count() == (int)m_appearances.count())
    {
        m_widget->customAppearance->setEnabled(true);
    }

    m_widget->appearance->setCurrentItem(m_appearances.count());
}

void TaskbarConfig::updateCustomAppearance()
{
    m_widget->kcfg_DrawButtons      ->setChecked(m_settingsObject->drawButtons());
    m_widget->kcfg_HaloText         ->setChecked(m_settingsObject->haloText());
    m_widget->kcfg_ShowButtonOnHover->setChecked(m_settingsObject->showButtonOnHover());
}

void TaskbarConfig::appearanceChanged(int selected)
{
    if (selected < m_appearances.count())
    {
        m_widget->customAppearance->setEnabled(false);
        unmanagedWidgetChangeState(!m_appearances[selected].matchesSettings());
    }
    else if(selected == m_appearances.count())
    {
        m_widget->customAppearance->setEnabled(true);
        updateCustomAppearance();
    }
}

void TaskbarConfig::load()
{
    TDECModule::load();
    slotUpdateComboBox();
    updateAppearanceCombo();
    m_widget->showAllScreens->setChecked(!m_settingsObject->showCurrentScreenOnly());
}

void TaskbarConfig::save()
{
    TDECModule::save();

    m_settingsObject->setShowCurrentScreenOnly(!m_widget->showAllScreens->isChecked());
    int selectedAppearance = m_widget->appearance->currentItem();
    if (selectedAppearance < (int)m_appearances.count())
    {
        m_appearances[selectedAppearance].alterSettings();
        m_settingsObject->writeConfig();
    }

    TQByteArray data;
    kapp->dcopClient()->emitDCOPSignal("kdeTaskBarConfigChanged()", data);
}

void TaskbarConfig::defaults()
{
    TDECModule::defaults();
    slotUpdateComboBox();
    updateAppearanceCombo();
}

void TaskbarConfig::notChanged()
{
    emit changed(false);
}
