/*

   This file is part of the KDE project, module kcmbackground.

   Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqheader.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqcombobox.h>
#include <tqpushbutton.h>
#include <tqslider.h>
#include <tqspinbox.h>
#include <tqwhatsthis.h>

#include <tdeconfig.h>
#include <kcolorbutton.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kpixmap.h>
#include <kstandarddirs.h>
#include <twin.h>

#include "bgrender.h"
#include "bgadvanced.h"
#include "bgadvanced_ui.h"

#include <X11/Xlib.h>

/**** BGAdvancedDialog ****/

static TQCString desktopConfigname()
{
    int desktop=0;
    if (tqt_xdisplay())
        desktop = DefaultScreen(tqt_xdisplay());
    TQCString name;
    if (desktop == 0)
        name = "kdesktoprc";
    else
        name.sprintf("kdesktop-screen-%drc", desktop);

    return name;
}


BGAdvancedDialog::BGAdvancedDialog(KBackgroundRenderer *_r,
                                   TQWidget *parent,
                                   bool m_multidesktop)
    : KDialogBase(parent, "BGAdvancedDialog",
                  true, i18n("Advanced Background Settings"),
                  Ok | Cancel, Ok, true),
      r(_r)
{
   dlg = new BGAdvancedBase(this);
   setMainWidget(dlg);

   dlg->m_listPrograms->header()->setStretchEnabled ( true, 1 );
   dlg->m_listPrograms->setAllColumnsShowFocus(true);

   connect(dlg->m_listPrograms, TQT_SIGNAL(clicked(TQListViewItem *)),
         TQT_SLOT(slotProgramItemClicked(TQListViewItem *)));

   // Load programs
   TQStringList lst = KBackgroundProgram::list();
   TQStringList::Iterator it;
   for (it=lst.begin(); it != lst.end(); ++it)
      addProgram(*it);

   if (m_multidesktop)
   {
      TDEConfig cfg(desktopConfigname(), false, false);
      cfg.setGroup( "General" );
      if (!cfg.readBoolEntry( "Enabled", true ))
      {
         dlg->m_groupIconText->hide();
      }

      dlg->m_spinCache->setSteps(512, 1024);
      dlg->m_spinCache->setRange(0, 40960);
      dlg->m_spinCache->setSpecialValueText(i18n("Unlimited"));
      dlg->m_spinCache->setSuffix(i18n(" KB"));

      connect(dlg->m_buttonAdd, TQT_SIGNAL(clicked()),
         TQT_SLOT(slotAdd()));
      connect(dlg->m_buttonRemove, TQT_SIGNAL(clicked()),
         TQT_SLOT(slotRemove()));
      connect(dlg->m_buttonModify, TQT_SIGNAL(clicked()),
         TQT_SLOT(slotModify()));

      connect(dlg->m_listPrograms, TQT_SIGNAL(doubleClicked(TQListViewItem *)),
         TQT_SLOT(slotProgramItemDoubleClicked(TQListViewItem *)));
   }
   else
   {
      dlg->m_buttonAdd->hide();
      dlg->m_buttonRemove->hide();
      dlg->m_buttonModify->hide();
      dlg->m_groupIconText->hide();
      dlg->m_groupCache->hide();
   }

   connect( dlg->m_cbProgram, TQT_SIGNAL(toggled(bool)),
            TQT_SLOT(slotEnableProgram(bool)));

   m_backgroundMode = m_oldBackgroundMode = r->backgroundMode();
   if (m_oldBackgroundMode == KBackgroundSettings::Program)
      m_oldBackgroundMode = KBackgroundSettings::Flat;

   dlg->adjustSize();
   updateUI();
}

void BGAdvancedDialog::makeReadOnly()
{
   dlg->m_cbProgram->setEnabled(false);
   dlg->m_listPrograms->setEnabled(false);
}

void BGAdvancedDialog::setCacheSize(int s)
{
   dlg->m_spinCache->setValue(s);
}

int BGAdvancedDialog::cacheSize()
{
   return dlg->m_spinCache->value();
}

TQColor BGAdvancedDialog::textColor()
{
    return dlg->m_colorText->color();
}

void BGAdvancedDialog::setTextColor(const TQColor &color)
{
    dlg->m_colorText->setColor(color);
}

TQColor BGAdvancedDialog::textBackgroundColor()
{
    return dlg->m_cbSolidTextBackground->isChecked() ?
           dlg->m_colorTextBackground->color() : TQColor();
}

void BGAdvancedDialog::setTextBackgroundColor(const TQColor &color)
{
    dlg->m_colorTextBackground->blockSignals(true);
    dlg->m_cbSolidTextBackground->blockSignals(true);
    if (color.isValid())
    {
        dlg->m_cbSolidTextBackground->setChecked(true);
        dlg->m_colorTextBackground->setColor(color);
        dlg->m_colorTextBackground->setEnabled(true);
    }
    else
    {
        dlg->m_cbSolidTextBackground->setChecked(false);
        dlg->m_colorTextBackground->setColor(Qt::white);
        dlg->m_colorTextBackground->setEnabled(false);
    }
    dlg->m_colorTextBackground->blockSignals(false);
    dlg->m_cbSolidTextBackground->blockSignals(false);
}

bool BGAdvancedDialog::shadowEnabled()
{
    return dlg->m_cbShadow->isChecked();
}

void BGAdvancedDialog::setShadowEnabled(bool enabled)
{
    dlg->m_cbShadow->setChecked(enabled);
}

void BGAdvancedDialog::setTextLines(int lines)
{
    dlg->m_spinTextLines->setValue(lines);
}

int BGAdvancedDialog::textLines() const
{
    return dlg->m_spinTextLines->value();
}

void BGAdvancedDialog::setTextWidth(int width)
{
    dlg->m_spinTextWidth->setValue(width);
}

int BGAdvancedDialog::textWidth() const
{
    return dlg->m_spinTextWidth->value();
}

void BGAdvancedDialog::updateUI()
{
    TQString prog = r->KBackgroundProgram::name();

    dlg->m_cbProgram->blockSignals(true);
    if ((r->backgroundMode() == KBackgroundSettings::Program)
        && !prog.isEmpty())
    {
        dlg->m_cbProgram->setChecked(true);
        dlg->m_listPrograms->setEnabled(true);
        dlg->m_buttonAdd->setEnabled(true);
        dlg->m_buttonRemove->setEnabled(true);
        dlg->m_buttonModify->setEnabled(true);
        selectProgram(prog);
    }
    else
    {
        dlg->m_cbProgram->setChecked(false);
        dlg->m_listPrograms->setEnabled(false);
        dlg->m_buttonAdd->setEnabled(false);
        dlg->m_buttonRemove->setEnabled(false);
        dlg->m_buttonModify->setEnabled(false);
    }
    dlg->m_cbProgram->blockSignals(false);
}

void BGAdvancedDialog::removeProgram(const TQString &name)
{
   if (m_programItems.find(name))
   {
      delete m_programItems[name];
      m_programItems.remove(name);
   }
}

void BGAdvancedDialog::addProgram(const TQString &name)
{
   removeProgram(name);

   KBackgroundProgram prog(name);
   if (prog.command().isEmpty() || (prog.isGlobal() && !prog.isAvailable()))
      return;

   TQListViewItem *item = new TQListViewItem(dlg->m_listPrograms);
   item->setText(0, prog.name());
   item->setText(1, prog.comment());
   item->setText(2, i18n("%1 min.").arg(prog.refresh()));

   m_programItems.insert(name, item);
}

void BGAdvancedDialog::selectProgram(const TQString &name)
{
   if (m_programItems.find(name))
   {
      TQListViewItem *item = m_programItems[name];
      dlg->m_listPrograms->ensureItemVisible(item);
      dlg->m_listPrograms->setSelected(item, true);
      m_selectedProgram = name;
   }
}

void BGAdvancedDialog::slotAdd()
{
   KProgramEditDialog dlg;
   dlg.exec();
   if (dlg.result() == TQDialog::Accepted)
   {
      TQString program = dlg.program();
      addProgram(program);
      selectProgram(program);
   }
}

void BGAdvancedDialog::slotRemove()
{
   if (m_selectedProgram.isEmpty())
      return;

   KBackgroundProgram prog(m_selectedProgram);
   if (prog.isGlobal())
   {
      KMessageBox::sorry(this,
		i18n("Unable to remove the program: the program is global "
		"and can only be removed by the system administrator."),
		i18n("Cannot Remove Program"));
      return;
   }
   if (KMessageBox::warningContinueCancel(this,
	    i18n("Are you sure you want to remove the program `%1'?")
	    .arg(prog.name()),
	    i18n("Remove Background Program"),
	    i18n("&Remove")) != KMessageBox::Continue)
      return;

   prog.remove();
   removeProgram(m_selectedProgram);
   m_selectedProgram = TQString::null;
}

/*
 * Modify a program.
 */
void BGAdvancedDialog::slotModify()
{
   if (m_selectedProgram.isEmpty())
      return;

   KProgramEditDialog dlg(m_selectedProgram);
   dlg.exec();
   if (dlg.result() == TQDialog::Accepted)
   {
      TQString program = dlg.program();
      if (program != m_selectedProgram)
      {
         KBackgroundProgram prog(m_selectedProgram);
         prog.remove();
	 removeProgram(m_selectedProgram);
      }
      addProgram(dlg.program());
      selectProgram(dlg.program());
   }
}

void BGAdvancedDialog::slotProgramItemClicked(TQListViewItem *item)
{
   if (item)
      m_selectedProgram = item->text(0);
   slotProgramChanged();
}

void BGAdvancedDialog::slotProgramItemDoubleClicked(TQListViewItem *item)
{
   slotProgramItemClicked(item);
   slotModify();
}

void BGAdvancedDialog::slotProgramChanged()
{
   if (dlg->m_cbProgram->isChecked() && !m_selectedProgram.isEmpty())
      m_backgroundMode = KBackgroundSettings::Program;
   else
      m_backgroundMode = m_oldBackgroundMode;
}

void BGAdvancedDialog::slotEnableProgram(bool b)
{
   dlg->m_listPrograms->setEnabled(b);
   if (b)
   {
      dlg->m_listPrograms->blockSignals(true);
      TQListViewItem *cur = dlg->m_listPrograms->currentItem();
      dlg->m_listPrograms->setSelected(cur, true);
      dlg->m_listPrograms->ensureItemVisible(cur);
      dlg->m_listPrograms->blockSignals(false);
      slotProgramItemClicked(cur);
   }
   else
   {
      slotProgramChanged();
   }
}

TQString BGAdvancedDialog::backgroundProgram() const
{
   return m_selectedProgram;
}

int BGAdvancedDialog::backgroundMode() const
{
   return m_backgroundMode;
}

/**** KProgramEditDialog ****/

KProgramEditDialog::KProgramEditDialog(const TQString &program, TQWidget *parent, char *name)
    : KDialogBase(parent, name, true, i18n("Configure Background Program"),
	Ok | Cancel, Ok, true)
{
    TQFrame *frame = makeMainWidget();

    TQGridLayout *grid = new TQGridLayout(frame, 6, 2, 0, spacingHint());
    grid->addColSpacing(1, 300);

    TQLabel *lbl = new TQLabel(i18n("&Name:"), frame);
    grid->addWidget(lbl, 0, 0);
    m_NameEdit = new TQLineEdit(frame);
    lbl->setBuddy(m_NameEdit);
    grid->addWidget(m_NameEdit, 0, 1);

    lbl = new TQLabel(i18n("Co&mment:"), frame);
    grid->addWidget(lbl, 1, 0);
    m_CommentEdit = new TQLineEdit(frame);
    lbl->setBuddy(m_CommentEdit);
    grid->addWidget(m_CommentEdit, 1, 1);

    lbl = new TQLabel(i18n("Comman&d:"), frame);
    grid->addWidget(lbl, 2, 0);
    m_CommandEdit = new TQLineEdit(frame);
    lbl->setBuddy(m_CommandEdit);
    grid->addWidget(m_CommandEdit, 2, 1);

    lbl = new TQLabel(i18n("&Preview cmd:"), frame);
    grid->addWidget(lbl, 3, 0);
    m_PreviewEdit = new TQLineEdit(frame);
    lbl->setBuddy(m_PreviewEdit);
    grid->addWidget(m_PreviewEdit, 3, 1);

    lbl = new TQLabel(i18n("&Executable:"), frame);
    grid->addWidget(lbl, 4, 0);
    m_ExecEdit = new TQLineEdit(frame);
    lbl->setBuddy(m_ExecEdit);
    grid->addWidget(m_ExecEdit, 4, 1);

    lbl = new TQLabel(i18n("&Refresh time:"), frame);
    grid->addWidget(lbl, 5, 0);
    m_RefreshEdit = new TQSpinBox(frame);
    m_RefreshEdit->setRange(5, 60);
    m_RefreshEdit->setSteps(5, 10);
    m_RefreshEdit->setSuffix(i18n(" min"));
    m_RefreshEdit->setFixedSize(m_RefreshEdit->sizeHint());
    lbl->setBuddy(m_RefreshEdit);
    grid->addWidget(m_RefreshEdit, 5, 1, Qt::AlignLeft);

    m_Program = program;
    if (m_Program.isEmpty()) {
	KBackgroundProgram prog(i18n("New Command"));
	int i = 1;
	while (!prog.command().isEmpty())
	    prog.load(i18n("New Command <%1>").arg(i++));
	m_NameEdit->setText(prog.name());
	m_NameEdit->setSelection(0, 100);
	m_RefreshEdit->setValue(15);
	return;
    }

    // Fill in the fields
    m_NameEdit->setText(m_Program);
    KBackgroundProgram prog(m_Program);
    m_CommentEdit->setText(prog.comment());
    m_ExecEdit->setText(prog.executable());
    m_CommandEdit->setText(prog.command());
    m_PreviewEdit->setText(prog.previewCommand());
    m_RefreshEdit->setValue(prog.refresh());
}


TQString KProgramEditDialog::program()const
{
    return m_NameEdit->text();
}

void KProgramEditDialog::slotOk()
{
    TQString s = m_NameEdit->text();
    if (s.isEmpty()) {
	KMessageBox::sorry(this, i18n("You did not fill in the `Name' field.\n"
		"This is a required field."));
	return;
    }

    KBackgroundProgram prog(s);
    if ((s != m_Program) && !prog.command().isEmpty()) {
	int ret = KMessageBox::warningContinueCancel(this,
	    i18n("There is already a program with the name `%1'.\n"
	    "Do you want to overwrite it?").arg(s),TQString::null,i18n("Overwrite"));
	if (ret != KMessageBox::Continue)
	    return;
    }

    if (m_ExecEdit->text().isEmpty()) {
	KMessageBox::sorry(this, i18n("You did not fill in the `Executable' field.\n"
		"This is a required field."));
	return;
    }
    if (m_CommandEdit->text().isEmpty()) {
	KMessageBox::sorry(this, i18n("You did not fill in the `Command' field.\n"
		"This is a required field."));
	return;
    }

    prog.setComment(m_CommentEdit->text());
    prog.setExecutable(m_ExecEdit->text());
    prog.setCommand(m_CommandEdit->text());
    prog.setPreviewCommand(m_PreviewEdit->text());
    prog.setRefresh(m_RefreshEdit->value());

    prog.writeSettings();
    accept();
}


#include "bgadvanced.moc"
