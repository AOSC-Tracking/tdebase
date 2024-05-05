/*
Copyright (c) 2003 Maksim Orlovich <maksim.orlovich@kdemail.net>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <tqcheckbox.h>
#include <tqlayout.h>
#include <tqsettings.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdelocale.h>

#include "keramikconf.h"

extern "C"
{
	TDE_EXPORT TQWidget* allocate_tdestyle_config(TQWidget* parent)
	{
		return new KeramikStyleConfig(parent);
	}
}

KeramikStyleConfig::KeramikStyleConfig(TQWidget* parent): TQWidget(parent)
{
	//Should have no margins here, the dialog provides them
	TQVBoxLayout* layout = new TQVBoxLayout(this, 0, 0);
	TDEGlobal::locale()->insertCatalogue("tdestyle_keramik_config");

	//highlightLineEdits = new TQCheckBox(i18n("Highlight active lineedits"), this);
	highlightScrollBar = new TQCheckBox(i18n("Highlight scroll bar handles"), this);
	animateProgressBar = new TQCheckBox(i18n("Animate progress bars"), this);

	//layout->add(highlightLineEdits);
	layout->add(highlightScrollBar);
	layout->add(animateProgressBar);
	layout->addStretch(1);

	TQSettings s;
	//origHlLineEdit = s.readBoolEntry("/keramik/Settings/highlightLineEdits", false);
	//highlightLineEdits->setChecked(origHlLineEdit);

	origHlScrollbar = s.readBoolEntry("/keramik/Settings/highlightScrollBar", true);
	highlightScrollBar->setChecked(origHlScrollbar);

	origAnimProgressBar = s.readBoolEntry("/keramik/Settings/animateProgressBar", false);
	animateProgressBar->setChecked(origAnimProgressBar);
	
	//connect(highlightLineEdits, TQ_SIGNAL( toggled(bool) ), TQ_SLOT( updateChanged() ) );
	connect(highlightScrollBar, TQ_SIGNAL( toggled(bool) ), TQ_SLOT( updateChanged() ) );
	connect(animateProgressBar, TQ_SIGNAL( toggled(bool) ), TQ_SLOT( updateChanged() ) );
}

KeramikStyleConfig::~KeramikStyleConfig()
{
	TDEGlobal::locale()->removeCatalogue("tdestyle_keramik_config");
}


void KeramikStyleConfig::save()
{
	TQSettings s;
	//s.writeEntry("/keramik/Settings/highlightLineEdits", highlightLineEdits->isChecked());
	s.writeEntry("/keramik/Settings/highlightScrollBar", highlightScrollBar->isChecked());
	s.writeEntry("/keramik/Settings/animateProgressBar", animateProgressBar->isChecked());
}

void KeramikStyleConfig::defaults()
{
	//highlightLineEdits->setChecked(false);
	highlightScrollBar->setChecked(true);
	animateProgressBar->setChecked(false);
	//updateChanged would be done by setChecked already
}

void KeramikStyleConfig::updateChanged()
{
	if ( /*(highlightLineEdits->isChecked() == origHlLineEdit)  &&*/
		 (highlightScrollBar->isChecked() == origHlScrollbar) &&
		 (animateProgressBar->isChecked() == origAnimProgressBar) )
		emit changed(false);
	else
		emit changed(true);
}

#include "keramikconf.moc"
