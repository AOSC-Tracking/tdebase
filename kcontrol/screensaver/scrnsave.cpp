//-----------------------------------------------------------------------------
//
// KDE Display screen saver setup module
//
// Copyright (c)  Martin R. Jones 1996,1999,2002
//
// Converted to a kcc module by Matthias Hoelzer 1997
//


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqheader.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlistview.h>
#include <tqpushbutton.h>
#include <tqslider.h>
#include <tqtimer.h>
#include <tqfileinfo.h>
#include <tqwhatsthis.h>

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kdialogbase.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <tdeprocess.h>
#include <kservicegroup.h>
#include <tdestandarddirs.h>
#include <ksimpleconfig.h>

#include <X11/Xlib.h>
#include "scrnsave.h"
#include <fixx11h.h>

template class TQPtrList<SaverConfig>;

const uint widgetEventMask =                 // X event mask
(uint)(
       ExposureMask |
       PropertyChangeMask |
       StructureNotifyMask
      );

//===========================================================================
// DLL Interface for kcontrol
typedef KGenericFactory<KScreenSaver, TQWidget > KSSFactory;
K_EXPORT_COMPONENT_FACTORY (kcm_screensaver, KSSFactory("kcmscreensaver") )


static TQString findExe(const TQString &exe) {
    TQString result = locate("exe", exe);
    if (result.isEmpty())
        result = TDEStandardDirs::findExe(exe);
    return result;
}

KScreenSaver::KScreenSaver(TQWidget *parent, const char *name, const TQStringList&)
    : TDECModule(KSSFactory::instance(), parent, name)
{
    mSetupProc = 0;
    mPreviewProc = 0;
    mTestWin = 0;
    mTestProc = 0;
    mPrevSelected = -2;
    mMonitor = 0;
    mTesting = false;

    struct stat st;
    if( stat( KDE_CONFDIR "/tdm/tdmdistrc" , &st ) == 0) {
			mTDMConfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdm/tdmdistrc" ));
		}
		else {
			mTDMConfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdm/tdmrc" ));
		}
		mTDMConfig->setGroup("X-:*-Greeter");

    // Add non-TDE path
    TDEGlobal::dirs()->addResourceType("scrsav",
                                     TDEGlobal::dirs()->kde_default("apps") +
                                     "apps/ScreenSavers/");

    setQuickHelp( i18n("<h1>Screen Saver</h1> This module allows you to enable and"
       " configure a screen saver. Note that you can enable a screen saver"
       " even if you have power saving features enabled for your display.<p>"
       " Besides providing an endless variety of entertainment and"
       " preventing monitor burn-in, a screen saver also gives you a simple"
       " way to lock your display if you are going to leave it unattended"
       " for a while. If you want the screen saver to lock the session, make sure you enable"
       " the \"Require password\" feature of the screen saver; if you do not, you can still"
       " explicitly lock the session using the desktop's \"Lock Session\" action."));

    setButtons( TDECModule::Help | TDECModule::Default | TDECModule::Apply );

    // Add KDE specific screensaver path
    TQString relPath="System/ScreenSavers/";
    KServiceGroup::Ptr servGroup = KServiceGroup::baseGroup( "screensavers" );
    if (servGroup)
    {
      relPath=servGroup->relPath();
      kdDebug() << "relPath=" << relPath << endl;
    }

    TDEGlobal::dirs()->addResourceType("scrsav",
                                     TDEGlobal::dirs()->kde_default("apps") +
                                     relPath);

    readSettings( false );

    mSetupProc = new TDEProcess;
    connect(mSetupProc, TQ_SIGNAL(processExited(TDEProcess *)),
            this, TQ_SLOT(slotSetupDone(TDEProcess *)));

    mPreviewProc = new TDEProcess;
    connect(mPreviewProc, TQ_SIGNAL(processExited(TDEProcess *)),
            this, TQ_SLOT(slotPreviewExited(TDEProcess *)));

    TQBoxLayout *topLayout = new TQHBoxLayout(this, 0, KDialog::spacingHint());

    // left column
    TQVBoxLayout *leftColumnLayout =
        new TQVBoxLayout(topLayout, KDialog::spacingHint());
    TQBoxLayout *vLayout =
        new TQVBoxLayout(leftColumnLayout, KDialog::spacingHint());

    mSaverGroup = new TQGroupBox(i18n("Screen Saver"), this );
    mSaverGroup->setColumnLayout( 0, TQt::Horizontal );
    vLayout->addWidget(mSaverGroup);
    vLayout->setStretchFactor( mSaverGroup, 10 );
    TQBoxLayout *groupLayout = new TQVBoxLayout( mSaverGroup->layout(),
        KDialog::spacingHint() );

    mSaverListView = new TQListView( mSaverGroup );
    mSaverListView->setMinimumHeight( 120 );
    mSaverListView->setSizePolicy(TQSizePolicy::Preferred, TQSizePolicy::Expanding);
    mSaverListView->addColumn("");
    mSaverListView->header()->hide();
    mSelected = -1;
    groupLayout->addWidget( mSaverListView, 10 );
    connect( mSaverListView, TQ_SIGNAL(doubleClicked ( TQListViewItem *)), this, TQ_SLOT( slotSetup()));
    TQWhatsThis::add( mSaverListView, i18n("Select the screen saver to use.") );

    TQBoxLayout* hlay = new TQHBoxLayout(groupLayout, KDialog::spacingHint());
    mSetupBt = new TQPushButton( i18n("&Setup..."), mSaverGroup );
    connect( mSetupBt, TQ_SIGNAL( clicked() ), TQ_SLOT( slotSetup() ) );
    mSetupBt->setEnabled(false);
    hlay->addWidget( mSetupBt );
    TQWhatsThis::add( mSetupBt, i18n("Configure the screen saver's options, if any.") );

    mTestBt = new TQPushButton( i18n("&Test"), mSaverGroup );
    connect( mTestBt, TQ_SIGNAL( clicked() ), TQ_SLOT( slotTest() ) );
    mTestBt->setEnabled(false);
    hlay->addWidget( mTestBt );
    TQWhatsThis::add( mTestBt, i18n("Show a full screen preview of the screen saver.") );

    mSettingsGroup = new TQGroupBox( i18n("Settings"), this );
    mSettingsGroup->setColumnLayout( 0, TQt::Vertical );
    leftColumnLayout->addWidget( mSettingsGroup );
    TQGridLayout *settingsGroupLayout = new TQGridLayout( mSettingsGroup->layout(), 5, 2, KDialog::spacingHint() );

    mEnabledCheckBox = new TQCheckBox(i18n("Start a&utomatically"), mSettingsGroup);
    mEnabledCheckBox->setChecked(mEnabled);
    TQWhatsThis::add( mEnabledCheckBox, i18n("Automatically start the screen saver after a period of inactivity.") );
    connect(mEnabledCheckBox, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotEnable(bool)));
    settingsGroupLayout->addWidget(mEnabledCheckBox, 0, 0);

    TQBoxLayout *hbox = new TQHBoxLayout();
    settingsGroupLayout->addLayout(hbox, 1, 0);
    hbox->addSpacing(30);
    mActivateLbl = new TQLabel(i18n("After:"), mSettingsGroup);
    mActivateLbl->setEnabled(mEnabled);
    hbox->addWidget(mActivateLbl);
    mWaitEdit = new TQSpinBox(mSettingsGroup);
    mWaitEdit->setSteps(1, 10);
    mWaitEdit->setRange(1, INT_MAX);
    mWaitEdit->setSuffix(i18n(" min"));
    mWaitEdit->setValue(mTimeout/60);
    mWaitEdit->setEnabled(mEnabled);
    connect(mWaitEdit, TQ_SIGNAL(valueChanged(int)),
            this, TQ_SLOT(slotTimeoutChanged(int)));
    mActivateLbl->setBuddy(mWaitEdit);
    hbox->addWidget(mWaitEdit);
    hbox->addStretch(1);
    TQString wtstr = i18n("The period of inactivity after which the screen saver should start.");
    TQWhatsThis::add( mActivateLbl, wtstr );
    TQWhatsThis::add( mWaitEdit, wtstr );

    mLockCheckBox = new TQCheckBox( i18n("&Require password to stop"), mSettingsGroup );
    mLockCheckBox->setEnabled( mEnabled );
    mLockCheckBox->setChecked( mLock );
    connect( mLockCheckBox, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotLock( bool ) ) );
    settingsGroupLayout->addWidget(mLockCheckBox, 2, 0);
    TQWhatsThis::add( mLockCheckBox, i18n("Prevent potential unauthorized use by requiring a password to stop the screen saver.") );

    hbox = new TQHBoxLayout();
    settingsGroupLayout->addLayout(hbox, 3, 0);
    hbox->addSpacing(30);
    mLockLbl = new TQLabel(i18n("After:"), mSettingsGroup);
    mLockLbl->setEnabled(mEnabled && mLock);
    TQWhatsThis::add( mLockLbl, i18n("The amount of time, after the screen saver has started, to ask for the unlock password.") );
    hbox->addWidget(mLockLbl);
    mWaitLockEdit = new TQSpinBox(mSettingsGroup);
    mWaitLockEdit->setSteps(1, 10);
    mWaitLockEdit->setRange(1, 300);
    mWaitLockEdit->setSuffix(i18n(" sec"));
    mWaitLockEdit->setValue(mLockTimeout/1000);
    mWaitLockEdit->setEnabled(mEnabled && mLock);
    if ( mWaitLockEdit->sizeHint().width() <
         mWaitEdit->sizeHint().width() ) {
        mWaitLockEdit->setFixedWidth( mWaitEdit->sizeHint().width() );
        mWaitEdit->setFixedWidth( mWaitEdit->sizeHint().width() );
    }
    else {
        mWaitEdit->setFixedWidth( mWaitLockEdit->sizeHint().width() );
        mWaitLockEdit->setFixedWidth( mWaitLockEdit->sizeHint().width() );
    }
    connect(mWaitLockEdit, TQ_SIGNAL(valueChanged(int)), this, TQ_SLOT(slotLockTimeoutChanged(int)));
    mLockLbl->setBuddy(mWaitLockEdit);
    hbox->addWidget(mWaitLockEdit);
    hbox->addStretch(1);
    TQString wltstr = i18n("Choose the period after which the display will be locked. ");
    TQWhatsThis::add( mLockLbl, wltstr );
    TQWhatsThis::add( mWaitLockEdit, wltstr );

    mDelaySaverStartCheckBox = new TQCheckBox( i18n("&Delay saver start after lock"), mSettingsGroup );
    mDelaySaverStartCheckBox->setEnabled( true );
    mDelaySaverStartCheckBox->setChecked( mDelaySaverStart );
    connect( mDelaySaverStartCheckBox, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotDelaySaverStart( bool ) ) );
    settingsGroupLayout->addWidget(mDelaySaverStartCheckBox, 0, 1);
    TQWhatsThis::add( mDelaySaverStartCheckBox, i18n("When manually locking the screen, wait to start the screen saver until the configured start delay has elapsed.") );

    mUseTSAKCheckBox = new TQCheckBox( i18n("&Use Secure Attention Key"), mSettingsGroup );
    mUseTSAKCheckBox->setEnabled( true );
    mUseTSAKCheckBox->setChecked( mUseTSAK );
    connect( mUseTSAKCheckBox, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotUseTSAK( bool ) ) );
    settingsGroupLayout->addWidget(mUseTSAKCheckBox, 1, 1);
    TQWhatsThis::add( mUseTSAKCheckBox, i18n("Require Secure Attention Key prior to displaying the unlock dialog.") );

    mUseUnmanagedLockWindowsCheckBox = new TQCheckBox( i18n("Use &legacy lock windows"), mSettingsGroup );
    mUseUnmanagedLockWindowsCheckBox->setEnabled( true );
    mUseUnmanagedLockWindowsCheckBox->setChecked( mUseUnmanagedLockWindows );
    connect( mUseUnmanagedLockWindowsCheckBox, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotUseUnmanagedLockWindows( bool ) ) );
    settingsGroupLayout->addWidget(mUseUnmanagedLockWindowsCheckBox, 2, 1);
    TQWhatsThis::add( mUseUnmanagedLockWindowsCheckBox, i18n("Use old-style unmanaged X11 lock windows.") );

    mHideActiveWindowsFromSaverCheckBox = new TQCheckBox( i18n("Hide active &windows from saver"), mSettingsGroup );
    mHideActiveWindowsFromSaverCheckBox->setEnabled( true );
    mHideActiveWindowsFromSaverCheckBox->setChecked( mHideActiveWindowsFromSaver );
    connect( mHideActiveWindowsFromSaverCheckBox, TQ_SIGNAL( toggled( bool ) ), this, TQ_SLOT( slotHideActiveWindowsFromSaver( bool ) ) );
    settingsGroupLayout->addWidget(mHideActiveWindowsFromSaverCheckBox, 3, 1);
    TQWhatsThis::add( mHideActiveWindowsFromSaverCheckBox, i18n("Hide all active windows from the screen saver and use the desktop background as the screen saver input.") );

    mHideCancelButtonCheckBox = new TQCheckBox( i18n("Hide &cancel button"), mSettingsGroup );
    mHideCancelButtonCheckBox->setEnabled( true );
    mHideCancelButtonCheckBox->setChecked( mHideCancelButton );
    connect( mHideCancelButtonCheckBox, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(slotHideCancelButton(bool)) );
    settingsGroupLayout->addWidget(mHideCancelButtonCheckBox, 4, 1);
    TQWhatsThis::add(mHideCancelButtonCheckBox, i18n("Hide Cancel button from the \"Desktop Session Locked\" dialog."));

    // right column
    TQBoxLayout* rightColumnLayout = new TQVBoxLayout(topLayout, KDialog::spacingHint());

    mMonitorLabel = new TQLabel( this );
    mMonitorLabel->setAlignment( AlignCenter );
    mMonitorLabel->setPixmap( TQPixmap(locate("data", "kcontrol/pics/monitor.png")));
    rightColumnLayout->addWidget(mMonitorLabel, 0);
    TQWhatsThis::add( mMonitorLabel, i18n("A preview of the selected screen saver.") );

    TQBoxLayout* advancedLayout = new TQHBoxLayout( rightColumnLayout, 3 );
    advancedLayout->addWidget( new TQWidget( this ) );
    TQPushButton* advancedBt = new TQPushButton(
        i18n( "Advanced &Options" ), this, "advancedBtn" );
    advancedBt->setSizePolicy( TQSizePolicy(
        TQSizePolicy::Fixed, TQSizePolicy::Fixed) );
    connect( advancedBt, TQ_SIGNAL( clicked() ),
             this, TQ_SLOT( slotAdvanced() ) );
    advancedLayout->addWidget( advancedBt );
    advancedLayout->addWidget( new TQWidget( this ) );

    rightColumnLayout->addStretch();

    if (mImmutable)
    {
       setButtons(buttons() & ~Default);
       mSettingsGroup->setEnabled(false);
       mSaverGroup->setEnabled(false);
    }

    // finding the savers can take some time, so defer loading until
    // we've started up.
    mNumLoaded = 0;
    mLoadTimer = new TQTimer( this );
    connect( mLoadTimer, TQ_SIGNAL(timeout()), TQ_SLOT(findSavers()) );
    mLoadTimer->start( 100 );
    mChanged = false;
    emit changed(false);

    TDEAboutData *about =
    new TDEAboutData(I18N_NOOP("kcmscreensaver"), I18N_NOOP("TDE Screen Saver Control Module"),
                  0, 0, TDEAboutData::License_GPL,
                  I18N_NOOP("(c) 1997-2002 Martin R. Jones\n"
                  "(c) 2003-2004 Chris Howells"));
    about->addAuthor("Chris Howells", 0, "howells@kde.org");
    about->addAuthor("Martin R. Jones", 0, "jones@kde.org");

    setAboutData( about );

    mSaverList.setAutoDelete(true);

    processLockouts();
}

//---------------------------------------------------------------------------
//
void KScreenSaver::resizeEvent( TQResizeEvent * )
{

  if (mMonitor)
    {
      mMonitor->setGeometry( (mMonitorLabel->width()-200)/2+23,
                 (mMonitorLabel->height()-186)/2+14, 151, 115 );
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::mousePressEvent( TQMouseEvent *)
{
    if ( mTesting )
	slotStopTest();
}

//---------------------------------------------------------------------------
//
void KScreenSaver::keyPressEvent( TQKeyEvent *)
{
    if ( mTesting )
	slotStopTest();
}
//---------------------------------------------------------------------------
//
KScreenSaver::~KScreenSaver()
{
    if (mPreviewProc)
    {
        if (mPreviewProc->isRunning())
        {
            int pid = mPreviewProc->pid();
            mPreviewProc->kill( );
            waitpid(pid, (int *) 0,0);
        }
        delete mPreviewProc;
    }

    delete mTestProc;
    delete mSetupProc;
    delete mTestWin;

    delete mTDMConfig;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::load()
{
	load( false );
}

void KScreenSaver::load( bool useDefaults )
{
    readSettings( useDefaults);

//with the following line, the Test and Setup buttons are not enabled correctly
//if no saver was selected, the "Reset" and the "Enable screensaver", it is only called when starting and when pressing reset, aleXXX
//    mSelected = -1;
    int i = 0;
    TQListViewItem *selectedItem = 0;
    for (SaverConfig* saver = mSaverList.first(); saver != 0; saver = mSaverList.next()) {
        if (saver->file() == mSaver)
        {
            selectedItem = mSaverListView->findItem ( saver->name(), 0 );
            if (selectedItem) {
                mSelected = i;
                break;
            }
        }
        i++;
    }
    if ( selectedItem )
    {
      mSaverListView->setSelected( selectedItem, true );
      mSaverListView->setCurrentItem( selectedItem );
      slotScreenSaver( selectedItem );
    }

    updateValues();
    mChanged = useDefaults;
    emit changed( useDefaults );
}

//------------------------------------------------------------After---------------
//
void KScreenSaver::readSettings( bool useDefaults )
{
    TDEConfig *config = new TDEConfig( "kdesktoprc");

	 config->setReadDefaults( useDefaults );

    mImmutable = config->groupIsImmutable("ScreenSaver");

    config->setGroup( "ScreenSaver" );

    mEnabled = config->readBoolEntry("Enabled", false);
    mTimeout = config->readNumEntry("Timeout", 300);
    mLockTimeout = config->readNumEntry("LockGrace", 60000);
    mLock = config->readBoolEntry("Lock", false);
    mDelaySaverStart = config->readBoolEntry("DelaySaverStart", true);
    mUseTSAK = config->readBoolEntry("UseTDESAK", true);
    mUseUnmanagedLockWindows = config->readBoolEntry("UseUnmanagedLockWindows", false);
    mHideActiveWindowsFromSaver = config->readBoolEntry("HideActiveWindowsFromSaver", true);
    mHideCancelButton = config->readBoolEntry("HideCancelButton", false);
    mSaver = config->readEntry("Saver");

    if (mTimeout < 60) mTimeout = 60;
    if (mLockTimeout < 0) mLockTimeout = 0;
    if (mLockTimeout > 300000) mLockTimeout = 300000;

    mChanged = false;
    delete config;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::updateValues()
{
    if (mEnabled)
    {
        mWaitEdit->setValue(mTimeout/60);
    }
    else
    {
        mWaitEdit->setValue(0);
    }

    mWaitLockEdit->setValue(mLockTimeout/1000);
    mLockCheckBox->setChecked(mLock);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::defaults()
{
	load( true );
}

//---------------------------------------------------------------------------
//
void KScreenSaver::save()
{
    if ( !mChanged )
        return;

    TDEConfig *config = new TDEConfig( "kdesktoprc");
    config->setGroup( "ScreenSaver" );

    config->writeEntry("Enabled", mEnabled);
    config->writeEntry("Timeout", mTimeout);
    config->writeEntry("LockGrace", mLockTimeout);
    config->writeEntry("Lock", mLock);
    config->writeEntry("DelaySaverStart", mDelaySaverStart);
    config->writeEntry("UseTDESAK", mUseTSAK);
    config->writeEntry("UseUnmanagedLockWindows", mUseUnmanagedLockWindows);
    config->writeEntry("HideActiveWindowsFromSaver", mHideActiveWindowsFromSaver);
    config->writeEntry("HideCancelButton", mHideCancelButton);

    if ( !mSaver.isEmpty() )
        config->writeEntry("Saver", mSaver);
    config->sync();
    delete config;

    // TODO (GJ): When you changed anything, these two lines will give a segfault
    // on exit. I don't know why yet.

    DCOPClient *client = tdeApp->dcopClient();
    client->send("kdesktop", "KScreensaverIface", "configure()", TQString(""));

    mChanged = false;
    emit changed(false);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::findSavers()
{
    if ( !mNumLoaded ) {
        mSaverFileList = TDEGlobal::dirs()->findAllResources("scrsav",
                            "*.desktop", false, true);
        new TQListViewItem ( mSaverListView, i18n("Loading...") );
        if ( mSaverFileList.isEmpty() )
            mLoadTimer->stop();
        else
            mLoadTimer->start( 50 );
    }

    for ( int i = 0; i < 5 &&
            (unsigned)mNumLoaded < mSaverFileList.count();
            i++, mNumLoaded++ ) {
        TQString file = mSaverFileList[mNumLoaded];
        SaverConfig *saver = new SaverConfig;
        bool saverFound = false;
        if (saver->read(file))
        {
            // find the xscreensaver executable
            //work around a TDEStandardDirs::findExe() "feature" where it looks in $TDEDIR/bin first no matter what and sometimes finds the wrong executable
            TQFileInfo checkExe;
            TQStringList saverPaths = TQStringList::split(':', XSCREENSAVER_HACKS_DIRS);
            for (TQStringList::ConstIterator it = saverPaths.begin(); it != saverPaths.end(); ++it)
            {
                checkExe.setFile((*it) + "/" + saver->exec());
                if (checkExe.exists() && checkExe.isExecutable() && checkExe.isFile())
                {
                    mSaverList.append(saver);
                    saverFound = true;
                    break;
                }
            }

            if (!saverFound)
            {
                // Executable not present in XScreenSaver directory!
                // Try standard paths
                if (TDEStandardDirs::findExe(saver->exec()) != TQString::null)
                {
                    mSaverList.append(saver);
                    saverFound = true;
                }
            }
        }

        if (!saverFound)
        {
          delete saver;
        }
    }

    if ( (unsigned)mNumLoaded == mSaverFileList.count() ) {
        TQListViewItem *selectedItem = 0;
        int categoryCount = 0;
        int indx = 0;

        mLoadTimer->stop();
        delete mLoadTimer;
        mSaverList.sort();

        mSelected = -1;
        mSaverListView->clear();
        for ( SaverConfig *s = mSaverList.first(); s != 0; s = mSaverList.next())
        {
            TQListViewItem *item;
            if (s->category().isEmpty())
                item = new TQListViewItem ( mSaverListView, s->name(), "2" + s->name() );
            else
            {
                TQListViewItem *categoryItem = mSaverListView->findItem( s->category(), 0 );
                if ( !categoryItem ) {
                    categoryItem = new TQListViewItem ( mSaverListView, s->category(), "1" + s->category() );
                    categoryItem->setPixmap ( 0, SmallIcon ( "tdescreensaver" ) );
                }
                item = new TQListViewItem ( categoryItem, s->name(), s->name() );
                categoryCount++;
            }
            if (s->file() == mSaver) {
                mSelected = indx;
                selectedItem = item;
            }
            indx++;
        }

        // Delete categories with only one item
        TQListViewItemIterator it ( mSaverListView );
        for ( ; it.current(); it++ )
            if ( it.current()->childCount() == 1 ) {
               TQListViewItem *item = it.current()->firstChild();
               it.current()->takeItem( item );
               mSaverListView->insertItem ( item );
               delete it.current();
               categoryCount--;
            }

        mSaverListView->setRootIsDecorated ( categoryCount > 0 );
        mSaverListView->setSorting ( 1 );

        if ( mSelected > -1 )
        {
            mSaverListView->setSelected(selectedItem, true);
            mSaverListView->setCurrentItem(selectedItem);
            mSaverListView->ensureItemVisible(selectedItem);
            mSetupBt->setEnabled(!mSaverList.at(mSelected)->setup().isEmpty());
            mTestBt->setEnabled(true);
        }

        connect( mSaverListView, TQ_SIGNAL( currentChanged( TQListViewItem * ) ),
                 this, TQ_SLOT( slotScreenSaver( TQListViewItem * ) ) );

        setMonitor();
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::setMonitor()
{
    if (mPreviewProc->isRunning())
    // CC: this will automatically cause a "slotPreviewExited"
    // when the viewer exits
    mPreviewProc->kill();
    else
    slotPreviewExited(mPreviewProc);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotPreviewExited(TDEProcess *)
{
    // Ugly hack to prevent continual respawning of savers that crash
    if (mSelected == mPrevSelected)
        return;

    if ( mSaverList.isEmpty() ) // safety check
        return;

    // Some xscreensaver hacks do something nasty to the window that
    // requires a new one to be created (or proper investigation of the
    // problem).
    delete mMonitor;

    mMonitor = new KSSMonitor(mMonitorLabel);
    mMonitor->setBackgroundColor(black);
    mMonitor->setGeometry((mMonitorLabel->width()-200)/2+23,
                          (mMonitorLabel->height()-186)/2+14, 151, 115);
    mMonitor->show();
    // So that hacks can XSelectInput ButtonPressMask
    XSelectInput(tqt_xdisplay(), mMonitor->winId(), widgetEventMask );

    if (mSelected >= 0) {
        mPreviewProc->clearArguments();

        TQString saver = mSaverList.at(mSelected)->saver();
        TQTextStream ts(&saver, IO_ReadOnly);

        TQString word;
        ts >> word;
        TQString path = findExe(word);

        if (!path.isEmpty())
        {
            (*mPreviewProc) << path;

            while (!ts.atEnd())
            {
                ts >> word;
                if (word == "%w")
                {
                    word = word.setNum(mMonitor->winId());
                }
                (*mPreviewProc) << word;
            }

            mPreviewProc->start();
        }
    }

    mPrevSelected = mSelected;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotEnable(bool e)
{
    mEnabled = e;
    processLockouts();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::processLockouts()
{
    bool useSAK = mTDMConfig->readBoolEntry("UseSAK", false);
    mActivateLbl->setEnabled( mEnabled );
    mWaitEdit->setEnabled( mEnabled );
    mLockCheckBox->setEnabled( mEnabled );
    if (mEnabled && !mUseUnmanagedLockWindows) {
        mDelaySaverStartCheckBox->setEnabled( true );
        mDelaySaverStartCheckBox->setChecked( mDelaySaverStart );
    }
    else {
        mDelaySaverStartCheckBox->setEnabled( false );
        mDelaySaverStartCheckBox->setChecked( false );
    }
    if (!mUseUnmanagedLockWindows && useSAK) {
        mUseTSAKCheckBox->setEnabled( true );
        mUseTSAKCheckBox->setChecked( mUseTSAK );
    }
    else {
        mUseTSAKCheckBox->setEnabled( false );
        mUseTSAKCheckBox->setChecked( false );
    }
    if (!mUseUnmanagedLockWindows) {
        mHideActiveWindowsFromSaverCheckBox->setEnabled( true );
        mHideActiveWindowsFromSaverCheckBox->setChecked( mHideActiveWindowsFromSaver );
    }
    else {
        mHideActiveWindowsFromSaverCheckBox->setEnabled( false );
        mHideActiveWindowsFromSaverCheckBox->setChecked( false );
    }
    if (mUseUnmanagedLockWindows || (useSAK && mUseTSAK)) {
        mHideCancelButtonCheckBox->setEnabled( false );
        mHideCancelButtonCheckBox->setChecked( false );
    }
    else {
        mHideCancelButtonCheckBox->setEnabled( true );
        mHideCancelButtonCheckBox->setChecked( mHideCancelButton );
    }
    mLockLbl->setEnabled( mEnabled && mLock );
    mWaitLockEdit->setEnabled( mEnabled && mLock );
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotScreenSaver(TQListViewItem *item)
{
    if (!item)
      return;

    int i = 0, indx = -1;
    for (SaverConfig* saver = mSaverList.first(); saver != 0; saver = mSaverList.next()) {
        if ( item->parent() )
        {
            if (  item->parent()->text( 0 ) == saver->category() && saver->name() == item->text (0))
            {
                indx = i;
                break;
            }
        }
        else
        {
            if (  saver->name() == item->text (0) )
            {
                indx = i;
                break;
            }
        }        
		i++;
    }
    if (indx == -1) {
        mSelected = -1;
        return;
    }

    bool bChanged = (indx != mSelected);

    if (!mSetupProc->isRunning())
        mSetupBt->setEnabled(!mSaverList.at(indx)->setup().isEmpty());
    mTestBt->setEnabled(true);
    mSaver = mSaverList.at(indx)->file();

    mSelected = indx;
    setMonitor();
    if (bChanged)
    {
       mChanged = true;
       emit changed(true);
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotSetup()
{
    if ( mSelected < 0 )
    return;

    if (mSetupProc->isRunning())
    return;

    mSetupProc->clearArguments();

    TQString saver = mSaverList.at(mSelected)->setup();
    if( saver.isEmpty())
        return;
    TQTextStream ts(&saver, IO_ReadOnly);

    TQString word;
    ts >> word;
    bool kxsconfig = word == "kxsconfig";
    TQString path = findExe(word);

    if (!path.isEmpty())
    {
        (*mSetupProc) << path;

        // Add caption and icon to about dialog
        if (!kxsconfig) {
            word = "-caption";
            (*mSetupProc) << word;
            word = mSaverList.at(mSelected)->name();
            (*mSetupProc) << word;
            word = "-icon";
            (*mSetupProc) << word;
            word = "tdescreensaver";
            (*mSetupProc) << word;
        }

        while (!ts.atEnd())
        {
            ts >> word;
            (*mSetupProc) << word;
        }

        // Pass translated name to kxsconfig
        if (kxsconfig) {
          word = mSaverList.at(mSelected)->name();
          (*mSetupProc) << word;
        }

        mSetupBt->setEnabled( false );
        tdeApp->flushX();

        mSetupProc->start();
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotAdvanced()
{
   KScreenSaverAdvancedDialog dlg( topLevelWidget() );
   if ( dlg.exec() ) {
       mChanged = true;
       emit changed(true);
  }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotTest()
{
    if ( mSelected == -1 )
        return;

    if (!mTestProc) {
        mTestProc = new TDEProcess;
    }

    mTestProc->clearArguments();
    TQString saver = mSaverList.at(mSelected)->saver();
    TQTextStream ts(&saver, IO_ReadOnly);

    TQString word;
    ts >> word;
    TQString path = findExe(word);

    if (!path.isEmpty())
    {
        (*mTestProc) << path;

        if (!mTestWin)
        {
            mTestWin = new TestWin();
            mTestWin->setBackgroundMode(TQWidget::NoBackground);
            mTestWin->setGeometry(0, 0, tdeApp->desktop()->width(),
                                    tdeApp->desktop()->height());
        }

        mTestWin->show();
        mTestWin->raise();
        mTestWin->setFocus();
	// So that hacks can XSelectInput ButtonPressMask
	XSelectInput(tqt_xdisplay(), mTestWin->winId(), widgetEventMask );

	grabMouse();
	grabKeyboard();

        mTestBt->setEnabled( FALSE );
	mPreviewProc->kill();

        while (!ts.atEnd())
        {
            ts >> word;
            if (word == "%w")
            {
                word = word.setNum(mTestWin->winId());
            }
            (*mTestProc) << word;
        }

	mTesting = true;
        mTestProc->start(TDEProcess::NotifyOnExit);
    }
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotStopTest()
{
    if (mTestProc->isRunning()) {
        mTestProc->kill();
    }
    releaseMouse();
    releaseKeyboard();
    mTestWin->hide();
    mTestBt->setEnabled(true);
    mPrevSelected = -1;
    setMonitor();
    mTesting = false;
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotTimeoutChanged(int to )
{
    mTimeout = to * 60;
    mChanged = true;
    emit changed(true);
}

//-----------------------------------------------------------------------
//
void KScreenSaver::slotLockTimeoutChanged(int to )
{
    mLockTimeout = to * 1000;
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotLock( bool l )
{
    mLock = l;
    mLockLbl->setEnabled( l );
    mWaitLockEdit->setEnabled( l );
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotDelaySaverStart( bool d )
{
    if (mDelaySaverStartCheckBox->isEnabled()) mDelaySaverStart = d;
    processLockouts();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotUseTSAK( bool u )
{
    if (mUseTSAKCheckBox->isEnabled()) mUseTSAK = u;
    processLockouts();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotUseUnmanagedLockWindows( bool u )
{
    if (mUseUnmanagedLockWindowsCheckBox->isEnabled()) mUseUnmanagedLockWindows = u;
    processLockouts();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotHideActiveWindowsFromSaver( bool h )
{
    if (mHideActiveWindowsFromSaverCheckBox->isEnabled()) mHideActiveWindowsFromSaver = h;
    processLockouts();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotHideCancelButton( bool h )
{
    if (mHideCancelButtonCheckBox->isEnabled()) mHideCancelButton = h;
    processLockouts();
    mChanged = true;
    emit changed(true);
}

//---------------------------------------------------------------------------
//
void KScreenSaver::slotSetupDone(TDEProcess *)
{
    mPrevSelected = -1;  // see ugly hack in slotPreviewExited()
    setMonitor();
    mSetupBt->setEnabled( true );
    emit changed(true);
}

#include "scrnsave.moc"
