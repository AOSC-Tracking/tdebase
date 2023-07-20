#ifndef KIOPREFERENCES_H
#define KIOPREFERENCES_H

#include <tdecmodule.h>

class TQLabel;
class TQVGroupBox;
class TQCheckBox;
class TQVBoxLayout;
class TQHBoxLayout;
class TQGridLayout;

class KIntNumInput;

class KIOPreferences : public TDECModule
{
    TQ_OBJECT

public:
    KIOPreferences( TQWidget* parent = 0);
    ~KIOPreferences();

    void load();
    void save();
    void defaults();

    TQString quickHelp() const;

protected slots:
    void configChanged() { emit changed(true); }

private:
    TQVGroupBox* gb_Ftp;
    TQVGroupBox* gb_Timeout;
    TQCheckBox* cb_ftpEnablePasv;
    TQCheckBox* cb_ftpMarkPartial;

    KIntNumInput* sb_socketRead;
    KIntNumInput* sb_proxyConnect;
    KIntNumInput* sb_serverConnect;
    KIntNumInput* sb_serverResponse;
};

#endif // KIOPREFERENCES_H
