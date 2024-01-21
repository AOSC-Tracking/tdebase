/*
 * Copyright (c) 2001  Lucas Fisher       <ljfisher@purdue.edu>
 * Copyright (c) 2009  Andreas Schneider  <mail@cynapses.org>
 * Copyright (c) 2020  Martin Sandsmark   <martin@sandsmark.ninja>
 *                     KDE2 port
 * Copyright (c) 2022  Mavridis Philippe  <mavridisf@gmail.com>
 *                     Trinity port
 *
 * Portions Copyright (c) 2020-2021 Harald Sitter <sitter@kde.org>
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

#include "tdeio_sftp.h"

#include <fcntl.h>

#include <tqapplication.h>
#include <tqfile.h>
#include <tqdir.h>

#include <functional>

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <tdeapplication.h>
#include <kdebug.h>
#include <tdemessagebox.h>
#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <tdelocale.h>
#include <kurl.h>
#include <tdeio/ioslave_defaults.h>
#include <kmimetype.h>
#include <kmimemagic.h>
#include <signal.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <libssh/callbacks.h>

#define TDEIO_SFTP_SPECIAL_TIMEOUT 30
#define ZERO_STRUCTP(x) do { if ((x) != NULL) memset((char *)(x), 0, sizeof(*(x))); } while(0)

using namespace TDEIO;
extern "C"
{
  int KDE_EXPORT kdemain( int argc, char **argv )
  {
    TDEInstance instance( "tdeio_sftp" );

    kdDebug(TDEIO_SFTP_DB) << "*** Starting tdeio_sftp " << endl;

    if (argc != 4) {
      kdDebug(TDEIO_SFTP_DB) << "Usage: tdeio_sftp  protocol domain-socket1 domain-socket2" << endl;
      exit(-1);
    }
    sftpProtocol slave(argv[2], argv[3]);

    if (getenv("DEBUG_TDEIO_SFTP")) {
        // Give us a coredump in the journal
        signal(6, SIG_DFL);
    }

    slave.dispatchLoop();

    kdDebug(TDEIO_SFTP_DB) << "*** tdeio_sftp Done" << endl;
    return 0;
  }
}

// Some helper functions/classes
namespace {

// A quick and dirty scope guard implementation
class ExitGuard {
public:
  template<class Callable>
  ExitGuard(Callable && undo_func) : f(std::forward<Callable>(undo_func)) {}
  ExitGuard(ExitGuard && other) : f(std::move(other.f)) {
      other.f = nullptr;
  }

  ~ExitGuard() {
    run();
  }

  void run() noexcept {
    if(f) { f(); f = nullptr; }
  }

  void abort() {
    f = nullptr;
  }

  ExitGuard(const ExitGuard&) = delete;
  void operator= (const ExitGuard&) = delete;

private:
    std::function<void()> f;
};

// A small helper to purge passwords. Paranoiac's note: this is not enough to guarantee the
// complete purge of the password and all its copy from memory (ioslaves are sending the passwords
// via dcop, so it's far beyond calling it "secure" in any way), but it's still better than nothing.
void purgeString(TQString &s) {
  s.fill('\0');
  s.setLength(0);
  s = TQString::null;
}

// A helper class to cleanup password when it goes out of the scope
class PasswordPurger: public ExitGuard {
public:
  PasswordPurger(TQString &pw) : ExitGuard( [&pw](){purgeString(pw);} ) {}
};

} /* namespace */

// The callback function for libssh
int auth_callback(const char *prompt, char *buf, size_t len,
                  int echo, int verify, void *userdata)
{
  if (userdata == NULL) {
    return -1;
  }

  sftpProtocol *slave = (sftpProtocol *) userdata;

  if (slave->auth_callback(prompt, buf, len, echo, verify, userdata) < 0) {
    return -1;
  }

  return 0;
}

void log_callback(ssh_session session, int priority, const char *message,
    void *userdata) {
  if (userdata == NULL) {
    return;
  }

  sftpProtocol *slave = (sftpProtocol *) userdata;

  slave->log_callback(session, priority, message, userdata);
}

// Public key authentication
int sftpProtocol::auth_callback(const char *prompt, char *buf, size_t len,
                                int echo, int verify, void *userdata)
{
  // unused variables
  (void) echo;
  (void) verify;
  (void) userdata;
  (void) prompt;

  Q_ASSERT(len>0);

  kdDebug(TDEIO_SFTP_DB) << "Entering public key authentication callback" << endl;

  int rc=0;
  mPubKeyAuthData.wasCalled = true;

  AuthInfo pubKeyInfo = authInfo();

  pubKeyInfo.readOnly     = false;
  pubKeyInfo.keepPassword = false; // don't save passwords for public key,
                                   // that's the task of ssh-agent.
  TQString errMsg;
  TQString keyFile;
#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 10, 0)
  // no way to determine keyfile name on older libssh
#else
  char *ssh_key_file = 0;
  rc = ssh_userauth_publickey_auto_get_current_identity(mSession, &ssh_key_file);

  if (rc == 0 && ssh_key_file && ssh_key_file[0]) {
    keyFile = ssh_key_file;
  }
  ssh_string_free_char(ssh_key_file);
#endif

  bool firstTry = !mPubKeyAuthData.attemptedKeys.contains(keyFile);

  if (!firstTry) {
    errMsg = i18n("Incorrect or invalid passphrase.");
  }

  // libssh prompt is trash and we know we use this function only for publickey auth, so we'll give
  // the user a descent prompt
  if (!keyFile.isEmpty()) {
      pubKeyInfo.prompt = i18n("Please enter the passphrase for next public key:\n%1").arg(keyFile);
  } else { // Generally shouldn't happend but on older libssh
      pubKeyInfo.prompt = i18n("Please enter the passphrase for your public key.");
  }

  // We don't want to clobber with normal passwords in kpasswdserver's cache
  pubKeyInfo.realmValue = "keyfile passphrase:" + keyFile;

  if (openPassDlg(pubKeyInfo, errMsg)) {
    if (len < pubKeyInfo.password.utf8().length()+1) {
      kdDebug(TDEIO_SFTP_DB) << "Insufficient buffer size for password: " << len
        << " (" << pubKeyInfo.password.utf8().length()+1 << "needed)" << endl;
    }

    strncpy(buf, pubKeyInfo.password.utf8().data(), len-1);
    buf[len-1]=0; // Just to be on the safe side

    purgeString(pubKeyInfo.password);
  } else {
    kdDebug(TDEIO_SFTP_DB) << "User canceled entry of public key passphrase" << endl;
    rc = -1;
    mPubKeyAuthData.wasCanceled = true;
  }

  // take a note that we already tried unlocking this keyfile
  if(firstTry) {
    mPubKeyAuthData.attemptedKeys.append(keyFile);
  }

  return rc;
}

void sftpProtocol::log_callback(ssh_session session, int priority,
    const char *message, void *userdata) {
  (void) session;
  (void) userdata;

  kdDebug(TDEIO_SFTP_DB) << "[" << priority << "] " << message << endl;
}

int sftpProtocol::authenticateKeyboardInteractive(AuthInfo &info) {
  TQString name, instruction, prompt;
  int err = SSH_AUTH_ERROR;

  kdDebug(TDEIO_SFTP_DB) << "Entering keyboard interactive function" << endl;

  while (1) {
    int n = 0;
    int i = 0;

    err = ssh_userauth_kbdint(mSession, NULL, NULL);

    if (err != SSH_AUTH_INFO) {
      kdDebug(TDEIO_SFTP_DB) << "Finishing kbdint auth err=" << err
        << " ssh_err=" << ssh_get_error_code(mSession)
        << " (" <<  ssh_get_error(mSession) << ")" << endl;

      break;
    }

    name = TQString::fromUtf8(ssh_userauth_kbdint_getname(mSession));
    instruction = TQString::fromUtf8(ssh_userauth_kbdint_getinstruction(mSession));
    n = ssh_userauth_kbdint_getnprompts(mSession);

    kdDebug(TDEIO_SFTP_DB) << "name=" << name << " instruction=" << instruction
      << " prompts" << n << endl;

    for (i = 0; i < n; ++i) {
      // See RFC4256 Section 3.3 User Interface
      char echo;
      TQString answer;

      prompt = TQString::fromUtf8(ssh_userauth_kbdint_getprompt(mSession, i, &echo));
      kdDebug(TDEIO_SFTP_DB) << "prompt=" << prompt << " echo=" << TQString::number(echo) << endl;

      TDEIO::AuthInfo infoKbdInt(info);
      infoKbdInt.keepPassword = false;

      if (!name.isEmpty()) {
        infoKbdInt.caption = TQString(i18n("SFTP Login") + " - " + name);
      }

      // Those strings might or might not contain some sensitive information
      PasswordPurger answerPurger{answer};
      PasswordPurger infoPurger{infoKbdInt.password};

      if (!echo) {
        // ssh server requests us to ask user a question without displaying an answer. In normal
        // circumstances this is probably a password, but it might be something else depending
        // on the server configuration.
        bool isPassword = false;
        if (prompt.lower().startsWith("password")) {
          // We can assume that the ssh server asks for a password and we want a more
          // user-friendly prompt in that case
          infoKbdInt.prompt = i18n("Please enter your password.");
          isPassword = true;
          infoKbdInt.keepPassword = true;
          if (!mPassword.isNull()) { // if we have a cached password we might use it
            kdDebug(TDEIO_SFTP_DB) << "Using cached password" << endl;
            answer = mPassword;
          }
        } else {
          // If the server's request doesn't look like a password, keep the servers prompt and
          // don't bother saving it
          infoKbdInt.prompt = prompt;
        }

        infoKbdInt.readOnly = true; // set username readonly
        /* FIXME: We can query a new user name but we will have to reinitialize the connection if
         * it changes <2024-01-10 Fat-Zer> */
        if (answer.isNull()) {
          if (openPassDlg(infoKbdInt)) {
            kdDebug(TDEIO_SFTP_DB) << "Got the answer from the password dialog" << endl;
            answer = infoKbdInt.password;
            if(isPassword) {
              info.password = infoKbdInt.password; // return the answer to the caller
              info.setModified( true );
            }
          } else {
            /* FIXME: Some reasonable action upon cancellation? <2024-01-10 Fat-Zer> */
          }
        }
      } else {
        // ssh server asks for some clear-text information from a user (e.g. a one-time
        // identification code) which should be echoed while user enters it. As for now tdeio has
        // no means of handle that correctly, so we will have to be creative with the password
        // dialog.
        TQString newPrompt;

        if (!instruction.isEmpty()) {
          newPrompt = instruction + "\n\n";
        }
        newPrompt.append(prompt + "\n\n");
        newPrompt.append(i18n("Use the username input field to answer this question."));
        infoKbdInt.prompt = newPrompt;

        infoKbdInt.readOnly = false;

        if (openPassDlg(infoKbdInt)) {
          answer = infoKbdInt.username;
          kdDebug(TDEIO_SFTP_DB) << "Got the answer from the password dialog: " << answer << endl;
        } else {
          /* FIXME: Some reasonable action upon cancellation? <2024-01-10 Fat-Zer> */
        }
      }

      if (ssh_userauth_kbdint_setanswer(mSession, i, answer.utf8().data()) < 0) {
        kdDebug(TDEIO_SFTP_DB) << "An error occurred setting the answer: "
          << ssh_get_error(mSession) << endl;
        /* FIXME: display the error to the user <2024-01-10 Fat-Zer> */
        return SSH_AUTH_ERROR;
      }
    } // for each ssh_userauth_kbdint_getprompt()
  } // while (1)

  return err;
}

TDEIO::AuthInfo sftpProtocol::authInfo() {
  TDEIO::AuthInfo rv;

  rv.url.setProtocol("sftp");
  rv.url.setHost(mHost);
  rv.url.setPort(mPort);
  rv.url.setUser(mUsername);

  rv.caption = i18n("SFTP Login");
  rv.comment = "sftp://" + mHost + ':' + TQString::number(mPort);
  rv.commentLabel = i18n("site:");
  rv.username = mUsername;

  return rv;
}

void sftpProtocol::reportError(const KURL &url, const int err) {
  kdDebug(TDEIO_SFTP_DB) << "url = " << url.url() << " - err=" << err << endl;

  switch (err) {
    case SSH_FX_OK:
      break;
    case SSH_FX_NO_SUCH_FILE:
    case SSH_FX_NO_SUCH_PATH:
      error(TDEIO::ERR_DOES_NOT_EXIST, url.prettyURL());
      break;
    case SSH_FX_PERMISSION_DENIED:
      error(TDEIO::ERR_ACCESS_DENIED, url.prettyURL());
      break;
    case SSH_FX_FILE_ALREADY_EXISTS:
      error(TDEIO::ERR_FILE_ALREADY_EXIST, url.prettyURL());
      break;
    case SSH_FX_INVALID_HANDLE:
      error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
      break;
    case SSH_FX_OP_UNSUPPORTED:
      error(TDEIO::ERR_UNSUPPORTED_ACTION, url.prettyURL());
      break;
    case SSH_FX_BAD_MESSAGE:
      error(TDEIO::ERR_UNKNOWN, url.prettyURL());
      break;
    default:
      error(TDEIO::ERR_INTERNAL, url.prettyURL());
      break;
  }
}

bool sftpProtocol::createUDSEntry(const TQString &filename, const TQByteArray &path,
      UDSEntry &entry, short int details) {
  mode_t type;
  mode_t access;
  char *link;

  Q_ASSERT(entry.count() == 0);

  sftp_attributes sb = sftp_lstat(mSftp, path.data());
  if (sb == NULL) {
    return false;
  }

  UDSAtom atom;
  atom.m_uds = UDS_NAME;
  atom.m_str = filename;
  entry.append(atom);

  if (sb->type == SSH_FILEXFER_TYPE_SYMLINK) {
    atom.m_uds = UDS_FILE_TYPE;
    atom.m_long = S_IFREG;
    entry.append(atom);
    link = sftp_readlink(mSftp, path.data());
    if (link == NULL) {
      sftp_attributes_free(sb);
      return false;
    }
    atom.m_uds = UDS_LINK_DEST;
    atom.m_str = TQFile::decodeName(link);
    entry.append(atom);
    delete link;
    // A symlink -> follow it only if details > 1
    if (details > 1) {
      sftp_attributes sb2 = sftp_stat(mSftp, path.data());
      if (sb2 == NULL) {
        // It is a link pointing to nowhere
        type = S_IFMT - 1;
        access = S_IRWXU | S_IRWXG | S_IRWXO;
        atom.m_uds = UDS_FILE_TYPE;
        atom.m_long = type;
        entry.append(atom);

        atom.m_uds = UDS_ACCESS;
        atom.m_long = access;
        entry.append(atom);

        atom.m_uds = UDS_SIZE;
        atom.m_long = 0LL;
        entry.append(atom);

        goto notype;
      }
      sftp_attributes_free(sb);
      sb = sb2;
    }
  }

  switch (sb->type) {
    case SSH_FILEXFER_TYPE_REGULAR:
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_long = S_IFREG;
      entry.append(atom);
      break;
    case SSH_FILEXFER_TYPE_DIRECTORY:
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_long = S_IFDIR;
      entry.append(atom);
      break;
    case SSH_FILEXFER_TYPE_SYMLINK:
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_long = S_IFLNK;
      entry.append(atom);
      break;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
      atom.m_uds = UDS_FILE_TYPE;
      atom.m_long = S_IFMT - 1;
      entry.append(atom);
      break;
  }

  access = sb->permissions & 07777;
  atom.m_uds = UDS_ACCESS;
  atom.m_long = access;
  entry.append(atom);

  atom.m_uds = UDS_SIZE;
  atom.m_long = sb->size;
  entry.append(atom);

notype:
  if (details > 0) {
    if (sb->owner) {
      atom.m_uds = UDS_USER;
      atom.m_str = TQString::fromUtf8(sb->owner);
      entry.append(atom);
    } else {
      atom.m_uds = UDS_USER;
      atom.m_str = TQString::number(sb->uid);
      entry.append(atom);
    }

    if (sb->group) {
      atom.m_uds = UDS_GROUP;
      atom.m_str = TQString::fromUtf8(sb->group);
      entry.append(atom);
    } else {
      atom.m_uds = UDS_GROUP;
      atom.m_str = TQString::number(sb->gid);
      entry.append(atom);
    }
    atom.m_uds = UDS_ACCESS_TIME;
    atom.m_long = sb->atime;
    entry.append(atom);

    atom.m_uds = UDS_MODIFICATION_TIME;
    atom.m_long = sb->mtime;
    entry.append(atom);

    atom.m_uds = UDS_MODIFICATION_TIME;
    atom.m_long = sb->createtime;
    entry.append(atom);
  }

  sftp_attributes_free(sb);

  return true;
}

TQString sftpProtocol::canonicalizePath(const TQString &path) {
  kdDebug(TDEIO_SFTP_DB) << "Path to canonicalize: " << path << endl;
  TQString cPath;
  char *sPath = NULL;

  if (path.isEmpty()) {
    return cPath;
  }

  sPath = sftp_canonicalize_path(mSftp, path.utf8().data());
  if (sPath == NULL) {
    kdDebug(TDEIO_SFTP_DB) << "Could not canonicalize path: " << path << endl;
    return cPath;
  }

  cPath = TQFile::decodeName(sPath);
  delete sPath;

  kdDebug(TDEIO_SFTP_DB) << "Canonicalized path: " << cPath << endl;

  return cPath;
}

sftpProtocol::sftpProtocol(const TQCString &pool_socket, const TQCString &app_socket)
             : SlaveBase("tdeio_sftp", pool_socket, app_socket),
                  mConnected(false), mPort(-1), mSession(NULL), mSftp(NULL) {
#ifndef TQ_WS_WIN
  kdDebug(TDEIO_SFTP_DB) << "pid = " << getpid() << endl;

  kdDebug(TDEIO_SFTP_DB) << "debug = " << getenv("TDEIO_SFTP_LOG_VERBOSITY") << endl;
#endif

  mCallbacks = (ssh_callbacks) malloc(sizeof(struct ssh_callbacks_struct));
  if (mCallbacks == NULL) {
    error(TDEIO::ERR_OUT_OF_MEMORY, i18n("Could not allocate callbacks"));
    return;
  }
  ZERO_STRUCTP(mCallbacks);

  mCallbacks->userdata = this;
  mCallbacks->auth_function = ::auth_callback;
  if (getenv("TDEIO_SFTP_LOG_VERBOSITY")) {
    mCallbacks->log_function = ::log_callback;
  }

  ssh_callbacks_init(mCallbacks);
}

sftpProtocol::~sftpProtocol() {
#ifndef TQ_WS_WIN
  kdDebug(TDEIO_SFTP_DB) << "pid = " << getpid() << endl;
#endif
  closeConnection();

  free(mCallbacks);

  /* cleanup and shut down cryto stuff */
  ssh_finalize();
}

void sftpProtocol::setHost(const TQString& h, int port, const TQString& user, const TQString& pass) {
  kdDebug(TDEIO_SFTP_DB) << "setHost(): " << user << "@" << h << ":" << port << endl;

  if (mConnected) {
    closeConnection();
  }

  mHost = h;

  if (port > 0) {
    mPort = port;
  } else {
    struct servent *pse;
    if ((pse = getservbyname("ssh", "tcp") ) == NULL) {
      mPort = 22;
    } else {
      mPort = ntohs(pse->s_port);
    }
  }

  kdDebug(TDEIO_SFTP_DB) << "setHost(): mPort=" << mPort << endl;

  mUsername = user;
  mPassword = pass;
}


int sftpProtocol::initializeConnection() {
  TQString msg;     // msg for dialog box
  TQString caption; // dialog box caption
  unsigned char *hash = NULL; // the server hash
  char *hexa;
  char *verbosity;
  int rc, state;
  int timeout_sec = 30, timeout_usec = 0;

  mSession = ssh_new();
  if (mSession == NULL) {
    error(TDEIO::ERR_INTERNAL, i18n("Could not create a new SSH session."));
    return SSH_ERROR;
  }

  kdDebug(TDEIO_SFTP_DB) << "Creating the SSH session and setting options" << endl;

  // Set timeout
  rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT, &timeout_sec);
  if (rc < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Could not set a timeout.";
  }
  rc = ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT_USEC, &timeout_usec);
  if (rc < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Could not set a timeout in usec.";
  }

  // Don't use any compression
  rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_C_S, "none");
  if (rc < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Could not set compression client <- server.";
  }

  rc = ssh_options_set(mSession, SSH_OPTIONS_COMPRESSION_S_C, "none");
  if (rc < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Could not set compression server -> client.";
  }

  // Set host and port
  rc = ssh_options_set(mSession, SSH_OPTIONS_HOST, mHost.utf8().data());
  if (rc < 0) {
    error(TDEIO::ERR_OUT_OF_MEMORY, i18n("Could not set host."));
    return SSH_ERROR;
  }

  if (mPort > 0) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_PORT, &mPort);
    if (rc < 0) {
        error(TDEIO::ERR_OUT_OF_MEMORY, i18n("Could not set port."));
      return SSH_ERROR;
    }
  }

  // Set the username
  if (!mUsername.isEmpty()) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_USER, mUsername.utf8().data());
    if (rc < 0) {
      error(TDEIO::ERR_OUT_OF_MEMORY, i18n("Could not set username."));
      return rc;
    }
  }

  verbosity = getenv("TDEIO_SFTP_LOG_VERBOSITY");
  if (verbosity) {
    rc = ssh_options_set(mSession, SSH_OPTIONS_LOG_VERBOSITY_STR, verbosity);
    if (rc < 0) {
      error(TDEIO::ERR_OUT_OF_MEMORY, i18n("Could not set log verbosity."));
      return rc;
    }
  }

  // Read ~/.ssh/config
  rc = ssh_options_parse_config(mSession, NULL);
  if (rc < 0) {
    error(TDEIO::ERR_INTERNAL, i18n("Could not parse the config file."));
    return rc;
  }

  ssh_set_callbacks(mSession, mCallbacks);

  kdDebug(TDEIO_SFTP_DB) << "Trying to connect to the SSH server" << endl;

  /* try to connect */
  rc = ssh_connect(mSession);
  if (rc < 0) {
    error(TDEIO::ERR_COULD_NOT_CONNECT, TQString::fromUtf8(ssh_get_error(mSession)));
    return rc;
  }

  ExitGuard connectionCloser([this](){ closeConnection(); });

  kdDebug(TDEIO_SFTP_DB) << "Getting the SSH server hash" << endl;

  /* get the hash */
  ssh_key serverKey;
#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 7, 90)
  rc = ssh_get_publickey(mSession, &serverKey);
#else
  rc = ssh_get_server_publickey(mSession, &serverKey);
#endif
  if (rc<0) {
    error(TDEIO::ERR_COULD_NOT_CONNECT, TQString::fromUtf8(ssh_get_error(mSession)));
    return rc;
  }

  size_t hlen;
#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 8, 90)
  rc = ssh_get_publickey_hash(serverKey, SSH_PUBLICKEY_HASH_MD5, &hash, &hlen);
#else
  rc = ssh_get_publickey_hash(serverKey, SSH_PUBLICKEY_HASH_SHA256, &hash, &hlen);
#endif
  if (rc<0) {
    error(TDEIO::ERR_COULD_NOT_CONNECT, TQString::fromUtf8(ssh_get_error(mSession)));
    return rc;
  }

  kdDebug(TDEIO_SFTP_DB) << "Checking if the SSH server is known" << endl;

  /* check the server public key hash */
#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 7, 90)
  state = ssh_is_server_known(mSession);
#else
  state = ssh_session_is_known_server(mSession);
#endif
  switch (state) {
    case TDEIO_SSH_KNOWN_HOSTS_OK:
      break;
    case TDEIO_SSH_KNOWN_HOSTS_OTHER:
      delete hash;
      error(TDEIO::ERR_CONNECTION_BROKEN, i18n("The host key for this server was "
            "not found, but another type of key exists.\n"
            "An attacker might change the default server key to confuse your "
            "client into thinking the key does not exist.\n"
            "Please contact your system administrator.\n%1").arg(TQString::fromUtf8(ssh_get_error(mSession))));
      return SSH_ERROR;
    case TDEIO_SSH_KNOWN_HOSTS_CHANGED:
      hexa = ssh_get_hexa(hash, hlen);
      delete hash;
      /* TODO print known_hosts file, port? */
      error(TDEIO::ERR_CONNECTION_BROKEN, i18n("The host key for the server %1 has changed.\n"
          "This could either mean that DNS SPOOFING is happening or the IP "
          "address for the host and its host key have changed at the same time.\n"
          "The fingerprint for the key sent by the remote host is:\n %2\n"
          "Please contact your system administrator.\n%3").arg(
          mHost).arg(TQString::fromUtf8(hexa)).arg(TQString::fromUtf8(ssh_get_error(mSession))));
      delete hexa;
      return SSH_ERROR;
    case TDEIO_SSH_KNOWN_HOSTS_NOT_FOUND:
    case TDEIO_SSH_KNOWN_HOSTS_UNKNOWN:
      hexa = ssh_get_hexa(hash, hlen);
      delete hash;
      caption = i18n("Warning: Cannot verify host's identity.");
      msg = i18n("The authenticity of host %1 cannot be established.\n"
        "The key fingerprint is: %2\n"
        "Are you sure you want to continue connecting?").arg(mHost).arg(hexa);
      delete hexa;

      if (KMessageBox::Yes != messageBox(WarningYesNo, msg, caption)) {
        error(TDEIO::ERR_USER_CANCELED, TQString());
        return SSH_ERROR;
      }

      /* write the known_hosts file */
      kdDebug(TDEIO_SFTP_DB) << "Adding server to known_hosts file." << endl;
#if LIBSSH_VERSION_INT < SSH_VERSION_INT(0, 7, 90)
      if (ssh_write_knownhost(mSession) != SSH_OK) {
#else
      if (ssh_session_update_known_hosts(mSession) != SSH_OK) {
#endif
        error(TDEIO::ERR_USER_CANCELED, TQString::fromUtf8(ssh_get_error(mSession)));
        return SSH_ERROR;
      }
      break;
    case TDEIO_SSH_KNOWN_HOSTS_ERROR:
      delete hash;
      error(TDEIO::ERR_COULD_NOT_CONNECT, TQString::fromUtf8(ssh_get_error(mSession)));
      return SSH_ERROR;
  }

  kdDebug(TDEIO_SFTP_DB) << "Trying to authenticate with the server" << endl;

  // If no username was set upon connection, get the name from connection
  // (probably it'd be the current user's name)
  if (mUsername.isEmpty()) {
    char *ssh_username = NULL;
    rc = ssh_options_get(mSession, SSH_OPTIONS_USER, &ssh_username);
    if (rc == 0 && ssh_username && ssh_username[0]) {
      mUsername = ssh_username;
    }
    ssh_string_free_char(ssh_username);
  }

  connectionCloser.abort();

  return SSH_OK;
}


void sftpProtocol::openConnection() {

  if (mConnected) {
    return;
  }

  kdDebug(TDEIO_SFTP_DB) << "username=" << mUsername << ", host=" << mHost << ", port=" << mPort << endl;

  infoMessage(i18n("Opening SFTP connection to host %1:%2").arg(mHost).arg(mPort));

  if (mHost.isEmpty()) {
    kdDebug(TDEIO_SFTP_DB) << "openConnection(): Need hostname..." << endl;
    error(TDEIO::ERR_UNKNOWN_HOST, i18n("No hostname specified."));
    return;
  }

  // Setup AuthInfo for use with password caching and the
  // password dialog box.
  AuthInfo info = authInfo();
  info.keepPassword = true; // make the "keep Password" check box visible to the user.

  PasswordPurger pwPurger{mPassword};
  PasswordPurger infoPurger{info.password};

  // Check for cached authentication info if no password is specified...
  if (mPassword.isEmpty()) {
    kdDebug(TDEIO_SFTP_DB) << "checking cache: info.username = " << info.username
      << ", info.url = " << info.url.prettyURL() << endl;

    if (checkCachedAuthentication(info)) {
      kdDebug() << "using cached" << endl;
      mUsername = info.username;
      mPassword = info.password;
    }
  }

  int rc;

  // Start the ssh connection.
  if (initializeConnection() < 0) {
    return;
  }

  ExitGuard connectionCloser([this](){ closeConnection(); });

  // Try to authenticate
  rc = ssh_userauth_none(mSession, NULL);
  if (rc == SSH_AUTH_ERROR) {
    error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed (method: %1).")
                                      .arg(i18n("none")));
    return;
  }

  int method = ssh_auth_list(mSession);
  if (!method && rc != SSH_AUTH_SUCCESS) {
    error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed."
          " The server did not send any authentication methods!"));
    return;
  }

  bool firstTime = true;
  bool dlgResult;
  while (rc != SSH_AUTH_SUCCESS) {
    /* FIXME: if there are problems with auth we are likely to stuck in this loop <2024-01-20 Fat-Zer> */

    // Try to authenticate with public key first
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_PUBLICKEY) && !mPassword) {
      // might mess up next login attempt if we won't clean it up
      ExitGuard pubKeyInfoCleanser([this]() {
          mPubKeyAuthData.wasCalled = 0;
          mPubKeyAuthData.wasCanceled = 0;
          mPubKeyAuthData.attemptedKeys.clear();
        });

      kdDebug(TDEIO_SFTP_DB) << "Trying to authenticate with public key" << endl;
      bool keepTryingPasskey=true;
      while (keepTryingPasskey) {
        mPubKeyAuthData.wasCalled = 0;
        rc = ssh_userauth_publickey_auto(mSession, nullptr, nullptr);

        kdDebug(TDEIO_SFTP_DB) << "ssh_userauth_publickey_auto returned rc=" << rc
          << " ssh_err=" << ssh_get_error_code(mSession)
          << " (" <<  ssh_get_error(mSession) << ")" << endl;

        switch (rc) {
          case SSH_AUTH_SUCCESS:
          case SSH_AUTH_PARTIAL:
            keepTryingPasskey=false;
            break;
          case SSH_AUTH_AGAIN:
            // Returned in case of some errors like if server hangs up or there were too many auth attempts
          case SSH_AUTH_ERROR:
            error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed (method: %1).")
                                              .arg(i18n("public key")));
            /* FIXME: add some additional info from ssh_get_error() if available <2024-01-20 Fat-Zer> */
            return;

          case SSH_AUTH_DENIED:
            if (!mPubKeyAuthData.wasCalled) {
              kdDebug(TDEIO_SFTP_DB) << "Passkey auth denied because it has no matching key" << endl;
              keepTryingPasskey = false;
            } else if (mPubKeyAuthData.wasCanceled) {
              kdDebug(TDEIO_SFTP_DB) << "Passkey auth denied because user canceled" << endl;
              keepTryingPasskey = false;
            } else {
              kdDebug(TDEIO_SFTP_DB) << "User entered wrong passphrase for the key" << endl;
            }
            break;
        }
      }
    }

    // Try to authenticate with keyboard interactive
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_INTERACTIVE))
    {
      kdDebug(TDEIO_SFTP_DB) << "Trying to authenticate with keyboard interactive" << endl;

      TDEIO::AuthInfo tmpInfo(info);
      rc = authenticateKeyboardInteractive(tmpInfo);
      if (rc == SSH_AUTH_SUCCESS)
      {
        info = tmpInfo;
        mUsername = info.username;
      }
      else if (rc == SSH_AUTH_ERROR)
      {
        error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed (method: %1).")
                                          .arg(i18n("keyboard interactive")));
        return;
      }
    }

    // Try to authenticate with password
    if (rc != SSH_AUTH_SUCCESS && (method & SSH_AUTH_METHOD_PASSWORD))
    {
      kdDebug(TDEIO_SFTP_DB) << "Trying to authenticate with password" << endl;

      info.keepPassword = true;
      for(;;)
      {
        if(!firstTime || mPassword.isEmpty())
        {
          if (firstTime) {
            info.prompt = i18n("Please enter your username and password.");
          } else {
            info.prompt =  i18n("Login failed.\nPlease confirm your username and password, and enter them again.");
          }
          dlgResult = openPassDlg(info);

          // Handle user canceled or dialog failed to open...
          if (!dlgResult) {
            kdDebug(TDEIO_SFTP_DB) << "User canceled, dlgResult = " << dlgResult << endl;
            error(TDEIO::ERR_USER_CANCELED, TQString());
            return;
          }

          firstTime = false;
        }

        if (mUsername != info.username) {
          kdDebug(TDEIO_SFTP_DB) << "Username changed from " << mUsername
                                 << " to " << info.username << endl;
        }
        mUsername = info.username;
        /* FIXME: libssh doc says that most servers won't allow user switching in-session
         * <2024-01-21 Fat-Zer> */
        rc = ssh_userauth_password(mSession, mUsername.utf8().data(),
                                  info.password.utf8().data());
        if (rc == SSH_AUTH_ERROR) {
          error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Authentication failed (method: %1).")
                                            .arg(i18n("password")));
          return;
        } else if (rc == SSH_AUTH_SUCCESS) {
          break;
        }
      }
    }
  }

  // start sftp session
  kdDebug(TDEIO_SFTP_DB) << "Trying to request the sftp session" << endl;
  mSftp = sftp_new(mSession);
  if (mSftp == NULL) {
    error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Unable to request the SFTP subsystem. "
          "Make sure SFTP is enabled on the server."));
    return;
  }

  kdDebug(TDEIO_SFTP_DB) << "Trying to initialize the sftp session" << endl;
  if (sftp_init(mSftp) < 0) {
    error(TDEIO::ERR_COULD_NOT_LOGIN, i18n("Could not initialize the SFTP session."));
    return;
  }

  // Login succeeded!
  infoMessage(i18n("Successfully connected to %1").arg(mHost));

  //setTimeoutSpecialCommand(TDEIO_SFTP_SPECIAL_TIMEOUT);

  mConnected = true;
  connectionCloser.abort();

  connected();

  return;
}

void sftpProtocol::closeConnection() {
  kdDebug(TDEIO_SFTP_DB) << "closeConnection()" << endl;

  sftp_free(mSftp);
  mSftp = NULL;

  ssh_disconnect(mSession);
  mSession = NULL;

  mConnected = false;
}

#if 0
void sftpProtocol::special(const TQByteArray &data) {
    int rc;
    kdDebug(TDEIO_SFTP_DB) << "special(): polling";

    /*
     * channel_poll() returns the number of bytes that may be read on the
     * channel. It does so by checking the input buffer and eventually the
     * network socket for data to read. If the input buffer is not empty, it
     * will not probe the network (and such not read packets nor reply to
     * keepalives).
     *
     * As channel_poll can act on two specific buffers (a channel has two
     * different stream: stdio and stderr), polling for data on the stderr
     * stream has more chance of not being in the problematic case (data left
     * in the buffer). Checking the return value (for >0) would be a good idea
     * to debug the problem.
     */
    rc = channel_poll(mSftp->channel, 0);
    if (rc > 0) {
        rc = channel_poll(mSftp->channel, 1);
    }

    if (rc < 0) {
        kdDebug(TDEIO_SFTP_DB) << "channel_poll failed: " << ssh_get_error(mSession);
    }

    setTimeoutSpecialCommand(TDEIO_SFTP_SPECIAL_TIMEOUT);
}
#endif

void sftpProtocol::statMime(const KURL &url) {
  kdDebug(TDEIO_SFTP_DB) << "stat: " << url.url() << endl;

  openConnection();
  if (!mConnected) {
    error(TDEIO::ERR_CONNECTION_BROKEN, url.prettyURL());
    return;
  }

  const TQString path = url.path();
  const TQByteArray path_c = path.utf8();

  sftp_attributes sb = sftp_lstat(mSftp, path_c.data());
  if (sb == NULL) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
      sftp_attributes_free(sb);
      emit mimeType("inode/directory");
      return;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
      error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
      sftp_attributes_free(sb);
      return;
    case SSH_FILEXFER_TYPE_SYMLINK:
    case SSH_FILEXFER_TYPE_REGULAR:
      break;
  }

  size_t fileSize = sb->size;
  sftp_attributes_free(sb);

  int flags = 0;

  flags = O_RDONLY;

  mOpenFile = sftp_open(mSftp, path_c.data(), flags, 0);

  if (mOpenFile == NULL) {
    error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, path);
    return;
  }

  // Determine the mimetype of the file to be retrieved, and emit it.
  // This is mandatory in all slaves (for KRun/BrowserRun to work).
  // If we're not opening the file ReadOnly or ReadWrite, don't attempt to
  // read the file and send the mimetype.
  size_t bytesRequested = 1024;
  ssize_t bytesRead = 0;
  TQByteArray buffer(bytesRequested);

  bytesRead = sftp_read(mOpenFile, buffer.data(), bytesRequested);
  if (bytesRead < 0) {
      error(TDEIO::ERR_COULD_NOT_READ, mOpenUrl.prettyURL());
      closeFile();
      return;
  } else {
      TQByteArray fileData;
      fileData.setRawData(buffer.data(), bytesRead);
      KMimeMagicResult *p_mimeType = KMimeMagic::self()->findBufferFileType(fileData, mOpenUrl.fileName());
      emit mimeType(p_mimeType->mimeType());
  }

  sftp_close(mOpenFile);

  mOpenFile = NULL;
}

#if 0
void sftpProtocol::read(TDEIO::filesize_t bytes) {
  kdDebug(TDEIO_SFTP_DB) << "read, offset = " << openOffset << ", bytes = " << bytes;

  Q_ASSERT(mOpenFile != NULL);

  TQVarLengthArray<char> buffer(bytes);

  ssize_t bytesRead = sftp_read(mOpenFile, buffer.data(), bytes);
  Q_ASSERT(bytesRead <= static_cast<ssize_t>(bytes));

  if (bytesRead < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Could not read " << mOpenUrl;
    error(TDEIO::ERR_COULD_NOT_READ, mOpenUrl.prettyURL());
    close();
    return;
  }

  TQByteArray fileData = TQByteArray::fromRawData(buffer.data(), bytesRead);
  data(fileData);
}

void sftpProtocol::write(const TQByteArray &data) {
  kdDebug(TDEIO_SFTP_DB) << "write, offset = " << openOffset << ", bytes = " << data.size();

  Q_ASSERT(mOpenFile != NULL);

  ssize_t bytesWritten = sftp_write(mOpenFile, data.data(), data.size());
  if (bytesWritten < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Could not write to " << mOpenUrl;
    error(TDEIO::ERR_COULD_NOT_WRITE, mOpenUrl.prettyURL());
    close();
    return;
  }

  written(bytesWritten);
}

void sftpProtocol::seek(TDEIO::filesize_t offset) {
  kdDebug(TDEIO_SFTP_DB) << "seek, offset = " << offset;

  Q_ASSERT(mOpenFile != NULL);

  if (sftp_seek64(mOpenFile, static_cast<uint64_t>(offset)) < 0) {
    error(TDEIO::ERR_COULD_NOT_SEEK, mOpenUrl.path());
    close();
  }

  position(sftp_tell64(mOpenFile));
}
#endif

void sftpProtocol::closeFile() {
  if (mOpenFile) {
    sftp_close(mOpenFile);

    mOpenFile = NULL;
    finished();
  }
}

void sftpProtocol::get(const KURL& url) {
  kdDebug(TDEIO_SFTP_DB) << "get(): " << url.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  TQByteArray path = url.path().utf8();

  char buf[MAX_XFER_BUF_SIZE] = {0};
  sftp_file file = NULL;
  ssize_t bytesread = 0;
  // time_t curtime = 0;
  time_t lasttime = 0;
  time_t starttime = 0;
  ssize_t totalbytesread  = 0;

  sftp_attributes sb = sftp_lstat(mSftp, path.data());
  if (sb == NULL) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  switch (sb->type) {
    case SSH_FILEXFER_TYPE_DIRECTORY:
      error(TDEIO::ERR_IS_DIRECTORY, url.prettyURL());
      sftp_attributes_free(sb);
      return;
    case SSH_FILEXFER_TYPE_SPECIAL:
    case SSH_FILEXFER_TYPE_UNKNOWN:
    error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
      sftp_attributes_free(sb);
      return;
    case SSH_FILEXFER_TYPE_SYMLINK:
    case SSH_FILEXFER_TYPE_REGULAR:
      break;
  }

  // Open file
  file = sftp_open(mSftp, path.data(), O_RDONLY, 0);
  if (file == NULL) {
    error( TDEIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
    sftp_attributes_free(sb);
    return;
  }

  // Determine the mimetype of the file to be retrieved, and emit it.
  // This is mandatory in all slaves (for KRun/BrowserRun to work)
  // In real "remote" slaves, this is usually done using findByNameAndContent
  // after receiving some data. But we don't know how much data the mimemagic rules
  // need, so for local files, better use findByUrl with localUrl=true.
  KMimeType::Ptr mt = KMimeType::findByURL( url, sb->permissions, false /* remote URL */ );
  emit mimeType( mt->name() ); // FIXME test me

  kdDebug(TDEIO_SFTP_DB) << "Total size: " << TQString::number(sb->size) << endl;
  // Set the total size
  totalSize(sb->size);

  const TQString resumeOffset = metaData(TQString("resume"));
  if (!resumeOffset.isEmpty()) {
    bool ok;
    ssize_t offset = resumeOffset.toLong(&ok);
    if (ok && (offset > 0) && ((unsigned long long) offset < sb->size))
    {
      if (sftp_seek64(file, offset) == 0) {
        canResume();
        totalbytesread = offset;
        kdDebug(TDEIO_SFTP_DB) << "Resume offset: " << TQString::number(offset) << endl;
      }
    }
  }

  if (file != NULL) {
    bool isFirstPacket = true;
    lasttime = starttime = time(NULL);

    for (;;) {
      bytesread = sftp_read(file, buf, MAX_XFER_BUF_SIZE);
      kdDebug(TDEIO_SFTP_DB) << "bytesread=" << TQString::number(bytesread) << endl;
      if (bytesread == 0) {
        // All done reading
        break;
      } else if (bytesread < 0) {
        kdDebug(TDEIO_SFTP_DB) << "Failed to read";
        error(TDEIO::ERR_COULD_NOT_READ, url.prettyURL());
        sftp_attributes_free(sb);
        return;
      }

      TQByteArray  filedata;
      filedata.setRawData(buf, bytesread);
      if (isFirstPacket) {
        KMimeMagicResult *p_mimeType = KMimeMagic::self()->findBufferFileType(filedata, mOpenUrl.fileName());
        mimeType(p_mimeType->mimeType());
        kdDebug(TDEIO_SFTP_DB) << "mimetype=" << p_mimeType->mimeType() << endl;
        isFirstPacket = false;
      }
      data(filedata);
      filedata.resetRawData(buf, bytesread);

      // increment total bytes read
      totalbytesread += bytesread;

      processedSize(totalbytesread);
    }

    kdDebug(TDEIO_SFTP_DB) << "size processed=" << totalbytesread << endl;
    sftp_close(file);
    //data(TQByteArray());
    processedSize((sb->size));
  }

  sftp_attributes_free(sb);
  finished();
}

void sftpProtocol::put(const KURL& url, int permissions, bool overwrite, bool resume) {
  kdDebug(TDEIO_SFTP_DB) << "put(): " << url.url()
                      << " , permissions = " << TQString::number(permissions)
                      << ", overwrite = " << overwrite
                      << ", resume = " << resume << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  const TQString dest_orig = url.path();
  const TQByteArray dest_orig_c = dest_orig.utf8();
  const TQString dest_part = dest_orig + ".part";
  const TQByteArray dest_part_c = dest_part.utf8();
  uid_t owner = 0;
  gid_t group = 0;

  sftp_attributes sb = sftp_lstat(mSftp, dest_orig_c.data());
  const bool bOrigExists = (sb != NULL);
  bool bPartExists = false;
  const bool bMarkPartial = config()->readEntry("MarkPartial", "true") == "true";

  // Don't change permissions of the original file
  if (bOrigExists) {
      permissions = sb->permissions;
      owner = sb->uid;
      group = sb->gid;
  }

  if (bMarkPartial) {
    sftp_attributes sbPart = sftp_lstat(mSftp, dest_part_c.data());
    bPartExists = (sbPart != NULL);

    if (bPartExists && !resume && !overwrite &&
        sbPart->size > 0 && sbPart->type == SSH_FILEXFER_TYPE_REGULAR) {
      kdDebug(TDEIO_SFTP_DB) << "put : calling canResume with "
        << TQString::number(sbPart->size) << endl;

      // Maybe we can use this partial file for resuming
      // Tell about the size we have, and the app will tell us
      // if it's ok to resume or not.
      if (canResume(sbPart->size)) {
          resume = true;
      }

      kdDebug(TDEIO_SFTP_DB) << "put got answer " << resume << endl;

      delete sbPart;
    }
  }

  if (bOrigExists && !(overwrite) && !(resume)) {
    if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
      error(TDEIO::ERR_DIR_ALREADY_EXIST, dest_orig);
    } else {
      error(TDEIO::ERR_FILE_ALREADY_EXIST, dest_orig);
    }
    sftp_attributes_free(sb);
    return;
  }

  int result;
  TQByteArray dest;
  sftp_file file = NULL;

  // Loop until we got 0 (end of data)
  do {
    TQByteArray buffer;
    dataReq(); // Request for data
    result = readData(buffer);

    if (result >= 0 && buffer.size()) {
      kdDebug(TDEIO_SFTP_DB) << TQString("Got %1 bytes of data").arg(buffer.size()) << endl;
      if (dest.isEmpty()) {
        if (bMarkPartial) {
          kdDebug(TDEIO_SFTP_DB) << "Appending .part extension to " << dest_orig << endl;
          dest = dest_part_c;
          if (bPartExists && !(resume)) {
            kdDebug(TDEIO_SFTP_DB) << "Deleting partial file " << dest_part << endl;
            sftp_unlink(mSftp, dest_part_c.data());
            // Catch errors when we try to open the file.
          }
        } else {
          dest = dest_orig_c;
          if (bOrigExists && !(resume)) {
            kdDebug(TDEIO_SFTP_DB) << "Deleting destination file " << dest_orig << endl;
            sftp_unlink(mSftp, dest_orig_c.data());
            // Catch errors when we try to open the file.
          }
        } // bMarkPartial

        if ((resume)) {
          sftp_attributes fstat;

          kdDebug(TDEIO_SFTP_DB) << "Trying to append: " << dest.data() << endl;
          file = sftp_open(mSftp, dest.data(), O_RDWR, 0);  // append if resuming
          if (file) {
             fstat = sftp_fstat(file);
             if (fstat) {
                sftp_seek64(file, fstat->size); // Seek to end TODO
                sftp_attributes_free(fstat);
             }
          }
        } else {
          mode_t initialMode;

          if (permissions != -1) {
            initialMode = permissions | S_IWUSR | S_IRUSR;
          } else {
            initialMode = 0644;
          }

          kdDebug(TDEIO_SFTP_DB) << "Trying to open: " << dest.data() << ", mode=" << TQString::number(initialMode) << endl;
          file = sftp_open(mSftp, dest.data(), O_CREAT | O_TRUNC | O_WRONLY, initialMode);
        } // resume

        if (file == NULL) {
          kdDebug(TDEIO_SFTP_DB) << "COULD NOT WRITE " << dest.data()
                              << " permissions=" << permissions
                              << " error=" << ssh_get_error(mSession) << endl;
          if (sftp_get_error(mSftp) == SSH_FX_PERMISSION_DENIED) {
            error(TDEIO::ERR_WRITE_ACCESS_DENIED, TQString::fromUtf8(dest));
          } else {
            error(TDEIO::ERR_CANNOT_OPEN_FOR_WRITING, TQString::fromUtf8(dest));
          }
          sftp_attributes_free(sb);
          finished();
          return;
        } // file
      } // dest.isEmpty

      ssize_t bytesWritten = sftp_write(file, buffer.data(), buffer.size());
      kdDebug(TDEIO_SFTP_DB) << TQString("Written %1 bytes").arg(bytesWritten) << endl;
      if (bytesWritten < 0) {
        error(TDEIO::ERR_COULD_NOT_WRITE, dest_orig);
        result = -1;
      }
    } // result
  } while (result > 0);
  sftp_attributes_free(sb);

  // An error occurred deal with it.
  if (result < 0) {
    kdDebug(TDEIO_SFTP_DB) << "Error during 'put'. Aborting." << endl;

    if (file != NULL) {
      sftp_close(file);

      sftp_attributes attr = sftp_stat(mSftp, dest.data());
      if (bMarkPartial && attr != NULL) {
        size_t size = config()->readLongNumEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE);
        if (attr->size < size) {
          sftp_unlink(mSftp, dest.data());
        }
      }
      delete attr;
      sftp_attributes_free(attr);
    }

    //::exit(255);
    finished();
    return;
  }

  if (file == NULL) { // we got nothing to write out, so we never opened the file
    finished();
    return;
  }

  if (sftp_close(file) < 0) {
    kdWarning(TDEIO_SFTP_DB) << "Error when closing file descriptor" << endl;
    error(TDEIO::ERR_COULD_NOT_WRITE, dest_orig);
    return;
  }

  // after full download rename the file back to original name
  if (bMarkPartial) {
    // If the original URL is a symlink and we were asked to overwrite it,
    // remove the symlink first. This ensures that we do not overwrite the
    // current source if the symlink points to it.
    if ((overwrite)) {
      sftp_unlink(mSftp, dest_orig_c.data());
    }

    if (sftp_rename(mSftp, dest.data(), dest_orig_c.data()) < 0) {
      kdWarning(TDEIO_SFTP_DB) << " Couldn't rename " << dest.data() << " to " << dest_orig << endl;
      error(TDEIO::ERR_CANNOT_RENAME_PARTIAL, dest_orig);
      return;
    }
  }

  // set final permissions
  if (permissions != -1 && !(resume)) {
    kdDebug(TDEIO_SFTP_DB) << "Trying to set final permissions of " << dest_orig << " to " << TQString::number(permissions) << endl;
    if (sftp_chmod(mSftp, dest_orig_c.data(), permissions) < 0) {
      warning(i18n( "Could not change permissions for\n%1").arg(dest_orig));
    }
  }

  // set original owner and group
  if (bOrigExists) {
      kdDebug(TDEIO_SFTP_DB) << "Trying to restore original owner and group of " << dest_orig << endl;
      if (sftp_chown(mSftp, dest_orig_c.data(), owner, group) < 0) {
          // warning(i18n( "Could not change owner and group for\n%1", dest_orig));
      }
  }

  // set modification time
#if 0
  const TQString mtimeStr = metaData("modified");
  if (!mtimeStr.isEmpty()) {
    TQDateTime dt = TQDateTime::fromString(mtimeStr, TQt::ISODate);
    if (dt.isValid()) {
      struct timeval times[2];

      sftp_attributes attr = sftp_lstat(mSftp, dest_orig_c.data());
      if (attr != NULL) {
        times[0].tv_sec = attr->atime; //// access time, unchanged
        times[1].tv_sec =  dt.toTime_t(); // modification time
        times[0].tv_usec = times[1].tv_usec = 0;

        sftp_utimes(mSftp, dest_orig_c.data(), times);
        sftp_attributes_free(attr);
      }
    }
  }
#endif
  // We have done our job => finish
  finished();
}

void sftpProtocol::copy(const KURL &src, const KURL &dest, int permissions, bool overwrite)
{
  kdDebug(TDEIO_SFTP_DB) << src.url() << " -> " << dest.url() << " , permissions = " << TQString::number(permissions)
                                      << ", overwrite = " << overwrite << endl;

  error(TDEIO::ERR_UNSUPPORTED_ACTION, TQString());
}

void sftpProtocol::stat(const KURL& url) {
  kdDebug(TDEIO_SFTP_DB) << url.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  if (! url.hasPath() || TQDir::isRelativePath(url.path()) ||
      url.path().contains("/./") || url.path().contains("/../")) {
    TQString cPath;

    if (url.hasPath()) {
      cPath = canonicalizePath(url.path());
    } else {
      cPath = canonicalizePath(TQString("."));
    }

    if (cPath.isEmpty()) {
      error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
      return;
    }
    KURL redir(url);
    redir.setPath(cPath);
    redirection(redir);

    kdDebug(TDEIO_SFTP_DB) << "redirecting to " << redir.url() << endl;

    finished();
    return;
  }

  TQByteArray path = url.path().utf8();

  const TQString sDetails = metaData(TQString("details"));
  const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();

  UDSEntry entry;
  entry.clear();
  if (!createUDSEntry(url.fileName(), path, entry, details)) {
    error(TDEIO::ERR_DOES_NOT_EXIST, url.prettyURL());
    return;
  }

  statEntry(entry);

  finished();
}

void sftpProtocol::mimetype(const KURL& url){
  kdDebug(TDEIO_SFTP_DB) << url.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  // stat() feeds the mimetype
  statMime(url);
  closeFile();

  finished();
}

void sftpProtocol::listDir(const KURL& url) {
  kdDebug(TDEIO_SFTP_DB) << "list directory: " << url.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  if (! url.hasPath() || TQDir::isRelativePath(url.path()) ||
      url.path().contains("/./") || url.path().contains("/../")) {
    TQString cPath;

    if (url.hasPath()) {
      cPath = canonicalizePath(url.path());
    } else {
      cPath = canonicalizePath(TQString("."));
    }

    if (cPath.isEmpty()) {
      error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
      return;
    }
    KURL redir(url);
    redir.setPath(cPath);
    redirection(redir);

    kdDebug(TDEIO_SFTP_DB) << "redirecting to " << redir.url() << endl;

    finished();
    return;
  }

  TQByteArray path = url.path().utf8();

  sftp_dir dp = sftp_opendir(mSftp, path.data());
  if (dp == NULL) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  sftp_attributes dirent = NULL;
  const TQString sDetails = metaData(TQString("details"));
  const int details = sDetails.isEmpty() ? 2 : sDetails.toInt();
  TQValueList<TQByteArray> entryNames;
  UDSEntry entry;

  kdDebug(TDEIO_SFTP_DB) << "readdir: " << path.data() << ", details: " << TQString::number(details) << endl;

  UDSAtom atom;

  for (;;) {
    mode_t access;
    mode_t type;
    char *link;

    dirent = sftp_readdir(mSftp, dp);
    if (dirent == NULL) {
      break;
    }

    entry.clear();
    atom.m_uds = UDS_NAME;
    atom.m_str = TQFile::decodeName(dirent->name);
    entry.append(atom);

    if (dirent->type == SSH_FILEXFER_TYPE_SYMLINK) {
      TQCString file = (TQString::fromUtf8(path) + "/" + TQFile::decodeName(dirent->name)).utf8().data();

      atom.m_uds = UDS_FILE_TYPE;
      atom.m_long = S_IFREG;
      entry.append(atom);

      link = sftp_readlink(mSftp, file.data());
      if (link == NULL) {
        sftp_attributes_free(dirent);
        error(TDEIO::ERR_INTERNAL, i18n("Could not read link: %1").arg(TQString::fromUtf8(file)));
        return;
      }
      atom.m_uds = UDS_LINK_DEST;
      atom.m_str = TQFile::decodeName(link);
      entry.append(atom);
      delete link;
      // A symlink -> follow it only if details > 1
      if (details > 1) {
        sftp_attributes sb = sftp_stat(mSftp, file.data());
        if (sb == NULL) {
          // It is a link pointing to nowhere
          type = S_IFMT - 1;
          access = S_IRWXU | S_IRWXG | S_IRWXO;
          atom.m_uds = UDS_FILE_TYPE;
          atom.m_long = type;
          entry.append(atom);
          atom.m_uds = UDS_ACCESS;
          atom.m_long = access;
          entry.append(atom);
          atom.m_uds = UDS_SIZE;
          atom.m_long = 0;
          entry.append(atom);

          goto notype;
        }
        sftp_attributes_free(dirent);
        dirent = sb;
      }
    }

    switch (dirent->type) {
      case SSH_FILEXFER_TYPE_REGULAR:
        atom.m_uds = UDS_FILE_TYPE;
        atom.m_long = S_IFREG;
        entry.append(atom);
        break;
      case SSH_FILEXFER_TYPE_DIRECTORY:
        atom.m_uds = UDS_FILE_TYPE;
        atom.m_long = S_IFDIR;
        entry.append(atom);
        break;
      case SSH_FILEXFER_TYPE_SYMLINK:
        atom.m_uds = UDS_FILE_TYPE;
        atom.m_long = S_IFLNK;
        entry.append(atom);
        break;
      case SSH_FILEXFER_TYPE_SPECIAL:
      case SSH_FILEXFER_TYPE_UNKNOWN:
        break;
    }

    access = dirent->permissions & 07777;
    atom.m_uds = UDS_ACCESS;
    atom.m_long = access;
    entry.append(atom);

    atom.m_uds = UDS_SIZE;
    atom.m_long = dirent->size;
    entry.append(atom);

notype:
    if (details > 0) {
      atom.m_uds = UDS_USER;
      if (dirent->owner) {
          atom.m_str = TQString::fromUtf8(dirent->owner);
      } else {
          atom.m_str = TQString::number(dirent->uid);
      }
      entry.append(atom);

      atom.m_uds = UDS_GROUP;
      if (dirent->group) {
          atom.m_str = TQString::fromUtf8(dirent->group);
      } else {
          atom.m_str = TQString::number(dirent->gid);
      }
      entry.append(atom);

      atom.m_uds = UDS_ACCESS_TIME;
      atom.m_long = dirent->atime;
      entry.append(atom);

      atom.m_uds = UDS_MODIFICATION_TIME;
      atom.m_long = dirent->mtime;
      entry.append(atom);

      atom.m_uds = UDS_MODIFICATION_TIME;
      atom.m_long = dirent->createtime;
      entry.append(atom);
    }

    sftp_attributes_free(dirent);
    listEntry(entry, false);
  } // for ever
  sftp_closedir(dp);
  listEntry(entry, true); // ready

  finished();
}

void sftpProtocol::mkdir(const KURL &url, int permissions) {
  kdDebug(TDEIO_SFTP_DB) << "create directory: " << url.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  if (url.path().isEmpty()) {
    error(TDEIO::ERR_MALFORMED_URL, url.prettyURL());
    return;
  }
  const TQString path = url.path();
  const TQByteArray path_c = path.utf8();

  // Remove existing file or symlink, if requested.
  if (metaData(TQString("overwrite")) == TQString("true")) {
    kdDebug(TDEIO_SFTP_DB) << "overwrite set, remove existing file or symlink: " << url.url() << endl;
    sftp_unlink(mSftp, path_c.data());
  }

  kdDebug(TDEIO_SFTP_DB) << "Trying to create directory: " << path << endl;
  sftp_attributes sb = sftp_lstat(mSftp, path_c.data());
  if (sb == NULL) {
    if (sftp_mkdir(mSftp, path_c.data(), 0777) < 0) {
      reportError(url, sftp_get_error(mSftp));
      sftp_attributes_free(sb);
      return;
    } else {
      kdDebug(TDEIO_SFTP_DB) << "Successfully created directory: " << url.url() << endl;
      if (permissions != -1) {
        chmod(url, permissions);
      } else {
        finished();
      }
      sftp_attributes_free(sb);
      return;
    }
  }

  if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
    error(TDEIO::ERR_DIR_ALREADY_EXIST, path);
  } else {
    error(TDEIO::ERR_FILE_ALREADY_EXIST, path);
  }

  sftp_attributes_free(sb);
  return;
}

void sftpProtocol::rename(const KURL& src, const KURL& dest, bool overwrite) {
  kdDebug(TDEIO_SFTP_DB) << "rename " << src.url() << " to " << dest.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  TQByteArray qsrc = src.path().utf8();
  TQByteArray qdest = dest.path().utf8();

  sftp_attributes sb = sftp_lstat(mSftp, qdest.data());
  if (sb != NULL) {
    if (!overwrite) {
      if (sb->type == SSH_FILEXFER_TYPE_DIRECTORY) {
        error(TDEIO::ERR_DIR_ALREADY_EXIST, dest.url());
      } else {
        error(TDEIO::ERR_FILE_ALREADY_EXIST, dest.url());
      }
      sftp_attributes_free(sb);
      return;
    }

    del(dest, sb->type == SSH_FILEXFER_TYPE_DIRECTORY ? true : false);
  }
  sftp_attributes_free(sb);

  if (sftp_rename(mSftp, qsrc.data(), qdest.data()) < 0) {
    reportError(dest, sftp_get_error(mSftp));
    return;
  }

  finished();
}

void sftpProtocol::symlink(const TQString& target, const KURL& dest, bool overwrite) {
  kdDebug(TDEIO_SFTP_DB) << "link " << target << "->" << dest.url()
                      << ", overwrite = " << overwrite << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  TQByteArray t = target.utf8();
  TQByteArray d = dest.path().utf8();

  bool failed = false;
  if (sftp_symlink(mSftp, t.data(), d.data()) < 0) {
    if (overwrite) {
      sftp_attributes sb = sftp_lstat(mSftp, d.data());
      if (sb == NULL) {
        failed = true;
      } else {
        if (sftp_unlink(mSftp, d.data()) < 0) {
          failed = true;
        } else {
          if (sftp_symlink(mSftp, t.data(), d.data()) < 0) {
            failed = true;
          }
        }
      }
      sftp_attributes_free(sb);
    }
  }

  if (failed) {
    reportError(dest, sftp_get_error(mSftp));
    return;
  }

  finished();
}

void sftpProtocol::chmod(const KURL& url, int permissions) {
  kdDebug(TDEIO_SFTP_DB) << "change permission of " << url.url() << " to " << TQString::number(permissions) << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  TQByteArray path = url.path().utf8();

  if (sftp_chmod(mSftp, path.data(), permissions) < 0) {
    reportError(url, sftp_get_error(mSftp));
    return;
  }

  finished();
}

void sftpProtocol::del(const KURL &url, bool isfile){
  kdDebug(TDEIO_SFTP_DB) << "deleting " << (isfile ? "file: " : "directory: ") << url.url() << endl;

  openConnection();
  if (!mConnected) {
    return;
  }

  TQByteArray path = url.path().utf8();

  if (isfile) {
    if (sftp_unlink(mSftp, path.data()) < 0) {
      reportError(url, sftp_get_error(mSftp));
      return;
    }
  } else {
    if (sftp_rmdir(mSftp, path.data()) < 0) {
      reportError(url, sftp_get_error(mSftp));
      return;
    }
  }

  finished();
}

void sftpProtocol::slave_status() {
  kdDebug(TDEIO_SFTP_DB) << "connected to " << mHost << "?: " << mConnected << endl;
  slaveStatus((mConnected ? mHost : TQString()), mConnected);
}
