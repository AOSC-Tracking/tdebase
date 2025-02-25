/*
    This file is part of KHelpcenter.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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

#include "formatter.h"

#include <tdelocale.h>
#include <tdeglobal.h>
#include <kdebug.h>
#include <tdeconfig.h>
#include <tdestandarddirs.h>

#include <tqfile.h>
#include <tqtextstream.h>

using namespace KHC;

Formatter::Formatter()
  : mHasTemplate( false )
{
}

Formatter:: ~Formatter()
{
}

bool Formatter::readTemplates()
{
  TDEConfig *cfg = TDEGlobal::config();
  cfg->setGroup( "Templates" );
  TQString mainTemplate = cfg->readEntry( "MainTemplate" );

  if ( mainTemplate.isEmpty() ) {
    mainTemplate = locate( "appdata", "maintemplate" );
  }
  
  if ( mainTemplate.isEmpty() ) {
    kdDebug() << "Main template file name is empty." << endl;
    return false;
  }
  
  TQFile f( mainTemplate );
  if ( !f.open( IO_ReadOnly ) ) {
    kdWarning() << "Unable to open main template file '" << mainTemplate
                << "'." << endl;
    return false;
  }

  TQTextStream ts( &f );
  TQString line;
  enum State { IDLE, SINGLELINE, MULTILINE };
  State state = IDLE;
  TQString symbol;
  TQString endMarker;
  TQString value;
  while( !( line = ts.readLine() ).isNull() ) {
    switch ( state ) {
      case IDLE:
        if ( !line.isEmpty() && !line.startsWith( "#" ) ) {
          int pos = line.find( "<<" );
          if ( pos >= 0 ) {
            state = MULTILINE;
            symbol = line.left( pos ).stripWhiteSpace();
            endMarker = line.mid( pos + 2 ).stripWhiteSpace();
          } else {
            state = SINGLELINE;
            symbol = line.stripWhiteSpace();
          }
        }
        break;
      case SINGLELINE:
        mSymbols.insert( symbol, line );
        state = IDLE;
        break;
      case MULTILINE:
        if ( line.startsWith( endMarker ) ) {
          mSymbols.insert( symbol, value );
          value = "";
          state = IDLE;
        } else {
          value += line + '\n';
        }
        break;
      default:
        kdError() << "Formatter::readTemplates(): Illegal state: "
                  << state << endl;
        break;
    }
  }

  f.close();

#if 0
  TQMap<TQString,TQString>::ConstIterator it;
  for( it = mSymbols.begin(); it != mSymbols.end(); ++it ) {
    kdDebug() << "KEY: " << it.key() << endl;
    kdDebug() << "VALUE: " << it.data() << endl;
  }
#endif

  TQStringList requiredSymbols;
  requiredSymbols << "HEADER" << "FOOTER";

  bool success = true;
  TQStringList::ConstIterator it2;
  for( it2 = requiredSymbols.begin(); it2 != requiredSymbols.end(); ++it2 ) {
    if ( !mSymbols.contains( *it2 ) ) {
      success = false;
      kdError() << "Symbol '" << *it2 << "' is missing from main template file."
                << endl;
    }
  }

  if ( success ) mHasTemplate = true;

  return success;
}

TQString Formatter::header( const TQString &title )
{
  TQString s;
  if ( mHasTemplate ) {
    s = mSymbols[ "HEADER" ];
    s.replace( "--TITLE:--", title );
  } else {
    s = "<html><head><title>" + title + "</title></head>\n<body>\n";
  }
  return s;
}

TQString Formatter::footer()
{
  if ( mHasTemplate ) {
    return mSymbols[ "FOOTER" ];
  } else {
    return "</body></html>";
  }
}

TQString Formatter::separator()
{
//  return "<table width=100%><tr><td bgcolor=\"#7B8962\">&nbsp;"
//         "</td></tr></table>";
  return "<hr>";
}

TQString Formatter::docTitle( const TQString &title )
{
  return "<h3><font color=\"red\">" + title + "</font></h3>";
}

TQString Formatter::sectionHeader( const TQString &section )
{
  return "<h2><font color=\"blue\">" + section + "</font></h2>";
}

TQString Formatter::processResult( const TQString &data )
{
  TQString result;

  enum { Header, BodyTag, Body, Footer };

  int state = Header;

  for( uint i = 0; i < data.length(); ++i ) {
    TQChar c = data[i];
    switch ( state ) {
      case Header:
        if ( c == '<' && data.mid( i, 5 ).lower() == "<body" ) {
          state = BodyTag;
          i += 4;
        }
        break;
      case BodyTag:
        if ( c == '>' ) state = Body;
        break;
      case Body:
        if ( c == '<' && data.mid( i, 7 ).lower() == "</body>" ) {
          state = Footer;
        } else {
          result.append( c );
        }
        break;
      case Footer:
        break;
      default:
        result.append( c );
        break;
    }
  }

  if ( state == Header ) return data;
  else return result;
}

TQString Formatter::paragraph( const TQString &str )
{
  return "<p>" + str + "</p>";
}

TQString Formatter::title( const TQString &title )
{
  return "<h2>" + title + "</h2>";
}
