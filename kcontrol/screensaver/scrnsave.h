//-----------------------------------------------------------------------------
//
// KDE Display screen saver setup module
//
// Copyright (c)  Martin R. Jones 1996
// Copyright (C) Chris Howells 2004
//

#ifndef __SCRNSAVE_H__
#define __SCRNSAVE_H__

#include <tqwidget.h>
#include <kcmodule.h>

#include "kssmonitor.h"
#include "saverconfig.h"
#include "testwin.h"
#include "advanceddialog.h"
#include "kssmonitor.h"
#include "saverlist.h"

class TQTimer;
class TQSpinBox;
class TQSlider;
class TQCheckBox;
class TQLabel;
class TQListView;
class TQListViewItem;
class TQPushButton;
class KIntNumInput;
class KProcess;

//===========================================================================
class KScreenSaver : public KCModule
{
    Q_OBJECT
public:
    KScreenSaver(TQWidget *parent, const char *name, const TQStringList &);
    ~KScreenSaver();

    virtual void load();
    virtual void load(bool useDefaults);
    virtual void save();
    virtual void defaults();

    void updateValues();
    void readSettings(bool useDefaults);

protected slots:
    void slotEnable( bool );
    void slotScreenSaver( TQListViewItem* );
    void slotSetup();
    void slotAdvanced();
    void slotTest();
    void slotStopTest();
    void slotTimeoutChanged( int );
    void slotLockTimeoutChanged( int );
    void slotLock( bool );
    void slotDelaySaverStart( bool );
    void slotUseTSAK( bool );
    void slotUseUnmanagedLockWindows( bool );
    void slotHideActiveWindowsFromSaver( bool );
    void processLockouts();
    void slotSetupDone(KProcess*);
    // when selecting a new screensaver, the old preview will
    // be killed. -- This callback is responsible for restarting the
    // new preview
    void slotPreviewExited(KProcess *);
    void findSavers();

protected:
    void writeSettings();
    void getSaverNames();
    void setMonitor();
    void setDefaults();
    void resizeEvent( TQResizeEvent * );
    void mousePressEvent(TQMouseEvent *);
    void keyPressEvent(TQKeyEvent *);

protected:
    TestWin     *mTestWin;
    KProcess    *mTestProc;
    KProcess    *mSetupProc;
    KProcess    *mPreviewProc;
    KSSMonitor  *mMonitor;
    TQPushButton *mSetupBt;
    TQPushButton *mTestBt;
    TQListView   *mSaverListView;
    TQSpinBox	*mWaitEdit;
    TQSpinBox    *mWaitLockEdit;
    TQCheckBox   *mLockCheckBox;
    TQCheckBox   *mStarsCheckBox;
    TQCheckBox   *mEnabledCheckBox;
    TQLabel      *mMonitorLabel;
    TQLabel      *mActivateLbl;
    TQLabel      *mLockLbl;
    TQStringList mSaverFileList;
    SaverList   mSaverList;
    TQTimer      *mLoadTimer;
    TQGroupBox   *mSaverGroup;
    TQGroupBox   *mSettingsGroup;
    TQCheckBox   *mDelaySaverStartCheckBox;
    TQCheckBox   *mUseTSAKCheckBox;
    TQCheckBox   *mUseUnmanagedLockWindowsCheckBox;
    TQCheckBox   *mHideActiveWindowsFromSaverCheckBox;

    int         mSelected;
    int         mPrevSelected;
    int		mNumLoaded;
    bool        mChanged;
    bool	mTesting;

    // Settings
    int         mTimeout;
    int         mLockTimeout;
    bool        mLock;
    bool        mEnabled;
    TQString    mSaver;
    bool        mImmutable;
    bool        mDelaySaverStart;
    bool        mUseTSAK;
    bool        mUseUnmanagedLockWindows;
    bool        mHideActiveWindowsFromSaver;

    KSimpleConfig* mTDMConfig;
};

#endif
