/*

    tdeshorturifilter.h

    This file is part of the KDE project
    Copyright (C) 2000 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 2000 Malte Starostik <starosti@zedat.fu-berlin.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

#include <tqdir.h>
#include <tqregexp.h>

#include <tdeapplication.h>
#include <kdebug.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <tdeconfig.h>
#include <kmimetype.h>

#include "tdeshorturifilter.h"

#define FQDN_PATTERN    "(?:[a-zA-Z0-9][a-zA-Z0-9+-]*\\.[a-zA-Z]+)"
#define IPv4_PATTERN    "[0-9]{1,3}\\.[0-9]{1,3}(?:\\.[0-9]{0,3})?(?:\\.[0-9]{0,3})?"
#define IPv6_PATTERN    "^\\[.*\\]"
#define ENV_VAR_PATTERN "\\$[a-zA-Z_][a-zA-Z0-9_]*"

#define QFL1(x) TQString::fromLatin1(x)

 /**
  * IMPORTANT:
  *  If you change anything here, please run the regression test
  *  tdelibs/tdeio/tests/kurifiltertest.
  *
  *  If you add anything here, make sure to add a corresponding
  *  test code to tdelibs/tdeio/tests/kurifiltertest.
  */

typedef TQMap<TQString,TQString> EntryMap;

static bool isValidShortURL( const TQString& cmd, bool verbose = false )
{
  // Examples of valid short URLs:
  // "kde.org", "foo.bar:8080", "user@foo.bar:3128"
  // "192.168.1.0", "127.0.0.1:3128"
  // "[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]"
  TQRegExp exp;

  // Match FQDN_PATTERN
  exp.setPattern( QFL1(FQDN_PATTERN) );
  if ( cmd.contains( exp ) )
  {
    if (verbose)
      kdDebug() << "TDEShortURIFilter::isValidShortURL: " << cmd
                << " matches FQDN_PATTERN" << endl;

    // bug 133687
#if 0 
    // stuff like wallpaper.png matches the FQDN_PATTERN but is most
    // likely not a domain
    if (KMimeType::findByPath(cmd, 0, true /* fast mode */) != KMimeType::defaultMimeTypePtr())
        return false;
#endif

    return true;
  }

  // Match IPv4 addresses
  exp.setPattern( QFL1(IPv4_PATTERN) );
  if ( cmd.contains( exp ) )
  {
    if (verbose)
      kdDebug() << "TDEShortURIFilter::isValidShortURL: " << cmd
                << " matches IPv4_PATTERN" << endl;
    return true;
  }

  // Match IPv6 addresses
  exp.setPattern( QFL1(IPv6_PATTERN) );
  if ( cmd.contains( exp ) )
  {
    if (verbose)
      kdDebug() << "TDEShortURIFilter::isValidShortURL: " << cmd
                << " matches IPv6_PATTERN" << endl;
    return true;
  }

  if (verbose)
    kdDebug() << "TDEShortURIFilter::isValidShortURL: '" << cmd
              << "' is not a short URL." << endl;

  return false;
}

static TQString removeArgs( const TQString& _cmd )
{
  TQString cmd( _cmd );

  if( cmd[0] != '\'' && cmd[0] != '"' )
  {
    // Remove command-line options (look for first non-escaped space)
    int spacePos = 0;

    do
    {
      spacePos = cmd.find( ' ', spacePos+1 );
    } while ( spacePos > 1 && cmd[spacePos - 1] == '\\' );

    if( spacePos > 0 )
    {
      cmd = cmd.left( spacePos );
      //kdDebug() << k_funcinfo << "spacePos=" << spacePos << " returning " << cmd << endl;
    }
  }

  return cmd;
}

TDEShortURIFilter::TDEShortURIFilter( TQObject *parent, const char *name,
                                  const TQStringList & /*args*/ )
                :KURIFilterPlugin( parent, name ? name : "tdeshorturifilter", 1.0),
                 DCOPObject("TDEShortURIFilterIface")
{
    configure();
}

bool TDEShortURIFilter::filterURI( KURIFilterData& data ) const
{
 /*
  * Here is a description of how the shortURI deals with the supplied
  * data.  First it expands any environment variable settings and then
  * deals with special shortURI cases. These special cases are the "smb:"
  * URL scheme which is very specific to KDE, "#" and "##" which are
  * shortcuts for man:/ and info:/ protocols respectively. It then handles
  * local files.  Then it checks to see if the URL is valid and one that is
  * supported by KDE's IO system.  If all the above checks fails, it simply
  * lookups the URL in the user-defined list and returns without filtering
  * if it is not found. TODO: the user-defined table is currently only manually
  * hackable and is missing a config dialog.
  */

  KURL url = data.uri();
  TQString cmd = data.typedString();
  bool isMalformed = !url.isValid();
  //kdDebug() << "url=" << url.url() << " cmd=" << cmd << " isMalformed=" << isMalformed << endl;

  if (!isMalformed &&
      (url.protocol().length() == 4) &&
      (url.protocol() != TQString::fromLatin1("http")) &&
      (url.protocol()[0]=='h') &&
      (url.protocol()[1]==url.protocol()[2]) &&
      (url.protocol()[3]=='p'))
  {
     // Handle "encrypted" URLs like: h++p://www.kde.org
     url.setProtocol( TQString::fromLatin1("http"));
     setFilteredURI( data, url);
     setURIType( data, KURIFilterData::NET_PROTOCOL );
     return true;
  }

  // TODO: Make this a bit more intelligent for Minicli! There
  // is no need to make comparisons if the supplied data is a local
  // executable and only the argument part, if any, changed! (Dawit)
  // You mean caching the last filtering, to try and reuse it, to save stat()s? (David)

  const TQString starthere_proto = QFL1("start-here:");
  if (cmd.find(starthere_proto, 0, true) == 0 )
  {
    setFilteredURI( data, KURL("system:/") );
    setURIType( data, KURIFilterData::LOCAL_DIR );
    return true;
  }

  // Handle MAN & INFO pages shortcuts...
  const TQString man_proto = QFL1("man:");
  const TQString info_proto = QFL1("info:");
  if( cmd[0] == '#' ||
      cmd.find( man_proto, 0, true ) == 0 ||
      cmd.find( info_proto, 0, true ) == 0 )
  {
    if( cmd.left(2) == QFL1("##") )
      cmd = QFL1("info:/") + cmd.mid(2);
    else if ( cmd[0] == '#' )
      cmd = QFL1("man:/") + cmd.mid(1);

    else if ((cmd==info_proto) || (cmd==man_proto))
      cmd+='/';

    setFilteredURI( data, KURL( cmd ));
    setURIType( data, KURIFilterData::HELP );
    return true;
  }

  // Detect UNC style (aka windows SMB) URLs
  if ( cmd.startsWith( TQString::fromLatin1( "\\\\") ) )
  {
    // make sure path is unix style
    cmd.replace('\\', '/');
    cmd.prepend( TQString::fromLatin1( "smb:" ) );
    setFilteredURI( data, KURL( cmd ));
    setURIType( data, KURIFilterData::NET_PROTOCOL );
    return true;
  }

  bool expanded = false;

  // Expanding shortcut to HOME URL...
  TQString path;
  TQString ref;
  TQString query;
  TQString nameFilter;

  if (KURL::isRelativeURL(cmd) && TQDir::isRelativePath(cmd)) {
     path = cmd;
  }
  else
  {
    if (url.isLocalFile())
    {
      // Split path from ref/query if the path exists
      // but not for "/tmp/a#b", if "a#b" is an existing file,
      // or for "/tmp/a?b" (#58990)
      if ( ( url.hasRef() || !url.query().isEmpty() ) // avoid the calling exists() when not needed
           && TQFile::exists(url.path())
           && !url.path().endsWith(QFL1("/")) ) // /tmp/?foo is a namefilter, not a query
      {
        path = url.path();
        ref = url.ref();
        query = url.query();
        if (path.isEmpty() && url.hasHost())
          path = '/';
      }
      else
      {
        path = cmd;
      }
    }
  }

  if( path[0] == '~' )
  {
    int slashPos = path.find('/');
    if( slashPos == -1 )
      slashPos = path.length();
    if( slashPos == 1 )   // ~/
    {
      path.replace ( 0, 1, TQDir::homeDirPath() );
    }
    else // ~username/
    {
      TQString user = path.mid( 1, slashPos-1 );
      struct passwd *dir = getpwnam(user.local8Bit().data());
      if( dir && strlen(dir->pw_dir) )
      {
        path.replace (0, slashPos, TQString::fromLocal8Bit(dir->pw_dir));
      }
      else
      {
        TQString msg = dir ? i18n("<qt><b>%1</b> does not have a home folder.</qt>").arg(user) :
                            i18n("<qt>There is no user called <b>%1</b>.</qt>").arg(user);
        setErrorMsg( data, msg );
        setURIType( data, KURIFilterData::ERROR );
        // Always return true for error conditions so
        // that other filters will not be invoked !!
        return true;
      }
    }
    expanded = true;
  }
  else if ( path[0] == '$' ) {
    // Environment variable expansion.
    TQRegExp r (QFL1(ENV_VAR_PATTERN));
    if ( r.search( path ) == 0 )
    {
      const char* exp = getenv( path.mid( 1, r.matchedLength() - 1 ).local8Bit().data() );
      if(exp)
      {
        path.replace( 0, r.matchedLength(), TQString::fromLocal8Bit(exp) );
        expanded = true;
      }
    }
  }

  if ( expanded )
  {
    // Look for #ref again, after $ and ~ expansion (testcase: $QTDIR/doc/html/functions.html#s)
    // Can't use KURL here, setPath would escape it...
    int pos = path.find('#');
    if ( pos > -1 )
    {
      ref = path.mid( pos + 1 );
      path = path.left( pos );
      //kdDebug() << "Extracted ref: path=" << path << " ref=" << ref << endl;
    }
  }


  bool isLocalFullPath = (!path.isEmpty() && path[0] == '/');

  // Checking for local resource match...
  // Determine if "uri" is an absolute path to a local resource  OR
  // A local resource with a supplied absolute path in KURIFilterData
  TQString abs_path = data.absolutePath();

  bool canBeAbsolute = (isMalformed && !abs_path.isEmpty());
  bool canBeLocalAbsolute = (canBeAbsolute && abs_path[0] =='/');
  bool exists = false;

  /*kdDebug() << "abs_path=" << abs_path << " malformed=" << isMalformed
            << " canBeLocalAbsolute=" << canBeLocalAbsolute << endl;*/

  struct stat buff;
  if ( canBeLocalAbsolute )
  {
    TQString abs = TQDir::cleanDirPath( abs_path );
    // combine absolute path (abs_path) and relative path (cmd) into abs_path
    int len = path.length();
    if( (len==1 && path[0]=='.') || (len==2 && path[0]=='.' && path[1]=='.') )
        path += '/';
    //kdDebug() << "adding " << abs << " and " << path << endl;
    abs = TQDir::cleanDirPath(abs + '/' + path);
    //kdDebug() << "checking whether " << abs << " exists." << endl;
    // Check if it exists
    if( stat( TQFile::encodeName(abs).data(), &buff ) == 0 )
    {
      path = abs; // yes -> store as the new cmd
      exists = true;
      isLocalFullPath = true;
    }
  }

  if( isLocalFullPath && !exists )
  {
    exists = ( stat( TQFile::encodeName(path).data() , &buff ) == 0 );

    if ( !exists ) {
      // Support for name filter (/foo/*.txt), see also KonqMainWindow::detectNameFilter
      // If the app using this filter doesn't support it, well, it'll simply error out itself
      int lastSlash = path.findRev( '/' );
      if ( lastSlash > -1 && path.find( ' ', lastSlash ) == -1 ) // no space after last slash, otherwise it's more likely command-line arguments
      {
        TQString fileName = path.mid( lastSlash + 1 );
        TQString testPath = path.left( lastSlash + 1 );
        if ( ( fileName.find( '*' ) != -1 || fileName.find( '[' ) != -1 || fileName.find( '?' ) != -1 )
           && stat( TQFile::encodeName(testPath).data(), &buff ) == 0 )
        {
          nameFilter = fileName;
          kdDebug() << "Setting nameFilter to " << nameFilter << endl;
          path = testPath;
          exists = true;
        }
      }
    }
  }

  //kdDebug() << "path =" << path << " isLocalFullPath=" << isLocalFullPath << " exists=" << exists << endl;
  if( exists )
  {
    KURL u;
    u.setPath(path);
    u.setRef(ref);
    u.setQuery(query);

    if (kapp && !kapp->authorizeURLAction( TQString::fromLatin1("open"), KURL(), u))
    {
      // No authorisation, we pretend it's a file will get
      // an access denied error later on.
      setFilteredURI( data, u );
      setURIType( data, KURIFilterData::LOCAL_FILE );
      return true;
    }

    // Can be abs path to file or directory, or to executable with args
    bool isDir = S_ISDIR( buff.st_mode );
    if( !isDir && access ( TQFile::encodeName(path).data(), X_OK) == 0 )
    {
      // ::access() is not always correct, especially on network file systems
      // Verify that we actually have at least one execute permission bit set before flagging the file as executable...
      struct stat buffer;
      int status;
      status = stat(TQFile::encodeName(path).data(), &buffer);
      if (status == 0) {
          bool is_executable = false;
          int file_mode = ((buffer.st_mode & S_IRWXU) >> 6);		// User
          if (file_mode & 0x1) is_executable = true;
          file_mode = file_mode + ((buffer.st_mode & S_IRWXG) >> 3);	// Group
          if (file_mode & 0x1) is_executable = true;
          file_mode = file_mode + ((buffer.st_mode & S_IRWXO) >> 0);	// Other
          if (file_mode & 0x1) is_executable = true;
          if (is_executable == true) {
              //kdDebug() << "Abs path to EXECUTABLE" << endl;
              setFilteredURI( data, u );
              setURIType( data, KURIFilterData::EXECUTABLE );
              return true;
          }
      }
      else {
          //kdDebug() << "Abs path to EXECUTABLE" << endl;
          setFilteredURI( data, u );
          setURIType( data, KURIFilterData::EXECUTABLE );
          return true;
      }
    }

    // Open "uri" as file:/xxx if it is a non-executable local resource.
    if( isDir || S_ISREG( buff.st_mode ) )
    {
      //kdDebug() << "Abs path as local file or directory" << endl;
      if ( !nameFilter.isEmpty() )
        u.setFileName( nameFilter );
      setFilteredURI( data, u );
      setURIType( data, ( isDir ) ? KURIFilterData::LOCAL_DIR : KURIFilterData::LOCAL_FILE );
      return true;
    }

    // Should we return LOCAL_FILE for non-regular files too?
    kdDebug() << "File found, but not a regular file nor dir... socket?" << endl;
  }

  // Let us deal with possible relative URLs to see
  // if it is executable under the user's $PATH variable.
  // We try hard to avoid parsing any possible command
  // line arguments or options that might have been supplied.
  TQString exe = removeArgs( cmd );
  //kdDebug() << k_funcinfo << "findExe with " << exe << endl;
  if( data.checkForExecutables() && !TDEStandardDirs::findExe( exe ).isNull() )
  {
    //kdDebug() << "EXECUTABLE  exe=" << exe << endl;
    setFilteredURI( data, KURL( exe ));
    // check if we have command line arguments
    if( exe != cmd )
        setArguments(data, cmd.right(cmd.length() - exe.length()));
    setURIType( data, KURIFilterData::EXECUTABLE );
    return true;
  }

  // Process URLs of known and supported protocols so we don't have
  // to resort to the pattern matching scheme below which can possibly
  // be slow things down...
  if ( !isMalformed && !isLocalFullPath )
  {
    const TQStringList protocols = KProtocolInfo::protocols();
    for( TQStringList::ConstIterator it = protocols.begin(); it != protocols.end(); ++it )
    {
      if( (url.protocol() == *it) )
      {
        setFilteredURI( data, url );
        if ( *it == QFL1("man") || *it == QFL1("help") )
          setURIType( data, KURIFilterData::HELP );
        else
          setURIType( data, KURIFilterData::NET_PROTOCOL );
        return true;
      }
    }
  }

  // Okay this is the code that allows users to supply custom matches for
  // specific URLs using Qt's regexp class. This is hard-coded for now.
  // TODO: Make configurable at some point...
  if ( !cmd.contains( ' ' ) )
  {
    TQValueList<URLHint>::ConstIterator it;
    for( it = m_urlHints.begin(); it != m_urlHints.end(); ++it )
    {
      TQRegExp match( (*it).regexp );
      if ( match.search( cmd, 0 ) == 0 )
      {
        //kdDebug() << "match - prepending " << (*it).prepend << endl;
        cmd.prepend( (*it).prepend );
        setFilteredURI( data, KURL( cmd ) );
        setURIType( data, (*it).type );
        return true;
      }
    }

    // If cmd is NOT a local resource, check if it is a valid "shortURL"
    // candidate and append the default protocol the user supplied. (DA)
    if ( isMalformed && isValidShortURL(cmd, m_bVerbose) )
    {
      if (m_bVerbose)
        kdDebug() << "Valid short url, from malformed url -> using default proto="
                  << m_strDefaultProtocol << endl;

      cmd.insert( 0, m_strDefaultProtocol );
      setFilteredURI( data, KURL( cmd ));
      setURIType( data, KURIFilterData::NET_PROTOCOL );
      return true;
    }
  }

  // If we previously determined that the URL might be a file,
  // and if it doesn't exist, then error
  if( isLocalFullPath && !exists )
  {
    KURL u;
    u.setPath(path);
    u.setRef(ref);

    if (kapp && !kapp->authorizeURLAction( TQString::fromLatin1("open"), KURL(), u))
    {
      // No authorisation, we pretend it exists and will get
      // an access denied error later on.
      setFilteredURI( data, u );
      setURIType( data, KURIFilterData::LOCAL_FILE );
      return true;
    }
    //kdDebug() << "fileNotFound -> ERROR" << endl;
    setErrorMsg( data, i18n( "<qt>The file or folder <b>%1</b> does not exist." ).arg( data.uri().prettyURL() ) );
    setURIType( data, KURIFilterData::ERROR );
    return true;
  }

  // If we reach this point, we cannot filter this thing so simply return false
  // so that other filters, if present, can take a crack at it.
  return false;
}

TDECModule* TDEShortURIFilter::configModule( TQWidget*, const char* ) const
{
    return 0; //new TDEShortURIOptions( parent, name );
}

TQString TDEShortURIFilter::configName() const
{
    return i18n("&ShortURLs");
}

void TDEShortURIFilter::configure()
{
  TDEConfig config( name() + QFL1("rc"), false, false );
  m_bVerbose = config.readBoolEntry( "Verbose", false );

  if ( m_bVerbose )
    kdDebug() << "TDEShortURIFilter::configure: Config reload request..." << endl;

  m_strDefaultProtocol = config.readEntry( "DefaultProtocol", QFL1("http://") );
  EntryMap patterns = config.entryMap( QFL1("Pattern") );
  const EntryMap protocols = config.entryMap( QFL1("Protocol") );
  config.setGroup("Type");

  for( EntryMap::Iterator it = patterns.begin(); it != patterns.end(); ++it )
  {
    TQString protocol = protocols[it.key()];
    if (!protocol.isEmpty())
    {
      int type = config.readNumEntry(it.key(), -1);
      if (type > -1 && type <= KURIFilterData::UNKNOWN)
        m_urlHints.append( URLHint(it.data(), protocol, static_cast<KURIFilterData::URITypes>(type) ) );
      else
        m_urlHints.append( URLHint(it.data(), protocol) );
    }
  }
}

K_EXPORT_COMPONENT_FACTORY( libtdeshorturifilter,
                            KGenericFactory<TDEShortURIFilter>( "kcmkurifilt" ) )

#include "tdeshorturifilter.moc"
