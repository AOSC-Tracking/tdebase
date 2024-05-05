/*
 *	This file contains the B2 configuration widget
 *
 *	Copyright (c) 2001
 *		Karol Szwed <gallium@kde.org>
 *		http://gallium.n3.net/
 */

#include "config.h"
#include <tdeglobal.h>
#include <tqwhatsthis.h>
#include <tqvbox.h>
#include <tdelocale.h>


extern "C"
{
	TDE_EXPORT TQObject* allocate_config( TDEConfig* conf, TQWidget* parent )
	{
		return(new B2Config(conf, parent));
	}
}


/* NOTE:
 * 'conf' 	is a pointer to the twindecoration modules open twin config,
 *			and is by default set to the "Style" group.
 *
 * 'parent'	is the parent of the TQObject, which is a VBox inside the
 *			Configure tab in twindecoration
 */

B2Config::B2Config( TDEConfig* conf, TQWidget* parent )
	: TQObject( parent )
{
	TDEGlobal::locale()->insertCatalogue("twin_b2_config");
	b2Config = new TDEConfig("twinb2rc");
	gb = new TQVBox(parent);

	cbColorBorder = new TQCheckBox(
			i18n("Draw window frames using &titlebar colors"), gb);
	TQWhatsThis::add(cbColorBorder,
			i18n("When selected, the window borders "
				"are drawn using the titlebar colors; otherwise, they are "
				"drawn using normal border colors."));

	// Grab Handle
    showGrabHandleCb = new TQCheckBox(
	    i18n("Draw &resize handle"), gb);
    TQWhatsThis::add(showGrabHandleCb,
	    i18n("When selected, decorations are drawn with a \"grab handle\" "
		 "in the bottom right corner of the windows; "
		 "otherwise, no grab handle is drawn."));

    // Double click menu option support
    actionsGB = new TQHGroupBox(i18n("Actions Settings"), gb);
    TQLabel *menuDblClickLabel = new TQLabel(actionsGB);
    menuDblClickLabel->setText(i18n("Double click on menu button:"));
    menuDblClickOp = new TQComboBox(actionsGB);
    menuDblClickOp->insertItem(i18n("Do Nothing"));
    menuDblClickOp->insertItem(i18n("Minimize Window"));
    menuDblClickOp->insertItem(i18n("Shade Window"));
    menuDblClickOp->insertItem(i18n("Close Window"));

    TQWhatsThis::add(menuDblClickOp,
	    i18n("An action can be associated to a double click "
		 "of the menu button. Leave it to none if in doubt."));

	// Load configuration options
	load(conf);

	// Ensure we track user changes properly
	connect(cbColorBorder, TQ_SIGNAL(clicked()),
			this, TQ_SLOT(slotSelectionChanged()));
    connect(showGrabHandleCb, TQ_SIGNAL(clicked()),
		    this, TQ_SLOT(slotSelectionChanged()));
    connect(menuDblClickOp, TQ_SIGNAL(activated(int)),
		    this, TQ_SLOT(slotSelectionChanged()));
	// Make the widgets visible in twindecoration
	gb->show();
}


B2Config::~B2Config()
{
    delete b2Config;
	delete gb;
}


void B2Config::slotSelectionChanged()
{
	emit changed();
}


// Loads the configurable options from the twinrc config file
// It is passed the open config from twindecoration to improve efficiency
void B2Config::load(TDEConfig * /*conf*/)
{
	b2Config->setGroup("General");

	bool override = b2Config->readBoolEntry("UseTitleBarBorderColors", false);
	cbColorBorder->setChecked(override);

    override = b2Config->readBoolEntry( "DrawGrabHandle", true );
    showGrabHandleCb->setChecked(override);

    TQString returnString = b2Config->readEntry(
					"MenuButtonDoubleClickOperation", "NoOp");

    int op;
    if (returnString == "Close") {
		op = 3;
	} else if (returnString == "Shade") {
		op = 2;
    } else if (returnString == "Minimize") {
		op = 1;
    } else {
		op = 0;
    }

    menuDblClickOp->setCurrentItem(op);

}

static TQString opToString(int op)
{
    switch (op) {
    case 1:
	    return "Minimize";
    case 2:
	    return "Shade";
    case 3:
	    return "Close";
    case 0:
    default:
	    return "NoOp";
    }
}


// Saves the configurable options to the twinrc config file
void B2Config::save(TDEConfig * /*conf*/)
{
	b2Config->setGroup("General");
	b2Config->writeEntry("UseTitleBarBorderColors", cbColorBorder->isChecked());
    b2Config->writeEntry("DrawGrabHandle", showGrabHandleCb->isChecked());
    b2Config->writeEntry("MenuButtonDoubleClickOperation",
	    opToString(menuDblClickOp->currentItem()));
	// Ensure others trying to read this config get updated
	b2Config->sync();
}


// Sets UI widget defaults which must correspond to style defaults
void B2Config::defaults()
{
	cbColorBorder->setChecked(false);
    showGrabHandleCb->setChecked(true);
    menuDblClickOp->setCurrentItem(0);
}

#include "config.moc"
