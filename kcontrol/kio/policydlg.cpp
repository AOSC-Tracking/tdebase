/**
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
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

#include <tqpushbutton.h>
#include <tqwhatsthis.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqvalidator.h>

#include <klineedit.h>
#include <kcombobox.h>
#include <klocale.h>

#include "policydlg.h"
#include "policydlg_ui.h"


class DomainLineValidator : public TQValidator
{
public:
  DomainLineValidator(TQObject *parent)
  :TQValidator(parent, "domainValidator")
  {
  }

  State validate(TQString &input, int &) const
  {
    if (input.isEmpty() || (input == "."))
      return Intermediate;

    int length = input.length();

    for(int i = 0 ; i < length; i++)
    {
      if (!input[i].isLetterOrNumber() && input[i] != '.' && input[i] != '-')
        return Invalid;
    }

    return Acceptable;
  }
};


PolicyDlg::PolicyDlg (const TQString& caption, TQWidget *parent,
    const char *name)
    : KDialogBase(parent, name, true, caption, Ok|Cancel, Ok, true)
{
  m_dlgUI = new PolicyDlgUI (this);
  setMainWidget(m_dlgUI);

  m_dlgUI->leDomain->setValidator(new DomainLineValidator(TQT_TQOBJECT(m_dlgUI->leDomain)));
  m_dlgUI->cbPolicy->setMinimumWidth( m_dlgUI->cbPolicy->fontMetrics().maxWidth() * 25 );
  
  enableButtonOK( false );
  connect(m_dlgUI->leDomain, TQT_SIGNAL(textChanged(const TQString&)),
    TQT_SLOT(slotTextChanged(const TQString&)));

  setFixedSize (sizeHint());
  m_dlgUI->leDomain->setFocus ();
}

void PolicyDlg::setEnableHostEdit( bool state, const TQString& host )
{
  if ( !host.isEmpty() )
    m_dlgUI->leDomain->setText( host );
  m_dlgUI->leDomain->setEnabled( state );
}

void PolicyDlg::setPolicy (int policy)
{
  if ( policy > -1 && policy <= static_cast<int>(m_dlgUI->cbPolicy->count()) )
    m_dlgUI->cbPolicy->setCurrentItem(policy-1);

  if ( !m_dlgUI->leDomain->isEnabled() )
    m_dlgUI->cbPolicy->setFocus();
}

int PolicyDlg::advice () const
{
  return m_dlgUI->cbPolicy->currentItem() + 1;
}

TQString PolicyDlg::domain () const
{
  return m_dlgUI->leDomain->text();
}

void PolicyDlg::slotTextChanged( const TQString& text )
{
  enableButtonOK( text.length() > 1 );
}
#include "policydlg.moc"
