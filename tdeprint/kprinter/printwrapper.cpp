/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2001 Michael Goffioul <tdeprint@swing.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#include "printwrapper.h"

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include <tqstring.h>
#include <tqstringlist.h>
#include <stdlib.h>
#include <tdemessagebox.h>
#include <tqfile.h>
#include <tqtimer.h>
#include <tqregexp.h>
#include <tqsocketnotifier.h>

#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <tdeio/netaccess.h>
#include <kurl.h>
#include <kdebug.h>
#include <tdetempfile.h>

#include <kprinter.h>
#include <tdeprint/kmmanager.h>
#include <tdeprint/kmprinter.h>
#include <tdeprint/kprintdialog.h>

void signal_handler(int);
TQString tempFile;
bool fromStdin = false;
char job_output = 0;	// 0: dialog, 1: console, 2: none
char readchar = '\0';
bool dataread = false;
bool docopy = false;

void showmsgdialog(const TQString& msg, int type = 0)
{
	switch (type)
	{
	   case 0: KMessageBox::information(NULL,msg,i18n("Print Information")); break;
	   case 1: KMessageBox::sorry(NULL,msg,i18n("Print Warning")); break;
	   case 2: KMessageBox::error(NULL,msg,i18n("Print Error")); break;
	}
}

void showmsgconsole(const TQString& msg, int type = 0)
{
	TQString	errmsg = TQString::fromLatin1("%1 : ").arg((type == 0 ? i18n("Print info") : (type == 1 ? i18n("Print warning") : i18n("Print error"))));
	kdDebug() << errmsg << msg << endl;
}

void showmsg(const TQString& msg, int type = 0)
{
	switch (job_output) {
	   case 0: showmsgdialog(msg,type); break;
	   case 1: showmsgconsole(msg,type); break;
	   default: break;
	}
}

void errormsg(const TQString& msg)
{
	showmsg(msg,2);
	exit(1);
}

void signal_handler(int s)
{
	TQFile::remove(tempFile);
	exit(s);
}

TQString copyfile( const TQString& filename )
{
	kdDebug( 500 ) << "Copying file " << filename << endl;
	TQString result;
	TQFile f( filename );
	if ( f.open( IO_ReadOnly ) )
	{
		KTempFile temp;
		temp.setAutoDelete( false );
		TQFile *tf = temp.file();
		if ( tf )
		{
			char buffer[ 0xFFFF ];
			int b = 0;
			while ( ( b = f.readBlock( buffer, 0xFFFF ) ) > 0 )
			{
				if ( tf->writeBlock( buffer, b ) != b )
					break;
			}
			tf->close();
			if ( b > 0 )
				temp.setAutoDelete( true );
			else
			{
				kdDebug( 500 ) << "File copied to " << temp.name() << endl;
				result = temp.name();
			}
		}
		else
			temp.setAutoDelete( true );
		f.close();
	}
	return result;
}

//******************************************************************************************************

PrintWrapper::PrintWrapper()
: TQWidget(), force_stdin(false), check_stdin(true)
{
}

void PrintWrapper::slotPrint()
{
	TDECmdLineArgs	*args = TDECmdLineArgs::parsedArgs();

#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
	struct sigaction action;
#endif /* HAVE_SIGACTION && !HAVE_SIGSET*/

	// read variables from command line
	TQString	printer = args->getOption("d");
	TQString	title = args->getOption("t");
	int	ncopies = TQString(args->getOption("n")).toInt();
	TQString	job_mode = args->getOption("j");
	TQString	system = args->getOption("system");
	QCStringList	optlist = args->getOptionList("o");
	TQMap<TQString,TQString>	opts;
	KURL::List	files;
	TQStringList	filestoprint;
	force_stdin = args->isSet("stdin");
	docopy = args->isSet( "c" );
	bool	nodialog = !(args->isSet("dialog"));
	
	if( isatty( 0 ))
	    {
	    kdDebug( 500 ) << "stdin is a terminal, disabling it" << endl;
	    check_stdin = false;
	    }

	// parse options
	for (QCStringList::ConstIterator it=optlist.begin(); it!=optlist.end(); ++it)
	{
		TQStringList	l = TQStringList::split('=',TQString(*it),false);
		if (l.count() >= 1) opts[l[0]] = (l.count() == 2 ? l[1] : TQString::null);
	}

	// read file list
	for (int i=0; i<args->count(); i++)
		files.append(args->url(i));

	// some clean-up
	args->clear();

	// set default values if necessary
	if (job_mode == "console") job_output = 1;
	else if (job_mode == "none") job_output = 2;
	else job_output = 0;

	// some checking
	if ( files.count() > 0)
	{
		check_stdin = false;
		
		if( force_stdin )
		{
			showmsg(i18n("A file has been specified on the command line. Printing from STDIN will be disabled."), 1);
			force_stdin = false;
		}
	}
	if (nodialog && files.count() == 0 &&!force_stdin && !check_stdin )
	{
		errormsg(i18n("When using '--nodialog', you must at least specify one file to print or use the '--stdin' flag."));
	}
	
	if( check_stdin )
	{ // check if there's any input on stdin
	    fd_set in;
	    struct timeval tm;
	    tm.tv_sec = 0;
	    tm.tv_usec = 0;
	    FD_ZERO( &in );
	    FD_SET( 0, &in );
	    if( select( 1, &in, NULL, NULL, &tm ) )
	    { // we have data on stdin
			if ( read( 0, &readchar, 1 ) > 0 )
			{
				force_stdin = true;
				check_stdin = false;
				dataread = true;
				kdDebug( 500 ) << "input detected on stdin" << endl;
			}
			else
			{
				force_stdin = check_stdin = false;
				kdDebug( 500 ) << "stdin closed and empty" << endl;
			}
	    } 
		else
			kdDebug( 500 ) << "no input on stdin at startup" << endl;
	}

        // force_stdin ? or also check_stdin ?
	KPrinter::ApplicationType	dialog_mode = (force_stdin || nodialog ? KPrinter::StandAlone : KPrinter::StandAlonePersistent);
	KPrinter::setApplicationType(dialog_mode);
	if (!force_stdin)
		KPrinter::addStandardPage(KPrinter::FilesPage);

	KPrinter	kprinter;
	if (nodialog)
	{
		KMPrinter	*prt(0);
		KMManager	*mgr = KMManager::self();

		mgr->printerList(false);
		if (!printer.isEmpty())
			prt = mgr->findPrinter(printer);
		else
			prt = mgr->defaultPrinter();

		if (prt == 0)
			errormsg(i18n("The specified printer or the default printer could not be found."));
		else if (!prt->autoConfigure(&kprinter))
			errormsg(i18n("Operation aborted."));
	}
	else if (!printer.isEmpty())
		kprinter.setSearchName(printer);
	kprinter.setDocName(title);
	kprinter.initOptions(opts);
	kprinter.setOption("kde-filelist", files.toStringList().join("@@"));
	kdDebug( 500 ) << kprinter.option( "kde-filelist" ) << endl;
	if (ncopies > 0)
		kprinter.setNumCopies(ncopies);

	if (nodialog)
		slotPrintRequested(&kprinter);
	else
	{
		dlg = KPrintDialog::printerDialog(&kprinter, 0);
		if (dlg)
		{
			connect(dlg, TQ_SIGNAL(printRequested(KPrinter*)), TQ_SLOT(slotPrintRequested(KPrinter*)));
			if( check_stdin )
			{
			    notif = new TQSocketNotifier( 0, TQSocketNotifier::Read, this );
			    connect( notif, TQ_SIGNAL( activated( int )), this, TQ_SLOT( slotGotStdin()));
			    kdDebug( 500 ) << "waiting for input on stdin" << endl;
			}
			dlg->exec();
			delete dlg;
		}
		else
			errormsg(i18n("Unable to construct the print dialog."));
	}

	TQTimer::singleShot(10,tdeApp,TQ_SLOT(quit()));
}

void hack( KPrintDialog* dlg );

void PrintWrapper::slotGotStdin()
{
	delete notif;
	if ( read( 0, &readchar, 1 ) > 0 )
	{
		force_stdin = true;
		check_stdin = false;
		dataread = true;
		dlg->enableDialogPage( 0, false );
		kdDebug( 500 ) << "got delayed input on stdin" << endl;
	}
}

void PrintWrapper::slotPrintRequested(KPrinter *kprinter)
{
	// re-initialize docName
	kprinter->setDocName(TQString::null);

	// download files if needed
	TQStringList	files = TQStringList::split("@@", kprinter->option("kde-filelist"), false), filestoprint;
	for (TQStringList::ConstIterator it=files.begin(); it!=files.end(); ++it)
	{
		TQString	tmpFile;
		KURL	url = KURL::fromPathOrURL(*it);
		kdDebug( 500 ) << url.url() << endl;
		if (TDEIO::NetAccess::download(url, tmpFile, this))
		{
			filestoprint << tmpFile;
			kprinter->setDocName(url.fileName());
		}
	}

	if (filestoprint.count() > 1)
		kprinter->setDocName(i18n("Multiple files (%1)").arg(filestoprint.count()));
	else if (kprinter->docName().isEmpty())
		kprinter->setDocName(force_stdin ? "<STDIN>" : "KPrinter");
	if (filestoprint.count() == 0)
	{
		// At this point force_stdin should be true
		if (!force_stdin)
			errormsg(i18n("Nothing to print."));

		// print from stdin

#  if defined(HAVE_SIGSET)
		sigset(SIGHUP, signal_handler);
		sigset(SIGINT, signal_handler);
		sigset(SIGTERM, signal_handler);
#  elif defined(HAVE_SIGACTION)
		memset(&action, 0, sizeof(action));
		action.sa_handler = signal_handler;

		sigaction(SIGHUP, &action, NULL);
		sigaction(SIGINT, &action, NULL);
		sigaction(SIGTERM, &action, NULL);
#  else
		signal(SIGHUP, signal_handler);
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);
#  endif

		tempFile = locateLocal("tmp","kprinter_")+TQString::number(getpid());
		filestoprint.append(tempFile);
		fromStdin = true;
		FILE	*fout = fopen(TQFile::encodeName(filestoprint[0]),"w");
		if (!fout) errormsg(i18n("Unable to open temporary file."));
		char	buffer[8192];
		int	s;

		// check for previously read data
		if ( dataread )
			fwrite( &readchar, 1, 1, fout );
		// read stdin and write to temporary file
		while ((s=fread(buffer,1,sizeof(buffer),stdin)) > 0)
			fwrite(buffer,1,s,fout);

		s = ftell(fout);
		fclose(fout);
		if (s <= 0)
		{
			showmsg(i18n("Stdin is empty, no job sent."), 2);
			TQFile::remove(filestoprint[0]);
			return;
		}
	}
	else if ( docopy )
	{
		for ( TQStringList::Iterator it=filestoprint.begin(); it!=filestoprint.end(); ++it )
		{
			TQString tmp = copyfile( *it );
			if ( tmp.isEmpty() )
			{
				errormsg( i18n( "Unable to copy file %1." ).arg( *it ) );
				return;
			}
			*it = tmp;
		}
		fromStdin = true;
	}
	else
		fromStdin = false;

	// print all files. Remove it after if printing from
	// stdin. "kprinter" shouldn't remove temp file itself,
	// otherwise the temp file might get removed before the
	// print process finishes.
	bool ok = kprinter->printFiles(filestoprint, fromStdin);

	if (!ok)
		showmsg(i18n("Error while printing files"), 2);
	// Do not show this dialog anymore. Code sould be removed definitively
	// if nobody complains.
	/*else
	{
		TQString msg = i18n("<nobr>File(s) sent to printer <b>%1</b>.</nobr>").arg(kprinter->printerName());
		showmsg(msg,0);
	}*/
}

#include "printwrapper.moc"
