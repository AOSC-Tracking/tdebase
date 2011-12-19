/*
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
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
 */

#include <tqcheckbox.h>
#include <tqgroupbox.h>
#include <tqdir.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqcombobox.h>
#include <tqbuttongroup.h>
#include <tqlineedit.h>

#include <dcopref.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knuminput.h>
#include <kstandarddirs.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include "main.h"

#include "kickerSettings.h"

#include "menutab_impl.h"
#include "menutab_impl.moc"

kSubMenuItem::kSubMenuItem(TQListView* parent,
                           const TQString& visibleName,
                           const TQString& desktopFile,
                           const TQPixmap& icon,
                           bool checked)
    : TQCheckListItem(parent, visibleName, TQCheckListItem::CheckBox),
      m_desktopFile(desktopFile)
{
    setPixmap(0, icon);
    setOn(checked);
}

TQString kSubMenuItem::desktopFile()
{
    return m_desktopFile;
}

void kSubMenuItem::stateChange(bool state)
{
    emit toggled(state);
}

MenuTab::MenuTab( TQWidget *parent, const char* name )
  : MenuTabBase (parent, name),
    m_bookmarkMenu(0),
    m_quickBrowserMenu(0),
    m_kmenu_button_changed(false)
{
    // connections
    connect(m_editKMenuButton, TQT_SIGNAL(clicked()), TQT_SLOT(launchMenuEditor()));
    connect(btnCustomKMenuIcon, TQT_SIGNAL(clicked()), TQT_SLOT(launchIconEditor()));
    connect(kcfg_KMenuText, TQT_SIGNAL(textChanged(const TQString&)), TQT_SLOT(kmenuChanged()));
    connect(kcfg_ShowKMenuText, TQT_SIGNAL(toggled(bool)), TQT_SLOT(kmenuChanged()));
    //connect(kcfg_ButtonFont, TQT_SIGNAL(fontSelected(const TQFont &)), TQT_SLOT(kmenuChanged()));
    connect(maxrecentdocs, TQT_SIGNAL(valueChanged(int)), this, TQT_SLOT(kmenuChanged()));

    KIconLoader * ldr = KGlobal::iconLoader();
    TQPixmap kmenu_icon;
    m_kmenu_icon = KickerSettings::customKMenuIcon();
    if (m_kmenu_icon.isNull() == true) {
        m_kmenu_icon = TQString("kmenu");
    }
    kmenu_icon = ldr->loadIcon(m_kmenu_icon, KIcon::Small, KIcon::SizeSmall);
    btnCustomKMenuIcon->setPixmap(kmenu_icon);

    KConfig *config;
    config = new KConfig(TQString::fromLatin1("kdeglobals"), false, false);
    config->setGroup(TQString::fromLatin1("RecentDocuments"));
    maxrecentdocs->setValue(config->readNumEntry(TQString::fromLatin1("MaxEntries"), 10));

    m_browserGroupLayout->setColStretch( 1, 1 );
    m_pRecentOrderGroupLayout->setColStretch( 1, 1 );
}

void MenuTab::load()
{
   load( false );
}

void MenuTab::load( bool useDefaults )
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig(KickerConfig::the()->configName());
    
    c->setReadDefaults( useDefaults );

    c->setGroup("menus");

    m_subMenus->clear();

    // show the bookmark menu?
    m_bookmarkMenu = new kSubMenuItem(m_subMenus,
                                      i18n("Bookmarks"),
                                      TQString::null,
                                      SmallIcon("bookmark"),
                                      c->readBoolEntry("UseBookmarks", false));
    connect(m_bookmarkMenu, TQT_SIGNAL(toggled(bool)), TQT_SIGNAL(changed()));

    // show the quick menus menu?
    m_quickBrowserMenu = new kSubMenuItem(m_subMenus,
                                          i18n("Quick Browser"),
                                          TQString::null,
                                          SmallIcon("kdisknav"),
                                          c->readBoolEntry("UseBrowser", false));
    connect(m_quickBrowserMenu, TQT_SIGNAL(toggled(bool)), TQT_SIGNAL(changed()));

    TQStringList ext_default;
    ext_default << "prefmenu.desktop" << "systemmenu.desktop";
    TQStringList ext = c->readListEntry("Extensions", ext_default);
    TQStringList dirs = KGlobal::dirs()->findDirs("data", "kicker/menuext");
    kSubMenuItem* menuItem(0);
    for (TQStringList::ConstIterator dit=dirs.begin(); dit!=dirs.end(); ++dit)
    {
        TQDir d(*dit, "*.desktop");
        TQStringList av = d.entryList();
        for (TQStringList::ConstIterator it=av.begin(); it!=av.end(); ++it)
        {
            KDesktopFile df(d.absFilePath(*it), true);
            menuItem = new kSubMenuItem(m_subMenus,
                                        df.readName(),
                                        *it,
                                        SmallIcon(df.readIcon()),
                                        tqFind(ext.begin(), ext.end(), *it) != ext.end());
            connect(menuItem, TQT_SIGNAL(toggled(bool)), TQT_SIGNAL(changed()));
        }
    }

    c->setGroup("General");
    m_comboMenuStyle->setCurrentItem( c->readBoolEntry("LegacyKMenu", true) ? 1 : 0 );
    m_openOnHover->setChecked( c->readBoolEntry("OpenOnHover", true) );
    menuStyleChanged();

    connect(m_comboMenuStyle, TQT_SIGNAL(activated(int)), TQT_SIGNAL(changed()));
    connect(m_comboMenuStyle, TQT_SIGNAL(activated(int)), TQT_SLOT(menuStyleChanged()));
    connect(m_openOnHover, TQT_SIGNAL(clicked()), TQT_SIGNAL(changed()));

    m_showFrequent->setChecked(true);

    if ( useDefaults )
       emit changed();
}

void MenuTab::menuStyleChanged()
{
    if (m_comboMenuStyle->currentItem()==1) {
       m_openOnHover->setEnabled(false);
       m_subMenus->setEnabled(true);
       kcfg_UseSidePixmap->setEnabled(true);
       kcfg_MenuEntryFormat->setEnabled(true);
       kcfg_RecentVsOften->setEnabled(true);
       m_showFrequent->setEnabled(true);
       kcfg_UseSearchBar->setEnabled(true);
       kcfg_MaxEntries2->setEnabled(true);
       maxrecentdocs->setEnabled(true);
       kcfg_NumVisibleEntries->setEnabled(true);
    }
    else {
       m_openOnHover->setEnabled(true);
       m_subMenus->setEnabled(false);
       kcfg_UseSidePixmap->setEnabled(false);
       kcfg_MenuEntryFormat->setEnabled(false);
       kcfg_RecentVsOften->setEnabled(false);
       m_showFrequent->setEnabled(false);
       kcfg_UseSearchBar->setEnabled(false);
       kcfg_MaxEntries2->setEnabled(false);
       maxrecentdocs->setEnabled(false);
       kcfg_NumVisibleEntries->setEnabled(false);
    }
}

void MenuTab::save()
{
    KSharedConfig::Ptr c = KSharedConfig::openConfig(KickerConfig::the()->configName());

    c->setGroup("menus");

    TQStringList ext;
    TQListViewItem *item(0);
    for (item = m_subMenus->firstChild(); item; item = item->nextSibling())
    {
        bool isOn = static_cast<kSubMenuItem*>(item)->isOn();
        if (item == m_bookmarkMenu)
        {
            c->writeEntry("UseBookmarks", isOn);
        }
        else if (item == m_quickBrowserMenu)
        {
            c->writeEntry("UseBrowser", isOn);
        }
        else if (isOn)
        {
            ext << static_cast<kSubMenuItem*>(item)->desktopFile();
        }
    }
    c->writeEntry("Extensions", ext);
    c->setGroup("General");

    bool kmenusetting = m_comboMenuStyle->currentItem()==1;
    bool oldkmenusetting = c->readBoolEntry("LegacyKMenu", true);

    c->writeEntry("LegacyKMenu", kmenusetting);
    c->writeEntry("OpenOnHover", m_openOnHover->isChecked());
    c->sync();

    if (kmenusetting != oldkmenusetting)
        DCOPRef ("kicker", "default").call("restart()");

    c->setGroup("KMenu");
    bool sidepixmapsetting = kcfg_UseSidePixmap->isChecked();
    bool oldsidepixmapsetting = c->readBoolEntry("UseSidePixmap", true);

    if (sidepixmapsetting != oldsidepixmapsetting)
        DCOPRef ("kicker", "default").call("restart()");

    // Save KMenu settings
    c->setGroup("KMenu");
    c->writeEntry("CustomIcon", m_kmenu_icon);
    c->sync();

    // Save recent documents
    KConfig *config;
    config = new KConfig(TQString::fromLatin1("kdeglobals"), false, false);
    config->setGroup(TQString::fromLatin1("RecentDocuments"));
    config->writeEntry("MaxEntries", maxrecentdocs->value());
    config->sync();

    if (m_kmenu_button_changed == true) {
        DCOPRef ("kicker", "default").call("restart()");
    }
}

void MenuTab::defaults()
{
   load( true );
}

void MenuTab::launchMenuEditor()
{
    if ( KApplication::startServiceByDesktopName( "kmenuedit",
                                                  TQString::null /*url*/,
                                                  0 /*error*/,
                                                  0 /*dcopservice*/,
                                                  0 /*pid*/,
                                                  "" /*startup_id*/,
                                                  true /*nowait*/ ) != 0 )
    {
        KMessageBox::error(this,
                           i18n("The KDE menu editor (kmenuedit) could not be launched.\n"
                           "Perhaps it is not installed or not in your path."),
                           i18n("Application Missing"));
    }
}

void MenuTab::launchIconEditor()
{
    KIconDialog dlg(this);
    TQString newIcon = dlg.selectIcon(KIcon::Small, KIcon::Application);
    if (newIcon.isEmpty())
        return;

    m_kmenu_icon = newIcon;
    KIconLoader * ldr = KGlobal::iconLoader();
    TQPixmap kmenu_icon;
    kmenu_icon = ldr->loadIcon(m_kmenu_icon, KIcon::Small, KIcon::SizeSmall);
    btnCustomKMenuIcon->setPixmap(kmenu_icon);
    m_kmenu_button_changed = true;

    emit changed();
}

void MenuTab::kmenuChanged()
{
    //m_kmenu_button_changed = true;
    emit changed();
}
