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
#include <tdeapplication.h>
#include <kdebug.h>
#include <tdedesktopfile.h>
#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <knuminput.h>
#include <tdestandarddirs.h>
#include <tdefontrequester.h>
#include <kcolorbutton.h>
#include <kkeybutton.h>
#include <tdemessagebox.h>

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
    connect(m_editKMenuButton, TQ_SIGNAL(clicked()), TQ_SLOT(launchMenuEditor()));
    connect(btnCustomKMenuIcon, TQ_SIGNAL(clicked()), TQ_SLOT(launchIconEditor()));
    connect(kcfg_KMenuText, TQ_SIGNAL(textChanged(const TQString&)), TQ_SLOT(kmenuChanged()));
    connect(kcfg_ShowKMenuText, TQ_SIGNAL(toggled(bool)), TQ_SLOT(kmenuChanged()));
    //connect(kcfg_ButtonFont, TQ_SIGNAL(fontSelected(const TQFont &)), TQ_SLOT(kmenuChanged()));
    connect(maxrecentdocs, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(kmenuChanged()));

    // FIXME
    // When top pixmap support is ready for end-user visibility, replace
    // the separate top/side checkboxes with either a drop down or radio buttons
    kcfg_UseTopPixmap->hide();

    TDEIconLoader * ldr = TDEGlobal::iconLoader();
    m_kmenu_icon = KickerSettings::customKMenuIcon();
    if (m_kmenu_icon.isNull() == true) {
        m_kmenu_icon = TQString("kmenu");
    }
    TQIconSet kmenu_icon = ldr->loadIconSet(m_kmenu_icon, TDEIcon::Small, TDEIcon::SizeSmall);
    btnCustomKMenuIcon->setIconSet(kmenu_icon);

    TDEConfig *config;
    config = new TDEConfig(TQString::fromLatin1("kdeglobals"), false, false);
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
    TDESharedConfig::Ptr c = TDESharedConfig::openConfig(KickerConfig::the()->configName());

    c->setReadDefaults( useDefaults );

    c->setGroup("menus");

    m_subMenus->clear();

    // show the bookmark menu?
    m_bookmarkMenu = new kSubMenuItem(m_subMenus,
                                      i18n("Bookmarks"),
                                      TQString::null,
                                      SmallIcon("bookmark"),
                                      c->readBoolEntry("UseBookmarks", false));
    connect(m_bookmarkMenu, TQ_SIGNAL(toggled(bool)), TQ_SIGNAL(changed()));

    // show the quick menus menu?
    m_quickBrowserMenu = new kSubMenuItem(m_subMenus,
                                          i18n("Quick Browser"),
                                          TQString::null,
                                          SmallIcon("kdisknav"),
                                          c->readBoolEntry("UseBrowser", false));
    connect(m_quickBrowserMenu, TQ_SIGNAL(toggled(bool)), TQ_SIGNAL(changed()));

    TQStringList ext_default;
    ext_default << "prefmenu.desktop" << "systemmenu.desktop";
    TQStringList ext = c->readListEntry("Extensions", ext_default);
    TQStringList dirs = TDEGlobal::dirs()->findDirs("data", "kicker/menuext");
    kSubMenuItem* menuItem(0);
    for (TQStringList::ConstIterator dit=dirs.begin(); dit!=dirs.end(); ++dit)
    {
        TQDir d(*dit, "*.desktop");
        TQStringList av = d.entryList();
        for (TQStringList::ConstIterator it=av.begin(); it!=av.end(); ++it)
        {
            TDEDesktopFile df(d.absFilePath(*it), true);
            menuItem = new kSubMenuItem(m_subMenus,
                                        df.readName(),
                                        *it,
                                        SmallIcon(df.readIcon()),
                                        tqFind(ext.begin(), ext.end(), *it) != ext.end());
            connect(menuItem, TQ_SIGNAL(toggled(bool)), TQ_SIGNAL(changed()));
        }
    }

    c->setGroup("General");
    m_comboMenuStyle->setCurrentItem( c->readBoolEntry("LegacyKMenu", true) ? 1 : 0 );
    m_openOnHover->setChecked( c->readBoolEntry("OpenOnHover", true) );
    menuStyleChanged();

    connect(m_comboMenuStyle, TQ_SIGNAL(activated(int)), TQ_SIGNAL(changed()));
    connect(m_comboMenuStyle, TQ_SIGNAL(activated(int)), TQ_SLOT(menuStyleChanged()));
    connect(m_openOnHover, TQ_SIGNAL(clicked()), TQ_SIGNAL(changed()));

    m_showFrequent->setChecked(true);
    
    c->setGroup("KMenu");
    m_searchShortcut->setShortcut(TDEShortcut(c->readEntry("SearchShortcut", "/")), false);
    connect(m_searchShortcut, TQ_SIGNAL(capturedShortcut(const TDEShortcut&)), TQ_SIGNAL(changed()));
    connect(m_searchShortcut, TQ_SIGNAL(capturedShortcut(const TDEShortcut&)), TQ_SLOT(setSearchShortcut(const TDEShortcut&)));
    
    if ( useDefaults )
       emit changed();
}

void MenuTab::menuStyleChanged()
{
    // Classic K Menu	
    if (m_comboMenuStyle->currentItem()==1) {
       m_openOnHover->setEnabled(false);
       m_subMenus->setEnabled(true);
       kcfg_UseSidePixmap->setEnabled(true);
       kcfg_UseTopPixmap->setEnabled(true);
       kcfg_UseTooltip->setEnabled(true);
       kcfg_MenuEntryFormat->setEnabled(true);
       kcfg_RecentVsOften->setEnabled(true);
       m_showFrequent->setEnabled(true);
       kcfg_UseSearchBar->setEnabled(true);
       kcfg_MaxEntries2->setEnabled(true);
       maxrecentdocs->setEnabled(true);
       kcfg_NumVisibleEntries->setEnabled(true);
    }

    // Kickoff Menu
    else {
       m_openOnHover->setEnabled(true);
       m_subMenus->setEnabled(false);
       kcfg_UseSidePixmap->setEnabled(false);
       kcfg_UseTopPixmap->setEnabled(true);
       kcfg_UseTooltip->setEnabled(false);
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
    bool forceRestart = false;

    TDESharedConfig::Ptr c = TDESharedConfig::openConfig(KickerConfig::the()->configName());

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

    c->setGroup("KMenu");
    bool oldmenutextenabledsetting = c->readBoolEntry("ShowText", true);
    TQString oldmenutextsetting = c->readEntry("Text", "");

    c->setGroup("buttons");

    if (kmenusetting != oldkmenusetting) {
        forceRestart = true;
    }
    if (kcfg_ShowKMenuText->isChecked() != oldmenutextenabledsetting) {
        forceRestart = true;
    }
    if (kcfg_KMenuText->text() != oldmenutextsetting) {
        forceRestart = true;
    }
    if (kcfg_ButtonFont->font() != c->readFontEntry("Font")) {
        forceRestart = true;
    }
    if (kcfg_ButtonTextColor->color() != c->readColorEntry("TextColor")) {
        forceRestart = true;
    }

    c->setGroup("KMenu");
    bool sidepixmapsetting = kcfg_UseSidePixmap->isChecked();
    bool oldsidepixmapsetting = c->readBoolEntry("UseSidePixmap", true);

    if (sidepixmapsetting != oldsidepixmapsetting) {
        forceRestart = true;
    }

    bool toppixmapsetting = kcfg_UseTopPixmap->isChecked();
    bool oldtoppixmapsetting = c->readBoolEntry("UseTopPixmap", true);

    if (toppixmapsetting != oldtoppixmapsetting) {
        forceRestart = true;
    }

    bool tooltipsetting = kcfg_UseTooltip->isChecked();
    bool oldtooltipsetting = c->readBoolEntry("UseTooltip", false);

    if (tooltipsetting != oldtooltipsetting) {
        forceRestart = true;
    }

    // Save KMenu settings
    c->setGroup("KMenu");
    c->writeEntry("CustomIcon", m_kmenu_icon);
    c->writeEntry("SearchShortcut", (m_searchShortcut->shortcut()).toString());
    c->sync();

    // Save recent documents
    TDEConfig *config;
    config = new TDEConfig(TQString::fromLatin1("kdeglobals"), false, false);
    config->setGroup(TQString::fromLatin1("RecentDocuments"));
    config->writeEntry("MaxEntries", maxrecentdocs->value());
    config->sync();

    if (m_kmenu_button_changed == true) {
        forceRestart = true;
    }

    if (forceRestart) {
        DCOPRef ("kicker", "default").call("restart()");
    }
}

void MenuTab::defaults()
{
   load( true );
}

void MenuTab::launchMenuEditor()
{
    if ( TDEApplication::startServiceByDesktopName( "kmenuedit",
                                                  TQString::null /*url*/,
                                                  0 /*error*/,
                                                  0 /*dcopservice*/,
                                                  0 /*pid*/,
                                                  "" /*startup_id*/,
                                                  true /*nowait*/ ) != 0 )
    {
        KMessageBox::error(this,
                           i18n("The TDE menu editor (kmenuedit) could not be launched.\n"
                           "Perhaps it is not installed or not in your path."),
                           i18n("Application Missing"));
    }
}

void MenuTab::launchIconEditor()
{
    TDEIconDialog dlg(this);
    TQString newIcon = dlg.selectIcon(TDEIcon::Small, TDEIcon::Application);
    if (newIcon.isEmpty())
        return;

    m_kmenu_icon = newIcon;
    TDEIconLoader * ldr = TDEGlobal::iconLoader();    
    TQIconSet kmenu_icon = ldr->loadIconSet(m_kmenu_icon, TDEIcon::Small, TDEIcon::SizeSmall);
    btnCustomKMenuIcon->setIconSet(kmenu_icon);
    m_kmenu_button_changed = true;

    emit changed();
}

void MenuTab::kmenuChanged()
{
    //m_kmenu_button_changed = true;
    emit changed();
}

void MenuTab::setSearchShortcut(const TDEShortcut &cut)
{
    if( cut.contains( KKeySequence(KKey(TQt::Key_Escape)) ) )
    {
        int anotherTry = KMessageBox::warningYesNo(
  	    this,
	    i18n("Cannot set Escape as menu search shortcut.\nWould you like to set another shortcut?"),
	    i18n("Invalid shortcut")
	);

	if( anotherTry == KMessageBox::Yes )
  	    m_searchShortcut->captureShortcut();
	
        return;
    }

    m_searchShortcut->setShortcut(cut, false);
}
