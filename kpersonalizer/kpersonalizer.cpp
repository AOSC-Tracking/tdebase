/***************************************************************************
                          kpersonalizer.cpp  -  description
                             -------------------
    begin                : Die Mai 22 17:24:18 CEST 2001
    copyright            : (C) 2001 by Ralf Nolden
    email                : nolden@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <unistd.h>

#include <tqpushbutton.h>
#include <tqlabel.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqfile.h>
#include <tqtimer.h>
#include <tqcursor.h>

#include <tdesimpleconfig.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <tdestandarddirs.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <tdelistview.h>
#include <krun.h>
#include <tdemessagebox.h>
#include <tdeconfig.h>

#include <stdlib.h>

#include <kdebug.h>

#include "kcountrypage.h"
#include "kospage.h"
#include "keyecandypage.h"
#include "tdestylepage.h"
#include "krefinepage.h"

#include "kpersonalizer.h"
#include "kpersonalizer.moc"



bool KPersonalizer::before_session = false;

KPersonalizer::KPersonalizer(TQWidget *parent, const char *name)
	: KWizard(parent, name, true) {

	// first, reset the startup from true (see desktop file in share/autostart) to false
	setCaption(tdeApp->caption());
	tdeApp->config()->setGroup("General");
	os_dirty = eye_dirty = style_dirty=false;
	tdeApp->config()->writeEntry("FirstLogin", false);
	tdeApp->config()->sync();

	countrypage= new KCountryPage(this);
	addPage( countrypage, i18n( "Step 1: Introduction" ) );
	connect(countrypage, TQ_SIGNAL(countryLanguageChanged()),
	        TQ_SLOT(slotCountryPageLanguageChanged()));
	setHelpEnabled(TQWizard::page(0), false);

	ospage= new KOSPage(this);
	addPage(ospage, i18n( "Step 2: I want it my Way..." ) );
	setHelpEnabled(TQWizard::page(1), false);

	eyecandy= new KEyeCandyPage(this);
	addPage( eyecandy, i18n( "Step 3: Eyecandy-O-Meter" ) );
	setHelpEnabled(TQWizard::page(2), false);

	stylepage= new TDEStylePage(this);
	addPage( stylepage, i18n( "Step 4: Everybody loves Themes" ) );
	setHelpEnabled(TQWizard::page(3), false);

	refinepage=new KRefinePage(this);
	addPage( refinepage, i18n( "Step 5: Time to Refine" ) );
	setHelpEnabled(TQWizard::page(4), false);

	cancelButton()->setText(i18n("S&kip Wizard"));

	setFinishEnabled(TQWizard::page(4), true);

	locale = new TDELocale("kpersonalizer");
	locale->setLanguage(TDELocale::defaultLanguage());

	connect(ospage, TQ_SIGNAL(selectedOS(const TQString&)), stylepage, TQ_SLOT(presetStyle(const TQString&)));
	connect(ospage, TQ_SIGNAL(selectedOS(const TQString&)), eyecandy, TQ_SLOT(slotPresetSlider(const TQString&)));
	connect(refinepage->pb_kcontrol, TQ_SIGNAL(clicked()), this, TQ_SLOT(accept()));

	setPosition();

	/* hide the detail-box on eyecandypage. we need to call it from here, to be
	   able, to call it at last. Else we would run into layout-problems later. */
	eyecandy->klv_features->hide();
}

KPersonalizer::~KPersonalizer() {
}


void KPersonalizer::next() {
	if(currentPage()==countrypage) {
		// only restart kp, if the new language is different from the one selected in main.cpp
		// and none of the later pages is dirty
		if ( (countrypage->b_startedLanguageChanged) && !(os_dirty || eye_dirty || style_dirty) ) {
			if ( countrypage->save(countrypage->cb_country, countrypage->cb_language) )
				delayedRestart();
		} else {
			(void)countrypage->save(countrypage->cb_country, countrypage->cb_language);
			TQWizard::next();
		}
	}
	else if(currentPage()==ospage){
		os_dirty=true;  // set the dirty flag, changes done that need reverting
		ospage->save();
		TQWizard::next();
	}
	else if(currentPage()==eyecandy){
		eye_dirty=true;  // set the dirty flag, changes done that need reverting
		eyecandy->save();
		TQTimer::singleShot(0, this, TQ_SLOT(slotNext()));
	}
	else if(currentPage()==stylepage){
		style_dirty=true;  // set the dirty flag, changes done that need reverting
		stylepage->save();
		TQWizard::next();
	}
	if(currentPage()==refinepage) {
		finishButton()->setFocus();
	}
}

void KPersonalizer::slotNext() {
    TQWizard::next();
    stylepage->switchPrevStyle();  // We need to update the preview-widget, after the page changed
}

void KPersonalizer::back() {
	TQWizard::back();
}

bool KPersonalizer::askClose(){
	TQString text;
	if (currentPage()==countrypage) {
		text = i18n("<p>Are you sure you want to quit the Desktop Settings Wizard?</p>"
		            "<p>The Desktop Settings Wizard helps you to configure the Trinity desktop to your personal liking.</p>"
		            "<p>Click <b>Cancel</b> to return and finish your setup.</p>");
	} else {
		text = i18n("<p>Are you sure you want to quit the Desktop Settings Wizard?</p>"
		            "<p>If yes, click <b>Quit</b> and all changes will be lost."
		            "<br>If not, click <b>Cancel</b> to return and finish your setup.</p>");
	}
	int status = KMessageBox::warningContinueCancel(this,  text, i18n("All Changes Will Be Lost"), KStdGuiItem::quit());
	if(status==KMessageBox::Continue){
		setDefaults();
		return true;
	} else {
		return false;
	}
}

/** the cancel button is connected to the reject() slot of TQDialog,
 *  so we have to reimplement this here to add a dialogbox to ask if we
 *  really want to quit the wizard.
 */
void KPersonalizer::reject(){
	if (askClose()){
		exit(0);
	}
}

void KPersonalizer::closeEvent(TQCloseEvent* e){
	if ( askClose() )
		exit(0);
	else
		e->ignore();
}

/** maybe call a dialog that the wizard has finished. */
void KPersonalizer::accept(){
	exit(0);
}

/** calls all save functions after resetting all features/ OS/ theme selections to Trinity default */
void KPersonalizer::setDefaults(){
	// KCountryPage: The user may need his native language anyway
	if(os_dirty)
		ospage->save(false);
	if(eye_dirty)
		eyecandy->save(false);
	if(style_dirty)
		stylepage->save(false);
}


void KPersonalizer::slotCountryPageLanguageChanged()
{
	// Only the first page is visible at this point. The existing restart in
	// next() will rebuild and translate the remaining pages after the choice
	// is saved.
	setTitle(countrypage, i18n("Step 1: Introduction"));
	backButton()->setText(i18n("&Back"));
	nextButton()->setText(i18n("Opposite to Back", "&Next"));
	cancelButton()->setText(i18n("S&kip Wizard"));
}

/** restart kpersonalizer */
void KPersonalizer::slotRestart() {
	delete countrypage; countrypage = 0;
	delete ospage; ospage = 0;
	delete eyecandy; eyecandy = 0;
	delete stylepage; stylepage = 0;
	delete refinepage; refinepage = 0;

	if( !beforeSession() )
		KRun::runCommand("kpersonalizer -r", "kpersonalizer", "kpersonalizer");

	exit(1); // exit with value 1
}

void KPersonalizer::delayedRestart() {
	TQTimer::singleShot(0, this, TQ_SLOT(slotRestart()));
}

/** this session is restarted, so we want to start with ospage */
void KPersonalizer::restarted(){
	showPage(ospage);
}

/** when kpersonalizer is started before Trinity session, it doesn't
	offer a button for starting KControl, it also doesn't restart
	itself automatically and only exits with exitcode 1 */
void KPersonalizer::setBeforeSession(){
	before_session = true;
}

/** there seems to be a bug in TQWizard, that makes this evil hack necessary */
void KPersonalizer::setPosition() {
	TQSize hint = countrypage->sizeHint();
	TQSize os_size = ospage->sizeHint();
	TQSize candy_size = eyecandy->sizeHint();
	TQSize style_size = stylepage->sizeHint();
	TQSize refine_size = refinepage->sizeHint();

	// get the width of the broadest child-widget
	if ( hint.width() < os_size.width() )
		hint.setWidth(os_size.width());
	if ( hint.width() < candy_size.width() )
		hint.setWidth(candy_size.width());
	if ( hint.width() < style_size.width() )
		hint.setWidth(style_size.width());
	if ( hint.width() < refine_size.width() )
		hint.setWidth(refine_size.width());

	// get the height of the highest child-widget
	if ( hint.height() < os_size.height() )
		hint.setHeight(os_size.height());
	if ( hint.height() < candy_size.height() )
		hint.setHeight(candy_size.height());
	if ( hint.height() < style_size.height() )
		hint.setHeight(style_size.height());
	if ( hint.height() < refine_size.height() )
		hint.setHeight(refine_size.height());

	// set the position
	TQRect rect = TDEGlobalSettings::desktopGeometry(TQCursor::pos());
	int w = rect.x() + (rect.width() - hint.width())/2 - 9;
	int h = rect.y() + (rect.height() - hint.height())/2;
	move(w, h);
}
