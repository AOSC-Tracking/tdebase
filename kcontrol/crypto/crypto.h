/**
 * crypto.h
 *
 * Copyright (c) 2000-2001 George Staikos <staikos@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _CRYPTO_H
#define _CRYPTO_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tqcheckbox.h>
#include <tqdatetime.h>
#include <tqlineedit.h>
#include <tqlistview.h>
#include <tqptrlist.h>
#include <tqtabwidget.h>
#include <tqvbuttongroup.h>

#include <tdecmodule.h>
#include <tdesimpleconfig.h>

class TQGridLayout;
class TQVButtonGroup;

class KComboBox;
class KCryptoConfig;
class KPushButton;
class KSSLCertBox;
class KSSLSigners;
class KURLLabel;
class KURLRequester;

class CipherItem : public TQCheckListItem
{
public:
    CipherItem( TQListView *view, const TQString& cipher, int bits, int maxBits,
		KCryptoConfig *module );
    ~CipherItem() {}

    void setCipher( const TQString& cipher ) { m_cipher = cipher; }
    const TQString& cipher() const { return m_cipher; }
    void setBits( int bits ) { m_bits = bits; }
    int bits() const { return m_bits; }

    TQString configName() const;

protected:
    virtual void stateChange( bool );

private:
    int m_bits;
    TQString m_cipher;
    KCryptoConfig *m_module; // just to call configChanged()
};



class OtherCertItem : public TQListViewItem
{
public:
    OtherCertItem(TQListView *view, const TQString& sub, const TQString& md5, bool perm, int policy, TQDateTime exp, KCryptoConfig *module );
    ~OtherCertItem() {}

    TQString configName() const;
    const TQString& getSub() { return _sub; }
    int getPolicy() { return _policy; }
    const TQString& getMD5() { return _md5; }
    bool isPermanent() { return _perm; }
    TQDateTime getExpires() { return _exp; }
    void setPolicy(int x) { _policy = x; }
    void setPermanent(bool x) { _perm = x; }
    void setExpires(TQDateTime x) { _exp = x; }

protected:
    virtual void stateChange( bool );

private:
    TQString _sub, _md5;
    KCryptoConfig *m_module; // just to call configChanged()
    TQDateTime _exp;
    bool _perm;
    int _policy;
};




class YourCertItem : public TQListViewItem
{
public:
    YourCertItem(TQListView *view, TQString pkcs, TQString pass, TQString name, KCryptoConfig *module );
    ~YourCertItem() {}

    TQString configName() const;
    TQString& getPKCS() { return _pkcs; }
    void setPKCS(TQString pkcs) { _pkcs = pkcs; }
    TQString& getPass() { return _pass; }
    void setPass(TQString pass) { _pass = pass; }
    TQString& getName() { return _name; }
    void setName(TQString name) { _name = name; }
    TQString& getPassCache() { return _cpass; }
    void setPassCache(TQString pass) { _cpass = pass; }

protected:
    virtual void stateChange( bool );

private:
    TQString _pkcs;
    TQString _pass;
    TQString _cpass;
    TQString _name;
    KCryptoConfig *m_module; // just to call configChanged()
};




class CAItem : public TQListViewItem
{
public:
    CAItem(TQListView *view, TQString name, TQString cert, bool site, bool email, bool code, KCryptoConfig *module );
    ~CAItem() {}

    TQString configName() const;
    TQString& getName() { return _name; }
    void setName(TQString name) { _name = name; }

    inline TQString getCert() const { return _cert; }
    inline bool getSite() const { return _site; }
    inline bool getEmail() const { return _email; }
    inline bool getCode() const { return _code; }
    inline void setSite(bool x) { _site = x; }
    inline void setEmail(bool x) { _email = x; }
    inline void setCode(bool x) { _code = x; }

    bool isNew, modified;

protected:
    virtual void stateChange( bool );

private:
    TQString _name;
    TQString _cert;
    bool _site, _email, _code;
    KCryptoConfig *m_module; // just to call configChanged()
};





class HostAuthItem : public TQListViewItem
{
public:
    HostAuthItem(TQListView *view, TQString host, TQString name, KCryptoConfig *module ) : TQListViewItem(view, TQString::null ) {
                               _name = name;  _host = host;
                               m_module = module;
                               setText(0, _host);
                               setText(1, _name);
                               _oname = TQString::null;
                              }
    ~HostAuthItem() {}

    void setAction(KSSLCertificateHome::KSSLAuthAction aa) {
                 _aa = aa;
                               switch (aa) {
                               case KSSLCertificateHome::AuthSend:
                                 setText(2, i18n("Send"));
                                break;
                               case KSSLCertificateHome::AuthDont:
                                 setText(2, i18n("Don't Send"));
                                break;
                               case KSSLCertificateHome::AuthPrompt:
                                 setText(2, i18n("Prompt"));
                                break;
                               default:
                                break;
                               }
    }
    KSSLCertificateHome::KSSLAuthAction getAction() const { return _aa; }
    TQString configName() const { return _host; }
    TQString getCertName() const { return _name; }
    void setCertName(TQString name) { _name = name; setText(1, name); }
    void setHost(TQString name) { _host = name; setText(0, name); }
    void setOriginalName(TQString oname) { _oname = oname; }
    TQString originalName() const { return _oname; }

protected:

private:
    TQString _host;
    TQString _name, _oname;
    KSSLCertificateHome::KSSLAuthAction _aa;
    KCryptoConfig *m_module; // just to call configChanged()
};





class KCryptoConfig : public TDECModule
{
  TQ_OBJECT
public:
  KCryptoConfig(TQWidget *parent = 0L, const char *name = 0L, const TQStringList &list = TQStringList());
  virtual ~KCryptoConfig();

  void load();
  void load( bool useDefaults );
  void save();
  void defaults();
  virtual TQString handbookSection() const;

#ifdef HAVE_SSL
  bool loadCiphers();
#endif

public slots:
  void configChanged();

  void slotGeneratePersonal();
  void slotUseEGD();
  void slotUseEFile();
  void slotSelectCipher(int id);

  void slotTestOSSL();

  void slotExportCert();
  void slotRemoveCert();
  void slotVerifyCert();
  void slotOtherCertSelect();
  void slotPolicyChanged(int id);
  void slotPermanent();
  void slotUntil();
  void slotDatePick();

  void slotYourImport();
  void slotYourExport();
  void slotYourVerify();
  void slotYourRemove();
  void slotYourUnlock();
  void slotYourPass();
  void slotYourCertSelect();

  void slotNewHostAuth();
  void slotRemoveHostAuth();
  void slotAuthItemChanged();
  void slotAuthText(const TQString &t);
  void slotAuthButtons();
  void slotAuthCombo();

  void slotCAImport();
  void slotCARemove();
  void slotCARestore();
  void slotCAItemChanged();
  void slotCAChecked();

protected:
  void cwCompatible();
  void cwUS();
  void cwExp();
  void cwAll();

private:

  void offerImportToKMail( const TQString& certFile );
  void setAuthCertLists();
  void genCAList();

  TQTabWidget *tabs;
  TQWidget *tabSSL, *tabOSSL;

  TQWidget *tabYourSSLCert, *tabOtherSSLCert, *tabSSLCA, *tabSSLCOpts, *tabAuth;

  TQListView *SSLv2Box, *SSLv3Box;
  TQCheckBox *mUseTLS, *mUseSSLv2, *mUseSSLv3;
  TQCheckBox *mWarnOnEnter, *mWarnOnLeave;

  /* EGD stuff */
  TQLabel        *mEGDLabel;
  KURLRequester *mEGDPath;
  TQCheckBox     *mUseEGD;
  TQCheckBox     *mUseEFile;

  /* CipherWizards */
  TQPushButton *mCWall, *mCWus, *mCWexp, *mCWcompatible;

  TQCheckBox *mWarnOnUnencrypted, *mWarnOnMixed;
  TQListView *yourSSLBox, *otherSSLBox, *caList;
  TQCheckBox *mWarnSelfSigned, *mWarnExpired, *mWarnRevoked;
  TQPushButton *macAdd, *macRemove;
  KPushButton *macClear;
  TQListBox *macBox;
  TQPushButton *otherSSLExport, *otherSSLView, *otherSSLRemove, *otherSSLVerify;
  TQPushButton *yourSSLImport, *yourSSLPass, *yourSSLRemove, *yourSSLExport,
              *yourSSLUnlock, *yourSSLVerify;
  TQRadioButton *yourSSLUseDefault, *yourSSLList, *yourSSLDont;
  TQLineEdit *macCert;
  KSSLCertBox *oSubject, *oIssuer;
  KSSLCertBox *ySubject, *yIssuer;
  TQGridLayout *oGrid;

  TQVButtonGroup *policyGroup;
  TQVButtonGroup *cacheGroup;
  TQRadioButton *policyAccept, *policyReject, *policyPrompt;
  TQRadioButton *cacheUntil, *cachePerm;
  TQLabel *fromLabel, *untilLabel;
  TQLabel *validFrom, *validUntil;
  TQLabel *yValidFrom, *yValidUntil;
  KURLLabel *untilDate;

  TQVGroupBox  *oInfo;
  KURLRequester *oPath;
  TQPushButton *oTest;
  TQPtrList<OtherCertItem> otherCertDelList;
  TQPtrList<YourCertItem> yourCertDelList;
  TQPtrList<CAItem> caDelList;

  /* Personal Cert Policies tab */
  KComboBox *defCertBox;
  KComboBox *hostCertBox;
  TQVButtonGroup *defCertBG;
  TQHButtonGroup *hostCertBG;
  TQRadioButton *defSend, *defPrompt, *defDont;
  TQRadioButton *hostSend, *hostPrompt, *hostDont;
  TQListView *hostAuthList;
  TQPushButton *authAdd, *authRemove;
  TQLineEdit *authHost;
  TQPtrList<HostAuthItem> authDelList;
  TQLabel *yHash, *pHash;

  /* CA stuff */
  KSSLCertBox *caSubject, *caIssuer;
  TQPushButton *caSSLImport, *caSSLRemove, *caSSLRestore;
  TQCheckBox *caSite, *caEmail, *caCode;
  KSSLSigners *_signers;
  TQLabel *cHash;

  TDEConfig *config;
  TDESimpleConfig *policies, *pcerts, *authcfg;
  TDEConfig *cacfg;

  bool ___lehack;       // to hack around a lineedit problem
};

#endif
