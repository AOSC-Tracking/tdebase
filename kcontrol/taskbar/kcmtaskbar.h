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

#ifndef __kcmtaskbar_h__
#define __kcmtaskbar_h__

#include <tqvaluelist.h>

#include <tdecmodule.h>

class TaskbarConfigUI;
class TaskBarSettings;

class TaskbarAppearance
{
    public:
        typedef TQValueList<TaskbarAppearance> List;

        TaskbarAppearance();
        TaskbarAppearance(TQString name,
                          bool drawButtons,
                          bool haloText,
                          bool showButtonOnHover,
                          TaskBarSettings* settingsObject);

        bool matchesSettings() const;
        void alterSettings() const;
        TQString name() const { return m_name; }

    private:
        TQString m_name;
        bool m_drawButtons;
        bool m_haloText;
        bool m_showButtonOnHover;
        TaskBarSettings* m_settingsObject;
};

class TaskbarConfig : public TDECModule
{
    TQ_OBJECT

public:
    TaskbarConfig(TQWidget *parent = 0, const char* name = 0,
                  const TQStringList &list = TQStringList());
    ~TaskbarConfig();

public slots:
    void load();
    void save();
    void defaults();

protected slots:
    void slotUpdateComboBox();
    void appearanceChanged(int);
    void displayIconsNTextChanged(int);
    void slotScrollTaskbarActionChanged(int);
    void notChanged();
    void slotUpdateCustomColors();

private slots:
    void slotReloadConfigurationFromGlobals();
    void slotEditGlobalConfiguration();
    void processLockouts();

private:
    TaskbarAppearance::List m_appearances;
    void updateAppearanceCombo();
    void updateIconsTextCombo();
    void updateCustomAppearance();
    static const TQStringList& buttonActionList();
    static TQStringList i18nButtonActionList();
    static const TQStringList& wheelActionList();
    static TQStringList i18nWheelActionList();
    static const TQStringList& groupModeList();
    static TQStringList i18nGroupModeList();
    static const TQStringList& showTaskStatesList();
    static TQStringList i18nShowTaskStatesList();
    static const TQStringList& displayIconsNText();
    static TQStringList i18ndisplayIconsNText();
    TaskbarConfigUI *m_widget;
    TQString m_configFileName;
    TaskBarSettings* m_settingsObject;
    bool m_isGlobalConfig;
};

#endif
