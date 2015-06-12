/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "medianotifier.h"

#if defined (__OpenBSD__) || defined(__FreeBSD__)
#include <sys/statvfs.h>
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif

#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqdir.h>
#include <tqcheckbox.h>

#include <tdeapplication.h>
#include <tdeglobal.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <kprocess.h>
#include <krun.h>
#include <tdemessagebox.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>

#include "notificationdialog.h"
#include "notifiersettings.h"
#include "notifieraction.h"
#include "mediamanagersettings.h"

#include "dmctl.h"

MediaNotifier::MediaNotifier(const TQCString &name) : KDEDModule(name)
{
	connectDCOPSignal( "kded", "mediamanager", "mediumAdded(TQString, bool)",
	                   "onMediumChange(TQString, bool)", true );
	
	connectDCOPSignal( "kded", "mediamanager", "mediumChanged(TQString, bool)",
	                   "onMediumChange(TQString, bool)", true );

	connectDCOPSignal( "kded", "mediamanager", "mediumRemoved(TQString, bool)",
	                   "onMediumRemove(TQString, bool)", true );

	m_notificationDialogList.setAutoDelete(FALSE);
	m_freeTimer = new TQTimer( this );
	connect( m_freeTimer, TQT_SIGNAL( timeout() ), TQT_SLOT( checkFreeDiskSpace() ) );
	m_freeTimer->start( 1000*6*2 /* 20 minutes */ );
	m_freeDialog = 0;
}

MediaNotifier::~MediaNotifier()
{
	disconnectDCOPSignal( "kded", "mediamanager", "mediumAdded(TQString, bool)",
	                      "onMediumChange(TQString, bool)" );
	
	disconnectDCOPSignal( "kded", "mediamanager", "mediumChanged(TQString, bool)",
	                      "onMediumChange(TQString, bool)" );

	disconnectDCOPSignal( "kded", "mediamanager", "mediumRemoved(TQString, bool)",
	                      "onMediumRemove(TQString, bool)" );
}

void MediaNotifier::onMediumRemove( const TQString &name, bool allowNotification )
{
	kdDebug() << "MediaNotifier::onMediumRemove( " << name << ", "
	          << allowNotification << ")" << endl;

	KURL url(  "system:/media/"+name );

	NotificationDialog* dialog;
	for (dialog = m_notificationDialogList.first(); dialog; dialog = m_notificationDialogList.next()) {
		if (dialog->medium().url() == url) {
			dialog->close();
		}
	}
}

void MediaNotifier::onMediumChange( const TQString &name, bool allowNotification )
{
	kdDebug() << "MediaNotifier::onMediumChange( " << name << ", "
	          << allowNotification << ")" << endl;

	if ( !allowNotification ) {
	  return;
	}

	// Update user activity timestamp, otherwise the notification dialog will be shown
	// in the background due to focus stealing prevention. Entering a new media can
	// be seen as a kind of user activity after all. It'd be better to update the timestamp
	// as soon as the media is entered, but it apparently takes some time to get here.
	kapp->updateUserTimestamp();

	KURL url(  "system:/media/"+name );

	TDEIO::SimpleJob *job = TDEIO::stat( url, false );
	job->setInteractive( false );

	m_allowNotificationMap[job] = allowNotification;
	
	connect( job, TQT_SIGNAL( result( TDEIO::Job * ) ),
	         this, TQT_SLOT( slotStatResult( TDEIO::Job * ) ) );
}

void MediaNotifier::slotStatResult( TDEIO::Job *job )
{
	bool allowNotification = m_allowNotificationMap[job];
	m_allowNotificationMap.remove( job );
	
	if ( job->error() != 0 ) return;
	
	TDEIO::StatJob *stat_job = static_cast<TDEIO::StatJob *>( job );
	
	TDEIO::UDSEntry entry = stat_job->statResult();
	KURL url = stat_job->url();
	
	KFileItem medium( entry, url );

	if ( autostart( medium ) ) return;
	
	if ( allowNotification ) notify( medium );
}

bool MediaNotifier::autostart( const KFileItem &medium )
{
	TQString mimetype = medium.mimetype();

	bool is_cdrom = mimetype.startsWith( "media/cd" ) || mimetype.startsWith( "media/dvd" );
	bool is_mounted = mimetype.contains( "_mounted" );
	
	// We autorun only on CD/DVD or removable disks (USB, Firewire)
	if ( !( is_cdrom || is_mounted )
	  && !mimetype.startsWith("media/removable_mounted") )
	{
		return false;
	}


	// Here starts the 'Autostart Of Applications After Mount' implementation
	
	// The desktop environment MAY ignore Autostart files altogether
	// based on policy set by the user, system administrator or vendor.
	MediaManagerSettings::self()->readConfig();
	if ( !MediaManagerSettings::self()->autostartEnabled() )
	{
		return false;
	}
	
	// From now we're sure the medium is already mounted.
	// We can use the local path for stating, no need to use TDEIO here.
	bool local;
	TQString path = medium.mostLocalURL( local ).path(); // local is always true here...

	// When a new medium is mounted the root directory of the medium should
	// be checked for the following Autostart files in order of precedence:
	// .autorun, autorun, autorun.sh
	TQStringList autorun_list;
	autorun_list << ".autorun" << "autorun" << "autorun.sh";

	TQStringList::iterator it = autorun_list.begin();
	TQStringList::iterator end = autorun_list.end();

	for ( ; it!=end; ++it )
	{
		if ( TQFile::exists( path + "/" + *it ) )
		{
			return execAutorun( medium, path, *it );
		}
	}
	
	// When a new medium is mounted the root directory of the medium should
	// be checked for the following Autoopen files in order of precedence:
	// .autoopen, autoopen
	TQStringList autoopen_list;
	autoopen_list << ".autoopen" << "autoopen";

	it = autoopen_list.begin();
	end = autoopen_list.end();
	
	for ( ; it!=end; ++it )
	{
		if ( TQFile::exists( path + "/" + *it ) )
		{
			return execAutoopen( medium, path, *it );
		}
	}

	return false;
}

bool MediaNotifier::execAutorun( const KFileItem &medium, const TQString &path,
                                 const TQString &autorunFile )
{
	// The desktop environment MUST prompt the user for confirmation
	// before automatically starting an application.
	TQString mediumType = medium.mimeTypePtr()->name();
	TQString text = i18n( "An autorun file has been found on your '%1'."
	                     " Do you want to execute it?\n"
	                     "Note that executing a file on a medium may compromise"
	                     " your system's security").arg( mediumType );
	TQString caption = i18n( "Autorun - %1" ).arg( medium.url().prettyURL() );
	KGuiItem yes = KStdGuiItem::yes();
	KGuiItem no = KStdGuiItem::no();
	int options = KMessageBox::Notify | KMessageBox::Dangerous;

	int answer = KMessageBox::warningYesNo( 0L, text, caption, yes, no,
	                                        TQString::null, options );

	if ( answer == KMessageBox::Yes )
	{
		// When an Autostart file has been detected and the user has
		// confirmed its execution the autostart file MUST be executed
		// with the current working directory ( CWD ) set to the root
		// directory of the medium.
		TDEProcess proc;
		proc << "sh" << autorunFile;
		proc.setWorkingDirectory( path );
		proc.start();
		proc.detach();
	}
	
	return true;
}

bool MediaNotifier::execAutoopen( const KFileItem &medium, const TQString &path,
                                  const TQString &autoopenFile )
{
	// An Autoopen file MUST contain a single relative path that points
	// to a non-executable file contained on the medium. [...]
	TQFile file( path+"/"+autoopenFile );
	file.open( IO_ReadOnly );
	TQTextStream stream( &file );

	TQString relative_path = stream.readLine().stripWhiteSpace();

	// The relative path MUST NOT contain path components that
	// refer to a parent directory ( ../ )
	if ( relative_path.startsWith( "/" ) || relative_path.contains( "../" ) )
	{
		return false;
	}
	
	// The desktop environment MUST verify that the relative path points
	// to a file that is actually located on the medium [...]
	TQString resolved_path
		= TDEStandardDirs::realFilePath( path+"/"+relative_path );

	if ( !resolved_path.startsWith( path ) )
	{
		return false;
	}
	
	
	TQFile document( resolved_path );

	// TODO: What about FAT all files are executable...
	// If the relative path points to an executable file then the desktop
	// environment MUST NOT execute the file.
	if ( !document.exists() /*|| TQFileInfo(document).isExecutable()*/ )
	{
		return false;
	}

	KURL url = medium.url();
	url.addPath( relative_path );
	
	// The desktop environment MUST prompt the user for confirmation
	// before opening the file.
	TQString mediumType = medium.mimeTypePtr()->name();
	TQString filename = url.filename();
	TQString text = i18n( "An autoopen file has been found on your '%1'."
	                     " Do you want to open '%2'?\n"
	                     "Note that opening a file on a medium may compromise"
	                     " your system's security").arg( mediumType ).arg( filename );
	TQString caption = i18n( "Autoopen - %1" ).arg( medium.url().prettyURL() );
	KGuiItem yes = KStdGuiItem::yes();
	KGuiItem no = KStdGuiItem::no();
	int options = KMessageBox::Notify | KMessageBox::Dangerous;

	int answer = KMessageBox::warningYesNo( 0L, text, caption, yes, no,
	                                        TQString::null, options );

	// TODO: Take case of the "UNLESS" part?
	// When an Autoopen file has been detected and the user has confirmed
	// that the file indicated in the Autoopen file should be opened then
	// the file indicated in the Autoopen file MUST be opened in the
	// application normally preferred by the user for files of its kind
	// UNLESS the user instructed otherwise.
	if ( answer == KMessageBox::Yes )
	{
		( void ) new KRun( url );
	}
	
	return true;
}

void MediaNotifier::notify( KFileItem &medium )
{
	kdDebug() << "Notification triggered." << endl;

	DM dm;
	int currentActiveVT = dm.activeVT();
	int currentX11VT = TDEApplication::currentX11VT();

	if (currentX11VT < 0) {
		// Do not notify if user is not local
		return;
	}
	if ((currentActiveVT >= 0) && (currentX11VT != currentActiveVT)) {
		// Do not notify if VT is not active!
		return;
	}

	NotifierSettings *settings = new NotifierSettings();
	
	if ( settings->autoActionForMimetype( medium.mimetype() )==0L )
	{
		TQValueList<NotifierAction*> actions = settings->actionsForMimetype( medium.mimetype() );
		
		// If only one action remains, it's the "do nothing" action
		// no need to popup in this case.
		if ( actions.size()>1 )
		{
			NotificationDialog*  notifier = new NotificationDialog( medium, settings );
			connect(notifier, TQT_SIGNAL(destroyed(TQObject*)), this, TQT_SLOT(notificationDialogDestroyed(TQObject*)));
			m_notificationDialogList.append(notifier);
			notifier->show();
		}
	}
	else
	{
		NotifierAction *action = settings->autoActionForMimetype( medium.mimetype() );
		action->execute( medium );
		delete settings;
	}
}

void MediaNotifier::notificationDialogDestroyed(TQObject* object)
{
	m_notificationDialogList.remove(static_cast<NotificationDialog*>(object));
}

extern "C"
{
	KDE_EXPORT KDEDModule *create_medianotifier(const TQCString &name)
	{
		TDEGlobal::locale()->insertCatalogue("kay");
		return new MediaNotifier(name);
	}
}

void MediaNotifier::checkFreeDiskSpace()
{
    struct statfs sfs;
    long total, avail;
    if ( m_freeDialog )
        return;

    if ( statfs( TQFile::encodeName( TQDir::homeDirPath() ), &sfs ) == 0 )
    {
        total = sfs.f_blocks;
        avail = ( getuid() ? sfs.f_bavail : sfs.f_bfree );

        if (avail < 0 || total <= 0)
            return; // we better do not say anything about it

        int freeperc = static_cast<int>(100 * double(avail) / total);

        if ( freeperc < 5 && KMessageBox::shouldBeShownContinue( "dontagainfreespace" ) )    // free disk space dropped under a limit
        {
            m_freeDialog= new KDialogBase(
                i18n( "Low Disk Space" ),
                KDialogBase::Yes | KDialogBase::No,
                KDialogBase::Yes, KDialogBase::No,
                0, "warningYesNo", false, true,
                i18n( "Start Konqueror" ), KStdGuiItem::cancel());

            TQString text = i18n( "You are running low on disk space on your home partition (currently %1% free), would you like to "
                                 "run Konqueror to free some disk space and fix the problem?" ).arg( freeperc );
            bool checkboxResult = false;
            KMessageBox::createKMessageBox(m_freeDialog, TQMessageBox::Warning, text, TQStringList(),
                                           i18n("Do not ask again"),
                                           &checkboxResult, KMessageBox::Notify | KMessageBox::NoExec);
            m_freeDialog->show();
            connect( m_freeDialog, TQT_SIGNAL( yesClicked() ), TQT_SLOT( slotFreeContinue() ) );
            connect( m_freeDialog, TQT_SIGNAL( noClicked() ), TQT_SLOT( slotFreeCancel() ) );
        }
    }
}

void MediaNotifier::slotFreeContinue()
{
    slotFreeFinished( KMessageBox::Continue );
}

void MediaNotifier::slotFreeCancel()
{
    slotFreeFinished( KMessageBox::Cancel );
}

void MediaNotifier::slotFreeFinished( KMessageBox::ButtonCode res )
{
    TQCheckBox *checkbox = ::tqqt_cast<TQCheckBox*>( m_freeDialog->child( 0, TQCHECKBOX_OBJECT_NAME_STRING ) );
    if ( checkbox && checkbox->isChecked() )
        KMessageBox::saveDontShowAgainYesNo("dontagainfreespace", res);
    m_freeDialog->delayedDestruct();
    m_freeDialog = 0;

    if ( res == KMessageBox::Continue ) // start Konqi
    {
        ( void ) new KRun( KURL::fromPathOrURL( TQDir::homeDirPath() ) );
    }
    else                // people don't want to be bothered, at least stop the timer; there's no way to save the dontshowagain entry in this case
        m_freeTimer->stop();
}

#include "medianotifier.moc"
