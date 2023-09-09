//
// C++ Implementation: kxkbtraywindow
//
// Description:
//
//
// Author: Andriy Rysin <rysin@kde.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <tqtooltip.h>

#include <kdebug.h>
#include <tdelocale.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <tdepopupmenu.h>
#include <tdeaction.h>
#include <kuniqueapplication.h>

#include "kxkbtraywindow.h"
#include "pixmap.h"
#include "rules.h"
#include "kxkbconfig.h"


KxkbLabelController::KxkbLabelController(TQLabel* label_, TDEPopupMenu* contextMenu_) :
    label(label_),
    contextMenu(contextMenu_),
 	m_menuStartIndex(contextMenu_->count()),
	m_prevLayoutCount(0)
{
// 	kdDebug() << "Creating KxkbLabelController with " << label_ << ", " << contextMenu_ << endl;
// 	kdDebug() << "Creating KxkbLabelController with startMenuIndex " << m_menuStartIndex << endl;
}

void KxkbLabelController::setToolTip(const TQString& tip)
{
	TQToolTip::remove(label);
	TQToolTip::add(label, tip);
}

void KxkbLabelController::setPixmap(const TQPixmap& pixmap)
{
	TDEIconEffect iconeffect;
	label->setPixmap( iconeffect.apply(pixmap, TDEIcon::Panel, TDEIcon::DefaultState) );
}


void KxkbLabelController::setCurrentLayout(const LayoutUnit& layoutUnit)
{
	setToolTip(m_descriptionMap[layoutUnit.toPair()]);
	setPixmap( LayoutIcon::getInstance().findPixmap(layoutUnit.layout, PIXMAP_STYLE_INDICATOR, layoutUnit.displayName) );
}


void KxkbLabelController::setError(const TQString& layoutInfo)
{
    TQString msg = i18n("Error changing keyboard layout to '%1'").arg(layoutInfo);
	setToolTip(msg);

	label->setPixmap(LayoutIcon::getInstance().findPixmap("error", PIXMAP_STYLE_NORMAL));
}


void KxkbLabelController::initLayoutList(const TQValueList<LayoutUnit>& layouts, const XkbRules& rules)
{
	TDEPopupMenu* menu = contextMenu;
//	TQPopupMenu* menu = contextMenu;
//	int index = menu->indexOf(0);

    m_descriptionMap.clear();
//    menu->clear();
//    menu->insertTitle( kapp->miniIcon(), kapp->caption() );

	for(int ii=0; ii<m_prevLayoutCount; ++ii) {
		menu->removeItem(START_MENU_ID + ii);
		kdDebug() << "remove item: " << START_MENU_ID + ii << endl;
	}
/*	menu->removeItem(CONFIG_MENU_ID);
	menu->removeItem(HELP_MENU_ID);*/

    TDEIconEffect iconeffect;

	int cnt = 0;
    TQValueList<LayoutUnit>::ConstIterator it;
    for (it=layouts.begin(); it != layouts.end(); ++it)
    {
		const TQString layoutName = (*it).layout;
		const TQString variantName = (*it).variant;

		const TQPixmap& layoutPixmap = LayoutIcon::getInstance().findPixmap(layoutName, PIXMAP_STYLE_CONTEXTMENU, (*it).displayName);
        const TQPixmap pix = iconeffect.apply(layoutPixmap, TDEIcon::Small, TDEIcon::DefaultState);

		TQString fullName = i18n((rules.layouts()[layoutName]));
		if( variantName.isEmpty() == false )
			fullName += " (" + variantName + ")";
		contextMenu->insertItem(pix, fullName, START_MENU_ID + cnt, m_menuStartIndex + cnt);
		m_descriptionMap.insert((*it).toPair(), fullName);

		cnt++;
    }

	m_prevLayoutCount = cnt;

	// if show config, if show help
	if( menu->indexOf(CONFIG_MENU_ID) == -1 ) {
		contextMenu->insertSeparator();
		contextMenu->insertItem(SmallIcon("configure"), i18n("Configure…"), CONFIG_MENU_ID);
		if( menu->indexOf(HELP_MENU_ID) == -1 )
			contextMenu->insertItem(SmallIcon("help"), i18n("Help"), HELP_MENU_ID);
	}

/*    if( index != -1 ) { //not first start
		menu->insertSeparator();
		TDEAction* quitAction = KStdAction::quit(this, TQT_SIGNAL(quitSelected()), actionCollection());
        if (quitAction)
    	    quitAction->plug(menu);
    }*/
}

// void KxkbLabelController::mouseReleaseEvent(TQMouseEvent *ev)
// {
//     if (ev->button() == TQMouseEvent::LeftButton)
//         emit toggled();
//     KSystemTray::mouseReleaseEvent(ev);
// }

#include "kxkbtraywindow.moc"
