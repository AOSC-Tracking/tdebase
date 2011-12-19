/*  -*- c++ -*-
    capabilities.h

    This file is part of kio_smtp, the KDE SMTP kioslave.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KIOSMTP_CAPABILITIES_H__
#define __KIOSMTP_CAPABILITIES_H__

#include <tqmap.h>
#include <tqcstring.h>
#include <tqstring.h>
#include <tqstringlist.h>

class TQStrIList;

namespace KioSMTP {

  class Response;

  class Capabilities {
  public:
    Capabilities() {}

    static Capabilities fromResponse( const Response & response );

    void add( const TQString & cap, bool replace=false );
    void add( const TQString & name, const TQStringList & args, bool replace=false );
    void clear() { mCapabilities.clear(); }

    bool have( const TQString & cap ) const {
      return mCapabilities.find( cap.upper() ) != mCapabilities.end();
    }
    bool have( const TQCString & cap ) const { return have( TQString( cap.data() ) ); }
    bool have( const char * cap ) const { return have( TQString::fromLatin1( cap ) ); }

    TQString asMetaDataString() const;

    TQString authMethodMetaData() const;
    TQStrIList saslMethods() const;

    TQString createSpecialResponse( bool tls ) const;

    TQStringList saslMethodsQSL() const;
  private:

    TQMap<TQString,TQStringList> mCapabilities;
  };

} // namespace KioSMTP

#endif // __KIOSMTP_CAPABILITIES_H__
