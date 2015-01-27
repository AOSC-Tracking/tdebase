/*  -*- c++ -*-
    command.cc

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

#include <config.h>

#include "command.h"

#include "smtp.h"
#include "response.h"
#include "transactionstate.h"

#include <kidna.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <kmdcodec.h>
#include <tdeio/slavebase.h> // for test_commands, where SMTPProtocol is not derived from TCPSlaveBase

#include <assert.h>

namespace KioSMTP {

#ifdef HAVE_LIBSASL2
static sasl_callback_t callbacks[] = {
    { SASL_CB_ECHOPROMPT, NULL, NULL },
    { SASL_CB_NOECHOPROMPT, NULL, NULL },
    { SASL_CB_GETREALM, NULL, NULL },
    { SASL_CB_USER, NULL, NULL },
    { SASL_CB_AUTHNAME, NULL, NULL },
    { SASL_CB_PASS, NULL, NULL },
    { SASL_CB_CANON_USER, NULL, NULL },
    { SASL_CB_LIST_END, NULL, NULL }
};
#endif

  //
  // Command (base class)
  //

  Command::Command( SMTPProtocol * smtp, int flags )
    : mSMTP( smtp ),
      mComplete( false ), mNeedResponse( false ), mFlags( flags )
  {
    assert( smtp );
  }

  Command::~Command() {}

  bool Command::processResponse( const Response & r, TransactionState * ) {
    mComplete = true;
    mNeedResponse = false;
    return r.isOk();
  }

  void Command::ungetCommandLine( const TQCString &, TransactionState * ) {
    mComplete = false;
  }

  Command * Command::createSimpleCommand( int which, SMTPProtocol * smtp ) {
    switch ( which ) {
    case STARTTLS: return new StartTLSCommand( smtp );
    case DATA:     return new DataCommand( smtp );
    case NOOP:     return new NoopCommand( smtp );
    case RSET:     return new RsetCommand( smtp );
    case QUIT:     return new QuitCommand( smtp );
    default:       return 0;
    }
  }

  //
  // relay methods:
  //

  void Command::parseFeatures( const Response & r ) {
    mSMTP->parseFeatures( r );
  }

  int Command::startTLS() {
    return mSMTP->startTLS();
  }

  bool Command::usingSSL() const {
    return mSMTP->usingSSL();
  }

  bool Command::usingTLS() const {
    return mSMTP->usingTLS();
  }

  bool Command::haveCapability( const char * cap ) const {
    return mSMTP->haveCapability( cap );
  }

  //
  // EHLO / HELO
  //

  TQCString EHLOCommand::nextCommandLine( TransactionState * ) {
    mNeedResponse = true;
    mComplete = mEHLONotSupported;
    const char * cmd = mEHLONotSupported ? "HELO " : "EHLO " ;
    return cmd + KIDNA::toAsciiCString( mHostname ) + "\r\n";
  }

  bool EHLOCommand::processResponse( const Response & r, TransactionState * ) {
    mNeedResponse = false;
    // "command not {recognized,implemented}" response:
    if ( r.code() == 500 || r.code() == 502 ) {
      if ( mEHLONotSupported ) { // HELO failed...
	mSMTP->error( TDEIO::ERR_INTERNAL_SERVER,
		      i18n("The server rejected both EHLO and HELO commands "
			   "as unknown or unimplemented.\n"
			   "Please contact the server's system administrator.") );
	return false;
      }
      mEHLONotSupported = true; // EHLO failed, but that's ok.
      return true;
    }
    mComplete = true;
    if ( r.code() / 10 ==  25 ) { // 25x: success
      parseFeatures( r );
      return true;
    }
    mSMTP->error( TDEIO::ERR_UNKNOWN,
		  i18n("Unexpected server response to %1 command.\n%2")
		  .arg( mEHLONotSupported ? "HELO" : "EHLO" )
		  .arg( r.errorMessage() ) );
    return false;
  }

  //
  // STARTTLS - rfc 3207
  //

  TQCString StartTLSCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "STARTTLS\r\n";
  }

  bool StartTLSCommand::processResponse( const Response & r, TransactionState * ) {
    mNeedResponse = false;
    if ( r.code() != 220 ) {
      mSMTP->error( r.errorCode(),
		    i18n("Your SMTP server does not support TLS. "
			 "Disable TLS, if you want to connect "
			 "without encryption.") );
      return false;
    }

    int tlsrc = startTLS();

    if ( tlsrc == 1 )
      return true;

    if ( tlsrc != -3 )
      //kdDebug(7112) << "TLS negotiation failed!" << endl;
      mSMTP->messageBox(TDEIO::SlaveBase::Information,
			i18n("Your SMTP server claims to "
			     "support TLS, but negotiation "
			     "was unsuccessful.\nYou can "
			     "disable TLS in TDE using the "
			     "crypto settings module."),
			i18n("Connection Failed"));
    return false;
  }


#define SASLERROR mSMTP->error(TDEIO::ERR_COULD_NOT_AUTHENTICATE, \
  i18n("An error occured during authentication: %1").arg \
  ( TQString::fromUtf8( sasl_errdetail( conn ) )));

  //
  // AUTH - rfc 2554
  //
  AuthCommand::AuthCommand( SMTPProtocol * smtp,
			    const char *mechanisms,
          const TQString &aFQDN,
			    TDEIO::AuthInfo &ai )
    : Command( smtp, CloseConnectionOnError|OnlyLastInPipeline ),
      mAi( &ai ),
      mFirstTime( true )
  {
#ifdef HAVE_LIBSASL2
    int result;
    mMechusing = 0;
    conn = 0;
    client_interact = 0;
    mOut = 0; mOutlen = 0;
    mOneStep = false;

    result = sasl_client_new( "smtp", aFQDN.latin1(),
      0, 0, callbacks, 0, &conn );
    if ( result != SASL_OK ) {
      SASLERROR
      return;
    }
    do {
      result = sasl_client_start(conn, mechanisms,
        &client_interact, &mOut, &mOutlen, &mMechusing);

      if (result == SASL_INTERACT)
        if ( !saslInteract( client_interact ) ) {
          return;
        };
    } while ( result == SASL_INTERACT );
    if ( result != SASL_CONTINUE && result != SASL_OK ) {
      SASLERROR
      return;
    }
    if ( result == SASL_OK ) mOneStep = true;
    kdDebug(7112) << "Mechanism: " << mMechusing << " one step: " << mOneStep << endl;
#else
  mSMTP->error(TDEIO::ERR_COULD_NOT_AUTHENTICATE,
      i18n("Authentication support is not compiled into tdeio_smtp."));
#endif
  }

  AuthCommand::~AuthCommand()
  {
#ifdef HAVE_LIBSASL2
    if ( conn ) {
      kdDebug(7112) << "dispose sasl connection" << endl;
      sasl_dispose( &conn );
      conn = 0;
    }
#endif
  }

  bool AuthCommand::saslInteract( void *in )
  {
#ifdef HAVE_LIBSASL2
    kdDebug(7112) << "saslInteract: " << endl;
    sasl_interact_t *interact = ( sasl_interact_t * ) in;

    //some mechanisms do not require username && pass, so don't need a popup
    //window for getting this info
    for ( ; interact->id != SASL_CB_LIST_END; interact++ ) {
      if ( interact->id == SASL_CB_AUTHNAME ||
           interact->id == SASL_CB_PASS ) {

        if ( mAi->username.isEmpty() || mAi->password.isEmpty()) {
          if (!mSMTP->openPassDlg(*mAi)) {
            mSMTP->error(TDEIO::ERR_ABORTED, i18n("No authentication details supplied."));
            return false;
          }
        }
        break;
      }
    }

    interact = ( sasl_interact_t * ) in;
    while( interact->id != SASL_CB_LIST_END ) {
      switch( interact->id ) {
        case SASL_CB_USER:
        case SASL_CB_AUTHNAME:
          kdDebug(7112) << "SASL_CB_[USER|AUTHNAME]: " << mAi->username << endl;
          interact->result = strdup( mAi->username.utf8() );
          interact->len = strlen( (const char *) interact->result );
          break;
        case SASL_CB_PASS:
          kdDebug(7112) << "SASL_CB_PASS: [HIDDEN]" << endl;
          interact->result = strdup( mAi->password.utf8() );
          interact->len = strlen( (const char *) interact->result );
          break;
        default:
          interact->result = NULL; interact->len = 0;
          break;
      }
      interact++;
    }
    return true;
#else
    return false;
#endif
  }

  bool AuthCommand::doNotExecute( const TransactionState * ) const {
    return !mMechusing;
  }

  void AuthCommand::ungetCommandLine( const TQCString & s, TransactionState * ) {
    mUngetSASLResponse = s;
    mComplete = false;
  }

  TQCString AuthCommand::nextCommandLine( TransactionState * ) {
    mNeedResponse = true;
    TQCString cmd;
#ifdef HAVE_LIBSASL2
    TQByteArray tmp, challenge;
    if ( !mUngetSASLResponse.isNull() ) {
      // implement un-ungetCommandLine
      cmd = mUngetSASLResponse;
      mUngetSASLResponse = 0;
    } else if ( mFirstTime ) {
      TQString firstCommand = "AUTH " + TQString::fromLatin1( mMechusing );

      tmp.setRawData( mOut, mOutlen );
      KCodecs::base64Encode( tmp, challenge );
      tmp.resetRawData( mOut, mOutlen );
      if ( !challenge.isEmpty() ) {
        firstCommand += " ";
        firstCommand += TQString::fromLatin1( challenge.data(), challenge.size() );
      }
      cmd = firstCommand.latin1();

      if ( mOneStep ) mComplete = true;
    } else {
//      kdDebug(7112) << "SS: '" << mLastChallenge << "'" << endl;
      tmp.setRawData( mLastChallenge.data(), mLastChallenge.length() );
      KCodecs::base64Decode( tmp, challenge );
      tmp.resetRawData( mLastChallenge.data(), mLastChallenge.length() );
      int result;
      do {
        result = sasl_client_step(conn, challenge.isEmpty() ? 0 : challenge.data(),
                                  challenge.size(),
                                  &client_interact,
                                  &mOut, &mOutlen);
        if (result == SASL_INTERACT)
          if ( !saslInteract( client_interact ) ) {
            return "";
          };
      } while ( result == SASL_INTERACT );
      if ( result != SASL_CONTINUE && result != SASL_OK ) {
        kdDebug(7112) << "sasl_client_step failed with: " << result << endl;
        SASLERROR
        return "";
      }
      tmp.setRawData( mOut, mOutlen );
      cmd = KCodecs::base64Encode( tmp );
      tmp.resetRawData( mOut, mOutlen );

//      kdDebug(7112) << "CC: '" << cmd << "'" << endl;
      mComplete = ( result == SASL_OK );
    }
#endif //HAVE_LIBSASL2
    cmd += "\r\n";
    return cmd;
  }

  bool AuthCommand::processResponse( const Response & r, TransactionState * ) {
    if ( !r.isOk() ) {
      if ( mFirstTime )
	      if ( haveCapability( "AUTH" ) )
          mSMTP->error( TDEIO::ERR_COULD_NOT_LOGIN,
            i18n("Your SMTP server does not support %1.\nChoose a different authentication method.\n%2")
              .arg( mMechusing ).arg( r.errorMessage() ) );
	      else
	        mSMTP->error( TDEIO::ERR_COULD_NOT_LOGIN,
			      i18n("Your SMTP server does not support authentication.\n"
			     "  %2").arg( r.errorMessage() ) );
      else
	      mSMTP->error( TDEIO::ERR_COULD_NOT_LOGIN,
		      i18n("Authentication failed.\n"
			   "Most likely the password is wrong.\n"
			   "%1").arg( r.errorMessage() ) );
      return false;
    }
    mFirstTime = false;
    mLastChallenge = r.lines().front(); // ### better join all lines with \n?
    mNeedResponse = false;
    return true;
  }

  //
  // MAIL FROM:
  //

  TQCString MailFromCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    TQCString cmdLine = "MAIL FROM:<" + mAddr + '>';
    if ( m8Bit && haveCapability("8BITMIME") )
      cmdLine += " BODY=8BITMIME";
    if ( mSize && haveCapability("SIZE") )
      cmdLine += " SIZE=" + TQCString().setNum( mSize );
    return cmdLine + "\r\n";
  }

  bool MailFromCommand::processResponse( const Response & r, TransactionState * ts ) {
    assert( ts );
    mNeedResponse = false;

    if ( r.code() == 250 )
      return true;

    ts->setMailFromFailed( mAddr, r );
    return false;
  }

  //
  // RCPT TO:
  //

  TQCString RcptToCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "RCPT TO:<" + mAddr + ">\r\n";
  }

  bool RcptToCommand::processResponse( const Response & r, TransactionState * ts ) {
    assert( ts );
    mNeedResponse = false;

    if ( r.code() == 250 ) {
      ts->setRecipientAccepted();
      return true;
    }

    ts->addRejectedRecipient( mAddr, r.errorMessage() );
    return false;
  }

  //
  // DATA (only initial processing!)
  //

  TQCString DataCommand::nextCommandLine( TransactionState * ts ) {
    assert( ts );
    mComplete = true;
    mNeedResponse = true;
    ts->setDataCommandIssued( true );
    return "DATA\r\n";
  }

  void DataCommand::ungetCommandLine( const TQCString &, TransactionState * ts ) {
    assert( ts );
    mComplete = false;
    ts->setDataCommandIssued( false );
  }

  bool DataCommand::processResponse( const Response & r, TransactionState * ts ) {
    assert( ts );
    mNeedResponse = false;

    if ( r.code() == 354 ) {
      ts->setDataCommandSucceeded( true, r );
      return true;
    }

    ts->setDataCommandSucceeded( false, r );
    return false;
  }

  //
  // DATA (data transfer)
  //
  void TransferCommand::ungetCommandLine( const TQCString & cmd, TransactionState * ) {
    if ( cmd.isEmpty() )
      return; // don't change state when we can't detect the unget in
	      // the next nextCommandLine !!
    mWasComplete = mComplete;
    mComplete = false;
    mNeedResponse = false;
    mUngetBuffer = cmd;
  }

  bool TransferCommand::doNotExecute( const TransactionState * ts ) const {
    assert( ts );
    return ts->failed();
  }

  TQCString TransferCommand::nextCommandLine( TransactionState * ts ) {
    assert( ts ); // let's rely on it ( at least for the moment )
    assert( !isComplete() );
    assert( !ts->failed() );

    static const TQCString dotCRLF = ".\r\n";
    static const TQCString CRLFdotCRLF = "\r\n.\r\n";

    if ( !mUngetBuffer.isEmpty() ) {
      const TQCString ret = mUngetBuffer;
      mUngetBuffer = 0;
      if ( mWasComplete ) {
	mComplete = true;
	mNeedResponse = true;
      }
      return ret; // don't prepare(), it's slave-generated or already prepare()d
    }

    // normal processing:

    kdDebug(7112) << "requesting data" << endl;
    mSMTP->dataReq();
    TQByteArray ba;
    int result = mSMTP->readData( ba );
    kdDebug(7112) << "got " << result << " bytes" << endl;
    if ( result > 0 )
      return prepare( ba );
    else if ( result < 0 ) {
      ts->setFailedFatally( TDEIO::ERR_INTERNAL,
			    i18n("Could not read data from application.") );
      mComplete = true;
      mNeedResponse = true;
      return 0;
    }
    mComplete = true;
    mNeedResponse = true;
    return mLastChar == '\n' ? dotCRLF : CRLFdotCRLF ;
  }

  bool TransferCommand::processResponse( const Response & r, TransactionState * ts ) {
    mNeedResponse = false;
    assert( ts );
    ts->setComplete();
    if ( !r.isOk() ) {
      ts->setFailed();
      mSMTP->error( r.errorCode(),
		    i18n("The message content was not accepted.\n"
			 "%1").arg( r.errorMessage() ) );
      return false;
    }
    return true;
  }

  static TQCString dotstuff_lf2crlf( const TQByteArray & ba, char & last ) {
    TQCString result( ba.size() * 2 + 1 ); // worst case: repeated "[.]\n"
    const char * s = ba.data();
    const char * const send = ba.data() + ba.size();
    char * d = result.data();

    while ( s < send ) {
      const char ch = *s++;
      if ( ch == '\n' && last != '\r' )
	*d++ = '\r'; // lf2crlf
      else if ( ch == '.' && last == '\n' )
	*d++ = '.'; // dotstuff
      last = *d++ = ch;
    }

    result.truncate( d - result.data() );
    return result;
  }

  TQCString TransferCommand::prepare( const TQByteArray & ba ) {
    if ( ba.isEmpty() )
      return 0;
    if ( mSMTP->metaData("lf2crlf+dotstuff") == "slave" ) {
      kdDebug(7112) << "performing dotstuffing and LF->CRLF transformation" << endl;
      return dotstuff_lf2crlf( ba, mLastChar );
    } else {
      mLastChar = ba[ ba.size() - 1 ];
      return TQCString( ba.data(), ba.size() + 1 );
    }
  }

  //
  // NOOP
  //

  TQCString NoopCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "NOOP\r\n";
  }

  //
  // RSET
  //

  TQCString RsetCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "RSET\r\n";
  }

  //
  // QUIT
  //

  TQCString QuitCommand::nextCommandLine( TransactionState * ) {
    mComplete = true;
    mNeedResponse = true;
    return "QUIT\r\n";
  }

} // namespace KioSMTP

