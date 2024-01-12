/**
 * (c) Martin R. Jones 1996
 * (c) Bernd Wuebben 1998
 * (c) Christian Tibirna 1998
 * (c) David Faure 1998, 2000
 * (c) John Firebaugh 2003
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

#include "desktopbehavior_impl.h"

#include <tqlayout.h>
#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqcombobox.h>
#include <tqpushbutton.h>
#include <tqbuttongroup.h>
#include <tqspinbox.h>
#include <tqtabwidget.h>
#include <tqwhatsthis.h>
#include <tdelistview.h>
#include <kservice.h>
#include <tdelocale.h>
#include <tdeglobalsettings.h>
#include <kmimetype.h>
#include <ktrader.h>
#include <tdeapplication.h>
#include <kcustommenueditor.h>
#include <dcopclient.h>
#include <konq_defaults.h> // include default values directly from libkonq
#include <kipc.h>
#include <kprotocolinfo.h>

const int customMenu1ID = 5;
const int customMenu2ID = 6;

DesktopBehaviorModule::DesktopBehaviorModule(TDEConfig *config, TQWidget *parent, const char * )
    : TDECModule( parent, "kcmkonq" )
{
    TQVBoxLayout* layout = new TQVBoxLayout(this);
    m_behavior = new DesktopBehavior(config, this);
    layout->addWidget(m_behavior);
    connect(m_behavior, TQ_SIGNAL(changed()), this, TQ_SLOT(changed()));
}

void DesktopBehaviorModule::changed()
{
    emit TDECModule::changed( true );
}

class DesktopBehaviorPreviewItem : public TQCheckListItem
{
public:
    DesktopBehaviorPreviewItem(DesktopBehavior *rootOpts, TQListView *parent,
                const KService::Ptr &plugin, bool on)
        : TQCheckListItem(parent, plugin->name(), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = plugin->desktopEntryName();
        setOn(on);
    }
    DesktopBehaviorPreviewItem(DesktopBehavior *rootOpts, TQListView *parent,
                bool on)
        : TQCheckListItem(parent, i18n("Sound Files"), CheckBox),
          m_rootOpts(rootOpts)
    {
        m_pluginName = "audio/";
        setOn(on);
    }
    const TQString &pluginName() const { return m_pluginName; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    DesktopBehavior *m_rootOpts;
    TQString m_pluginName;
};


class DesktopBehaviorMediaItem : public TQCheckListItem
{
public:
    DesktopBehaviorMediaItem(DesktopBehavior *rootOpts, TQListView *parent,
                const TQString name, const TQString mimetype, bool on)
        : TQCheckListItem(parent, name, CheckBox),
          m_rootOpts(rootOpts),m_mimeType(mimetype){setOn(on);}

    const TQString &mimeType() const { return m_mimeType; }

protected:
    virtual void stateChange( bool ) { m_rootOpts->changed(); }

private:
    DesktopBehavior *m_rootOpts;
    TQString m_mimeType;
};


static const int choiceCount=7;
static const char * s_choices[7] = { "", "WindowListMenu", "DesktopMenu", "AppMenu", "BookmarksMenu", "CustomMenu1", "CustomMenu2" };

DesktopBehavior::DesktopBehavior(TDEConfig *config, TQWidget *parent, const char * )
    : DesktopBehaviorBase( parent, "kcmkonq" ), g_pConfig(config)
{
  TQString strMouseButton1, strMouseButton3, strButtonTxt1, strButtonTxt3;

  /*
   * The text on this form depends on the mouse setting, which can be right
   * or left handed.  The outer button functionality is actually swapped
   *
   */
  bool leftHandedMouse = ( TDEGlobalSettings::mouseSettings().handed == TDEGlobalSettings::KMouseSettings::LeftHanded);

  m_bHasMedia = KProtocolInfo::isKnownProtocol(TQString::fromLatin1("media"));

  connect(desktopMenuGroup, TQ_SIGNAL(clicked(int)), this, TQ_SIGNAL(changed()));
  connect(iconsEnabledBox, TQ_SIGNAL(clicked()), this, TQ_SLOT(enableChanged()));
  connect(showHiddenBox, TQ_SIGNAL(clicked()), this, TQ_SIGNAL(changed()));
  connect(vrootBox, TQ_SIGNAL(clicked()), this, TQ_SIGNAL(changed()));
  connect(lockInPlaceBox, TQ_SIGNAL(clicked()), this, TQ_SLOT(enableGridChanged()));
  connect(autoLineupIconsBox, TQ_SIGNAL(clicked()), this, TQ_SLOT(enableGridChanged()));
  connect(toolTipBox, TQ_SIGNAL(clicked()), this, TQ_SIGNAL(changed()));
  connect(mediaListView, TQ_SIGNAL(clicked(TQListViewItem *)), this, TQ_SLOT(mediaListViewChanged(TQListViewItem *)));
  connect(spacingValue, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(spacingChanged(int)));
  connect(spacingCtrlScroll, TQ_SIGNAL(clicked()), this, TQ_SIGNAL(changed()));

  strMouseButton1 = i18n("&Left button:");
  strButtonTxt1 = i18n( "You can choose what happens when"
   " you click the left button of your pointing device on the desktop:");

  strMouseButton3 = i18n("Right b&utton:");
  strButtonTxt3 = i18n( "You can choose what happens when"
   " you click the right button of your pointing device on the desktop:");

  if ( leftHandedMouse )
  {
     tqSwap(strMouseButton1, strMouseButton3);
     tqSwap(strButtonTxt1, strButtonTxt3);
  }

  leftLabel->setText( strMouseButton1 );
  leftLabel->setBuddy( leftComboBox );
  fillMenuCombo( leftComboBox );
  connect(leftEditButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(editButtonPressed()));
  connect(leftComboBox, TQ_SIGNAL(activated(int)), this, TQ_SIGNAL(changed()));
  connect(leftComboBox, TQ_SIGNAL(activated(int)), this, TQ_SLOT(comboBoxChanged()));
  TQString wtstr = strButtonTxt1 +
                  i18n(" <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
                       " <li><em>Window list menu:</em> a menu showing all windows on all"
                       " virtual desktops pops up. You can click on the desktop name to switch"
                       " to that desktop, or on a window name to shift focus to that window,"
                       " switching desktops if necessary, and restoring the window if it is"
                       " hidden. Hidden or minimized windows are represented with their names"
                       " in parentheses.</li>"
                       " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
                       " Among other things, this menu has options for configuring the display,"
                       " locking the screen, and logging out of TDE.</li>"
                       " <li><em>Application menu:</em> the \"TDE\" menu pops up. This might be"
                       " useful for quickly accessing applications if you like to keep the"
                       " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  TQWhatsThis::add( leftLabel, wtstr );
  TQWhatsThis::add( leftComboBox, wtstr );

  middleLabel->setBuddy( middleComboBox );
  fillMenuCombo( middleComboBox );
  connect(middleEditButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(editButtonPressed()));
  connect(middleComboBox, TQ_SIGNAL(activated(int)), this, TQ_SIGNAL(changed()));
  connect(middleComboBox, TQ_SIGNAL(activated(int)), this, TQ_SLOT(comboBoxChanged()));
  wtstr = i18n("You can choose what happens when"
               " you click the middle button of your pointing device on the desktop:"
               " <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
               " <li><em>Window list menu:</em> a menu showing all windows on all"
               " virtual desktops pops up. You can click on the desktop name to switch"
               " to that desktop, or on a window name to shift focus to that window,"
               " switching desktops if necessary, and restoring the window if it is"
               " hidden. Hidden or minimized windows are represented with their names"
               " in parentheses.</li>"
               " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
               " Among other things, this menu has options for configuring the display,"
               " locking the screen, and logging out of TDE.</li>"
               " <li><em>Application menu:</em> the \"TDE\" menu pops up. This might be"
               " useful for quickly accessing applications if you like to keep the"
               " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  TQWhatsThis::add( middleLabel, wtstr );
  TQWhatsThis::add( middleComboBox, wtstr );

  rightLabel->setText( strMouseButton3 );
  rightLabel->setBuddy( rightComboBox );
  fillMenuCombo( rightComboBox );
  connect(rightEditButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(editButtonPressed()));
  connect(rightComboBox, TQ_SIGNAL(activated(int)), this, TQ_SIGNAL(changed()));
  connect(rightComboBox, TQ_SIGNAL(activated(int)), this, TQ_SLOT(comboBoxChanged()));
  wtstr = strButtonTxt3 +
          i18n(" <ul><li><em>No action:</em> as you might guess, nothing happens!</li>"
               " <li><em>Window list menu:</em> a menu showing all windows on all"
               " virtual desktops pops up. You can click on the desktop name to switch"
               " to that desktop, or on a window name to shift focus to that window,"
               " switching desktops if necessary, and restoring the window if it is"
               " hidden. Hidden or minimized windows are represented with their names"
               " in parentheses.</li>"
               " <li><em>Desktop menu:</em> a context menu for the desktop pops up."
               " Among other things, this menu has options for configuring the display,"
               " locking the screen, and logging out of TDE.</li>"
               " <li><em>Application menu:</em> the \"TDE\" menu pops up. This might be"
               " useful for quickly accessing applications if you like to keep the"
               " panel (also known as \"Kicker\") hidden from view.</li></ul>");
  TQWhatsThis::add( rightLabel, wtstr );
  TQWhatsThis::add( rightComboBox, wtstr );

  if (m_bHasMedia)
  {
     connect(enableMediaBox, TQ_SIGNAL(clicked()), this, TQ_SLOT(enableChanged()));
     connect(enableMediaFreeSpaceOverlayBox, TQ_SIGNAL(clicked()), this, TQ_SLOT(enableChanged()));
  }
  else
  {
     delete behaviorTab->page(2);
  }

  load();
}

void DesktopBehavior::mediaListViewChanged(TQListViewItem * item)
{
    // FIXME: This should check to make sure an item was actually checked/unchecked before emitting changed()
    emit changed();
}

void DesktopBehavior::spacingChanged(int value)
{
    emit changed();
}

void DesktopBehavior::setMediaListViewEnabled(bool enabled)
{
    for (DesktopBehaviorMediaItem *it=static_cast<DesktopBehaviorMediaItem *>(mediaListView->firstChild());
        it; it=static_cast<DesktopBehaviorMediaItem *>(it->nextSibling()))
    {
        if (it->mimeType().startsWith("media/builtin-") == false)
            it->setVisible(enabled);
        else
            it->setVisible(TRUE);
    }
}

void DesktopBehavior::fillMediaListView()
{
    mediaListView->clear();
    mediaListView->setRootIsDecorated(false);
    KMimeType::List mimetypes = KMimeType::allMimeTypes();
    TQValueListIterator<KMimeType::Ptr> it2(mimetypes.begin());
    g_pConfig->setGroup( "Media" );
    enableMediaBox->setChecked(g_pConfig->readBoolEntry("enabled",true));
    enableMediaFreeSpaceOverlayBox->setChecked(g_pConfig->readBoolEntry("FreeSpaceDisplayEnabled",true));
    TQString excludedMedia=g_pConfig->readEntry("exclude","media/nfs_mounted,media/nfs_unmounted,media/hdd_mounted,media/hdd_unmounted,media/floppy_unmounted,media/cdrom_unmounted,media/floppy5_unmounted");
    for (; it2 != mimetypes.end(); ++it2) {
        if ( ((*it2)->name().startsWith("media/")) )
        {
            bool ok=excludedMedia.contains((*it2)->name())==0;
            new DesktopBehaviorMediaItem (this, mediaListView, (*it2)->comment(), (*it2)->name(),ok);
        }
    }
}

void DesktopBehavior::saveMediaListView()
{
    if (!m_bHasMedia)
        return;

    g_pConfig->setGroup( "Media" );
    g_pConfig->writeEntry("enabled",enableMediaBox->isChecked());
    g_pConfig->writeEntry("FreeSpaceDisplayEnabled",enableMediaFreeSpaceOverlayBox->isChecked());
    TQStringList exclude;
    for (DesktopBehaviorMediaItem *it=static_cast<DesktopBehaviorMediaItem *>(mediaListView->firstChild());
        it; it=static_cast<DesktopBehaviorMediaItem *>(it->nextSibling()))
    {
        if (!it->isOn()) exclude << it->mimeType();
    }
    g_pConfig->writeEntry("exclude",exclude);
}


void DesktopBehavior::fillMenuCombo( TQComboBox * combo )
{
  combo->insertItem( i18n("No Action") );
  combo->insertItem( i18n("Window List Menu") );
  combo->insertItem( i18n("Desktop Menu") );
  combo->insertItem( i18n("Application Menu") );
  combo->insertItem( i18n("Bookmarks Menu") );
  combo->insertItem( i18n("Custom Menu 1") );
  combo->insertItem( i18n("Custom Menu 2") );
}

void DesktopBehavior::load()
{
	load( false );
}

void DesktopBehavior::load( bool useDefaults )
{
    g_pConfig->setReadDefaults( useDefaults );
    g_pConfig->setGroup( "Desktop Icons" );
    bool bShowHidden = g_pConfig->readBoolEntry("ShowHidden", DEFAULT_SHOW_HIDDEN_ROOT_ICONS);
    showHiddenBox->setChecked(bShowHidden);
    spacingValue->setValue( g_pConfig->readNumEntry("IconSpacing", 5) );
    spacingCtrlScroll->setChecked( g_pConfig->readBoolEntry("SpacingCtrlScroll", false) );
    lockInPlaceBox->setChecked( g_pConfig->readBoolEntry( "LockIcons", false ) );
    //bool bVertAlign = g_pConfig->readBoolEntry("VertAlign", DEFAULT_VERT_ALIGN);
    TDETrader::OfferList plugins = TDETrader::self()->query("ThumbCreator");
    previewListView->clear();
    TQStringList previews = g_pConfig->readListEntry("Preview");
    for (TDETrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
        new DesktopBehaviorPreviewItem(this, previewListView, *it, previews.contains((*it)->desktopEntryName()));
    new DesktopBehaviorPreviewItem(this, previewListView, previews.contains("audio/"));
    //
    g_pConfig->setGroup( "FMSettings" );
    toolTipBox->setChecked(g_pConfig->readBoolEntry( "ShowFileTips", true ) );
    g_pConfig->setGroup( "Menubar" );
    TDEConfig config( "kdeglobals" );
    config.setGroup("KDE");
    bool globalMenuBar = config.readBoolEntry("macStyle", false);
    bool desktopMenuBar = g_pConfig->readBoolEntry("ShowMenubar", false);
    if ( globalMenuBar )
        desktopMenuGroup->setButton( 2 );
    else if ( desktopMenuBar )
        desktopMenuGroup->setButton( 1 );
    else
        desktopMenuGroup->setButton( 0 );
    g_pConfig->setGroup( "General" );
    vrootBox->setChecked( g_pConfig->readBoolEntry( "SetVRoot", false ) );
    iconsEnabledBox->setChecked( g_pConfig->readBoolEntry( "Enabled", true ) );
    autoLineupIconsBox->setChecked( g_pConfig->readBoolEntry( "AutoLineUpIcons", false ) );

    toggleSpacingOpts();

    //
    g_pConfig->setGroup( "Mouse Buttons" );
    TQString s;
    s = g_pConfig->readEntry( "Left", "" );
    for ( int c = 0 ; c < choiceCount ; c ++ )
    if (s == s_choices[c])
      { leftComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Middle", "WindowListMenu" );
    for ( int c = 0 ; c < choiceCount ; c ++ )
      if (s == s_choices[c])
      { middleComboBox->setCurrentItem( c ); break; }
    s = g_pConfig->readEntry( "Right", "DesktopMenu" );
    for ( int c = 0 ; c < choiceCount ; c ++ )
      if (s == s_choices[c])
      { rightComboBox->setCurrentItem( c ); break; }

    comboBoxChanged();
    if (m_bHasMedia)
        fillMediaListView();
    enableChanged();
}

void DesktopBehavior::defaults()
{
	load( true );
}


void DesktopBehavior::save()
{
    g_pConfig->setGroup( "Desktop Icons" );
    g_pConfig->writeEntry("ShowHidden", showHiddenBox->isChecked());
    g_pConfig->writeEntry("IconSpacing", spacingValue->value());
    g_pConfig->writeEntry("SpacingCtrlScroll", spacingCtrlScroll->isChecked());
    g_pConfig->writeEntry("LockIcons", lockInPlaceBox->isChecked());
    TQStringList previews;
    for ( DesktopBehaviorPreviewItem *item = static_cast<DesktopBehaviorPreviewItem *>( previewListView->firstChild() );
          item;
          item = static_cast<DesktopBehaviorPreviewItem *>( item->nextSibling() ) )
        if ( item->isOn() )
            previews.append( item->pluginName() );
    g_pConfig->writeEntry( "Preview", previews );
    g_pConfig->setGroup( "FMSettings" );
    g_pConfig->writeEntry( "ShowFileTips", toolTipBox->isChecked() );
    g_pConfig->setGroup( "Menubar" );
    g_pConfig->writeEntry("ShowMenubar", desktopMenuGroup->selectedId() == 1);
    TDEConfig config( "kdeglobals" );
    config.setGroup("KDE");
    bool globalMenuBar = desktopMenuGroup->selectedId() == 2;
    if ( globalMenuBar != config.readBoolEntry("macStyle", false) )
    {
        config.writeEntry( "macStyle", globalMenuBar, true, true );
        config.sync();
        KIPC::sendMessageAll(KIPC::ToolbarStyleChanged);
    }
    g_pConfig->setGroup( "Mouse Buttons" );
    g_pConfig->writeEntry("Left", s_choices[ leftComboBox->currentItem() ] );
    g_pConfig->writeEntry("Middle", s_choices[ middleComboBox->currentItem() ]);
    g_pConfig->writeEntry("Right", s_choices[ rightComboBox->currentItem() ]);

    g_pConfig->setGroup( "General" );
    g_pConfig->writeEntry( "SetVRoot", vrootBox->isChecked() );
    g_pConfig->writeEntry( "Enabled", iconsEnabledBox->isChecked() );
    g_pConfig->writeEntry( "AutoLineUpIcons", autoLineupIconsBox->isChecked() );

    toggleSpacingOpts();

    saveMediaListView();
    g_pConfig->sync();

    // Tell kdesktop about the new config file
    if ( !kapp->dcopClient()->isAttached() )
       kapp->dcopClient()->attach();
    TQByteArray data;

    int konq_screen_number = TDEApplication::desktop()->primaryScreen();
    TQCString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
    // for the standalone menubar setting
    kapp->dcopClient()->send( "menuapplet*", "menuapplet", "configure()", data );
    kapp->dcopClient()->send( "kicker", "kicker", "configureMenubar()", data );
    kapp->dcopClient()->send( "twin*", "", "reconfigure()", data );
}

void DesktopBehavior::enableChanged()
{
    bool enabled = iconsEnabledBox->isChecked();
    behaviorTab->setTabEnabled(behaviorTab->page(1), enabled);
    vrootBox->setEnabled(enabled);
    lockInPlaceBox->setEnabled(enabled);
    autoLineupIconsBox->setEnabled(enabled);
    enableGridChanged();

    if (m_bHasMedia)
    {
        behaviorTab->setTabEnabled(behaviorTab->page(2), enabled);
        enableMediaBox->setEnabled(enabled);
        enableMediaFreeSpaceOverlayBox->setEnabled(enabled);
        setMediaListViewEnabled(enableMediaBox->isChecked());
    }

    changed();
}

void DesktopBehavior::toggleSpacingOpts()
{
  bool enabled = (
    autoLineupIconsBox->isEnabled() &&  // Desktop grid available
    autoLineupIconsBox->isChecked() &&  // Desktop grid enabled
    !lockInPlaceBox->isChecked()        // Lock in Place is off
  );

  spacingCtrlScroll->setEnabled(enabled);
  spacingValue->setEnabled(enabled);
}

void DesktopBehavior::enableGridChanged()
{
  toggleSpacingOpts();
  changed();
}

void DesktopBehavior::comboBoxChanged()
{
  int i;
  i = leftComboBox->currentItem();
  leftEditButton->setEnabled((i == customMenu1ID) || (i == customMenu2ID));
  i = middleComboBox->currentItem();
  middleEditButton->setEnabled((i == customMenu1ID) || (i == customMenu2ID));
  i = rightComboBox->currentItem();
  rightEditButton->setEnabled((i == customMenu1ID) || (i == customMenu2ID));
}

void DesktopBehavior::editButtonPressed()
{
   int i = 0;
   if (sender() == leftEditButton)
      i = leftComboBox->currentItem();
   if (sender() == middleEditButton)
      i = middleComboBox->currentItem();
   if (sender() == rightEditButton)
      i = rightComboBox->currentItem();

   TQString cfgFile;
   if (i == customMenu1ID)
      cfgFile = "kdesktop_custom_menu1";
   if (i == customMenu2ID)
      cfgFile = "kdesktop_custom_menu2";

   if (cfgFile.isEmpty())
      return;

   KCustomMenuEditor editor(this);
   TDEConfig cfg(cfgFile, false, false);

   editor.load(&cfg);
   if (editor.exec())
   {
      editor.save(&cfg);
      cfg.sync();
      emit changed();
   }
}

TQString DesktopBehavior::quickHelp() const
{
  return i18n("<h1>Behavior</h1>\n"
    "This module allows you to choose various options\n"
    "for your desktop, including the way in which icons are arranged and\n"
    "the pop-up menus associated with clicks of the middle and right mouse\n"
    "buttons on the desktop.\n"
    "Use the \"What's This?\" (Shift+F1) to get help on specific options.");
}

TQString DesktopBehavior::handbookSection() const
{
  int index = behaviorTab->currentPageIndex();
  if (index == 0) {
    //return "desktop-desktop";
    return TQString::null;
  }
  else if (index == 1) {
    return "desktop-behavior-file-icons";
  }
  else if (index == 2) {
    return "desktop-behavior-device-icons";
  }
  else {
    return TQString::null;
  }
}

#include "desktopbehavior_impl.moc"
