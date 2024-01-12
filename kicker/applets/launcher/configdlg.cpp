/*****************************************************************

Copyright (c) 2005 Fred Schaettgen <kde.sch@ttgen.net>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/


#include <tqcombobox.h>
#include <tdelocale.h>
#include <kdebug.h>

#include "prefs.h"
#include "configdlg.h"
#include "configdlgbase.h"

ConfigDlg::ConfigDlg(TQWidget *parent, const char *name, Prefs *config,
                     int autoSize, TDEConfigDialog::DialogType dialogType,
                     int dialogButtons) :
    TDEConfigDialog(parent, name, config, dialogType, dialogButtons),
    m_settings(config),
    m_autoSize(autoSize)
{
    m_ui = new ConfigDlgBase(this->plainPage());
    addPage(m_ui, i18n("Configure"), "config");

    m_ui->iconDim->clear();
    m_ui->iconDim->insertItem(i18n("Automatic"));
    for (int n=0; n<int(m_settings->iconDimChoices().size()); ++n)
    {
        m_ui->iconDim->insertItem(TQString::number(
            m_settings->iconDimChoices()[n]));
    }
    connect(m_ui->iconDim, TQ_SIGNAL(textChanged(const TQString&)),
            this, TQ_SLOT(updateButtons()));
    updateWidgets();
    m_oldIconDimText = m_ui->iconDim->currentText();
    updateButtons();
}

void ConfigDlg::updateSettings()
{
    kdDebug() << "updateSettings" << endl;
    TDEConfigDialog::updateSettings();
    if (!hasChanged())
    {
        return;
    }
    m_oldIconDimText = m_ui->iconDim->currentText();
    if (m_ui->iconDim->currentText() == i18n("Automatic"))
    {
        m_settings->setIconDim(m_autoSize);
    }
    else 
    {
        m_settings->setIconDim(m_ui->iconDim->currentText().toInt());
    }
    settingsChangedSlot();
}

void ConfigDlg::updateWidgets()
{
    TDEConfigDialog::updateWidgets();
    if (m_settings->iconDim() == m_autoSize)
    {
        m_ui->iconDim->setEditText(i18n("Automatic"));
    }
    else
    {
        m_ui->iconDim->setEditText(TQString::number(m_settings->iconDim()));
    }
}

void ConfigDlg::updateWidgetsDefault()
{
    TDEConfigDialog::updateWidgetsDefault();
}

bool ConfigDlg::hasChanged()
{
    return m_oldIconDimText != m_ui->iconDim->currentText() ||
        TDEConfigDialog::hasChanged();
}

#include "configdlg.moc"
