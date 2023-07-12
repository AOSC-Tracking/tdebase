/*
 *   tdeprintfax - a small fax utility
 *   Copyright (C) 2001  Michael Goffioul
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "faxctrl.h"
#include "tdeprintfax.h"
#include "defcmds.h"

#include <ktextedit.h>
#include <tqfile.h>
#include <tqtextstream.h>
#include <kpushbutton.h>
#include <tqlayout.h>
#include <tqregexp.h>
#include <kprinter.h>
#include <tqsimplerichtext.h>
#include <tqpainter.h>
#include <tqpaintdevicemetrics.h>
#include <tqvaluestack.h>
#include <tqstylesheet.h>

#include <kprocess.h>
#include <tdeglobal.h>
#include <tdeconfig.h>
#include <tdelocale.h>
#include <kdialogbase.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <tdeapplication.h>
#include <twin.h>
#include <tdeemailsettings.h>
#include <kdebug.h>
#include <kstdguiitem.h>
#include <tdefiledialog.h>
#include <tdemessagebox.h>

#include <stdlib.h>
#include <stdarg.h>

#define quote(x) TDEProcess::quote(x)

/** \brief Return a string for the page size.
 *
 * @param size the page size, which is actually of type @c TQPrinter::PageSize
 * @return a pointer to a static string for the name of the page size.
 */
char const* pageSizeText(int size)
{
	switch(size)
	{
	case TQPrinter::A4:		return "a4";
	case TQPrinter::Legal:	return "legal";
	case TQPrinter::Letter:	return "letter";
	default:				return "letter";
	}
}

/** \brief Return the default page size.
 */
static TQString pageSize()
{
	TDEConfig	*conf = TDEGlobal::config();
	conf->setGroup("Fax");
	return conf->readEntry("Page", pageSizeText(TDEGlobal::locale()->pageSize()));
}

static TQString stripNumber( const TQString& s )
{
	TDEConfig *conf = TDEGlobal::config();
	conf->setGroup( "Personal" );

	// removes any non-numeric character, except ('+','*','#') (hope it's supported by faxing tools)
	TQString strip_s = s;
	strip_s.replace( TQRegExp( "[^\\d+*#]" ), "" );
	if ( strip_s.find( '+' ) != -1 && conf->readBoolEntry( "ReplaceIntChar", false ) )
		strip_s.replace( "+", conf->readEntry( "ReplaceIntCharVal" ) );
	return strip_s;
}

static TQString tagList( int n, ... )
{
	TQString t;

	va_list ap;
	va_start( ap, n );
	for ( int i=0; i<n; i++ )
	{
		TQString tag = va_arg( ap, const char* );
		tag.append( "(_(\\w|\\{[^\\}]*\\}))?" );
		if ( t.isEmpty() )
			t = tag;
		else
			t.append( "|" ).append( tag );
	}

	return t;
}

/** Process a @c %variable that has a boolean value.
 * The format is <tt>%name_{iftrue??iffalse}</tt>. Either
 * @em iftrue or @em iffalse can be empty (or both, but that would
 * be silly). For example, if the fax software uses the
 * option @c -l for low resolution, you might use <tt>%res_{??-l}</tt>,
 * and if it uses high resolution, use <tt>%res_{-h??}</tt>. As an
 * abbreviation, omit the trailing <tt>?</tt>, e.g., <tt>%res_{-h}</tt>.
 *
 * By the way, when using multiple, adjacent question marks,
 * always be careful about inadvertently using trigraphs.
 * Always escape the second and subsequent question marks, to be safe.
 * I suppose question marks are not the best string to use,
 * but I think they work well for the end-user.
 *
 * @param match the string to process
 * @param value the Boolean value to control the processing
 * @return the new value of the tag
 */
static TQString processTag( const TQString& match, bool value)
{
	TQString v;
	int p = match.find( '_' );
	if ( p != -1 && match[ p+1 ] == '{' )
	{
		// Find the ?? that separates the iftrue from the iffalse parts.
		int q = match.find( "?\?", p+2 );
		if ( q == -1 )
		{
			// No iffalse part
			if (value)
				v = match.mid( p+2 );
			// else value is false, so leave v empty.
		}
		else if ( value )
			// Extract only the iftrue part
			v = match.mid( p+2, q-p-2 );
		else
			// Extract only the iffalse part
			v = match.mid( q+2, match.length()-q-3 );
	}
	// Else the string is malformed: no _ or no { after the _
	// In either case, there isn't much the program can do,
	// so just leave the result string empty.

	return v;
}

static TQString processTag( const TQString& match, const TQString& value )
{
	TQString v;
	int p = match.find( '_' );
	if ( p != -1 )
	{
		if ( value.isEmpty() )
			v = "";
		else
		{
			if ( match[ p+1 ] == '{' )
			{
				v = match.mid( p+2, match.length()-p-3 );
				v.replace( "@@", quote( value ) );
			}
			else
				v = ( "-" + match.mid( p+1 ) + " " + quote( value ) );
		}
	}
	else
		v = quote( value );
	return v;
}

static bool isTag( const TQString& m, const TQString& t )
{
	return ( m == t || m.startsWith( t+"_" ) );
}

static TQString replaceTags( const TQString& s, const TQString& tags, KdeprintFax *fax = NULL, const KdeprintFax::FaxItem& item = KdeprintFax::FaxItem() )
{
	// unquote variables (they will be replaced with quoted values later)

	TQValueStack<bool> stack;
	TDEConfig	*conf = TDEGlobal::config();

	TQString cmd = s;

	bool issinglequote=false;
	bool isdoublequote=false;
	TQRegExp re_noquote("(\\$\\(|\\)|\\(|\"|'|\\\\|`|"+tags+")");
	TQRegExp re_singlequote("('|"+tags+")");
	TQRegExp re_doublequote("(\\$\\(|\"|\\\\|`|"+tags+")");
	for	( int i = re_noquote.search(cmd);
		i != -1;
		i = (issinglequote?re_singlequote.search(cmd,i)
			:isdoublequote?re_doublequote.search(cmd,i)
			:re_noquote.search(cmd,i))
	)
	{
		if (cmd[i]=='(') // (...)
		{
			// assert(isdoublequote == false)
			stack.push(isdoublequote);
			i++;
		}
		else if (cmd[i]=='$') // $(...)
		{
			stack.push(isdoublequote);
			isdoublequote = false;
			i+=2;
		}
		else if (cmd[i]==')') // $(...) or (...)
		{
			if (!stack.isEmpty())
				isdoublequote = stack.pop();
			else
				tqWarning("Parse error.");
			i++;
		}
		else if (cmd[i]=='\'')
		{
			issinglequote=!issinglequote;
			i++;
		}
		else if (cmd[i]=='"')
		{
			isdoublequote=!isdoublequote;
			i++;
		}
		else if (cmd[i]=='\\')
			i+=2;
		else if (cmd[i]=='`')
		{
			// Replace all `...` with safer $(...)
			cmd.replace (i, 1, "$(");
			TQRegExp re_backticks("(`|\\\\`|\\\\\\\\|\\\\\\$)");
			for (	int i2=re_backticks.search(cmd,i+2);
				i2!=-1;
				i2=re_backticks.search(cmd,i2)
			)
			{
				if (cmd[i2] == '`')
				{
					cmd.replace (i2, 1, ")");
					i2=cmd.length(); // leave loop
				}
				else
				{	// remove backslash and ignore following character
					cmd.remove (i2, 1);
					i2++;
				}
			}
			// Leave i unchanged! We need to process "$("
		}
		else
		{
			TQString match, v;

			// get match
			if (issinglequote)
				match=re_singlequote.cap();
			else if (isdoublequote)
				match=re_doublequote.cap();
			else
				match=re_noquote.cap();

			// substitute %variables
			// settings
			if ( isTag( match, "%dev" ) )
			{
				conf->setGroup("Fax");
				v = processTag( match, conf->readEntry("Device", "modem") );

			}
			else if (isTag( match, "%server" ))
			{
				conf->setGroup( "Fax" );
				v = conf->readEntry("Server");
				if (v.isEmpty())
					v = getenv("FAXSERVER");
				if (v.isEmpty())
					v = TQString::fromLatin1("localhost");
				v = processTag( match, v );
			}
			else if (isTag( match, "%page" ))
			{
				conf->setGroup( "Fax" );
				v = processTag( match, pageSize() );
			}
			else if (isTag( match, "%res" ))
			{
				conf->setGroup( "Fax" );
				v = processTag(match, conf->readEntry("Resolution", "High") == "High");
			}
			else if (isTag( match, "%user" ))
			{
				conf->setGroup("Personal");
				v = processTag(match, conf->readEntry("Name", getenv("USER")));
			}
			else if (isTag( match, "%from" ))
			{
				conf->setGroup( "Personal" );
				v = processTag(match, conf->readEntry("Number"));
			}
			else if (isTag( match, "%email" ))
			{
				KEMailSettings	e;
				v = processTag(match, e.getSetting(KEMailSettings::EmailAddress));
			}
			// arguments
			else if (isTag( match, "%number" ))
				v = processTag( match, stripNumber( item.number) );
			else if (isTag( match, "%rawnumber" ))
				v = processTag( match, item.number );
			else if (isTag( match, "%name" ))
				v = processTag(match, item.name);
			else if (isTag( match, "%comment" ))
				v = processTag(match, fax->comment());
			else if (isTag( match, "%enterprise" ))
				v = processTag(match, item.enterprise);
			else if ( isTag( match, "%time" ) )
				v = processTag( match, fax->time() );
			else if ( isTag( match, "%subject" ) )
				v = processTag( match, fax->subject() );
			else if (isTag( match, "%cover" ))
				v = processTag(match, fax->cover());

			// %variable inside of a quote?
			if (isdoublequote)
				v='"'+v+'"';
			else if (issinglequote)
				v="'"+v+"'";

			cmd.replace (i, match.length(), v);
			i+=v.length();
		}
	}

	return cmd;
}

FaxCtrl::FaxCtrl(TQWidget *parent, const char *name)
: TQObject(parent, name)
{
	m_process = new TDEProcess();
	m_process->setUseShell(true);
	connect(m_process, TQT_SIGNAL(receivedStdout(TDEProcess*,char*,int)), TQT_SLOT(slotReceivedStdout(TDEProcess*,char*,int)));
	connect(m_process, TQT_SIGNAL(receivedStderr(TDEProcess*,char*,int)), TQT_SLOT(slotReceivedStdout(TDEProcess*,char*,int)));
	connect(m_process, TQT_SIGNAL(processExited(TDEProcess*)), TQT_SLOT(slotProcessExited(TDEProcess*)));
	connect(this, TQT_SIGNAL(faxSent(bool)), TQT_SLOT(cleanTempFiles()));
	m_logview = 0;
}

FaxCtrl::~FaxCtrl()
{
	slotCloseLog();
	delete m_process;
}

bool FaxCtrl::send(KdeprintFax *f)
{
	m_command = faxCommand();
	if (m_command.isEmpty())
		return false;

	// replace tags common to all fax "operations"
	m_command = replaceTags( m_command, tagList( 11, "%dev", "%server", "%page", "%res", "%user", "%from", "%email", "%comment", "%time", "%subject", "%cover" ), f );

	m_log = TQString::null;
	m_filteredfiles.clear();
	cleanTempFiles();
	m_files = f->files();
	m_faxlist = f->faxList();

	addLogTitle( i18n( "Converting input files to PostScript" ) );
	filter();

	return true;
}

void FaxCtrl::slotReceivedStdout(TDEProcess*, char *buffer, int len)
{
	TQCString	str(buffer, len);
	kdDebug() << "Received stdout: " << str << endl;
	addLog(TQString(str));
}

void FaxCtrl::slotProcessExited(TDEProcess*)
{
	// we exited a process: if there's still entries in m_files, this was a filter
	// process, else this was the fax process
	bool	ok = (m_process->normalExit() && ((m_process->exitStatus() & (m_files.count() > 0 ? 0x1 : 0xFFFFFFFF)) == 0));
	if ( ok )
	{
		if ( m_files.count() > 0 )
		{
			// remove first element
			m_files.remove(m_files.begin());
			if (m_files.count() > 0)
				filter();
			else
				sendFax();
		}
		else if ( !m_faxlist.isEmpty() )
			sendFax();
		else
			faxSent( true );
	}
	else
	{
		emit faxSent(false);
	}
}

TQString FaxCtrl::faxCommand()
{
	TDEConfig	*conf = TDEGlobal::config();
	conf->setGroup("System");
	TQString	sys = conf->readPathEntry("System", "efax");
	TQString cmd;
	if (sys == "hylafax")
		cmd = conf->readPathEntry("HylaFax", hylafax_default_cmd);
	else if (sys == "mgetty")
		cmd = conf->readPathEntry("Mgetty", mgetty_default_cmd);
	else if ( sys == "other" )
		cmd = conf->readPathEntry( "Other", TQString::null );
	else
		cmd = conf->readPathEntry("EFax", efax_default_cmd);
	if (cmd.startsWith("%exe_"))
		cmd = defaultCommand(cmd);
	return cmd;
}

void FaxCtrl::sendFax()
{
	if ( m_command.find( "%files" ) != -1 )
	{
		// replace %files tag
		TQString	filestr;
		for (TQStringList::ConstIterator it=m_filteredfiles.begin(); it!=m_filteredfiles.end(); ++it)
			filestr += (quote(*it)+" ");
		m_command.replace("%files", filestr);
	}

	if ( !m_faxlist.isEmpty() )
	{
		KdeprintFax::FaxItem item = m_faxlist.first();
		m_faxlist.remove(m_faxlist.begin());

		addLogTitle( i18n( "Sending fax to %1 (%2)" ).arg( item.number ).arg( item.name ) );

		TQString cmd = replaceTags( m_command, tagList( 4, "%number", "%name", "%enterprise", "%rawnumber" ), NULL, item );
		m_process->clearArguments();
		*m_process << cmd;
		addLog(i18n("Sending to fax using: %1").arg(cmd));
		if (!m_process->start(TDEProcess::NotifyOnExit, TDEProcess::AllOutput))
			emit faxSent(false);
		else
			emit message(i18n("Sending fax to %1...").arg( item.number ));
	}
}

void FaxCtrl::filter()
{
	if (m_files.count() > 0)
	{
		TQString	mimeType = KMimeType::findByURL(KURL(m_files[0]), 0, true)->name();
		if (mimeType == "application/postscript" || mimeType == "image/tiff")
		{
			emit message(i18n("Skipping %1...").arg(m_files[0]));
			m_filteredfiles.prepend(m_files[0]);
			m_files.remove(m_files.begin());
			filter();
		}
		else
		{
			TQString	tmp = locateLocal("tmp","tdeprintfax_") + kapp->randomString(8);
			m_filteredfiles.prepend(tmp);
			m_tempfiles.append(tmp);
			m_process->clearArguments();
			*m_process << locate("data","tdeprintfax/anytops") << "-m" << TDEProcess::quote(locate("data","tdeprintfax/faxfilters"))
				<< TQString::fromLatin1("--mime=%1").arg(mimeType)
				<< "-p" << pageSize()
				<<  TDEProcess::quote(m_files[0]) << TDEProcess::quote(tmp);
			if (!m_process->start(TDEProcess::NotifyOnExit, TDEProcess::AllOutput))
				emit faxSent(false);
			else
				emit message(i18n("Filtering %1...").arg(m_files[0]));
		}
	}
	else
	{
		sendFax();
	}
}

bool FaxCtrl::abort()
{
	if (m_process->isRunning())
		return m_process->kill();
	else
		return false;
}

void FaxCtrl::viewLog(TQWidget *)
{
	if (!m_logview)
	{
		TQWidget	*topView = new TQWidget(0, "LogView", (WFlags)(WType_TopLevel|WStyle_DialogBorder|WDestructiveClose));
		m_logview = new KTextEdit(topView);
		m_logview->setTextFormat( TQt::LogText );
		m_logview->setWordWrap( TQTextEdit::WidgetWidth );
		m_logview->setPaper( Qt::white );
		//m_logview->setReadOnly(true);
		//m_logview->setWordWrap(TQTextEdit::NoWrap);
		TQPushButton	*m_clear = new KPushButton(KStdGuiItem::clear(), topView);
		TQPushButton	*m_close = new KPushButton(KStdGuiItem::close(), topView);
		TQPushButton *m_print = new KPushButton( KStdGuiItem::print(), topView );
		TQPushButton *m_save = new KPushButton( KStdGuiItem::saveAs(), topView );
		m_close->setDefault(true);
		connect(m_clear, TQT_SIGNAL(clicked()), TQT_SLOT(slotClearLog()));
		connect(m_close, TQT_SIGNAL(clicked()), TQT_SLOT(slotCloseLog()));
		connect(m_logview, TQT_SIGNAL(destroyed()), TQT_SLOT(slotCloseLog()));
		connect( m_print, TQT_SIGNAL( clicked() ), TQT_SLOT( slotPrintLog() ) );
		connect( m_save, TQT_SIGNAL( clicked() ), TQT_SLOT( slotSaveLog() ) );

		TQVBoxLayout	*l0 = new TQVBoxLayout(topView, 10, 10);
		l0->addWidget(m_logview);
		TQHBoxLayout	*l1 = new TQHBoxLayout(0, 0, 10);
		l0->addLayout(l1);
		l1->addStretch(1);
		l1->addWidget( m_save );
		l1->addWidget( m_print );
		l1->addWidget(m_clear);
		l1->addWidget(m_close);

		m_logview->setText(m_log);

		topView->resize(450, 350);
		topView->show();
	}
	else
	{
		KWin::activateWindow(m_logview->parentWidget()->winId());
	}
}

void FaxCtrl::addLogTitle( const TQString& s )
{
	TQString t( s );
	t.prepend( '\n' ).append( '\n' );
	addLog( t, true );
}

void FaxCtrl::addLog(const TQString& s, bool isTitle)
{
	TQString t = TQStyleSheet::escape(s);
	if ( isTitle )
		t.prepend( "<font color=red><b>" ).append( "</b></font>" );
	m_log.append( t + '\n' );
	if (m_logview)
		m_logview->append(t);
}

TQString FaxCtrl::faxSystem()
{
	TDEConfig	*conf = TDEGlobal::config();
	conf->setGroup("System");
	TQString	s = conf->readEntry("System", "efax");
	s[0] = s[0].upper();
	return s;
}

void FaxCtrl::cleanTempFiles()
{
	for (TQStringList::ConstIterator it=m_tempfiles.begin(); it!=m_tempfiles.end(); ++it)
		TQFile::remove(*it);
	m_tempfiles.clear();
}

void FaxCtrl::slotClearLog()
{
	m_log = TQString::null;
	if (m_logview)
		m_logview->clear();
}

void FaxCtrl::slotCloseLog()
{
	const TQObject	*obj = TQT_TQOBJECT_CONST(sender());
	if (m_logview)
	{
		TQTextEdit	*view = m_logview;
		m_logview = 0;
		if (obj && obj->inherits("TQPushButton"))
			delete view->parentWidget();
kdDebug() << "slotClose()" << endl;
	}
}

void FaxCtrl::slotPrintLog()
{
	if ( m_logview )
	{
		KPrinter printer;
		printer.setDocName( i18n( "Fax log" ) );
		printer.setDocFileName( "faxlog" );
		if ( printer.setup( m_logview->topLevelWidget(), i18n( "Fax Log" ) ) )
		{
			TQPainter painter( &printer );
			TQPaintDeviceMetrics metric( &printer );
			TQRect body( 0, 0, metric.width(), metric.height() ), view( body );
			//TQString txt = m_logview->text();
			TQString txt = m_log;

			txt.replace( '\n', "<br>" );
			txt.prepend( "<h2>" + i18n( "TDEPrint Fax Tool Log" ) + "</h2>" );

			kdDebug() << "Log: " << txt << endl;
			TQSimpleRichText richText( txt, m_logview->font() );

			richText.setWidth( &painter, body.width() );
			do
			{
				richText.draw( &painter, body.left(), body.top(), view, m_logview->colorGroup() );
				view.moveBy( 0, body.height() );
				painter.translate( 0, -body.height() );
				if ( view.top() >= richText.height() )
					break;
				printer.newPage();
			} while ( true );
		}
	}
}

void FaxCtrl::slotSaveLog()
{
	if ( m_logview )
	{
		TQString filename = KFileDialog::getSaveFileName( TQString::null, TQString::null, m_logview );
		if ( !filename.isEmpty() )
		{
			TQFile f( filename );
			if ( f.open( IO_WriteOnly ) )
			{
				TQTextStream t( &f );
				t << i18n( "TDEPrint Fax Tool Log" ) << endl;
				t << m_logview->text() << endl;
				f.close();
			}
			else
				KMessageBox::error( m_logview, i18n( "Cannot open file for writing." ) );
		}
	}
}

#include "faxctrl.moc"
