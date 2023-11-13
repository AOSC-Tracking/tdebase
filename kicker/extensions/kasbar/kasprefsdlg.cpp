/* kasprefsdlg.cpp
**
** Copyright (C) 2001-2004 Richard Moore <rich@kde.org>
** Contributor: Mosfet
**     All rights reserved.
**
** KasBar is dual-licensed: you can choose the GPL or the BSD license.
** Short forms of both licenses are included below.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

/*
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

/*
** Bug reports and questions can be sent to kde-devel@kde.org
*/

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqgrid.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqslider.h>
#include <tqspinbox.h>
#include <tqvbox.h>
#include <tqwhatsthis.h>

#include <kcolorbutton.h>
#include <tdeconfig.h>
#include <kdialogbase.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <knuminput.h>

#include "kastasker.h"

#include "kasprefsdlg.h"
#include "kasprefsdlg.moc"

#define Icon(x) TDEGlobal::iconLoader()->loadIcon( x, TDEIcon::NoGroup, TDEIcon::SizeMedium )
#define LargeIcon(x) TDEGlobal::iconLoader()->loadIcon( x, TDEIcon::NoGroup, TDEIcon::SizeLarge )


KasPrefsDialog::KasPrefsDialog( KasTasker *kas, TQWidget *parent )
   : KDialogBase( KDialogBase::IconList, i18n("Kasbar Preferences"),
		  KDialogBase::Ok | KDialogBase::Cancel,
		  KDialogBase::Ok,
		  parent, "kasbarPrefsDialog", /*true*/false ),
     kasbar( kas ),
     res( kas->resources() )
{
   addLookPage();
   addBackgroundPage();
   addThumbsPage();
   addBehavePage();
//   addIndicatorsPage();
   addColorsPage();
   addAdvancedPage();

   resize( 470, 500 );
}

KasPrefsDialog::~KasPrefsDialog()
{

}

void KasPrefsDialog::itemSizeChanged( int sz )
{
    customSize->setEnabled( sz == KasBar::Custom );
}

void KasPrefsDialog::addLookPage()
{
   TQVBox *lookPage = addVBoxPage( i18n("Appearance"), TQString::null, Icon( "appearance" ) );

   //
   // Item size
   //

   TQGrid *itemSizeBox = new TQGrid( 2, lookPage );
   itemSizeBox->setSpacing( spacingHint() );

   TQWhatsThis::add( itemSizeBox,
		    i18n( "Specifies the size of the task items." ) );

   TQLabel *itemSizeLabel = new TQLabel( i18n("Si&ze:"), itemSizeBox );

   itemSizeCombo = new TQComboBox( itemSizeBox );
   itemSizeCombo->insertItem( i18n( "Enormous" ) );
   itemSizeCombo->insertItem( i18n( "Huge" ) );
   itemSizeCombo->insertItem( i18n( "Large" ) );
   itemSizeCombo->insertItem( i18n( "Medium" ) );
   itemSizeCombo->insertItem( i18n( "Small" ) );
   itemSizeCombo->insertItem( i18n( "Custom" ) );

   itemSizeLabel->setBuddy( itemSizeCombo );

   connect( itemSizeCombo, TQT_SIGNAL( activated( int ) ),
	    kasbar, TQT_SLOT( setItemSize( int ) ) );
   connect( itemSizeCombo, TQT_SIGNAL( activated( int ) ), TQT_SLOT( itemSizeChanged( int ) ) );

   new TQWidget( itemSizeBox );

   customSize = new TQSpinBox( 5, 1000, 1, itemSizeBox );

   customSize->setValue( kasbar->itemExtent() );

   connect( customSize, TQT_SIGNAL( valueChanged( int ) ),
	    kasbar, TQT_SLOT( setItemExtent( int ) ) );
   connect( customSize, TQT_SIGNAL( valueChanged( int ) ),
	    kasbar, TQT_SLOT( customSizeChanged( int ) ) );

   int sz = kasbar->itemSize();
   itemSizeCombo->setCurrentItem( sz );
   customSize->setEnabled( sz == KasBar::Custom );

   //
   // Boxes per line
   //

   TQHBox *maxBoxesBox = new TQHBox( lookPage );
   TQWhatsThis::add( maxBoxesBox,
		    i18n( "Specifies the maximum number of items that should be placed in a line "
			  "before starting a new row or column. If the value is 0 then all the "
			  "available space will be used." ) );
   TQLabel *maxBoxesLabel = new TQLabel( i18n("Bo&xes per line: "), maxBoxesBox );

   TDEConfig *conf = kasbar->config();
   if ( conf )
       conf->setGroup( "Layout" );
   maxBoxesSpin = new KIntSpinBox( 0, 50, 1,
				   conf ? conf->readNumEntry( "MaxBoxes", 0 ) : 11,
				   10,
				   maxBoxesBox, "maxboxes" );
   connect( maxBoxesSpin, TQT_SIGNAL( valueChanged( int ) ), kasbar, TQT_SLOT( setMaxBoxes( int ) ) );
   maxBoxesLabel->setBuddy( maxBoxesSpin );

   //
   // Mode
   //

   detachedCheck = new TQCheckBox( i18n("&Detach from screen edge"), lookPage );
   TQWhatsThis::add( detachedCheck, i18n( "Detaches the bar from the screen edge and makes it draggable." ) );

   detachedCheck->setEnabled( !kasbar->isStandAlone() );
   detachedCheck->setChecked( kasbar->isDetached() );
   connect( detachedCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setDetached(bool) ) );

   (void) new TQWidget( lookPage, "spacer" );
   (void) new TQWidget( lookPage, "spacer" );
   (void) new TQWidget( lookPage, "spacer" );
}

void KasPrefsDialog::addBackgroundPage()
{
   TQVBox *bgPage = addVBoxPage( i18n("Background"), TQString::null, Icon( "background" ) );

   transCheck = new TQCheckBox( i18n("Trans&parent"), bgPage );
   TQWhatsThis::add( transCheck, i18n( "Enables pseudo-transparent mode." ) );
   transCheck->setChecked( kasbar->isTransparent() );
   connect( transCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setTransparent(bool) ) );

   tintCheck = new TQCheckBox( i18n("Enable t&int"), bgPage );
   TQWhatsThis::add( tintCheck,
		    i18n( "Enables tinting the background that shows through in transparent mode." ) );
   tintCheck->setChecked( kasbar->hasTint() );
   connect( tintCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setTint(bool) ) );

   TQHBox *tintColBox = new TQHBox( bgPage );
   TQWhatsThis::add( tintColBox,
		    i18n( "Specifies the color used for the background tint." ) );
   connect( tintCheck, TQT_SIGNAL( toggled(bool) ), tintColBox, TQT_SLOT( setEnabled(bool) ) );
   tintColBox->setEnabled( kasbar->hasTint() );

   TQLabel *tintLabel = new TQLabel( i18n("Tint &color:"), tintColBox );

   tintButton = new KColorButton( kasbar->tintColor(), tintColBox );
   connect( tintButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    kasbar, TQT_SLOT( setTintColor( const TQColor & ) ) );
   tintLabel->setBuddy( tintButton );

   TQHBox *tintAmtBox = new TQHBox( bgPage );
   TQWhatsThis::add( tintAmtBox,
		    i18n( "Specifies the strength of the background tint." ) );
   connect( tintCheck, TQT_SIGNAL( toggled(bool) ), tintAmtBox, TQT_SLOT( setEnabled(bool) ) );
   tintAmtBox->setEnabled( kasbar->hasTint() );

   TQLabel *tintStrengthLabel = new TQLabel( i18n("Tint &strength: "), tintAmtBox );

   int percent = (int) (kasbar->tintAmount() * 100.0);
   tintAmount = new TQSlider( 0, 100, 1, percent, TQt::Horizontal, tintAmtBox );
   tintAmount->setTracking( true );
   connect( tintAmount, TQT_SIGNAL( valueChanged( int ) ),
	    kasbar, TQT_SLOT( setTintAmount( int ) ) );
   tintStrengthLabel->setBuddy( tintAmount );

   (void) new TQWidget( bgPage, "spacer" );
   (void) new TQWidget( bgPage, "spacer" );
   (void) new TQWidget( bgPage, "spacer" );
}

void KasPrefsDialog::addThumbsPage()
{
   TQVBox *thumbsPage = addVBoxPage( i18n("Thumbnails"), TQString::null, Icon( "icons" ) );

   thumbsCheck = new TQCheckBox( i18n("Enable thu&mbnails"), thumbsPage );
   TQWhatsThis::add( thumbsCheck,
		    i18n( "Enables the display of a thumbnailed image of the window when "
			  "you move your mouse pointer over an item. The thumbnails are "
			  "approximate, and may not reflect the current window contents.\n\n"
			  "Using this option on a slow machine may cause performance problems." ) );
   thumbsCheck->setChecked( kasbar->thumbnailsEnabled() );
   connect( thumbsCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setThumbnailsEnabled(bool) ) );

   embedThumbsCheck = new TQCheckBox( i18n("&Embed thumbnails"), thumbsPage );
   embedThumbsCheck->setChecked( kasbar->embedThumbnails() );
   connect( embedThumbsCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setEmbedThumbnails(bool) ) );

   TQHBox *thumbSizeBox = new TQHBox( thumbsPage );
   TQWhatsThis::add( thumbSizeBox,
		    i18n( "Controls the size of the window thumbnails. Using large sizes may "
			  "cause performance problems." ) );
   TQLabel *thumbSizeLabel = new TQLabel( i18n("Thumbnail &size: "), thumbSizeBox );
   int percent = (int) (kasbar->thumbnailSize() * 100.0);
   thumbSizeSlider = new TQSlider( 0, 100, 1, percent, TQt::Horizontal, thumbSizeBox );
   connect( thumbSizeSlider, TQT_SIGNAL( valueChanged( int ) ),
	    kasbar, TQT_SLOT( setThumbnailSize( int ) ) );
   thumbSizeLabel->setBuddy( thumbSizeSlider );

   TQHBox *thumbUpdateBox = new TQHBox( thumbsPage );
   thumbUpdateBox->setSpacing( spacingHint() );
   TQWhatsThis::add( thumbUpdateBox,
		    i18n( "Controls the frequency with which the thumbnail of the active window "
			  "is updated. If the value is 0 then no updates will be performed.\n\n"
			  "Using small values may cause performance problems on slow machines." ) );
   TQLabel *thumbUpdateLabel = new TQLabel( i18n("&Update thumbnail every: "), thumbUpdateBox );
   thumbUpdateSpin = new TQSpinBox( 0, 1000, 1, thumbUpdateBox );
   thumbUpdateSpin->setValue( kasbar->thumbnailUpdateDelay() );
   connect( thumbUpdateSpin, TQT_SIGNAL( valueChanged( int ) ),
   	    kasbar, TQT_SLOT( setThumbnailUpdateDelay( int ) ) );
   (void) new TQLabel( i18n("seconds"), thumbUpdateBox );
   thumbUpdateLabel->setBuddy( thumbUpdateSpin );

   (void) new TQWidget( thumbsPage, "spacer" );
   (void) new TQWidget( thumbsPage, "spacer" );
   (void) new TQWidget( thumbsPage, "spacer" );
}

void KasPrefsDialog::addBehavePage()
{
   TQVBox *behavePage = addVBoxPage( i18n("Behavior"), TQString::null, Icon( "window_duplicate" ) );

   groupWindowsCheck = new TQCheckBox( i18n("&Group windows"), behavePage );
   TQWhatsThis::add( groupWindowsCheck,
		    i18n( "Enables the grouping together of related windows." ) );
   groupWindowsCheck->setChecked( kasbar->groupWindows() );
   connect( groupWindowsCheck, TQT_SIGNAL( toggled(bool) ),
	    kasbar, TQT_SLOT( setGroupWindows(bool) ) );

   showAllWindowsCheck = new TQCheckBox( i18n("Show all &windows"), behavePage );
   TQWhatsThis::add( showAllWindowsCheck,
		    i18n( "Enables the display of all windows, not just those on the current desktop." ) );
   showAllWindowsCheck->setChecked( kasbar->showAllWindows() );
   connect( showAllWindowsCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setShowAllWindows(bool) ) );

   groupInactiveCheck = new TQCheckBox( i18n("&Group windows on inactive desktops"), behavePage );
   TQWhatsThis::add( groupInactiveCheck,
		    i18n( "Enables the grouping together of windows that are not on the current desktop." ) );
   groupInactiveCheck->setChecked( kasbar->groupInactiveDesktops() );
   connect( groupInactiveCheck, TQT_SIGNAL( toggled(bool) ),
	    kasbar, TQT_SLOT( setGroupInactiveDesktops(bool) ) );

   onlyShowMinimizedCheck = new TQCheckBox( i18n("Only show &minimized windows"), behavePage );
   TQWhatsThis::add( onlyShowMinimizedCheck,
		    i18n( "When this option is checked only minimized windows are shown in the bar. " \
			  "This gives Kasbar similar behavior to the icon handling in older environments " \
			  "like CDE or OpenLook." ) );
   onlyShowMinimizedCheck->setChecked( kasbar->onlyShowMinimized() );
   connect( onlyShowMinimizedCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setOnlyShowMinimized(bool) ) );

   (void) new TQWidget( behavePage, "spacer" );
   (void) new TQWidget( behavePage, "spacer" );
}

void KasPrefsDialog::addColorsPage()
{
   TQVBox *colorsPage = addVBoxPage( i18n("Colors"), TQString::null, Icon( "colors" ) );

   // Item label colors
   TQGrid *group = new TQGrid( 2, colorsPage );

   TQLabel *labelPenLabel = new TQLabel( i18n("Label foreground:"), group );

   labelPenButton = new KColorButton( res->labelPenColor(), group );
   connect( labelPenButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setLabelPenColor( const TQColor & ) ) );
   labelPenLabel->setBuddy( labelPenButton );

   TQLabel *labelBackgroundLabel = new TQLabel( i18n("Label background:"), group );
   labelBackgroundButton = new KColorButton( res->labelBgColor(), group );
   connect( labelBackgroundButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setLabelBgColor( const TQColor & ) ) );
   labelBackgroundLabel->setBuddy( labelBackgroundButton );

   // Inactive colors
   group = new TQGrid( 2, colorsPage );

   TQLabel *inactivePenLabel = new TQLabel( i18n("Inactive foreground:"), group );
   inactivePenButton = new KColorButton( res->inactivePenColor(), group );
   connect( inactivePenButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setInactivePenColor( const TQColor & ) ) );
   inactivePenLabel->setBuddy( inactivePenButton );

   TQLabel *inactiveBgLabel = new TQLabel( i18n("Inactive background:"), group );
   inactiveBgButton = new KColorButton( res->inactiveBgColor(), group );
   connect( inactiveBgButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setInactiveBgColor( const TQColor & ) ) );
   inactiveBgLabel->setBuddy( inactiveBgButton );

   // Active colors
   group = new TQGrid( 2, colorsPage );

   TQLabel *activePenLabel = new TQLabel( i18n("Active foreground:"), group );
   activePenButton = new KColorButton( res->activePenColor(), group );
   connect( activePenButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setActivePenColor( const TQColor & ) ) );
   activePenLabel->setBuddy( activePenButton );

   TQLabel *activeBgLabel = new TQLabel( i18n("Active background:"), group );
   activeBgButton = new KColorButton( res->activeBgColor(), group );
   connect( activeBgButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setActiveBgColor( const TQColor & ) ) );
   activeBgLabel->setBuddy( activeBgButton );

   group = new TQGrid( 2, colorsPage );

   TQLabel *progressLabel = new TQLabel( i18n("&Progress color:"), group );
   progressButton = new KColorButton( res->progressColor(), group );
   connect( progressButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setProgressColor( const TQColor & ) ) );
   progressLabel->setBuddy( progressButton );

   TQLabel *attentionLabel = new TQLabel( i18n("&Attention color:"), group );
   attentionButton = new KColorButton( res->attentionColor(), group );
   connect( attentionButton, TQT_SIGNAL( changed( const TQColor & ) ),
	    res, TQT_SLOT( setAttentionColor( const TQColor & ) ) );
   attentionLabel->setBuddy( attentionButton );

   (void) new TQWidget( colorsPage, "spacer" );
}

void KasPrefsDialog::addIndicatorsPage()
{
   TQVBox *indicatorsPage = addVBoxPage( i18n("Indicators"), TQString::null, Icon( "bell" ) );

   (void) new TQWidget( indicatorsPage, "spacer" );
   (void) new TQWidget( indicatorsPage, "spacer" );
}

void KasPrefsDialog::addAdvancedPage()
{
   TQVBox *advancedPage = addVBoxPage( i18n("Advanced"), TQString::null, Icon( "misc" ) );

   // Startup notifier
   notifierCheck = new TQCheckBox( i18n("Enable &startup notifier"), advancedPage );
   TQWhatsThis::add( notifierCheck,
		    i18n( "Enables the display of tasks that are starting but have not yet "
			  "created a window." ) );
   notifierCheck->setChecked( kasbar->notifierEnabled() );
   connect( notifierCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setNotifierEnabled(bool) ) );

   // Status advanced
   modifiedCheck = new TQCheckBox( i18n("Enable &modified indicator"), advancedPage );
   TQWhatsThis::add( modifiedCheck,
		    i18n( "Enables the display of a floppy disk state icon for windows containing "
			  "a modified document." ) );
   modifiedCheck->setChecked( kasbar->showModified() );
   connect( modifiedCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setShowModified(bool) ) );

   progressCheck = new TQCheckBox( i18n("Enable &progress indicator"), advancedPage );
   TQWhatsThis::add( progressCheck,
		    i18n( "Enables the display of a progress indicator in the label of windows." ) );
   progressCheck->setChecked( kasbar->showProgress() );
   connect( progressCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setShowProgress(bool) ) );

   attentionCheck = new TQCheckBox( i18n("Enable &attention indicator"), advancedPage );
   TQWhatsThis::add( attentionCheck,
		    i18n( "Enables the display of an icon that indicates a window that needs attention." ) );
   attentionCheck->setChecked( kasbar->showAttention() );
   connect( attentionCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setShowAttention(bool) ) );

   inactiveFramesCheck = new TQCheckBox( i18n("Enable frames for inactive items"), advancedPage );
   TQWhatsThis::add( inactiveFramesCheck,
		    i18n( "Enables frames around inactive items, if you want the bar to disappear into " \
			  "the background you should probably uncheck this option." ) );
   inactiveFramesCheck->setChecked( kasbar->paintInactiveFrames() );
   connect( inactiveFramesCheck, TQT_SIGNAL( toggled(bool) ), kasbar, TQT_SLOT( setPaintInactiveFrames(bool) ) );

   (void) new TQWidget( advancedPage, "spacer" );
   (void) new TQWidget( advancedPage, "spacer" );
}

void KasPrefsDialog::customSizeChanged ( int value )
{
   customSize->setSuffix( i18n(" pixel", " pixels", value) );
}

void KasPrefsDialog::accept()
{
   TDEConfig *conf = kasbar->config();
   if ( conf ) {
       kasbar->writeConfig( conf );

       conf->setGroup("Layout");
       // TODO: This needs to be made independent of the gui and moved to kastasker
       conf->writeEntry( "MaxBoxes", maxBoxesSpin->value() );

       conf->sync();
   }

   TQDialog::accept();
}

void KasPrefsDialog::reject()
{
   kasbar->readConfig();
   TQDialog::reject();
}
