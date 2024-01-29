/*
 * Copyright (c) 2001  Lucas Fisher       <ljfisher@purdue.edu>
 * Copyright (c) 2009  Andreas Schneider  <mail@cynapses.org>
 * Copyright (c) 2020  Martin Sandsmark   <martin@sandsmark.ninja>
 *                     KDE2 port
 * Copyright (c) 2022  Mavridis Philippe  <mavridisf@gmail.com>
 *                     Trinity port
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License (LGPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __tdeio_sftp_h__
#define __tdeio_sftp_h__

#include <kurl.h>
#include <tdeio/global.h>
#include <tdeio/slavebase.h>
#include <kdebug.h>
#include <stdint.h>
#include <memory>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <libssh/callbacks.h>

// How big should each data packet be? Definitely not bigger than 64kb or
// you will overflow the 2 byte size variable in a sftp packet.
#define MAX_XFER_BUF_SIZE 60 * 1024
#define TDEIO_SFTP_DB 7120


#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 7, 90)
#define TDEIO_SSH_KNOWN_HOSTS_OK        SSH_SERVER_KNOWN_OK
#define TDEIO_SSH_KNOWN_HOSTS_OTHER     SSH_SERVER_FOUND_OTHER
#define TDEIO_SSH_KNOWN_HOSTS_CHANGED   SSH_SERVER_KNOWN_CHANGED
#define TDEIO_SSH_KNOWN_HOSTS_NOT_FOUND SSH_SERVER_FILE_NOT_FOUND
#define TDEIO_SSH_KNOWN_HOSTS_UNKNOWN   SSH_SERVER_NOT_KNOWN
#define TDEIO_SSH_KNOWN_HOSTS_ERROR     SSH_SERVER_ERROR

#else
#define TDEIO_SSH_KNOWN_HOSTS_OK        SSH_KNOWN_HOSTS_OK
#define TDEIO_SSH_KNOWN_HOSTS_OTHER     SSH_KNOWN_HOSTS_OTHER
#define TDEIO_SSH_KNOWN_HOSTS_CHANGED   SSH_KNOWN_HOSTS_CHANGED
#define TDEIO_SSH_KNOWN_HOSTS_NOT_FOUND SSH_KNOWN_HOSTS_NOT_FOUND
#define TDEIO_SSH_KNOWN_HOSTS_UNKNOWN   SSH_KNOWN_HOSTS_UNKNOWN
#define TDEIO_SSH_KNOWN_HOSTS_ERROR     SSH_KNOWN_HOSTS_ERROR
#endif

namespace TDEIO {
  class AuthInfo;
}

class sftpProtocol : public TDEIO::SlaveBase
{

public:
  sftpProtocol(const TQCString &pool_socket, const TQCString &app_socket);
  virtual ~sftpProtocol();
  virtual void setHost(const TQString& h, int port, const TQString& user, const TQString& pass) override;
  virtual void get(const KURL& url) override;
  virtual void listDir(const KURL& url)  override;
  virtual void mimetype(const KURL& url) override;
  virtual void stat(const KURL& url) override;
  virtual void put(const KURL& url, int permissions, bool overwrite, bool resume) override;
  virtual void copy(const KURL &src, const KURL &dest, int permissions, bool overwrite) override;
  virtual void closeConnection() override;
  virtual void slave_status() override;
  virtual void del(const KURL &url, bool isfile) override;
  virtual void chmod(const KURL& url, int permissions) override;
  virtual void symlink(const TQString& target, const KURL& dest, bool overwrite) override;
  virtual void rename(const KURL& src, const KURL& dest, bool overwrite) override;
  virtual void mkdir(const KURL& url, int permissions) override;
  virtual void openConnection() override;

  // libssh authentication callback (note that this is called by the
  // global ::auth_callback() call.
  int auth_callback(const char *prompt, char *buf, size_t len,
                    int echo, int verify, void *userdata);

  // libssh logging callback (note that this is called by the
  // global ::log_callback() call.
  void log_callback(ssh_session session, int priority, const char *message,
                    void *userdata);

  // Callbacks for SSHAuthMethod-derived strategies
  int authenticatePublicKey();
  int authenticateKeyboardInteractive(bool noPaswordQuery = false);
  int authenticatePassword(bool noPaswordQuery = false);

  /** Some extra authentication failure reasons intended to use alongside was declared in libssh */
  enum extra_ssh_auth_e {
    SSH_AUTH_CANCELED=128,   //< user canceled password entry dialog
    SSH_AUTH_NEED_RECONNECT  //< it is required to reinitialize connection from scratch
  };

private: // Private variables
  /** True if ioslave is connected to sftp server. */
  bool mConnected;

  /** Host we are connected to. */
  TQString mHost;

  /** Port we are connected to. */
  int mPort;

  /** The ssh session for the connection */
  ssh_session mSession;

  /** The sftp session for the connection */
  sftp_session mSftp;

  /** Username to use when connecting, Note: it's the one passed in the URL */
  TQString mUsername;

  /** Username to use with the next connection attempt: it's either from the cached data or from
   * the password dialog that was prompted to the user. */
  TQString mCachedUsername;

  /** User's password. Note: the password would be set only if it was somehow cached: passed to
   * setHost(), received from passwdserver's cache or was entered by user before reconnection
   */
  TQString mPassword;

  /** The open file */
  sftp_file mOpenFile;

  /** The open URL */
  KURL mOpenUrl;

  ssh_callbacks mCallbacks;

  /** Version of the sftp protocol we are using. */
  int sftpVersion;

  //struct Status
  //{
  //  int code;
  //  TDEIO::filesize_t size;
  //  TQString text;
  //};

  /** Some data needed to interact with auth_callback() */
  struct {
    /** List of keys user was already prompted to enter the passphrase for.
     *  Note: Under most sane circumstances the list shouldn't go beyond size=2,
     *        so no fancy containers here
     */
    TQStringList attemptedKeys;
    /** A backup for SlaveBase::s_seqNr to pass the same value to prompts for different keys */
    long current_seqNr;
    /** true if callback was called */
    bool wasCalled;
    /** true if user canceled all passphrase entry dialogues */
    bool wasCanceled;
  } mPubKeyAuthData;

  /** true if the password dialog was prompted to the user at leas once */
  bool mPasswordWasPrompted = false;

private: // private methods
  void statMime(const KURL &url);
  void closeFile();

  /** @returns username used by libssh during the connection */
  TQString sshUsername();

  /** Adds ssh error (if any) to the given message string */
  TQString sshError(TQString errMsg=TQString());

  /** A small helper function to construct auth info skeleton for the protocol */
  TDEIO::AuthInfo authInfo();

  /** A helper function encapsulating creation of an ssh connection before authentication */
  int initializeConnection();

  void reportError(const KURL &url, const int err);

  bool createUDSEntry(const TQString &filename, const TQByteArray &path,
      TDEIO::UDSEntry &entry, short int details);

  TQString canonicalizePath(const TQString &path);
};

/** A base class for ssh authentication methods. */
class SSHAuthMethod {
public:
  /** libssh's flag for he method */
  virtual int flag() = 0;
  /** The user-friendly (probably translated) name of the method */
  virtual TQString name() = 0;
  /** Actually do perform the auth process */
  virtual int authenticate(sftpProtocol *ioslave) const = 0;
  /** Creates a copy of derived class */
  virtual SSHAuthMethod* clone() = 0;

  virtual ~SSHAuthMethod() {};
};

#endif
