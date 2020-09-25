/*
    request.h

    This file is part of tdeio_smtp, the KDE SMTP tdeioslave.
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

#ifndef __KIOSMTP_REQUEST_H__
#define __KIOSMTP_REQUEST_H__

#include <tqstring.h>
#include <tqstringlist.h>

class KURL;

namespace KioSMTP {

  class Request {
  public:
    Request()
      : mSubject( "missing subject" ), mEmitHeaders( true ),
	m8Bit( false ), mSize( 0 ) {}

    static Request fromURL( const KURL & url );

    TQString profileName() const { return mProfileName; }
    void setProfileName( const TQString & profileName ) { mProfileName = profileName; }
    bool hasProfile() const { return !profileName().isNull(); }

    TQString subject() const { return mSubject; }
    void setSubject( const TQString & subject ) { mSubject = subject; }

    TQString fromAddress() const { return mFromAddress; }
    void setFromAddress( const TQString & fromAddress ) { mFromAddress = fromAddress; }
    bool hasFromAddress() const { return !mFromAddress.isEmpty(); }

    TQStringList recipients() const { return to() + cc() + bcc() ; }
    bool hasRecipients() const { return !to().empty() || !cc().empty() || !bcc().empty() ; }

    TQStringList to() const { return mTo; }
    TQStringList cc() const { return mCc; }
    TQStringList bcc() const { return mBcc; }
    void addTo( const TQString & to ) { mTo.push_back( to ); }
    void addCc( const TQString & cc ) { mCc.push_back( cc ); }
    void addBcc( const TQString & bcc ) { mBcc.push_back( bcc ); }

    TQString heloHostname() const { return mHeloHostname; }
    TQCString heloHostnameCString() const;
    void setHeloHostname( const TQString & hostname ) { mHeloHostname = hostname; }

    bool emitHeaders() const { return mEmitHeaders; }
    void setEmitHeaders( bool emitHeaders ) { mEmitHeaders = emitHeaders; }

    bool is8BitBody() const { return m8Bit; }
    void set8BitBody( bool a8Bit ) { m8Bit = a8Bit; }

    unsigned int size() const { return mSize; }
    void setSize( unsigned int size ) { mSize = size; }

    /** If @ref #emitHeaders() is true, returns the rfc2822
	serialization of the header fields "To", "Cc", "Subject" and
	"From", as determined by the respective settings. If @ref
	#emitHeaders() is false, returns a null string. */
    TQCString headerFields( const TQString & fromRealName=TQString::null ) const;

  private:
    TQStringList mTo, mCc, mBcc;
    TQString mProfileName, mSubject, mFromAddress, mHeloHostname;
    bool mEmitHeaders;
    bool m8Bit;
    unsigned int mSize;
  };

} // namespace KioSMTP

#endif // __KIOSMTP_REQUEST_H__
