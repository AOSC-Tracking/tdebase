/*
    Copyright (C) 2000,2002 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <tqbuttongroup.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqradiobutton.h>
#include <tqslider.h>
#include <tqvbox.h>

#include <dcopclient.h>

#include <tdeapplication.h>
#include <kcombobox.h>
#include <tdeconfig.h>
#include <knotifydialog.h>
#include <tdeparts/genericfactory.h>
#include <kstandarddirs.h>
#include <kurlcompletion.h>
#include <kurlrequester.h>


#include "knotify.h"
#include "playersettings.h"

static const int COL_FILENAME = 1;

typedef KGenericFactory<KCMKNotify, TQWidget> NotifyFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_knotify, NotifyFactory("kcmnotify") )

using namespace KNotify;

KCMKNotify::KCMKNotify(TQWidget *parent, const char *name, const TQStringList & )
    : TDECModule(NotifyFactory::instance(), parent, name),
      m_playerSettings( 0L )
{
    setButtons( Help | Default | Apply );

    setQuickHelp( i18n("<h1>System Notifications</h1>"
                "TDE allows for a great deal of control over how you "
                "will be notified when certain events occur. There are "
                "several choices as to how you are notified:"
                "<ul><li>As the application was originally designed."
                "<li>With a beep or other noise."
                "<li>Via a popup dialog box with additional information."
                "<li>By recording the event in a logfile without "
                "any additional visual or audible alert."
                "</ul>"));

    TQVBoxLayout *layout = new TQVBoxLayout( this, 0, KDialog::spacingHint() );

    TQLabel *label = new TQLabel( i18n( "Event source:" ), this );
    m_appCombo = new KComboBox( false, this, "app combo" );

    TQHBoxLayout *hbox = new TQHBoxLayout( layout );
    hbox->addWidget( label );
    hbox->addWidget( m_appCombo, 10 );

    m_notifyWidget = new KNotifyWidget( this, "knotify widget", true );
    connect( m_notifyWidget, TQT_SIGNAL( changed( bool )), TQT_SIGNAL( changed(bool)));

    layout->addWidget( m_notifyWidget );

    connect( m_appCombo, TQT_SIGNAL( activated( const TQString& ) ),
             TQT_SLOT( slotAppActivated( const TQString& )) );

    connect( m_notifyWidget->m_playerButton, TQT_SIGNAL( clicked() ),
             TQT_SLOT( slotPlayerSettings()));

    TDEAboutData* ab = new TDEAboutData(
        "kcmknotify", I18N_NOOP("KNotify"), "3.0",
        I18N_NOOP("System Notification Control Panel Module"),
        TDEAboutData::License_GPL, "(c) 2002 Carsten Pfeiffer", 0, 0 );
    ab->addAuthor( "Carsten Pfeiffer", 0, "pfeiffer@kde.org" );
    ab->addCredit( "Charles Samuels", I18N_NOOP("Original implementation"),
	       "charles@altair.dhs.org" );
    setAboutData( ab );

    load();
}

KCMKNotify::~KCMKNotify()
{
    TDEConfig config( "knotifyrc", false, false );
    config.setGroup( "Misc" );
    ApplicationList allApps = m_notifyWidget->allApps();
    ApplicationListIterator appIt( allApps );
    for ( ; appIt.current(); ++appIt )
    {
        if( appIt.current()->text() == m_appCombo->currentText())
            config.writeEntry( "LastConfiguredApp", appIt.current()->appName());
    }
}

Application * KCMKNotify::applicationByDescription( const TQString& text )
{
    // not really efficient, but this is not really time-critical
    ApplicationList& allApps = m_notifyWidget->allApps();
    ApplicationListIterator it ( allApps );
    for ( ; it.current(); ++it )
    {
        if ( it.current()->text() == text )
            return it.current();
    }

    return 0L;
}

void KCMKNotify::slotAppActivated( const TQString& text )
{
    Application *app = applicationByDescription( text );
    if ( app )
    {
        m_notifyWidget->clearVisible();
        m_notifyWidget->addVisibleApp( app );
    }
}

void KCMKNotify::slotPlayerSettings()
{
    // tdecmshell is a modal dialog, and apparently, we can't put a non-modal
    // dialog besides a modal dialog. sigh.
    if ( !m_playerSettings )
        m_playerSettings = new PlayerSettingsDialog( this, true );

    m_playerSettings->exec();
}


void KCMKNotify::defaults()
{
    m_notifyWidget->resetDefaults( true ); // ask user
	 load( true );
}

void KCMKNotify::load()
{
	load( false );
}

void KCMKNotify::load( bool useDefaults )
{
    setEnabled( false );

    m_appCombo->clear();
    m_notifyWidget->clear();

    TQStringList fullpaths =
        TDEGlobal::dirs()->findAllResources("data", "*/eventsrc", false, true );

    TQStringList::ConstIterator it = fullpaths.begin();
    for ( ; it != fullpaths.end(); ++it)
        m_notifyWidget->addApplicationEvents( *it );

    ApplicationList allApps = m_notifyWidget->allApps();
    allApps.sort();
    m_notifyWidget->setEnabled( !allApps.isEmpty() );

    TDEConfig config( "knotifyrc", true, false );
	 config.setReadDefaults( useDefaults );
    config.setGroup( "Misc" );
    TQString select = config.readEntry( "LastConfiguredApp" );
    if( select.isEmpty())
        select = "knotify"; // default to system notifications
    bool selected = false;

    ApplicationListIterator appIt( allApps );
    for ( ; appIt.current(); ++appIt )
    {
        m_appCombo->insertItem( appIt.current()->text() );
        if( appIt.current()->appName() == select )
        {
            m_appCombo->setCurrentItem( appIt.current()->text());
            selected = true;
        }
        else if( !selected && appIt.current()->appName() == "knotify" )
            m_appCombo->setCurrentItem( appIt.current()->text());
    }

     // sets the applicationEvents for KNotifyWidget
    slotAppActivated( m_appCombo->currentText() );

    // unsetCursor(); // unsetting doesn't work. sigh.
    setEnabled( true );
    emit changed( useDefaults );
}

void KCMKNotify::save()
{
    if ( m_playerSettings )
        m_playerSettings->save();

    m_notifyWidget->save(); // will dcop knotify about its new config

    emit changed( false );
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

PlayerSettingsDialog::PlayerSettingsDialog( TQWidget *parent, bool modal )
    : KDialogBase( parent, "player settings dialog", modal,
                   i18n("Player Settings"), Ok|Apply|Cancel, Ok, true )
{
    TQFrame *frame = makeMainWidget();

    TQVBoxLayout *topLayout = new TQVBoxLayout( frame, 0,
        KDialog::spacingHint() );

    m_ui = new PlayerSettingsUI(frame);
    topLayout->addWidget(m_ui);

    load( false );
    dataChanged = false;
    enableButton(Apply, false);

    connect( m_ui->cbExternal, TQT_SIGNAL( toggled( bool ) ), this, TQT_SLOT( externalToggled( bool ) ) );
    connect( m_ui->grpPlayers, TQT_SIGNAL( clicked( int ) ), this, TQT_SLOT( slotChanged() ) );
    connect( m_ui->volumeSlider, TQT_SIGNAL( valueChanged ( int ) ), this, TQT_SLOT( slotChanged() ) );
    connect( m_ui->reqExternal, TQT_SIGNAL( textChanged( const TQString& ) ), this, TQT_SLOT( slotChanged() ) );
}

void PlayerSettingsDialog::load( bool useDefaults )
{
    TDEConfig config( "knotifyrc", true, false );
	 config.setReadDefaults( useDefaults );
    config.setGroup( "Misc" );
    bool useExternal = config.readBoolEntry( "Use external player", false );
    m_ui->cbExternal->setChecked( useExternal );
    m_ui->reqExternal->setURL( config.readPathEntry( "External player" ) );
    m_ui->volumeSlider->setValue( config.readNumEntry( "Volume", 100 ) );

    if ( !m_ui->cbExternal->isChecked() )
    {
        config.setGroup( "StartProgress" );
        if ( config.readBoolEntry( "Use Arts", true ) )
        {
            m_ui->cbArts->setChecked( true );
        }
        else
        {
            m_ui->cbNone->setChecked( true );
        }
    }
}

void PlayerSettingsDialog::save()
{
    // see tdelibs/arts/knotify/knotify.cpp
    TDEConfig config( "knotifyrc", false, false );
    config.setGroup( "Misc" );

    config.writePathEntry( "External player", m_ui->reqExternal->url() );
    config.writeEntry( "Use external player", m_ui->cbExternal->isChecked() );
    config.writeEntry( "Volume", m_ui->volumeSlider->value() );

    config.setGroup( "StartProgress" );

    if ( m_ui->cbNone->isChecked() )
    {
        // user explicitly says "no sound!"
        config.writeEntry( "Use Arts", false );
    }
    else if ( m_ui->cbArts->isChecked() )
    {
        // use explicitly said to use aRts so we turn it back on
        // we don't want to always set this to the value of
        // m_ui->cbArts->isChecked() since we don't want to
        // turn off aRts support just because they also chose
        // an external player
        config.writeEntry( "Use Arts", true );
        config.writeEntry( "Arts Init", true ); // reset it for the next time
    }

    config.sync();
}

// reimplements KDialogBase::slotApply()
void PlayerSettingsDialog::slotApply()
{
    save();
    dataChanged = false;
    enableButton(Apply, false);
    kapp->dcopClient()->send("knotify", "", "reconfigure()", TQString(""));

    KDialogBase::slotApply();
}

// reimplements KDialogBase::slotOk()
void PlayerSettingsDialog::slotOk()
{
    if( dataChanged )
        slotApply();
    KDialogBase::slotOk();
}

void PlayerSettingsDialog::slotChanged()
{
    dataChanged = true;
    enableButton(Apply, true);
}

void PlayerSettingsDialog::externalToggled( bool on )
{
    if ( on )
        m_ui->reqExternal->setFocus();
    else
        m_ui->reqExternal->clearFocus();
}

#include "knotify.moc"
