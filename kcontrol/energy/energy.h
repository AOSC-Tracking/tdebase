/*
 * This file is part of the KDE project, module kcontrol.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 *
 * Based on kcontrol1 energy.h, Copyright (c) 1999 Tom Vijlbrief.
 */

#ifndef __Energy_h_Included__
#define __Energy_h_Included__

#include <tqobject.h>
#include <tdecmodule.h>

class TQCheckBox;
class KIntNumInput;
class TDEConfig;

extern "C" void init_energy();

/**
 * The Desktop/Energy tab in kcontrol.
 */
class KEnergy: public TDECModule
{
    Q_OBJECT

public:
    KEnergy(TQWidget *parent, const char *name);
    ~KEnergy();

    virtual void load();
    virtual void load( bool useDefaults );
    virtual void save();
    virtual void defaults();

private slots:
    void slotChangeEnable(bool);
    void slotChangeStandby(int);
    void slotChangeSuspend(int);
    void slotChangeOff(int);
    void slotLaunchKPowersave();
    void slotLaunchTDEPowersave();
    void openURL(const TQString &);

private:
    void readSettings();
    void writeSettings();
    void showSettings();

    static void applySettings(bool, int, int, int);
    friend void init_energy();

    bool m_bChanged, m_bDPMS, m_bKPowersave, m_bTDEPowersave, m_bEnabled, m_bMaintainSanity;
    int m_Standby, m_Suspend, m_Off;
    int m_StandbyDesired, m_SuspendDesired, m_OffDesired;

    TQCheckBox *m_pCBEnable;
    KIntNumInput *m_pStandbySlider;
    KIntNumInput *m_pSuspendSlider;
    KIntNumInput *m_pOffSlider;
    TDEConfig *m_pConfig;
};

#endif // __Energy_h_Included__
