/* This file is part of the KDE Display Manager Configuration package
    Copyright (C) 1997 Thomas Tanghus (tanghus@earthling.net)

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

#include <tqstyle.h>
#include <tqlayout.h>
#include <tqlabel.h>
#include <tqtooltip.h>
#include <tqvalidator.h>
#include <tqwhatsthis.h>
#include <tqvgroupbox.h>
#include <tqpushbutton.h>

#include <kfiledialog.h>
#include <kimageio.h>
#include <kimagefilepreview.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kurldrag.h>

#include "kdm-users.h"

#include <sys/stat.h>


extern KSimpleConfig *config;

KDMUsersWidget::KDMUsersWidget(TQWidget *parent, const char *name)
    : TQWidget(parent, name)
{
#ifdef __linux__
    struct stat st;
    if (!stat( "/etc/debian_version", &st )) {	/* debian */
	defminuid = "1000";
	defmaxuid = "29999";
    } else if (!stat( "/usr/portage", &st )) {	/* gentoo */
	defminuid = "1000";
	defmaxuid = "65000";
    } else if (!stat( "/etc/mandrake-release", &st )) {	/* mandrake - check before redhat! */
	defminuid = "500";
	defmaxuid = "65000";
    } else if (!stat( "/etc/redhat-release", &st )) {	/* redhat */
	defminuid = "100";
	defmaxuid = "65000";
    } else /* if (!stat( "/etc/SuSE-release", &st )) */ {	/* suse */
	defminuid = "500";
	defmaxuid = "65000";
    }
#else
    defminuid = "1000";
    defmaxuid = "65000";
#endif

    // We assume that $kde_datadir/kdm exists, but better check for pics/ and pics/users,
    // and create them if necessary.
    config->setGroup( "X-*-Greeter" );
    m_userPixDir = config->readEntry( "FaceDir", KGlobal::dirs()->resourceDirs("data").last() + "kdm/faces" ) + '/';
    m_notFirst = false;
    TQDir testDir( m_userPixDir );
    if ( !testDir.exists() && !testDir.mkdir( testDir.absPath() ) && !geteuid() )
        KMessageBox::sorry( this, i18n("Unable to create folder %1").arg( testDir.absPath() ) );
    chmod( TQFile::encodeName( m_userPixDir ), 0755 );

    m_defaultText = i18n("<default>");

    TQString wtstr;

    minGroup = new TQGroupBox( 2, Qt::Horizontal, i18n("System U&IDs"), this );
    TQWhatsThis::add( minGroup, i18n("Users with a UID (numerical user identification) outside this range will not be listed by KDM and this setup dialog."
      " Note that users with the UID 0 (typically root) are not affected by this and must be"
      " explicitly hidden in \"Not hidden\" mode."));
    TQSizePolicy sp_ign_fix( TQSizePolicy::Ignored, TQSizePolicy::Fixed );
    TQValidator *valid = new TQIntValidator( 0, 999999, TQT_TQOBJECT(minGroup) );
    TQLabel *minlab = new TQLabel( i18n("Below:"), minGroup );
    leminuid = new KLineEdit( minGroup );
    minlab->setBuddy( leminuid );
    leminuid->setSizePolicy( sp_ign_fix );
    leminuid->setValidator( valid );
    connect( leminuid, TQT_SIGNAL(textChanged( const TQString & )), TQT_SLOT(slotChanged()) );
    connect( leminuid, TQT_SIGNAL(textChanged( const TQString & )), TQT_SLOT(slotMinMaxChanged()) );
    TQLabel *maxlab = new TQLabel( i18n("Above:"), minGroup );
    lemaxuid = new KLineEdit( minGroup );
    maxlab->setBuddy( lemaxuid );
    lemaxuid->setSizePolicy( sp_ign_fix );
    lemaxuid->setValidator( valid );
    connect(lemaxuid, TQT_SIGNAL(textChanged( const TQString & )), TQT_SLOT(slotChanged()) );
    connect(lemaxuid, TQT_SIGNAL(textChanged( const TQString & )), TQT_SLOT(slotMinMaxChanged()) );

    usrGroup = new TQButtonGroup( 5, Qt::Vertical, i18n("Users"), this );
    connect( usrGroup, TQT_SIGNAL(clicked( int )), TQT_SLOT(slotShowOpts()) );
    connect( usrGroup, TQT_SIGNAL(clicked( int )), TQT_SLOT(slotChanged()) );
    cbshowlist = new TQCheckBox( i18n("Show list"), usrGroup );
    TQWhatsThis::add( cbshowlist, i18n("If this option is checked, KDM will show a list of users,"
      " so users can click on their name or image rather than typing in their login.") );
    cbcomplete = new TQCheckBox( i18n("Autocompletion"), usrGroup );
    TQWhatsThis::add( cbcomplete, i18n("If this option is checked, KDM will automatically complete"
      " user names while they are typed in the line edit.") );
    cbinverted = new TQCheckBox( i18n("Inverse selection"), usrGroup );
    TQWhatsThis::add( cbinverted, i18n("This option specifies how the users for \"Show list\" and \"Autocompletion\""
      " are selected in the \"Select users and groups\" list: "
      "If not checked, select only the checked users. "
      "If checked, select all non-system users, except the checked ones."));
    cbusrsrt = new TQCheckBox( i18n("Sor&t users"), usrGroup );
    connect( cbusrsrt, TQT_SIGNAL(toggled( bool )), TQT_SLOT(slotChanged()) );
    TQWhatsThis::add( cbusrsrt, i18n("If this is checked, KDM will alphabetically sort the user list."
      " Otherwise users are listed in the order they appear in the password file.") );

    wstack = new TQWidgetStack( this );
    s_label = new TQLabel( wstack, i18n("S&elect users and groups:"), this );
    optinlv = new KListView( this );
    optinlv->addColumn( i18n("Selected Users") );
    optinlv->setResizeMode( TQListView::LastColumn );
    TQWhatsThis::add( optinlv, i18n("KDM will show all checked users. Entries denoted with '@' are user groups. Checking a group is like checking all users in that group.") );
    wstack->addWidget( optinlv );
    connect( optinlv, TQT_SIGNAL(clicked( TQListViewItem * )),
	     TQT_SLOT(slotUpdateOptIn( TQListViewItem * )) );
    connect( optinlv, TQT_SIGNAL(clicked( TQListViewItem * )),
	     TQT_SLOT(slotChanged()) );
    optoutlv = new KListView( this );
    optoutlv->addColumn( i18n("Hidden Users") );
    optoutlv->setResizeMode( TQListView::LastColumn );
    TQWhatsThis::add( optoutlv, i18n("KDM will show all non-checked non-system users. Entries denoted with '@' are user groups. Checking a group is like checking all users in that group.") );
    wstack->addWidget( optoutlv );
    connect( optoutlv, TQT_SIGNAL(clicked( TQListViewItem * )),
	     TQT_SLOT(slotUpdateOptOut( TQListViewItem * )) );
    connect( optoutlv, TQT_SIGNAL(clicked( TQListViewItem * )),
	     TQT_SLOT(slotChanged()) );

    faceGroup = new TQButtonGroup( 5, Qt::Vertical, i18n("User Image Source"), this );
    TQWhatsThis::add( faceGroup, i18n("Here you can specify where KDM will obtain the images that represent users."
      " \"Admin\" represents the global folder; these are the pictures you can set below."
      " \"User\" means that KDM should read the user's $HOME/.face.icon file."
      " The two selections in the middle define the order of preference if both sources are available.") );
    connect( faceGroup, TQT_SIGNAL(clicked( int )), TQT_SLOT(slotFaceOpts()) );
    connect( faceGroup, TQT_SIGNAL(clicked( int )), TQT_SLOT(slotChanged()) );
    rbadmonly = new TQRadioButton( i18n("Admin"), faceGroup );
    rbprefadm = new TQRadioButton( i18n("Admin, user"), faceGroup );
    rbprefusr = new TQRadioButton( i18n("User, admin"), faceGroup );
    rbusronly = new TQRadioButton( i18n("User"), faceGroup );

    TQGroupBox *picGroup = new TQVGroupBox( i18n("User Images"), this );
    TQWidget *hlpw = new TQWidget( picGroup );
    usercombo = new KComboBox( hlpw );
    TQWhatsThis::add( usercombo, i18n("The user the image below belongs to.") );
    connect( usercombo, TQT_SIGNAL(activated( int )),
	     TQT_SLOT(slotUserSelected()) );
    TQLabel *userlabel = new TQLabel( usercombo, i18n("User:"), hlpw );
    userbutton = new TQPushButton( hlpw );
    userbutton->setAcceptDrops( true );
    userbutton->installEventFilter( this ); // for drag and drop
    uint sz = style().pixelMetric( TQStyle::PM_ButtonMargin ) * 2 + 48;
    userbutton->setFixedSize( sz, sz );
    connect( userbutton, TQT_SIGNAL(clicked()),
	     TQT_SLOT(slotUserButtonClicked()) );
    TQToolTip::add( userbutton, i18n("Click or drop an image here") );
    TQWhatsThis::add( userbutton, i18n("Here you can see the image assigned to the user selected in the combo box above. Click on the image button to select from a list"
      " of images or drag and drop your own image on to the button (e.g. from Konqueror).") );
    rstuserbutton = new TQPushButton( i18n("Unset"), hlpw );
    TQWhatsThis::add( rstuserbutton, i18n("Click this button to make KDM use the default image for the selected user.") );
    connect( rstuserbutton, TQT_SIGNAL(clicked()),
	     TQT_SLOT(slotUnsetUserPix()) );
    TQGridLayout *hlpl = new TQGridLayout( hlpw, 3, 2, 0, KDialog::spacingHint() );
    hlpl->addWidget( userlabel, 0, 0 );
//    hlpl->addSpacing( KDialog::spacingHint() );
    hlpl->addWidget( usercombo, 0, 1 );
    hlpl->addMultiCellWidget( userbutton, 1,1, 0,1, Qt::AlignHCenter );
    hlpl->addMultiCellWidget( rstuserbutton, 2,2, 0,1, Qt::AlignHCenter );

    TQHBoxLayout *main = new TQHBoxLayout( this, 10 );

    TQVBoxLayout *lLayout = new TQVBoxLayout( main, 10 );
    lLayout->addWidget( minGroup );
    lLayout->addWidget( usrGroup );
    lLayout->addStretch( 1 );

    TQVBoxLayout *mLayout = new TQVBoxLayout( main, 10 );
    mLayout->addWidget( s_label );
    mLayout->addWidget( wstack );
    mLayout->setStretchFactor( wstack, 1 );
    main->setStretchFactor( mLayout, 1 );

    TQVBoxLayout *rLayout = new TQVBoxLayout( main, 10 );
    rLayout->addWidget( faceGroup );
    rLayout->addWidget( picGroup );
    rLayout->addStretch( 1 );

}

void KDMUsersWidget::makeReadOnly()
{
    leminuid->setReadOnly(true);
    lemaxuid->setReadOnly(true);
    cbshowlist->setEnabled(false);
    cbcomplete->setEnabled(false);
    cbinverted->setEnabled(false);
    cbusrsrt->setEnabled(false);
    rbadmonly->setEnabled(false);
    rbprefadm->setEnabled(false);
    rbprefusr->setEnabled(false);
    rbusronly->setEnabled(false);
    wstack->setEnabled(false);
    disconnect( userbutton, TQT_SIGNAL(clicked()), this, TQT_SLOT(slotUserButtonClicked()) );
    userbutton->setAcceptDrops(false);
    rstuserbutton->setEnabled(false);
}

void KDMUsersWidget::slotShowOpts()
{
    bool en = cbshowlist->isChecked() || cbcomplete->isChecked();
    cbinverted->setEnabled( en );
    cbusrsrt->setEnabled( en );
    wstack->setEnabled( en );
    wstack->raiseWidget( cbinverted->isChecked() ? optoutlv : optinlv );
    en = cbshowlist->isChecked();
    faceGroup->setEnabled( en );
    if (!en) {
	usercombo->setEnabled( false );
	userbutton->setEnabled( false );
	rstuserbutton->setEnabled( false );
    } else
	slotFaceOpts();
}

void KDMUsersWidget::slotFaceOpts()
{
    bool en = !rbusronly->isChecked();
    usercombo->setEnabled( en );
    userbutton->setEnabled( en );
    if (en)
	slotUserSelected();
    else
	rstuserbutton->setEnabled( false );
}

void KDMUsersWidget::slotUserSelected()
{
    TQString user = usercombo->currentText();
    TQImage p;
    if (user != m_defaultText &&
	p.load( m_userPixDir + user + ".face.icon" )) {
	rstuserbutton->setEnabled( !getuid() );
    } else {
	p.load( m_userPixDir + ".default.face.icon" );
	rstuserbutton->setEnabled( false );
    }
    userbutton->setPixmap( p.smoothScale( 48, 48, TQ_ScaleMin ) );
}


void KDMUsersWidget::changeUserPix(const TQString &pix)
{
    TQString user( usercombo->currentText() );
    if (user == m_defaultText)
    {
       user = ".default";
       if (KMessageBox::questionYesNo(this, i18n("Save image as default image?"),TQString::null,KStdGuiItem::save(),KStdGuiItem::cancel())
            != KMessageBox::Yes)
          return;
    }

    TQImage p( pix );
    if (p.isNull()) {
	KMessageBox::sorry( this,
			    i18n("There was an error loading the image\n"
				 "%1").arg( pix ) );
	return;
    }

    p = p.smoothScale( 48, 48, TQ_ScaleMin );
    TQString userpix = m_userPixDir + user + ".face.icon";
    if (!p.save( userpix, "PNG" ))
        KMessageBox::sorry(this,
	    i18n("There was an error saving the image:\n%1")
		.arg( userpix ) );
    else
        chmod( TQFile::encodeName( userpix ), 0644 );

    slotUserSelected();
}

void KDMUsersWidget::slotUserButtonClicked()
{
    KFileDialog dlg(m_notFirst ? TQString::null :
	KGlobal::dirs()->resourceDirs("data").last() + "kdm/pics/users",
                    KImageIO::pattern( KImageIO::Reading ),
                    this, 0, true);
    dlg.setOperationMode( KFileDialog::Opening );
    dlg.setCaption( i18n("Choose Image") );
    dlg.setMode( KFile::File | KFile::LocalOnly );

    KImageFilePreview *ip = new KImageFilePreview( &dlg );
    dlg.setPreviewWidget( ip );
    if (dlg.exec() != TQDialog::Accepted)
	return;
    m_notFirst = true;

    changeUserPix( dlg.selectedFile() );
}

void KDMUsersWidget::slotUnsetUserPix()
{
    TQFile::remove( m_userPixDir + usercombo->currentText() + ".face.icon" );
    slotUserSelected();
}

bool KDMUsersWidget::eventFilter(TQObject *, TQEvent *e)
{
    if (e->type() == TQEvent::DragEnter) {
	TQDragEnterEvent *ee = (TQDragEnterEvent *) e;
	ee->accept( KURLDrag::canDecode(ee) );
        return true;
    }

    if (e->type() == TQEvent::Drop) {
        userButtonDropEvent((TQDropEvent *) e);
        return true;
    }

    return false;
}

KURL *decodeImgDrop(TQDropEvent *e, TQWidget *wdg);

void KDMUsersWidget::userButtonDropEvent(TQDropEvent *e)
{
    KURL *url = decodeImgDrop(e, this);
    if (url) {
	TQString pixpath;
	KIO::NetAccess::download(*url, pixpath, parentWidget());
	changeUserPix( pixpath );
	KIO::NetAccess::removeTempFile(pixpath);
	delete url;
    }
}

void KDMUsersWidget::save()
{
    config->setGroup( "X-*-Greeter" );

    config->writeEntry( "MinShowUID", leminuid->text() );
    config->writeEntry( "MaxShowUID", lemaxuid->text() );

    config->writeEntry( "UserList", cbshowlist->isChecked() );
    config->writeEntry( "UserCompletion", cbcomplete->isChecked() );
    config->writeEntry( "ShowUsers",
	cbinverted->isChecked() ? "NotHidden" : "Selected" );
    config->writeEntry( "SortUsers", cbusrsrt->isChecked() );

    config->writeEntry( "HiddenUsers", hiddenUsers );
    config->writeEntry( "SelectedUsers", selectedUsers );

    config->writeEntry( "FaceSource",
	rbadmonly->isChecked() ? "AdminOnly" :
	rbprefadm->isChecked() ? "PreferAdmin" :
	rbprefusr->isChecked() ? "PreferUser" : "UserOnly" );
}


void KDMUsersWidget::updateOptList( TQListViewItem *item, TQStringList &list )
{
    if ( !item )
        return;
    TQCheckListItem *itm = (TQCheckListItem *)item;
    TQStringList::iterator it = list.find( itm->text() );
    if (itm->isOn()) {
	if (it == list.end())
	    list.append( itm->text() );
    } else {
	if (it != list.end())
	    list.remove( it );
    }
}

void KDMUsersWidget::slotUpdateOptIn( TQListViewItem *item )
{
    updateOptList( item, selectedUsers );
}

void KDMUsersWidget::slotUpdateOptOut( TQListViewItem *item )
{
    updateOptList( item, hiddenUsers );
}

void KDMUsersWidget::slotClearUsers()
{
    optinlv->clear();
    optoutlv->clear();
    usercombo->clear();
    usercombo->insertItem( m_defaultText );
}

void KDMUsersWidget::slotAddUsers(const TQMap<TQString,int> &users)
{
    TQMapConstIterator<TQString,int> it;
    for (it = users.begin(); it != users.end(); ++it) {
      const TQString *name = &it.key();
      (new TQCheckListItem(optinlv, *name, TQCheckListItem::CheckBox))->
	      setOn(selectedUsers.find(*name) != selectedUsers.end());
      (new TQCheckListItem(optoutlv, *name, TQCheckListItem::CheckBox))->
	      setOn(hiddenUsers.find(*name) != hiddenUsers.end());
      if ((*name)[0] != '@')
        usercombo->insertItem(*name);
    }
    optinlv->sort();
    optoutlv->sort();
    if (usercombo->listBox())
        usercombo->listBox()->sort();
}

void KDMUsersWidget::slotDelUsers(const TQMap<TQString,int> &users)
{
    TQMapConstIterator<TQString,int> it;
    for (it = users.begin(); it != users.end(); ++it) {
        const TQString *name = &it.key();
        if (usercombo->listBox())
            delete usercombo->listBox()->findItem( *name, ExactMatch | CaseSensitive );
        delete optinlv->findItem( *name, 0 );
        delete optoutlv->findItem( *name, 0 );
    }
}

void KDMUsersWidget::load()
{
    TQString str;

    config->setGroup("X-*-Greeter");

    selectedUsers = config->readListEntry( "SelectedUsers");
    hiddenUsers = config->readListEntry( "HiddenUsers");

    leminuid->setText(config->readEntry("MinShowUID", defminuid));
    lemaxuid->setText(config->readEntry("MaxShowUID", defmaxuid));

    cbshowlist->setChecked( config->readBoolEntry( "UserList", true ) );
    cbcomplete->setChecked( config->readBoolEntry( "UserCompletion", false ) );
    cbinverted->setChecked( config->readEntry( "ShowUsers" ) != "Selected" );
    cbusrsrt->setChecked(config->readBoolEntry("SortUsers", true));

    TQString ps = config->readEntry( "FaceSource" );
    if (ps == TQString::fromLatin1("UserOnly"))
	rbusronly->setChecked(true);
    else if (ps == TQString::fromLatin1("PreferUser"))
	rbprefusr->setChecked(true);
    else if (ps == TQString::fromLatin1("PreferAdmin"))
	rbprefadm->setChecked(true);
    else
	rbadmonly->setChecked(true);

    slotUserSelected();

    slotShowOpts();
    slotFaceOpts();
}

void KDMUsersWidget::defaults()
{
    leminuid->setText( defminuid );
    lemaxuid->setText( defmaxuid );
    cbshowlist->setChecked( true );
    cbcomplete->setChecked( false );
    cbinverted->setChecked( true );
    cbusrsrt->setChecked( true );
    rbadmonly->setChecked( true );
    hiddenUsers.clear();
    selectedUsers.clear();
    slotShowOpts();
    slotFaceOpts();
}

void KDMUsersWidget::slotMinMaxChanged()
{
    emit setMinMaxUID( leminuid->text().toInt(), lemaxuid->text().toInt() );
}

void KDMUsersWidget::slotChanged()
{
  emit changed(true);
}

#include "kdm-users.moc"
