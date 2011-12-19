/* This file is part of the KDE Display Manager Configuration package

    Copyright (C) 2000 Oswald Buddenhagen <ossi@kde.org>
    Based on several other files.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <unistd.h>
#include <sys/types.h>


#include <tqlayout.h>
#include <tqlabel.h>
#include <tqhgroupbox.h>
#include <tqvgroupbox.h>
#include <tqvbuttongroup.h>
#include <tqwhatsthis.h>
#include <tqheader.h>

#include <kdialog.h>
#include <ksimpleconfig.h>
#include <klocale.h>

#include "kdm-conv.h"

extern KSimpleConfig *config;

KDMConvenienceWidget::KDMConvenienceWidget(TQWidget *parent, const char *name)
    : TQWidget(parent, name)
{
    TQString wtstr;

    TQLabel *paranoia = new TQLabel( i18n("<qt><center><font color=red><big><b>Attention!<br>Read help!</b></big></font></center></qt>"), this );

    TQSizePolicy vpref( TQSizePolicy::Minimum, TQSizePolicy::Fixed );

    alGroup = new TQVGroupBox( i18n("Enable Au&to-Login"), this );
    alGroup->setCheckable( true );
    alGroup->setSizePolicy( vpref );

    TQWhatsThis::add( alGroup, i18n("Turn on the auto-login feature."
	" This applies only to KDM's graphical login."
	" Think twice before enabling this!") );
    connect(alGroup, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChanged()));

    TQWidget *hlpw1 = new TQWidget( alGroup );
    userlb = new KComboBox( hlpw1 );
    u_label = new TQLabel( userlb, i18n("Use&r:"), hlpw1 );
    TQGridLayout *hlpl1 = new TQGridLayout(hlpw1, 2, 2, 0, KDialog::spacingHint());
    hlpl1->setColStretch(2, 1);
    hlpl1->addWidget(u_label, 0, 0);
    hlpl1->addWidget(userlb, 0, 1);
    connect(userlb, TQT_SIGNAL(highlighted(int)), TQT_SLOT(slotChanged()));
    wtstr = i18n("Select the user to be logged in automatically.");
    TQWhatsThis::add( u_label, wtstr );
    TQWhatsThis::add( userlb, wtstr );
    delaysb = new TQSpinBox( 0, 3600, 5, hlpw1 );
    delaysb->setSpecialValueText( i18n("delay", "none") );
    delaysb->setSuffix( i18n("seconds", " s") );
    d_label = new TQLabel( delaysb, i18n("D&elay:"), hlpw1 );
    hlpl1->addWidget(d_label, 1, 0);
    hlpl1->addWidget(delaysb, 1, 1);
    connect(delaysb, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(slotChanged()));
    wtstr = i18n("The delay (in seconds) before the automatic login kicks in. "
                 "This feature is also known as \"timed login\".");
    TQWhatsThis::add( d_label, wtstr );
    TQWhatsThis::add( delaysb, wtstr );
    againcb = new TQCheckBox( i18n("P&ersistent"), alGroup );
    connect( againcb, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChanged()) );
    TQWhatsThis::add( againcb, i18n("Normally, automatic login is performed only "
	"when KDM starts up. If this is checked, automatic login will kick in "
	"after finishing a session as well.") );
    autoLockCheck = new TQCheckBox( i18n("Loc&k session"), alGroup );
    connect( autoLockCheck, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChanged()) );
    TQWhatsThis::add( autoLockCheck, i18n("If checked, the automatically started session "
	"will be locked immediately (provided it is a KDE session). This can "
	"be used to obtain a super-fast login restricted to one user.") );


    puGroup = new TQVButtonGroup(i18n("Preselect User"), this );
    puGroup->setSizePolicy( vpref );

    connect(puGroup, TQT_SIGNAL(clicked(int)), TQT_SLOT(slotPresChanged()));
    connect(puGroup, TQT_SIGNAL(clicked(int)), TQT_SLOT(slotChanged()));
    npRadio = new TQRadioButton(i18n("preselected user", "&None"), puGroup);
    ppRadio = new TQRadioButton(i18n("Prev&ious"), puGroup);
    TQWhatsThis::add( ppRadio, i18n("Preselect the user that logged in previously. "
	"Use this if this computer is usually used several consecutive times by one user.") );
    spRadio = new TQRadioButton(i18n("Specif&y"), puGroup);
    TQWhatsThis::add( spRadio, i18n("Preselect the user specified in the combo box below. "
	"Use this if this computer is predominantly used by a certain user.") );
    TQWidget *hlpw = new TQWidget(puGroup);
    puserlb = new KComboBox(true, hlpw);
    pu_label = new TQLabel(puserlb, i18n("Us&er:"), hlpw);
    connect(puserlb, TQT_SIGNAL(textChanged(const TQString &)), TQT_SLOT(slotChanged()));
    wtstr = i18n("Select the user to be preselected for login. "
	"This box is editable, so you can specify an arbitrary non-existent "
	"user to mislead possible attackers.");
    TQWhatsThis::add( pu_label, wtstr );
    TQWhatsThis::add( puserlb, wtstr );
    TQBoxLayout *hlpl = new TQHBoxLayout(hlpw, 0, KDialog::spacingHint());
    hlpl->addWidget(pu_label);
    hlpl->addWidget(puserlb);
    hlpl->addStretch( 1 );
    cbjumppw = new TQCheckBox(i18n("Focus pass&word"), puGroup);
    TQWhatsThis::add( cbjumppw, i18n("When this option is on, KDM will place the cursor "
	"in the password field instead of the user field after preselecting a user. "
	"Use this to save one key press per login, if the preselection usually does not need to "
	"be changed.") );
    connect(cbjumppw, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChanged()));

    npGroup = new TQVGroupBox(i18n("Enable Password-&Less Logins"), this );
    npGroup->setCheckable( true );

    TQWhatsThis::add( npGroup, i18n("When this option is checked, the checked users from"
	" the list below will be allowed to log in without entering their"
	" password. This applies only to KDM's graphical login."
	" Think twice before enabling this!") );

    connect(npGroup, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChanged()));

    pl_label = new TQLabel(i18n("No password re&quired for:"), npGroup);
    npuserlv = new KListView(npGroup);
    pl_label->setBuddy(npuserlv);
    npuserlv->addColumn(TQString::null);
    npuserlv->header()->hide();
    npuserlv->setResizeMode(TQListView::LastColumn);
    TQWhatsThis::add(npuserlv, i18n("Check all users you want to allow a password-less login for."
	" Entries denoted with '@' are user groups. Checking a group is like checking all users in that group."));
    connect( npuserlv, TQT_SIGNAL(clicked( TQListViewItem * )),
	     TQT_SLOT(slotChanged()) );

    btGroup = new TQVGroupBox( i18n("Miscellaneous"), this );

    cbarlen = new TQCheckBox(i18n("Automatically log in again after &X server crash"), btGroup);
    TQWhatsThis::add( cbarlen, i18n("When this option is on, a user will be"
	" logged in again automatically when their session is interrupted by an"
	" X server crash; note that this can open a security hole: if you use"
	" a screen locker than KDE's integrated one, this will make"
	" circumventing a password-secured screen lock possible.") );
    connect(cbarlen, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotChanged()));

    TQGridLayout *main = new TQGridLayout(this, 5, 2, 10);
    main->addWidget(paranoia, 0, 0);
    main->addWidget(alGroup, 1, 0);
    main->addWidget(puGroup, 2, 0);
    main->addMultiCellWidget(npGroup, 0,3, 1,1);
    main->addMultiCellWidget(btGroup, 4,4, 0,1);
    main->setColStretch(0, 1);
    main->setColStretch(1, 2);
    main->setRowStretch(3, 1);

    connect( userlb, TQT_SIGNAL(activated( const TQString & )),
	     TQT_SLOT(slotSetAutoUser( const TQString & )) );
    connect( puserlb, TQT_SIGNAL(textChanged( const TQString & )),
	     TQT_SLOT(slotSetPreselUser( const TQString & )) );
    connect( npuserlv, TQT_SIGNAL(clicked( TQListViewItem * )),
	     TQT_SLOT(slotUpdateNoPassUser( TQListViewItem * )) );

}

void KDMConvenienceWidget::makeReadOnly()
{
    ((TQWidget*)alGroup->child("qt_groupbox_checkbox"))->setEnabled(false);
    userlb->setEnabled(false);
    delaysb->setEnabled(false);
    againcb->setEnabled(false);
    autoLockCheck->setEnabled(false);
    ((TQWidget*)npGroup->child("qt_groupbox_checkbox"))->setEnabled(false);
    npuserlv->setEnabled(false);
    cbarlen->setEnabled(false);
    npRadio->setEnabled(false);
    ppRadio->setEnabled(false);
    spRadio->setEnabled(false);
    puserlb->setEnabled(false);
    cbjumppw->setEnabled(false);
}

void KDMConvenienceWidget::slotPresChanged()
{
    bool en = spRadio->isChecked();
    pu_label->setEnabled(en);
    puserlb->setEnabled(en);
    cbjumppw->setEnabled(!npRadio->isChecked());
}

void KDMConvenienceWidget::save()
{
    config->setGroup("X-:0-Core");
    config->writeEntry( "AutoLoginEnable", alGroup->isChecked() );
    config->writeEntry( "AutoLoginUser", userlb->currentText() );
    config->writeEntry( "AutoLoginDelay", delaysb->value() );
    config->writeEntry( "AutoLoginAgain", againcb->isChecked() );
    config->writeEntry( "AutoLoginLocked", autoLockCheck->isChecked() );

    config->setGroup("X-:*-Core");
    config->writeEntry( "NoPassEnable", npGroup->isChecked() );
    config->writeEntry( "NoPassUsers", noPassUsers );

    config->setGroup("X-*-Core");
    config->writeEntry( "AutoReLogin", cbarlen->isChecked() );

    config->setGroup("X-:*-Greeter");
    config->writeEntry( "PreselectUser", npRadio->isChecked() ? "None" :
				    ppRadio->isChecked() ? "Previous" :
							   "Default" );
    config->writeEntry( "DefaultUser", puserlb->currentText() );
    config->writeEntry( "FocusPasswd", cbjumppw->isChecked() );
}


void KDMConvenienceWidget::load()
{
    config->setGroup("X-:0-Core");
    bool alenable = config->readBoolEntry( "AutoLoginEnable", false);
    autoUser = config->readEntry( "AutoLoginUser" );
    delaysb->setValue( config->readNumEntry( "AutoLoginDelay", 0 ) );
    againcb->setChecked( config->readBoolEntry( "AutoLoginAgain", false ) );
    autoLockCheck->setChecked( config->readBoolEntry( "AutoLoginLocked", false ) );
    if (autoUser.isEmpty())
	alenable=false;
    alGroup->setChecked( alenable );

    config->setGroup("X-:*-Core");
    npGroup->setChecked(config->readBoolEntry( "NoPassEnable", false) );
    noPassUsers = config->readListEntry( "NoPassUsers");

    config->setGroup("X-*-Core");
    cbarlen->setChecked(config->readBoolEntry( "AutoReLogin", false) );

    config->setGroup("X-:*-Greeter");
    TQString presstr = config->readEntry( "PreselectUser", "None" );
    if (presstr == "Previous")
	ppRadio->setChecked(true);
    else if (presstr == "Default")
	spRadio->setChecked(true);
    else
	npRadio->setChecked(true);
    preselUser = config->readEntry( "DefaultUser" );
    cbjumppw->setChecked(config->readBoolEntry( "FocusPasswd", false) );

    slotPresChanged();
}


void KDMConvenienceWidget::defaults()
{
    alGroup->setChecked(false);
    delaysb->setValue(0);
    againcb->setChecked(false);
    autoLockCheck->setChecked(false);
    npRadio->setChecked(true);
    npGroup->setChecked(false);
    cbarlen->setChecked(false);
    cbjumppw->setChecked(false);
    autoUser = "";
    preselUser = "";
    noPassUsers.clear();

    slotPresChanged();
}


void KDMConvenienceWidget::slotChanged()
{
  emit changed(true);
}

void KDMConvenienceWidget::slotSetAutoUser( const TQString &user )
{
    autoUser = user;
}

void KDMConvenienceWidget::slotSetPreselUser( const TQString &user )
{
    preselUser = user;
}

void KDMConvenienceWidget::slotUpdateNoPassUser( TQListViewItem *item )
{
    if ( !item )
        return;
    TQCheckListItem *itm = (TQCheckListItem *)item;
    TQStringList::iterator it = noPassUsers.find( itm->text() );
    if (itm->isOn()) {
	if (it == noPassUsers.end())
	    noPassUsers.append( itm->text() );
    } else {
	if (it != noPassUsers.end())
	    noPassUsers.remove( it );
    }
}

void KDMConvenienceWidget::slotClearUsers()
{
    userlb->clear();
    puserlb->clear();
    npuserlv->clear();
    if (!autoUser.isEmpty())
	userlb->insertItem(autoUser);
    if (!preselUser.isEmpty())
	puserlb->insertItem(preselUser);
}

void KDMConvenienceWidget::slotAddUsers(const TQMap<TQString,int> &users)
{
    TQMapConstIterator<TQString,int> it;
    for (it = users.begin(); it != users.end(); ++it) {
        if (it.data() > 0) {
            if (it.key() != autoUser)
                userlb->insertItem(it.key());
            if (it.key() != preselUser)
                puserlb->insertItem(it.key());
        }
        if (it.data() != 0)
            (new TQCheckListItem(npuserlv, it.key(), TQCheckListItem::CheckBox))->
    	        setOn(noPassUsers.find(it.key()) != noPassUsers.end());
    }

    if (userlb->listBox())
        userlb->listBox()->sort();

    if (puserlb->listBox())
        puserlb->listBox()->sort();

    npuserlv->sort();
    userlb->setCurrentItem(autoUser);
    puserlb->setCurrentItem(preselUser);
}

void KDMConvenienceWidget::slotDelUsers(const TQMap<TQString,int> &users)
{
    TQMapConstIterator<TQString,int> it;
    for (it = users.begin(); it != users.end(); ++it) {
	if (it.data() > 0) {
	    if (it.key() != autoUser && userlb->listBox())
	        delete userlb->listBox()->
		  findItem( it.key(), ExactMatch | CaseSensitive );
	    if (it.key() != preselUser && puserlb->listBox())
	        delete puserlb->listBox()->
		  findItem( it.key(), ExactMatch | CaseSensitive );
	}
	if (it.data() != 0)
	    delete npuserlv->findItem( it.key(), 0 );
    }
}

#include "kdm-conv.moc"
