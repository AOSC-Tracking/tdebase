/*  This file is part of the KDE libraries
    Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>

#include <tqdir.h>
#include <tqfile.h>
#include <tqtextstream.h>
#include <tqdatastream.h>
#include <tqcstring.h>
#include <tqptrlist.h>
#include <tqmap.h>
#include <tqregexp.h>

#include <kdebug.h>
#include <kinstance.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <klocale.h>
#include <kmimetype.h>

#include "kio_man.h"
#include "kio_man.moc"
#include "man2html.h"
#include <assert.h>
#include <kfilterbase.h>
#include <kfilterdev.h>

using namespace KIO;

MANProtocol *MANProtocol::_self = 0;

#define SGML2ROFF_DIRS "/usr/lib/sgml"

/*
 * Drop trailing ".section[.gz]" from name
 */
static
void stripExtension( TQString *name )
{
    int pos = name->length();

    if ( name->find(".gz", -3) != -1 )
        pos -= 3;
    else if ( name->find(".z", -2, false) != -1 )
        pos -= 2;
    else if ( name->find(".bz2", -4) != -1 )
        pos -= 4;
    else if ( name->find(".bz", -3) != -1 )
        pos -= 3;

    if ( pos > 0 )
        pos = name->findRev('.', pos-1);

    if ( pos > 0 )
        name->truncate( pos );
}

static
bool parseUrl(const TQString& _url, TQString &title, TQString &section)
{
    section = TQString::null;

    TQString url = _url;
    if (url.at(0) == '/') {
        if (KStandardDirs::exists(url)) {
            title = url;
            return true;
        } else
        {
            // If the directory does not exist, then it is perhaps a normal man page
            kdDebug(7107) << url << " does not exist" << endl;
        }
    }

    while (url.at(0) == '/')
        url.remove(0,1);

    title = url;

    int pos = url.find('(');
    if (pos < 0)
        return true;

    title = title.left(pos);

    section = url.mid(pos+1);
    section = section.left(section.length()-1);

    return true;
}


MANProtocol::MANProtocol(const TQCString &pool_socket, const TQCString &app_socket)
    : TQObject(), SlaveBase("man", pool_socket, app_socket)
{
    assert(!_self);
    _self = this;
    const TQString common_dir = KGlobal::dirs()->findResourceDir( "html", "en/common/kde-common.css" );
    const TQString strPath=TQString( "file:%1/en/common" ).arg( common_dir );
    m_htmlPath=strPath.local8Bit(); // ### TODO encode for HTML
    m_cssPath=strPath.local8Bit(); // ### TODO encode for CSS
    section_names << "1" << "2" << "3" << "3n" << "3p" << "4" << "5" << "6" << "7"
                  << "8" << "9" << "l" << "n";
    m_manCSSFile = locate( "data", "kio_man/kio_man.css" );
}

MANProtocol *MANProtocol::self() { return _self; }

MANProtocol::~MANProtocol()
{
    _self = 0;
}

void MANProtocol::parseWhatIs( TQMap<TQString, TQString> &i, TQTextStream &t, const TQString &mark )
{
    TQRegExp re( mark );
    TQString l;
    while ( !t.atEnd() )
    {
	l = t.readLine();
	int pos = re.search( l );
	if (pos != -1)
	{
	    TQString names = l.left(pos);
	    TQString descr = l.mid(pos + re.matchedLength());
	    while ((pos = names.find(",")) != -1)
	    {
		i[names.left(pos++)] = descr;
		while (names[pos] == ' ')
		    pos++;
		names = names.mid(pos);
	    }
	    i[names] = descr;
	}
    }
}

bool MANProtocol::addWhatIs(TQMap<TQString, TQString> &i, const TQString &name, const TQString &mark)
{
    TQFile f(name);
    if (!f.open(IO_ReadOnly))
        return false;
    TQTextStream t(&f);
    parseWhatIs( i, t, mark );
    return true;
}

TQMap<TQString, TQString> MANProtocol::buildIndexMap(const TQString &section)
{
    TQMap<TQString, TQString> i;
    TQStringList man_dirs = manDirectories();
    // Supplementary places for whatis databases
    man_dirs += m_mandbpath;
    if (man_dirs.find("/var/cache/man")==man_dirs.end())
        man_dirs << "/var/cache/man";
    if (man_dirs.find("/var/catman")==man_dirs.end())
        man_dirs << "/var/catman";
    
    TQStringList names;
    names << "whatis.db" << "whatis";
    TQString mark = "\\s+\\(" + section + "[a-z]*\\)\\s+-\\s+";

    for ( TQStringList::ConstIterator it_dir = man_dirs.begin();
          it_dir != man_dirs.end();
          ++it_dir )
    {
        if ( TQFile::exists( *it_dir ) ) {
    	    TQStringList::ConstIterator it_name;
            for ( it_name = names.begin();
	          it_name != names.end();
	          it_name++ )
            {
	        if (addWhatIs(i, (*it_dir) + "/" + (*it_name), mark))
		    break;
	    }
            if ( it_name == names.end() ) {
                KProcess proc;
                proc << "whatis" << "-M" << (*it_dir) << "-w" << "*";
                myStdStream = TQString::null;
                connect( &proc, TQT_SIGNAL( receivedStdout(KProcess *, char *, int ) ),
                         TQT_SLOT( slotGetStdOutput( KProcess *, char *, int ) ) );
                proc.start( KProcess::Block, KProcess::Stdout );
                TQTextStream t( &myStdStream, IO_ReadOnly );
                parseWhatIs( i, t, mark );
            }
        }
    }
    return i;
}

TQStringList MANProtocol::manDirectories()
{
    checkManPaths();
    //
    // Build a list of man directories including translations
    //
    TQStringList man_dirs;

    for ( TQStringList::ConstIterator it_dir = m_manpath.begin();
          it_dir != m_manpath.end();
          it_dir++ )
    {
        // Translated pages in "<mandir>/<lang>" if the directory
        // exists
        TQStringList languages = KGlobal::locale()->languageList();

        for (TQStringList::ConstIterator it_lang = languages.begin();
             it_lang != languages.end();
             it_lang++ )
        {
            if ( !(*it_lang).isEmpty() && (*it_lang) != TQString("C") ) {
                TQString dir = (*it_dir) + '/' + (*it_lang);

                struct stat sbuf;

                if ( ::stat( TQFile::encodeName( dir ), &sbuf ) == 0
                    && S_ISDIR( sbuf.st_mode ) )
                {
                    const TQString p = TQDir(dir).canonicalPath();
                    if (!man_dirs.contains(p)) man_dirs += p;	
                }
            }
        }

        // Untranslated pages in "<mandir>"
        const TQString p = TQDir(*it_dir).canonicalPath();
        if (!man_dirs.contains(p)) man_dirs += p;
    }
    return man_dirs;
}

TQStringList MANProtocol::findPages(const TQString &_section,
                                   const TQString &title,
                                   bool full_path)
{
    TQString section = _section;

    TQStringList list;

    // kdDebug() << "findPages '" << section << "' '" << title << "'\n";
    if (title.at(0) == '/') {
       list.append(title);
       return list;
    }

    const TQString star( "*" );

    //
    // Find man sections in this directory
    //
    TQStringList sect_list;
    if ( section.isEmpty() )
        section = star;

    if ( section != star )
    {
        //
        // Section given as argument
        //
        sect_list += section;
        while (section.at(section.length() - 1).isLetter())  {
            section.truncate(section.length() - 1);
            sect_list += section;
        }
    } else {
        sect_list += section;
    }

    TQStringList man_dirs = manDirectories();

    //
    // Find man pages in the sections listed above
    //
    for ( TQStringList::ConstIterator it_sect = sect_list.begin();
          it_sect != sect_list.end();
          it_sect++ )
    {
	    TQString it_real = (*it_sect).lower();
        //
        // Find pages
        //
        for ( TQStringList::ConstIterator it_dir = man_dirs.begin();
              it_dir != man_dirs.end();
              it_dir++ )
        {
            TQString man_dir = (*it_dir);

            //
            // Sections = all sub directories "man*" and "sman*"
            //
            DIR *dp = ::opendir( TQFile::encodeName( man_dir ) );

            if ( !dp )
                continue;

            struct dirent *ep;

            const TQString man = TQString("man");
            const TQString sman = TQString("sman");

            while ( (ep = ::readdir( dp )) != 0L ) {
                const TQString file = TQFile::decodeName( ep->d_name );
                TQString sect = TQString::null;

                if ( file.startsWith( man ) )
                    sect = file.mid(3);
                else if (file.startsWith(sman))
                    sect = file.mid(4);

		if (sect.lower()==it_real) it_real = sect;

                // Only add sect if not already contained, avoid duplicates
                if (!sect_list.contains(sect) && _section.isEmpty())  {
                    kdDebug() << "another section " << sect << endl;
                    sect_list += sect;
                }
            }

            ::closedir( dp );

            if ( *it_sect != star ) { // in that case we only look around for sections
                const TQString dir = man_dir + TQString("/man") + (it_real) + '/';
                const TQString sdir = man_dir + TQString("/sman") + (it_real) + '/';

                findManPagesInSection(dir, title, full_path, list);
                findManPagesInSection(sdir, title, full_path, list);
            }
        }
    }

//    kdDebug(7107) << "finished " << list << " " << sect_list << endl;

    return list;
}

void MANProtocol::findManPagesInSection(const TQString &dir, const TQString &title, bool full_path, TQStringList &list)
{
    kdDebug() << "findManPagesInSection " << dir << " " << title << endl;
    bool title_given = !title.isEmpty();

    DIR *dp = ::opendir( TQFile::encodeName( dir ) );

    if ( !dp )
        return;

    struct dirent *ep;

    while ( (ep = ::readdir( dp )) != 0L ) {
        if ( ep->d_name[0] != '.' ) {

            TQString name = TQFile::decodeName( ep->d_name );

            // check title if we're looking for a specific page
            if ( title_given ) {
                if ( !name.startsWith( title ) ) {
                    continue;
                }
                else {
                    // beginning matches, do a more thorough check...
                    TQString tmp_name = name;
                    stripExtension( &tmp_name );
                    if ( tmp_name != title )
                        continue;
                }
            }

            if ( full_path )
                name.prepend( dir );

            list += name ;
        }
    }
    ::closedir( dp );
}

void MANProtocol::output(const char *insert)
{
    if (insert)
    {
        m_outputBuffer.writeBlock(insert,strlen(insert));
    }
    if (!insert || m_outputBuffer.at() >= 2048)
    {
        m_outputBuffer.close();
        data(m_outputBuffer.buffer());
        m_outputBuffer.setBuffer(TQByteArray());
        m_outputBuffer.open(IO_WriteOnly);
    }
}

// called by man2html
char *read_man_page(const char *filename)
{
    return MANProtocol::self()->readManPage(filename);
}

// called by man2html
void output_real(const char *insert)
{
    MANProtocol::self()->output(insert);
}

static TQString text2html(const TQString& txt)
{
    TQString reply = txt;

    reply = reply.replace('&', "&amp;");
    reply = reply.replace('<', "&lt;");
    reply = reply.replace('>', "&gt;");
    reply = reply.replace('"', "&dquot;");
    reply = reply.replace('\'', "&quot;");
    return reply;
}

void MANProtocol::get(const KURL& url )
{
    kdDebug(7107) << "GET " << url.url() << endl;

    TQString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        showMainIndex();
        return;
    }

    // see if an index was requested
    if (url.query().isEmpty() && (title.isEmpty() || title == "/" || title == "."))
    {
        if (section == "index" || section.isEmpty())
            showMainIndex();
        else
            showIndex(section);
        return;
    }

    // tell the mimetype
    mimeType("text/html");

    const TQStringList foundPages=findPages(section, title);
    bool pageFound=true;
    if (foundPages.isEmpty())
    {
       outputError(i18n("No man page matching to %1 found.<br><br>"
           "Check that you have not mistyped the name of the page that you want.\n"
           "Be careful that you must take care about upper case and lower case characters!<br>"
           "If everything looks correct, then perhaps you need to set a better search path "
           "for man pages, be it by the environment variable MANPATH or a matching file "
           "in the directory /etc .").arg(text2html(title)));
       pageFound=false;
    }
    else if (foundPages.count()>1)
    {
       pageFound=false;
       //check for the case that there is foo.1 and foo.1.gz found:
       // ### TODO make it more generic (other extensions)
       if ((foundPages.count()==2) &&
           (((foundPages[0]+".gz") == foundPages[1]) ||
            (foundPages[0] == (foundPages[1]+".gz"))))
          pageFound=true;
       else
          outputMatchingPages(foundPages);
    }
    //yes, we found exactly one man page

    if (pageFound)
    {
       setResourcePath(m_htmlPath,m_cssPath);
       m_outputBuffer.open(IO_WriteOnly);
       const TQCString filename=TQFile::encodeName(foundPages[0]);
       char *buf = readManPage(filename);

       if (!buf)
       {
          outputError(i18n("Open of %1 failed.").arg(title));
          finished();
          return;
       }
       // will call output_real
       scan_man_page(buf);
       delete [] buf;

       output(0); // flush

       m_outputBuffer.close();
       data(m_outputBuffer.buffer());
       m_outputBuffer.setBuffer(TQByteArray());
       // tell we are done
       data(TQByteArray());
    }
    finished();
}

void MANProtocol::slotGetStdOutput(KProcess* /* p */, char *s, int len)
{
  myStdStream += TQString::fromLocal8Bit(s, len);
}

void MANProtocol::slotGetStdOutputUtf8(KProcess* /* p */, char *s, int len)
{
  myStdStream += TQString::fromUtf8(s, len);
}

char *MANProtocol::readManPage(const char *_filename)
{
    TQCString filename = _filename;

    char *buf = NULL;

    /* Determine type of man page file by checking its path. Determination by
     * MIME type with KMimeType doesn't work reliablely. E.g., Solaris 7:
     * /usr/man/sman7fs/pcfs.7fs -> text/x-csrc : WRONG
     * If the path name constains the string sman, assume that it's SGML and
     * convert it to roff format (used on Solaris). */
    //TQString file_mimetype = KMimeType::findByPath(TQString(filename), 0, false)->name();
    if (filename.contains("sman", false)) //file_mimetype == "text/html" || )
    {
        myStdStream =TQString::null;
	KProcess proc;

	/* Determine path to sgml2roff, if not already done. */
	getProgramPath();
	proc << mySgml2RoffPath << filename;

	TQApplication::connect(&proc, TQT_SIGNAL(receivedStdout (KProcess *, char *, int)),
			      this, TQT_SLOT(slotGetStdOutput(KProcess *, char *, int)));
	proc.start(KProcess::Block, KProcess::All);

        const TQCString cstr=myStdStream.latin1();
        const int len = cstr.size()-1;
        buf = new char[len + 4];
        tqmemmove(buf + 1, cstr.data(), len);
        buf[0]=buf[len]='\n'; // Start and end with a end of line
        buf[len+1]=buf[len+2]='\0'; // Two additional NUL characters at end
    }
    else
    {
        if (TQDir::isRelativePath(filename)) {
            kdDebug(7107) << "relative " << filename << endl;
            filename = TQDir::cleanDirPath(lastdir + "/" + filename).utf8();
            if (!KStandardDirs::exists(filename)) { // exists perhaps with suffix
                lastdir = filename.left(filename.findRev('/'));
                TQDir mandir(lastdir);
                mandir.setNameFilter(filename.mid(filename.findRev('/') + 1) + ".*");
                filename = lastdir + "/" + TQFile::encodeName(mandir.entryList().first());
            }
            kdDebug(7107) << "resolved to " << filename << endl;
        }
        lastdir = filename.left(filename.findRev('/'));
    
        myStdStream = TQString::null;
        KProcess proc;
        /* TODO: detect availability of 'man --recode' so that this can go
         * upstream */
        proc << "man" << "--recode" << "UTF-8" << filename;

        TQApplication::connect(&proc, TQT_SIGNAL(receivedStdout (KProcess *, char *, int)),
                              this, TQT_SLOT(slotGetStdOutputUtf8(KProcess *, char *, int)));
        proc.start(KProcess::Block, KProcess::All);

        const TQCString cstr=myStdStream.utf8();
        const int len = cstr.size()-1;
        buf = new char[len + 4];
        tqmemmove(buf + 1, cstr.data(), len);
        buf[0]=buf[len]='\n'; // Start and end with a end of line
        buf[len+1]=buf[len+2]='\0'; // Two NUL characters at end
    }
    return buf;
}


void MANProtocol::outputError(const TQString& errmsg)
{
    TQByteArray array;
    TQTextStream os(array, IO_WriteOnly);
    os.setEncoding(TQTextStream::UnicodeUTF8);

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << i18n("Man output") << "</title>\n" << endl;
    if ( !m_manCSSFile.isEmpty() )
        os << "<link href=\"file:///" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" << endl;
    os << i18n("<body><h1>KDE Man Viewer Error</h1>") << errmsg << "</body>" << endl;
    os << "</html>" << endl;

    data(array);
}

void MANProtocol::outputMatchingPages(const TQStringList &matchingPages)
{
    TQByteArray array;
    TQTextStream os(array, IO_WriteOnly);
    os.setEncoding(TQTextStream::UnicodeUTF8);

    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html>\n<head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"<<endl;
    os << "<title>" << i18n("Man output") <<"</title>" << endl;
    if ( !m_manCSSFile.isEmpty() )
        os << "<link href=\"file:///" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" <<endl;
    os << "<body><h1>" << i18n("There is more than one matching man page.");
    os << "</h1>\n<ul>\n";
    
    int acckey=1;
    for (TQStringList::ConstIterator it = matchingPages.begin(); it != matchingPages.end(); ++it)
    {
      os<<"<li><a href='man:"<<(*it)<<"' accesskey='"<< acckey <<"'>"<< *it <<"</a><br>\n<br>\n";
      acckey++;
    }
    os << "</ul>\n";
    os << "<hr>\n";
    os << "<p>" << i18n("Note: if you read a man page in your language,"
       " be aware it can contain some mistakes or be obsolete."
       " In case of doubt, you should have a look at the English version.") << "</p>";

    os << "</body>\n</html>"<<endl;

    data(array);
    finished();
}

void MANProtocol::stat( const KURL& url)
{
    kdDebug(7107) << "ENTERING STAT " << url.url() << endl;

    TQString title, section;

    if (!parseUrl(url.path(), title, section))
    {
        error(KIO::ERR_MALFORMED_URL, url.url());
        return;
    }

    kdDebug(7107) << "URL " << url.url() << " parsed to title='" << title << "' section=" << section << endl;

    UDSEntry entry;
    UDSAtom atom;

    atom.m_uds = UDS_NAME;
    atom.m_long = 0;
    atom.m_str = title;
    entry.append(atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFREG;
    entry.append(atom);

    atom.m_uds = UDS_URL;
    atom.m_long = 0;
    TQString newUrl = "man:"+title;
    if (!section.isEmpty())
        newUrl += TQString("(%1)").arg(section);
    atom.m_str = newUrl;
    entry.append(atom);

    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long = 0;
    atom.m_str = "text/html";
    entry.append(atom);

    statEntry(entry);

    finished();
}


extern "C"
{

    int KDE_EXPORT kdemain( int argc, char **argv ) {

        KInstance instance("kio_man");

        kdDebug(7107) <<  "STARTING " << getpid() << endl;

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_man protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        MANProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        kdDebug(7107) << "Done" << endl;

        return 0;
    }

}

void MANProtocol::mimetype(const KURL & /*url*/)
{
    mimeType("text/html");
    finished();
}

static TQString sectionName(const TQString& section)
{
    if (section == "1")
        return i18n("User Commands");
    else if (section == "2")
        return i18n("System Calls");
    else if (section == "3")
        return i18n("Subroutines");
    else if (section == "3p")
    	return i18n("Perl Modules");
    else if (section == "3n")
    	return i18n("Network Functions");
    else if (section == "4")
        return i18n("Devices");
    else if (section == "5")
        return i18n("File Formats");
    else if (section == "6")
        return i18n("Games");
    else if (section == "7")
        return i18n("Miscellaneous");
    else if (section == "8")
        return i18n("System Administration");
    else if (section == "9")
        return i18n("Kernel");
    else if (section == "l")
    	return i18n("Local Documentation");
    else if (section == "n")
        return i18n("New");

    return TQString::null;
}

TQStringList MANProtocol::buildSectionList(const TQStringList& dirs) const
{
    TQStringList l;

    for (TQStringList::ConstIterator it = section_names.begin();
	    it != section_names.end(); ++it)
    {
	    for (TQStringList::ConstIterator dir = dirs.begin();
		    dir != dirs.end(); ++dir)
	    {
		TQDir d((*dir)+"/man"+(*it));
		if (d.exists())
		{
		    l << *it;
		    break;
		}
	    }
    }
    return l;
}

void MANProtocol::showMainIndex()
{
    TQByteArray array;
    TQTextStream os(array, IO_WriteOnly);
    os.setEncoding(TQTextStream::UnicodeUTF8);

    // print header
    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << i18n("UNIX Manual Index") << "</title>" << endl;
    if (!m_manCSSFile.isEmpty())
        os << "<link href=\"file:///" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" << endl;
    os << "<body><h1>" << i18n("UNIX Manual Index") << "</h1>" << endl;

    // ### TODO: why still the environment variable
    const TQString sectList = getenv("MANSECT");
    TQStringList sections;
    if (sectList.isEmpty())
    	sections = buildSectionList(manDirectories());
    else
	sections = TQStringList::split(':', sectList);

    os << "<table>" << endl;

    TQStringList::ConstIterator it;
    for (it = sections.begin(); it != sections.end(); ++it)
        os << "<tr><td><a href=\"man:(" << *it << ")\" accesskey=\"" << 
	(((*it).length()==1)?(*it):(*it).right(1))<<"\">" << i18n("Section ") 
	<< *it << "</a></td><td>&nbsp;</td><td> " << sectionName(*it) << "</td></tr>" << endl;

    os << "</table>" << endl;

    // print footer
    os << "</body></html>" << endl;

    data(array);
    finished();
}

void MANProtocol::constructPath(TQStringList& constr_path, TQStringList constr_catmanpath)
{
    TQMap<TQString, TQString> manpath_map;
    TQMap<TQString, TQString> mandb_map;

    // Add paths from /etc/man.conf
    //
    // Explicit manpaths may be given by lines starting with "MANPATH" or
    // "MANDATORY_MANPATH" (depending on system ?).
    // Mappings from $PATH to manpath are given by lines starting with
    // "MANPATH_MAP"

    TQRegExp manpath_regex( "^MANPATH\\s" );
    TQRegExp mandatory_regex( "^MANDATORY_MANPATH\\s" );
    TQRegExp manpath_map_regex( "^MANPATH_MAP\\s" );
    TQRegExp mandb_map_regex( "^MANDB_MAP\\s" );
    //TQRegExp section_regex( "^SECTION\\s" );
    TQRegExp space_regex( "\\s+" ); // for parsing manpath map

    TQFile mc("/etc/man.conf");             // Caldera
    if (!mc.exists())
        mc.setName("/etc/manpath.config"); // SuSE, Debian
    if (!mc.exists())
        mc.setName("/etc/man.config");  // Mandrake

    if (mc.open(IO_ReadOnly))
    {
        TQTextStream is(&mc);
        is.setEncoding(TQTextStream::Locale);
    
        while (!is.eof())
        {
            const TQString line = is.readLine();
            if ( manpath_regex.search(line, 0) == 0 )
            {
                const TQString path = line.mid(8).stripWhiteSpace();
                constr_path += path;
            }
            else if ( mandatory_regex.search(line, 0) == 0 )
            {
                const TQString path = line.mid(18).stripWhiteSpace();
                constr_path += path;
            }
            else if ( manpath_map_regex.search(line, 0) == 0 )
            {
                        // The entry is "MANPATH_MAP  <path>  <manpath>"
                const TQStringList mapping =
                        TQStringList::split(space_regex, line);
    
                if ( mapping.count() == 3 )
                {
                    const TQString dir = TQDir::cleanDirPath( mapping[1] );
                    const TQString mandir = TQDir::cleanDirPath( mapping[2] );
    
                    manpath_map[ dir ] = mandir;
                }
            }
            else if ( mandb_map_regex.search(line, 0) == 0 )
            {
                        // The entry is "MANDB_MAP  <manpath>  <catmanpath>"
                const TQStringList mapping =
                        TQStringList::split(space_regex, line);
    
                if ( mapping.count() == 3 )
                {
                    const TQString mandir = TQDir::cleanDirPath( mapping[1] );
                    const TQString catmandir = TQDir::cleanDirPath( mapping[2] );
    
                    mandb_map[ mandir ] = catmandir;
                }
            }
    /* sections are not used
            else if ( section_regex.find(line, 0) == 0 )
            {
            if ( !conf_section.isEmpty() )
            conf_section += ':';
            conf_section += line.mid(8).stripWhiteSpace();
        }
    */
        }
        mc.close();
    }

    // Default paths
    static const char *manpaths[] = {
        "/usr/X11/man",
        "/usr/X11R6/man",
        "/usr/man",
        "/usr/local/man",
        "/usr/exp/man",
        "/usr/openwin/man",
        "/usr/dt/man",
        "/opt/freetool/man",
        "/opt/local/man",
        "/usr/tex/man",
        "/usr/www/man",
        "/usr/lang/man",
        "/usr/gnu/man",
        "/usr/share/man",
        "/usr/motif/man",
        "/usr/titools/man",
        "/usr/sunpc/man",
        "/usr/ncd/man",
        "/usr/newsprint/man",
        NULL };


    int i = 0;
    while (manpaths[i]) {
        if ( constr_path.findIndex( TQString( manpaths[i] ) ) == -1 )
            constr_path += TQString( manpaths[i] );
        i++;
    }

    // Directories in $PATH
    // - if a manpath mapping exists, use that mapping
    // - if a directory "<path>/man" or "<path>/../man" exists, add it
    //   to the man path (the actual existence check is done further down)

    if ( ::getenv("PATH") ) {
        const TQStringList path =
                TQStringList::split( ":",
                                    TQString::fromLocal8Bit( ::getenv("PATH") ) );

        for ( TQStringList::const_iterator it = path.begin();
              it != path.end();
              ++it )
        {
            const TQString dir = TQDir::cleanDirPath( *it );
            TQString mandir = manpath_map[ dir ];

            if ( !mandir.isEmpty() ) {
                                // a path mapping exists
                if ( constr_path.findIndex( mandir ) == -1 )
                    constr_path += mandir;
            }
            else {
                                // no manpath mapping, use "<path>/man" and "<path>/../man"

                mandir = dir + TQString( "/man" );
                if ( constr_path.findIndex( mandir ) == -1 )
                    constr_path += mandir;

                int pos = dir.findRev( '/' );
                if ( pos > 0 ) {
                    mandir = dir.left( pos ) + TQString("/man");
                    if ( constr_path.findIndex( mandir ) == -1 )
                        constr_path += mandir;
                }
            }
            TQString catmandir = mandb_map[ mandir ];
            if ( !mandir.isEmpty() )
            {
                if ( constr_catmanpath.findIndex( catmandir ) == -1 )
                    constr_catmanpath += catmandir;
            }
            else
            {
            // What is the default mapping?
                catmandir = mandir;
                catmandir.replace("/usr/share/","/var/cache/");
                if ( constr_catmanpath.findIndex( catmandir ) == -1 )
                    constr_catmanpath += catmandir;
            }
        }
    }
}

void MANProtocol::checkManPaths()
{
    static bool inited = false;

    if (inited)
        return;

    inited = true;

    const TQString manpath_env = TQString::fromLocal8Bit( ::getenv("MANPATH") );
    //TQString mansect_env = TQString::fromLocal8Bit( ::getenv("MANSECT") );

    // Decide if $MANPATH is enough on its own or if it should be merged
    // with the constructed path.
    // A $MANPATH starting or ending with ":", or containing "::",
    // should be merged with the constructed path.

    bool construct_path = false;

    if ( manpath_env.isEmpty()
        || manpath_env[0] == ':'
        || manpath_env[manpath_env.length()-1] == ':'
        || manpath_env.contains( "::" ) )
    {
        construct_path = true; // need to read config file
    }

    // Constucted man path -- consists of paths from
    //   /etc/man.conf
    //   default dirs
    //   $PATH
    TQStringList constr_path;
    TQStringList constr_catmanpath; // catmanpath

    TQString conf_section;

    if ( construct_path )
    {
        constructPath(constr_path, constr_catmanpath);
    }

    m_mandbpath=constr_catmanpath;
    
    // Merge $MANPATH with the constructed path to form the
    // actual manpath.
    //
    // The merging syntax with ":" and "::" in $MANPATH will be
    // satisfied if any empty string in path_list_env (there
    // should be 1 or 0) is replaced by the constructed path.

    const TQStringList path_list_env = TQStringList::split( ':', manpath_env , true );

    for ( TQStringList::const_iterator it = path_list_env.begin();
          it != path_list_env.end();
          ++it )
    {
        struct stat sbuf;

        TQString dir = (*it);

        if ( !dir.isEmpty() ) {
            // Add dir to the man path if it exists
            if ( m_manpath.findIndex( dir ) == -1 ) {
                if ( ::stat( TQFile::encodeName( dir ), &sbuf ) == 0
                    && S_ISDIR( sbuf.st_mode ) )
                {
                    m_manpath += dir;
                }
            }
        }
        else {
            // Insert constructed path ($MANPATH was empty, or
            // there was a ":" at an end or "::")

            for ( TQStringList::Iterator it2 = constr_path.begin();
                  it2 != constr_path.end();
                  it2++ )
            {
                dir = (*it2);

                if ( !dir.isEmpty() ) {
                    if ( m_manpath.findIndex( dir ) == -1 ) {
                        if ( ::stat( TQFile::encodeName( dir ), &sbuf ) == 0
                            && S_ISDIR( sbuf.st_mode ) )
                        {
                            m_manpath += dir;
                        }
                    }
                }
            }
        }
    }

/* sections are not used
    // Sections
    TQStringList m_mansect = TQStringList::split( ':', mansect_env, true );

    const char* default_sect[] =
        { "1", "2", "3", "4", "5", "6", "7", "8", "9", "n", 0L };

    for ( int i = 0; default_sect[i] != 0L; i++ )
        if ( m_mansect.findIndex( TQString( default_sect[i] ) ) == -1 )
            m_mansect += TQString( default_sect[i] );
*/

}


//#define _USE_OLD_CODE

#ifdef _USE_OLD_CODE
#warning "using old code"
#else

// Define this, if you want to compile with qsort from stdlib.h
// else the Qt Heapsort will be used.
// Note, qsort seems to be a bit faster (~10%) on a large man section
// eg. man section 3
#define _USE_QSORT

// Setup my own structure, with char pointers.
// from now on only pointers are copied, no strings
//
// containing the whole path string,
// the beginning of the man page name
// and the length of the name
struct man_index_t {
    char *manpath;  // the full path including man file
    const char *manpage_begin;  // pointer to the begin of the man file name in the path
    int manpage_len; // len of the man file name
};
typedef man_index_t *man_index_ptr;

#ifdef _USE_QSORT
int compare_man_index(const void *s1, const void *s2)
{
    struct man_index_t *m1 = *(struct man_index_t **)s1;
    struct man_index_t *m2 = *(struct man_index_t **)s2;
    int i;
    // Compare the names of the pages
    // with the shorter length.
    // Man page names are not '\0' terminated, so
    // this is a bit tricky
    if ( m1->manpage_len > m2->manpage_len)
    {
	i = qstrnicmp( m1->manpage_begin,
		      m2->manpage_begin,
		      m2->manpage_len);
	if (!i)
	    return 1;
	return i;
    }

    if ( m1->manpage_len < m2->manpage_len)
    {
	i = qstrnicmp( m1->manpage_begin,
		      m2->manpage_begin,
		      m1->manpage_len);
	if (!i)
	    return -1;
	return i;
    }

    return qstrnicmp( m1->manpage_begin,
		     m2->manpage_begin,
		     m1->manpage_len);
}

#else /* !_USE_QSORT */
#warning using heapsort
// Set up my own man page list,
// with a special compare function to sort itself
typedef TQPtrList<struct man_index_t> QManIndexListBase;
typedef TQPtrListIterator<struct man_index_t> QManIndexListIterator;

class QManIndexList : public QManIndexListBase
{
public:
private:
    int compareItems( TQPtrCollection::Item s1, TQPtrCollection::Item s2 )
	{
	    struct man_index_t *m1 = (struct man_index_t *)s1;
	    struct man_index_t *m2 = (struct man_index_t *)s2;
	    int i;
	    // compare the names of the pages
	    // with the shorter length
	    if (m1->manpage_len > m2->manpage_len)
	    {
		i = qstrnicmp(m1->manpage_begin,
			     m2->manpage_begin,
			     m2->manpage_len);
		if (!i)
		    return 1;
		return i;
	    }

	    if (m1->manpage_len > m2->manpage_len)
	    {

		i = qstrnicmp(m1->manpage_begin,
			     m2->manpage_begin,
			     m1->manpage_len);
		if (!i)
		    return -1;
		return i;
	    }

	    return qstrnicmp(m1->manpage_begin,
			    m2->manpage_begin,
			    m1->manpage_len);
	}
};

#endif /* !_USE_QSORT */
#endif /* !_USE_OLD_CODE */




void MANProtocol::showIndex(const TQString& section)
{
    TQByteArray array;
    TQTextStream os(array, IO_WriteOnly);
    os.setEncoding(TQTextStream::UnicodeUTF8);

    // print header
    os << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Strict//EN\">" << endl;
    os << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">" << endl;
    os << "<title>" << i18n("UNIX Manual Index") << "</title>" << endl;
    if ( !m_manCSSFile.isEmpty() )
        os << "<link href=\"file:///" << m_manCSSFile << "\" type=\"text/css\" rel=\"stylesheet\">" << endl;
    os << "</head>" << endl;
    os << "<body><div class=\"secidxmain\">" << endl;
    os << "<h1>" << i18n( "Index for Section %1: %2").arg(section).arg(sectionName(section)) << "</h1>" << endl;

    // compose list of search paths -------------------------------------------------------------

    checkManPaths();
    infoMessage(i18n("Generating Index"));

    // search for the man pages
	TQStringList pages = findPages( section, TQString::null );

	TQMap<TQString, TQString> indexmap = buildIndexMap(section);

    // print out the list
    os << "<table>" << endl;

#ifdef _USE_OLD_CODE
    pages.sort();

    TQMap<TQString, TQString> pagemap;

    TQStringList::ConstIterator page;
    for (page = pages.begin(); page != pages.end(); ++page)
    {
        TQString fileName = *page;

        stripExtension( &fileName );

        pos = fileName.findRev('/');
        if (pos > 0)
            fileName = fileName.mid(pos+1);

        if (!fileName.isEmpty())
            pagemap[fileName] = *page;

    }

    for (TQMap<TQString,TQString>::ConstIterator it = pagemap.begin();
	 it != pagemap.end(); ++it)
    {
	os << "<tr><td><a href=\"man:" << it.data() << "\">\n"
	   << it.key() << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(it.key()) ? indexmap[it.key()] : "" )
	   << "</td></tr>"  << endl;
    }

#else /* ! _USE_OLD_CODE */

#ifdef _USE_QSORT

    int listlen = pages.count();
    man_index_ptr *indexlist = new man_index_ptr[listlen];
    listlen = 0;

#else /* !_USE_QSORT */

    QManIndexList manpages;
    manpages.setAutoDelete(TRUE);

#endif /* _USE_QSORT */

    TQStringList::const_iterator page;
    for (page = pages.begin(); page != pages.end(); ++page)
    {
	// I look for the beginning of the man page name
	// i.e. "bla/pagename.3.gz" by looking for the last "/"
	// Then look for the end of the name by searching backwards
	// for the last ".", not counting zip extensions.
	// If the len of the name is >0,
	// store it in the list structure, to be sorted later

        char *manpage_end;
        struct man_index_t *manindex = new man_index_t;
	manindex->manpath = strdup((*page).utf8());

	manindex->manpage_begin = strrchr(manindex->manpath, '/');
	if (manindex->manpage_begin)
	{
	    manindex->manpage_begin++;
	    assert(manindex->manpage_begin >= manindex->manpath);
	}
	else
	{
	    manindex->manpage_begin = manindex->manpath;
	    assert(manindex->manpage_begin >= manindex->manpath);
	}

	// Skip extension ".section[.gz]"

	char *begin = (char*)(manindex->manpage_begin);
	int len = strlen( begin );
	char *end = begin+(len-1);

	if ( len >= 3 && strcmp( end-2, ".gz" ) == 0 )
	    end -= 3;
	else if ( len >= 2 && strcmp( end-1, ".Z" ) == 0 )
	    end -= 2;
	else if ( len >= 2 && strcmp( end-1, ".z" ) == 0 )
	    end -= 2;
	else if ( len >= 4 && strcmp( end-3, ".bz2" ) == 0 )
	    end -= 4;

	while ( end >= begin && *end != '.' )
	    end--;

	if ( end < begin )
	    manpage_end = 0;
	else
	    manpage_end = end;

	if (NULL == manpage_end)
	{
	    // no '.' ending ???
	    // set the pointer past the end of the filename
	    manindex->manpage_len = (*page).length();
	    manindex->manpage_len -= (manindex->manpage_begin - manindex->manpath);
	    assert(manindex->manpage_len >= 0);
	}
	else
	{
	    manindex->manpage_len = (manpage_end - manindex->manpage_begin);
	    assert(manindex->manpage_len >= 0);
	}

	if (0 < manindex->manpage_len)
	{

#ifdef _USE_QSORT

	    indexlist[listlen] = manindex;
	    listlen++;

#else /* !_USE_QSORT */

	    manpages.append(manindex);

#endif /* _USE_QSORT */

	}
    }

    //
    // Now do the sorting on the page names
    // and the printout afterwards
    // While printing avoid duplicate man page names
    //

    struct man_index_t dummy_index = {0l,0l,0};
    struct man_index_t *last_index = &dummy_index;

#ifdef _USE_QSORT

    // sort and print
    qsort(indexlist, listlen, sizeof(struct man_index_t *), compare_man_index);

    TQChar firstchar, tmp;
    TQString indexLine="<div class=\"secidxshort\">\n";
    if (indexlist[0]->manpage_len>0)
    {
	firstchar=TQChar((indexlist[0]->manpage_begin)[0]).lower();

	const TQString appendixstr = TQString(
	    " [<a href=\"#%1\" accesskey=\"%2\">%3</a>]\n"
	).arg(firstchar).arg(firstchar).arg(firstchar);
	indexLine.append(appendixstr);
    }
    os << "<tr><td class=\"secidxnextletter\"" << " colspan=\"3\">\n  <a name=\"" 
       << firstchar << "\">" << firstchar <<"</a>\n</td></tr>" << endl;

    for (int i=0; i<listlen; i++)
    {
	struct man_index_t *manindex = indexlist[i];

	// tqstrncmp():
	// "last_man" has already a \0 string ending, but
	// "manindex->manpage_begin" has not,
	// so do compare at most "manindex->manpage_len" of the strings.
	if (last_index->manpage_len == manindex->manpage_len &&
	    !tqstrncmp(last_index->manpage_begin,
		      manindex->manpage_begin,
		      manindex->manpage_len)
	    )
	{
	    continue;
	}
	
	tmp=TQChar((manindex->manpage_begin)[0]).lower();
	if (firstchar != tmp)
	{
	    firstchar = tmp;
	    os << "<tr><td class=\"secidxnextletter\"" << " colspan=\"3\">\n  <a name=\"" 
	       << firstchar << "\">" << firstchar << "</a>\n</td></tr>" << endl;

    	    const TQString appendixstr = TQString(
    		" [<a href=\"#%1\" accesskey=\"%2\">%3</a>]\n"
	    ).arg(firstchar).arg(firstchar).arg(firstchar);
	    indexLine.append(appendixstr);
	}
	os << "<tr><td><a href=\"man:"
	   << manindex->manpath << "\">\n";

	((char *)manindex->manpage_begin)[manindex->manpage_len] = '\0';
	os << manindex->manpage_begin
	   << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(manindex->manpage_begin) ? indexmap[manindex->manpage_begin] : "" )
	   << "</td></tr>"  << endl;
	last_index = manindex;
    }
    indexLine.append("</div>");

    for (int i=0; i<listlen; i++) {
	::free(indexlist[i]->manpath);   // allocated by strdup
	delete indexlist[i];
    }

    delete [] indexlist;

#else /* !_USE_QSORT */

    manpages.sort(); // using

    for (QManIndexListIterator mit(manpages);
	 mit.current();
	 ++mit )
    {
	struct man_index_t *manindex = mit.current();

	// tqstrncmp():
	// "last_man" has already a \0 string ending, but
	// "manindex->manpage_begin" has not,
	// so do compare at most "manindex->manpage_len" of the strings.
	if (last_index->manpage_len == manindex->manpage_len &&
	    !tqstrncmp(last_index->manpage_begin,
		      manindex->manpage_begin,
		      manindex->manpage_len)
	    )
	{
	    continue;
	}

	os << "<tr><td><a href=\"man:"
	   << manindex->manpath << "\">\n";

	manindex->manpage_begin[manindex->manpage_len] = '\0';
	os << manindex->manpage_begin
	   << "</a></td><td>&nbsp;</td><td> "
	   << (indexmap.contains(manindex->manpage_begin) ? indexmap[manindex->manpage_begin] : "" )
	   << "</td></tr>"  << endl;
	last_index = manindex;
    }
#endif /* _USE_QSORT */
#endif /* _USE_OLD_CODE */

    os << "</table></div>" << endl;
    
    os << indexLine << endl;

    // print footer
    os << "</body></html>" << endl;

    infoMessage(TQString::null);
    mimeType("text/html");
    data(array);
    finished();
}

void MANProtocol::listDir(const KURL &url)
{
    kdDebug( 7107 ) << "ENTER listDir: " << url.prettyURL() << endl;

    TQString title;
    TQString section;

    if ( !parseUrl(url.path(), title, section) ) {
        error( KIO::ERR_MALFORMED_URL, url.url() );
        return;
    }

    TQStringList list = findPages( section, TQString::null, false );

    UDSEntryList uds_entry_list;
    UDSEntry     uds_entry;
    UDSAtom      uds_atom;

    uds_atom.m_uds = KIO::UDS_NAME; // we only do names...
    uds_entry.append( uds_atom );

    TQStringList::Iterator it = list.begin();
    TQStringList::Iterator end = list.end();

    for ( ; it != end; ++it ) {
        stripExtension( &(*it) );

        uds_entry[0].m_str = *it;
        uds_entry_list.append( uds_entry );
    }

    listEntries( uds_entry_list );
    finished();
}

void MANProtocol::getProgramPath()
{
  if (!mySgml2RoffPath.isEmpty())
    return;

  mySgml2RoffPath = KGlobal::dirs()->findExe("sgml2roff");
  if (!mySgml2RoffPath.isEmpty())
    return;

  /* sgml2roff isn't found in PATH. Check some possible locations where it may be found. */
  mySgml2RoffPath = KGlobal::dirs()->findExe("sgml2roff", TQString(SGML2ROFF_DIRS));
  if (!mySgml2RoffPath.isEmpty())
    return;

  /* Cannot find sgml2roff programm: */
  outputError(i18n("Could not find the sgml2roff program on your system. Please install it, if necessary, and extend the search path by adjusting the environment variable PATH before starting KDE."));
  finished();
  exit();
}
