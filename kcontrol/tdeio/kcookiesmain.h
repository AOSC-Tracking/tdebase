// kcookiesmain.h - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#ifndef __KCOOKIESMAIN_H
#define __KCOOKIESMAIN_H

#include <tdecmodule.h>

class TQTabWidget;
class DCOPClient;
class KCookiesPolicies;
class KCookiesManagement;

class KCookiesMain : public TDECModule
{
    TQ_OBJECT
public:
    KCookiesMain(TQWidget *parent = 0L);
    ~KCookiesMain();

    KCookiesPolicies* policyDlg() { return policies; }

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual TQString quickHelp() const;

    virtual TQString handbookSection() const;

private:

    TQTabWidget* tab;
    KCookiesPolicies* policies;
    KCookiesManagement* management;
    int policiesTabNumber;
    int managementTabNumber;
};

#endif // __KCOOKIESMAIN_H
