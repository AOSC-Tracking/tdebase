/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>
   Copyright 2006 Michael Pyne <michael.pyne@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include <stdlib.h>

#include <tqdir.h>
#include <tqregexp.h>

#include <kdebug.h>
#include <tdeprocess.h>
#include <tdestandarddirs.h>
#include <kinstance.h>
#include <tdelocale.h>
#include <tdeconfig.h>

#include "cgi.h"

using namespace TDEIO;

CgiProtocol::CgiProtocol( const TQCString &pool, const TQCString &app )
    : SlaveBase( "cgi", pool, app )
{
  kdDebug(7124) << "CgiProtocol::CgiProtocol" << endl;

  TDEConfig cfg( "kcmcgirc" );
  cfg.setGroup( "General" );
  mCgiPaths = cfg.readListEntry( "Paths" );
}

CgiProtocol::~CgiProtocol()
{
  kdDebug(7124) << "CgiProtocol::~CgiProtocol" << endl;
}

/**
 * Search in reverse order through a TQByteArray for a given character.  The position
 * of the character is returned, or -1 if it was not found.
 */
static int qByteArrayFindRev( const TQByteArray &ba, char c, int startIndex )
{
  for ( int i = startIndex; i >= 0; --i )
    if ( ba[i] == c ) return i;

  return -1;
}

/**
 * Extract data in ba from start, including no more than len characters from ba.
 * Should be exactly comparable to TQCString::mid()
 */
static TQCString extractQCString( const TQByteArray &ba, uint start, uint len = 0xffffffff )
{
  uint realLen = len;

  if ( ( ba.size() - start ) < len )
    realLen = ba.size() - start;

//   return TQCString( &ba[ start ], realLen + 1 );
  return TQCString( ba ).mid(start, realLen + 1);
}

/**
 * Search through a TQByteArray for a given string.  The position of the string
 * is returned, or -1 if it was not found.
 */
static int qByteArrayFindStr( const TQByteArray &ba, const char *str )
{
  int strLen = tqstrlen( str );
  int searchLen = ba.size() - strLen;

  for ( int i = 0; i <= searchLen; ++i ) {
    TQCString temp = extractQCString( ba, i, strLen );
    if ( temp == str )
      return i;
  }

  return -1;
}

void CgiProtocol::get( const KURL& url )
{
  kdDebug(7124) << "CgiProtocol::get()" << endl;
  kdDebug(7124) << " URL: " << url.url() << endl;
#if 0
  kdDebug(7124) << " Path: " << url.path() << endl;
  kdDebug(7124) << " Query: " << url.query() << endl;
  kdDebug(7124) << " Protocol: " << url.protocol() << endl;
  kdDebug(7124) << " Filename: " << url.filename() << endl;
#endif
  TQCString protocol = "SERVER_PROTOCOL=HTTP";
  putenv( protocol.data() );

  TQCString requestMethod = "REQUEST_METHOD=GET";
  putenv( requestMethod.data() );

  TQCString query = url.query().mid( 1 ).local8Bit();
  query.prepend( "QUERY_STRING=" );
  putenv( query.data() );

  TQString path = url.path();

  TQString file;

  int pos = path.findRev('/');
  if ( pos >= 0 ) file = path.mid( pos + 1 );
  else file = path;

  TQString cmd;

  bool stripHeader = false;
  bool forwardFile = true;

  TQStringList::ConstIterator it;
  for( it = mCgiPaths.begin(); it != mCgiPaths.end(); ++it ) {
    cmd = *it;
    if ( !(*it).endsWith("/") )
        cmd += "/";
    cmd += file;
    if ( TDEStandardDirs::exists( cmd ) ) {
      forwardFile = false;
      stripHeader = true;
      break;
    }
  }

  FILE *fd;

  if ( forwardFile ) {
    kdDebug(7124) << "Forwarding to '" << path << "'" << endl;

    TQCString filepath = TQFile::encodeName( path );

    fd = fopen( filepath.data(), "r" );

    if ( !fd ) {
      kdDebug(7124) << "Error opening '" << filepath << "'" << endl;
      error( TDEIO::ERR_CANNOT_OPEN_FOR_READING, filepath );
      return;
    }
  } else {
    kdDebug(7124) << "Cmd: " << cmd << endl;

    fd = popen( TQFile::encodeName(TDEProcess::quote( cmd )).data(), "r" );

    if ( !fd ) {
      kdDebug(7124) << "Error running '" << cmd << "'" << endl;
      error( TDEIO::ERR_CANNOT_OPEN_FOR_READING, cmd );
      return;
    }
  }

  char buffer[ 4090 ];

  while ( !feof( fd ) )
  {
    int n = fread( buffer, 1, 2048, fd );

    if ( n == -1 )
    {
      // ERROR
      if ( forwardFile ) {
        fclose( fd );
      } else {
        pclose( fd );
      }
      return;
    }

    buffer[n] = 0;

    if ( stripHeader ) {
      TQByteArray output;

      // Access the buffer in-place by using setRawData()
      output.setRawData( buffer, n );

      int colon = output.find( ':' );
      int newline = output.find( '\n' );
      int semicolon = qByteArrayFindRev( output, ';', newline );
      int end;
      if ( semicolon < 0 ) end = newline;
      else end = semicolon;

#if 0
      kdDebug(7124) << "  colon: " << colon << endl;
      kdDebug(7124) << "  newline: " << newline << endl;
      kdDebug(7124) << "  semicolon: " << semicolon << endl;
      kdDebug(7124) << "  end: " << end << endl;
#endif

      TQCString contentType = extractQCString( output, colon + 1, end - colon - 1 );

      contentType = contentType.stripWhiteSpace();

      kdDebug(7124) << "ContentType: '" << contentType << "'" << endl;

      mimeType( contentType );

      int start = qByteArrayFindStr( output, "\r\n\r\n" );
      if ( start >= 0 ) start += 4;
      else {
        start = qByteArrayFindStr( output, "\n\n" );
        if ( start >= 0 ) start += 2;
      }

      if ( start < 0 )
        start = 0;

      // We're done with the part of the buffer we're using.
      output.resetRawData ( buffer, n );

      // Now access the part of the buffer after the header.
      output.setRawData ( buffer + start, n - start );
      data( output );
      output.resetRawData ( buffer + start, n - start );

      stripHeader = false;
    } else {
      TQByteArray array;
      array.setRawData( buffer, n );
      data( array );
      array.resetRawData( buffer, n );
    }
  }

  if ( forwardFile ) {
    fclose( fd );
  } else {
    pclose( fd );
  }

  finished();

  kdDebug(7124) << "CgiProtocol::get - done" << endl;
}

extern "C" { int TDE_EXPORT kdemain( int argc, char **argv ); }

/*! The kdemain function generates an instance of the ioslave and starts its
 * dispatch loop. */

int kdemain( int argc, char **argv )
{
  TDEInstance instance( "tdeio_cgi" );

  kdDebug(7124) << "tdeio_cgi starting " << getpid() << endl;

  if (argc != 4)
  {
     fprintf(stderr, "Usage: tdeio_cgi protocol domain-socket1 domain-socket2\n");
     exit(-1);
  }

  CgiProtocol slave( argv[2], argv[3] );
  slave.dispatchLoop();

  return 0;
}
