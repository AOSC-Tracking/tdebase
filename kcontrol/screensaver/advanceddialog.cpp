#include <tdelocale.h>
#include <tdestandarddirs.h>
#include <tqcombobox.h>
#include <kdebug.h>

#include <tqwhatsthis.h>
#include <tqstring.h>

#include <config.h>

#include "advanceddialog.h"
#include "advanceddialogimpl.h"
#include "stdlib.h"

#include "advanceddialog.moc"

KScreenSaverAdvancedDialog::KScreenSaverAdvancedDialog(TQWidget *parent, const char* name)
 : KDialogBase( parent, name, true, i18n( "Advanced Options" ),
                Ok | Cancel, Ok, true )
{
	
	dialog = new AdvancedDialog(this);
	setMainWidget(dialog);

	readSettings();

	connect(dialog->qcbPriority, TQ_SIGNAL(activated(int)),
		this, TQ_SLOT(slotPriorityChanged(int)));

	connect(dialog->qcbTopLeft, TQ_SIGNAL(activated(int)),
		this, TQ_SLOT(slotChangeTopLeftCorner(int)));
	connect(dialog->qcbTopRight, TQ_SIGNAL(activated(int)),
		this, TQ_SLOT(slotChangeTopLeftCorner(int)));
	connect(dialog->qcbBottomLeft, TQ_SIGNAL(activated(int)),
		this, TQ_SLOT(slotChangeTopLeftCorner(int)));
	connect(dialog->qcbBottomRight, TQ_SIGNAL(activated(int)),
		this, TQ_SLOT(slotChangeTopLeftCorner(int)));

#ifndef HAVE_SETPRIORITY
    dialog->qgbPriority->setEnabled(false);
#endif
}

void KScreenSaverAdvancedDialog::readSettings()
{
	TDEConfig *config = new TDEConfig("kdesktoprc");
	config->setGroup("ScreenSaver");
	
	mPriority = config->readNumEntry("Priority", 19);
	if (mPriority < 0) mPriority = 0;
	if (mPriority > 19) mPriority = 19;
	
	dialog->qcbTopLeft->setCurrentItem(config->readNumEntry("ActionTopLeft", 0));    
	dialog->qcbTopRight->setCurrentItem(config->readNumEntry("ActionTopRight", 0));
	dialog->qcbBottomLeft->setCurrentItem(config->readNumEntry("ActionBottomLeft", 0));
	dialog->qcbBottomRight->setCurrentItem(config->readNumEntry("ActionBottomRight", 0));


	switch(mPriority)
	{
		case 19: // Low
			dialog->qcbPriority->setCurrentItem(0);
			kdDebug() << "setting low" << endl;
			break;
		case 10: // Medium
			dialog->qcbPriority->setCurrentItem(1);
			kdDebug() << "setting medium" << endl;
			break;
		case 0: // High
			dialog->qcbPriority->setCurrentItem(2);
			kdDebug() << "setting high" << endl;
			break;
	}
	
	mChanged = false;
	delete config;
}

void KScreenSaverAdvancedDialog::slotPriorityChanged(int val)
{
	switch (val)
	{
		case 0: // Low
			mPriority = 19;
			kdDebug() << "low priority" << endl;
			break;
		case 1: // Medium
			mPriority = 10;
			kdDebug() << "medium priority" << endl;
			break;
		case 2: // High
			mPriority = 0;
			kdDebug() << "high priority" << endl;
			break;
	}
	mChanged = true;
}

void KScreenSaverAdvancedDialog::slotOk()
{
	if (mChanged)
	{
		TDEConfig *config = new TDEConfig("kdesktoprc");
		config->setGroup( "ScreenSaver" );
	
		config->writeEntry("Priority", mPriority);
		config->writeEntry(
		"ActionTopLeft", dialog->qcbTopLeft->currentItem());
		config->writeEntry(
		"ActionTopRight", dialog->qcbTopRight->currentItem());
		config->writeEntry(
		"ActionBottomLeft", dialog->qcbBottomLeft->currentItem());
		config->writeEntry(
		"ActionBottomRight", dialog->qcbBottomRight->currentItem());
		config->sync();
		delete config;
	}
	accept();
}

void KScreenSaverAdvancedDialog::slotChangeBottomRightCorner(int)
{
	mChanged = true;
}

void KScreenSaverAdvancedDialog::slotChangeBottomLeftCorner(int)
{
	mChanged = true;
}

void KScreenSaverAdvancedDialog::slotChangeTopRightCorner(int)
{
	mChanged = true;
}

void KScreenSaverAdvancedDialog::slotChangeTopLeftCorner(int)
{
	mChanged = true;
}

/* =================================================================================================== */

AdvancedDialog::AdvancedDialog(TQWidget *parent, const char *name) : AdvancedDialogImpl(parent, name)
{
	monitorLabel->setPixmap(TQPixmap(locate("data", "kcontrol/pics/monitor.png")));
	TQWhatsThis::add(qcbPriority, "<qt>" + i18n("Specify the priority that the screensaver will run at. A higher priority may mean that the screensaver runs faster, though may reduce the speed that other programs run at while the screensaver is active.") + "</qt>");
	TQString qsTopLeft("<qt>" +  i18n("The action to take when the mouse cursor is located in the top left corner of the screen for 15 seconds.") + "</qt>");
        TQString qsTopRight("<qt>" +  i18n("The action to take when the mouse cursor is located in the top right corner of the screen for 15 seconds.") + "</qt>");
        TQString qsBottomLeft("<qt>" +  i18n("The action to take when the mouse cursor is located in the bottom left corner of the screen for 15 seconds.") + "</qt>");
        TQString qsBottomRight("<qt>" +  i18n("The action to take when the mouse cursor is located in the bottom right corner of the screen for 15 seconds.") + "</qt>");
	TQWhatsThis::add(qlTopLeft, qsTopLeft);
	TQWhatsThis::add(qcbTopLeft, qsTopLeft);
	TQWhatsThis::add(qlTopRight, qsTopRight);
	TQWhatsThis::add(qcbTopRight, qsTopRight);
	TQWhatsThis::add(qlBottomLeft, qsBottomLeft);
	TQWhatsThis::add(qcbBottomLeft, qsBottomLeft);
	TQWhatsThis::add(qlBottomRight, qsBottomRight);
	TQWhatsThis::add(qcbBottomRight, qsBottomRight);
}

AdvancedDialog::~AdvancedDialog()
{
 
}

void AdvancedDialog::setMode(TQComboBox *box, int i)
{
	box->setCurrentItem(i);
}

int AdvancedDialog::mode(TQComboBox *box)
{
	return box->currentItem();
}
