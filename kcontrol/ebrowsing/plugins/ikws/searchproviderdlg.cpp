/*
 * Copyright (c) 2000 Malte Starostik <malte@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqwhatsthis.h>

#include <tdeapplication.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <kcharsets.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <tdemessagebox.h>

#include "searchproviderdlg_ui.h"
#include "searchproviderdlg.h"
#include "searchprovider.h"

SearchProviderDialog::SearchProviderDialog(SearchProvider *provider, const TQString& category,
                                           TQWidget *parent, const char *name)
                     : KDialogBase(parent, name, true, TQString::null, Ok|Cancel),
                       m_provider(provider)
{
    m_dlg = new SearchProviderDlgUI (this);
    setMainWidget(m_dlg);

    enableButtonSeparator(true);

    m_dlg->leQuery->setMinimumWidth(kapp->fontMetrics().maxWidth() * 16);
    m_dlg->cbCategory->setDuplicatesEnabled(false);
    m_dlg->cbCategory->setEditable(true);

    connect(m_dlg->leName, TQ_SIGNAL(textChanged(const TQString &)), TQ_SLOT(slotChanged()));
    connect(m_dlg->leQuery, TQ_SIGNAL(textChanged(const TQString &)), TQ_SLOT(slotChanged()));
    connect(m_dlg->leShortcut, TQ_SIGNAL(textChanged(const TQString &)), TQ_SLOT(slotChanged()));

    // Data init
    TQStringList charsets = TDEGlobal::charsets()->availableEncodingNames();
    charsets.prepend(i18n("Default"));
    m_dlg->cbCharset->insertStringList(charsets);

    m_dlg->cbCategory->insertStringList(SearchProvider::searchCategories.values());
    if (!category.isNull())
    {
        m_dlg->cbCategory->setCurrentText(SearchProvider::searchCategoryName(category));
    }

    if (m_provider)
    {
        setPlainCaption(i18n("Modify Search Provider"));
        m_dlg->leName->setText(m_provider->name());
        m_dlg->leQuery->setText(m_provider->query());
        m_dlg->leShortcut->setText(m_provider->keys().join(","));
        m_dlg->cbCategory->setCurrentText(SearchProvider::searchCategoryName(m_provider->category()));
        m_dlg->cbCharset->setCurrentItem(m_provider->charset().isEmpty() ? 0 : charsets.findIndex(m_provider->charset()));
        m_dlg->leName->setEnabled(false);
        m_dlg->leQuery->setFocus();
    }
    else
    {
        setPlainCaption(i18n("New Search Provider"));
        m_dlg->leName->setFocus();
        enableButton(Ok, false);
    }
}

void SearchProviderDialog::slotChanged()
{
    enableButton(Ok, !(m_dlg->leName->text().isEmpty()
                       || m_dlg->leShortcut->text().isEmpty()
                       || m_dlg->leQuery->text().isEmpty()));
}

void SearchProviderDialog::slotOk()
{
    if ((m_dlg->leQuery->text().find("\\{") == -1)
        && KMessageBox::warningContinueCancel(0,
            i18n("The URI does not contain a \\{...} placeholder for the user query.\n"
                 "This means that the same page is always going to be visited, "
                 "regardless of what the user types."),
            TQString::null, i18n("Keep It")) == KMessageBox::Cancel)
        return;

    if (!m_provider)
        m_provider = new SearchProvider;
    m_provider->setName(m_dlg->leName->text().stripWhiteSpace());
    m_provider->setQuery(m_dlg->leQuery->text().stripWhiteSpace());
    m_provider->setKeys(TQStringList::split(",", m_dlg->leShortcut->text().stripWhiteSpace()));
    m_provider->setCharset(m_dlg->cbCharset->currentItem() ? m_dlg->cbCharset->currentText() : TQString::null);
    m_provider->setCategory(m_dlg->cbCategory->currentText());
    KDialog::accept();
}

#include "searchproviderdlg.moc"
