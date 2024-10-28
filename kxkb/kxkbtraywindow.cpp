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

KxkbSystemTray::KxkbSystemTray(KxkbConfig *kxkbConfig)
    : KSystemTray(nullptr),
      m_prevLayoutCount(0)
{
    m_icoMgr = new LayoutIconManager(kxkbConfig);
}

KxkbSystemTray::~KxkbSystemTray()
{
    delete m_icoMgr;
}

void KxkbSystemTray::setToolTip(const TQString& tip)
{
    TQToolTip::remove(this);
    TQToolTip::add(this, tip);
}

void KxkbSystemTray::setPixmap(const TQPixmap& pix)
{
    TDEIconEffect iconeffect;
    KSystemTray::setPixmap(iconeffect.apply(pix, TDEIcon::Panel, TDEIcon::DefaultState));
}

void KxkbSystemTray::setCurrentLayout(const LayoutUnit& layoutUnit)
{
    setToolTip(m_descriptionMap[layoutUnit.toPair()]);
    setPixmap(m_icoMgr->find(layoutUnit.layout, PIXMAP_STYLE_INDICATOR, layoutUnit.displayName));
}

void KxkbSystemTray::setError(const TQString& layoutInfo)
{
    TQString layout(layoutInfo);
    if (layout.isNull()) {
        layout = i18n("Unknown");
    }

    TQString msg = i18n("Error changing keyboard layout to '%1'").arg(layoutInfo);
    setToolTip(msg);
    setPixmap(m_icoMgr->find(ERROR_CODE, PIXMAP_STYLE_NORMAL));
}

void KxkbSystemTray::initLayoutList(const TQValueList<LayoutUnit>& layouts, const XkbRules& rules)
{
    m_descriptionMap.clear();
//    menu->clear();
//    menu->insertTitle( kapp->miniIcon(), kapp->caption() );

    int i;
    for (i = 0; i < m_prevLayoutCount; ++i) {
        contextMenu()->removeItem(START_MENU_ID + i);
    }

    TDEIconEffect iconeffect;

    i = 0;
    TQValueList<LayoutUnit>::ConstIterator it;
    for (it = layouts.begin(); it != layouts.end(); ++it)
    {
        const TQString layoutName = (*it).layout;
        const TQString variantName = (*it).variant;

        const TQPixmap& layoutPixmap = m_icoMgr->find((*it).layout, PIXMAP_STYLE_CONTEXTMENU, (*it).displayName);
        const TQPixmap pix = iconeffect.apply(layoutPixmap, TDEIcon::Small, TDEIcon::DefaultState);

        TQString fullName = rules.getLayoutName((*it));
        contextMenu()->insertItem(pix, fullName, START_MENU_ID + i, i + 1);

        m_descriptionMap.insert((*it).toPair(), fullName);

        ++i;
    }

    m_prevLayoutCount = i;

    if (contextMenu()->indexOf(CONFIG_MENU_ID) == -1) {
        contextMenu()->insertSeparator();
        contextMenu()->insertItem(SmallIcon("configure"), i18n("Configure..."), CONFIG_MENU_ID);

        if (contextMenu()->indexOf(HELP_MENU_ID) == -1) {
            contextMenu()->insertItem(SmallIcon("help"), i18n("Help"), HELP_MENU_ID);
        }
    }

    connect(contextMenu(), TQ_SIGNAL(activated(int)), this, TQ_SIGNAL(menuActivated(int)));
}

void KxkbSystemTray::mouseReleaseEvent(TQMouseEvent *ev) {
    if (ev->button() == TQt::LeftButton) {
        emit toggled();
    }
    KSystemTray::mouseReleaseEvent(ev);
}

#include "kxkbtraywindow.moc"
