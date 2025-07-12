/* This file is part of the KDE Display Manager Configuration package
    Copyright (C) 1997 Thomas Tanghus (tanghus@earthling.net)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <unistd.h>
#include <sys/types.h>


#include <tqapplication.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqwhatsthis.h>

#include <kdialog.h>
#include <tdesimpleconfig.h>
#include <tdefontrequester.h>
#include <tdelocale.h>

#include "tdm-font.h"


extern TDESimpleConfig *config;

TDMFontWidget::TDMFontWidget(TQWidget *parent, const char *name)
  : TQWidget(parent, name)
{
  TQGridLayout *ml = new TQGridLayout(this, 5, 2, KDialog::marginHint(), KDialog::spacingHint());
  TQLabel *label = new TQLabel(i18n("&General:"), this);
  stdFontChooser = new TDEFontRequester(this);
  label->setBuddy(stdFontChooser);
  TQWhatsThis::add( stdFontChooser, i18n("This changes the font which is used for all the text in the login manager except for the greeting and failure messages.") );
  connect(stdFontChooser, TQ_SIGNAL(fontSelected(const TQFont&)),this,TQ_SLOT(configChanged()));
  ml->addWidget(label, 1, 0);
  ml->addWidget(stdFontChooser, 1, 1);

  label = new TQLabel(i18n("&Failures:"), this);
  failFontChooser = new TDEFontRequester(this);
  label->setBuddy(failFontChooser);
  TQWhatsThis::add( failFontChooser, i18n("This changes the font which is used for failure messages in the login manager.") );
  connect(failFontChooser, TQ_SIGNAL(fontSelected(const TQFont&)),this,TQ_SLOT(configChanged()));
  ml->addWidget(label, 2, 0);
  ml->addWidget(failFontChooser, 2, 1);

  label = new TQLabel(i18n("Gree&ting:"), this);
  greetingFontChooser = new TDEFontRequester(this);
  label->setBuddy(greetingFontChooser);
  TQWhatsThis::add( greetingFontChooser, i18n("This changes the font which is used for the login manager's greeting.") );
  connect(greetingFontChooser, TQ_SIGNAL(fontSelected(const TQFont&)),this,TQ_SLOT(configChanged()));
  ml->addWidget(label, 3, 0);
  ml->addWidget(greetingFontChooser, 3, 1);

  aacb = new TQCheckBox (i18n("Use anti-aliasing for fonts"), this);
  TQWhatsThis::add( aacb, i18n("If you check this box and your X-Server has the Xft extension, "
	"fonts will be antialiased (smoothed) in the login dialog.") );
  connect(aacb, TQ_SIGNAL(toggled ( bool )),this,TQ_SLOT(configChanged()));
  ml->addMultiCellWidget(aacb, 4, 4, 0, 1);
  ml->setRowStretch(5, 10);
}

void TDMFontWidget::makeReadOnly()
{
  stdFontChooser->button()->setEnabled(false);
  failFontChooser->button()->setEnabled(false);
  greetingFontChooser->button()->setEnabled(false);
  aacb->setEnabled(false);
}

void TDMFontWidget::configChanged()
{
    emit changed(true);
}

void TDMFontWidget::set_def()
{
  stdFontChooser->setFont(TQFont("Sans Serif", 10));
  failFontChooser->setFont(TQFont("Sans Serif", 10, TQFont::Bold));
  greetingFontChooser->setFont(TQFont("Sans Serif", 22));
}

void TDMFontWidget::save()
{
  config->setGroup("X-*-Greeter");

  // write font
  config->writeEntry("StdFont", stdFontChooser->font());
  config->writeEntry("GreetFont", greetingFontChooser->font());
  config->writeEntry("FailFont", failFontChooser->font());
  config->writeEntry("AntiAliasing", aacb->isChecked());
}


void TDMFontWidget::load()
{
  set_def();

  config->setGroup("X-*-Greeter");

  // Read the fonts
  TQFont font = stdFontChooser->font();
  stdFontChooser->setFont(config->readFontEntry("StdFont", &font));
  font = failFontChooser->font();
  failFontChooser->setFont(config->readFontEntry("FailFont", &font));
  font = greetingFontChooser->font();
  greetingFontChooser->setFont(config->readFontEntry("GreetFont",  &font));

  aacb->setChecked(config->readBoolEntry("AntiAliasing"));
}


void TDMFontWidget::defaults()
{
  set_def();
  aacb->setChecked(true);
}

#include "tdm-font.moc"
