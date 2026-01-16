/*
 * This file is part of the KDE project, module kcontrol.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 *
 * Based on kcontrol1 energy.cpp, Copyright (c) 1999 Tom Vijlbrief
 */


/*
 * KDE Energy setup module.
 */

#include <config.h>

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqpushbutton.h>

#include <tdeconfig.h>
#include <kcursor.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <tdelocale.h>
#include <knuminput.h>
#include <krun.h>
#include <tdestandarddirs.h>
#include <kurllabel.h>
#include <dcopref.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "energy.h"


#ifdef HAVE_DPMS
#include <X11/Xmd.h>
extern "C" {
#include <X11/extensions/dpms.h>
Status DPMSInfo ( Display *, CARD16 *, BOOL * );
Bool DPMSCapable( Display * );
int __kde_do_not_unload = 1;

#ifndef HAVE_DPMSCAPABLE_PROTO
Bool DPMSCapable ( Display * );
#endif

#ifndef HAVE_DPMSINFO_PROTO
Status DPMSInfo ( Display *, CARD16 *, BOOL * );
#endif
}

#if defined(XIMStringConversionRetrival) || defined (__sun)
extern "C" {
#endif
    Bool DPMSQueryExtension(Display *, int *, int *);
    Status DPMSEnable(Display *);
    Status DPMSDisable(Display *);
    Bool DPMSGetTimeouts(Display *, CARD16 *, CARD16 *, CARD16 *);
    Bool DPMSSetTimeouts(Display *, CARD16, CARD16, CARD16);
#if defined(XIMStringConversionRetrival) || defined (__sun)
}
#endif
#endif

static const int DFLT_STANDBY   = 0;
static const int DFLT_SUSPEND   = 30;
static const int DFLT_OFF   = 60;


/**** DLL Interface ****/

extern "C" {

    TDE_EXPORT TDECModule *create_energy(TQWidget *parent, char *) {
	return new KEnergy(parent, "kcmenergy");
    }

    TDE_EXPORT void init_energy() {
#ifdef HAVE_DPMS
        TDEConfig *cfg = new TDEConfig("kcmdisplayrc", true /*readonly*/, false /*no globals*/);
        cfg->setGroup("DisplayEnergy");

	Display *dpy = tqt_xdisplay();
	CARD16 pre_configured_status;
	BOOL pre_configured_enabled;
	CARD16 pre_configured_standby;
	CARD16 pre_configured_suspend;
	CARD16 pre_configured_off;
        bool enabled;
        CARD16 standby;
        CARD16 suspend;
        CARD16 off;
	int dummy;
	/* query the running X server if DPMS is supported */
	if (DPMSQueryExtension(dpy, &dummy, &dummy) && DPMSCapable(dpy)) {
	    DPMSGetTimeouts(dpy, &pre_configured_standby, &pre_configured_suspend, &pre_configured_off);
	    DPMSInfo(dpy, &pre_configured_status, &pre_configured_enabled);
	    /* let the user override the settings */
	    enabled = cfg->readBoolEntry("displayEnergySaving", pre_configured_enabled);
	    standby = cfg->readNumEntry("displayStandby", pre_configured_standby/60);
	    suspend = cfg->readNumEntry("displaySuspend", pre_configured_suspend/60);
	    off = cfg->readNumEntry("displayPowerOff", pre_configured_off/60);
	} else {
	/* provide our defauts */
	    enabled = true;
	    standby = DFLT_STANDBY;
	    suspend = DFLT_SUSPEND;
	    off = DFLT_OFF;
	}

        delete cfg;

        KEnergy::applySettings(enabled, standby, suspend, off);
#endif
    }
}

/**** KEnergy ****/

KEnergy::KEnergy(TQWidget *parent, const char *name)
    : TDECModule(parent, name)
{
    m_bChanged = false;
    m_bEnabled = false;
    m_Standby = DFLT_STANDBY;
    m_Suspend = DFLT_SUSPEND;
    m_Off = DFLT_OFF;
    m_bDPMS = false;
    m_bKPowersave = false;
    m_bTDEPowersave = false;
    m_bMaintainSanity = true;

    setQuickHelp( i18n("<h1>Display Power Control</h1> If your display supports"
      " power saving features, you can configure them using this module.<p>"
      " There are three levels of power saving: standby, suspend, and off."
      " The greater the level of power saving, the longer it takes for the"
      " display to return to an active state.<p>"
      " To wake up the display from a power saving mode, you can make a small"
      " movement with the mouse, or press a key that is not likely to cause"
      " any unintentional side-effects, for example, the \"Shift\" key."));

#ifdef HAVE_DPMS
    int dummy;
    m_bDPMS = DPMSQueryExtension(tqt_xdisplay(), &dummy, &dummy);

    DCOPRef kpowersave("kpowersave", "KPowersaveIface");
    DCOPReply managingDPMS = kpowersave.call("currentSchemeManagesDPMS()");
    if (managingDPMS.isValid()) {
        m_bKPowersave = managingDPMS;
        m_bDPMS = !m_bKPowersave;
    }

    DCOPRef tdepowersave("tdepowersave", "tdepowersaveIface");
    managingDPMS = tdepowersave.call("currentSchemeManagesDPMS()");
    if (managingDPMS.isValid()) {
        m_bTDEPowersave = managingDPMS;
        m_bDPMS = !m_bTDEPowersave;
    }
#endif

    TQVBoxLayout *top = new TQVBoxLayout(this, 0, KDialog::spacingHint());
    TQHBoxLayout *hbox = new TQHBoxLayout();
    top->addLayout(hbox);

    TQLabel *lbl;
    if (m_bDPMS) {
        TDEGlobal::locale()->insertCatalogue("kpowersave");
        // ### these i18n strings need to be synced with kpowersave !!
        m_pCBEnable= new TQCheckBox(i18n("&Enable display power management" ), this);
        connect(m_pCBEnable, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotChangeEnable(bool)));
        hbox->addWidget(m_pCBEnable);

        TQWhatsThis::add( m_pCBEnable, i18n("Check this option to enable the"
           " power saving features of your display.") );
        // ###
    } else if(m_bKPowersave || m_bTDEPowersave) {
        m_pCBEnable = new TQCheckBox(i18n("&Enable specific display power management"), this);
        hbox->addWidget(m_pCBEnable);
        m_bEnabled = false;
        m_pCBEnable->setChecked(true);
        m_pCBEnable->setEnabled(false);
    } else {
        lbl = new TQLabel(i18n("Your display does not support power saving."), this);
        hbox->addWidget(lbl);
    }

    KURLLabel *logo = new KURLLabel(this);
    logo->setURL("http://www.energystar.gov");
    logo->setPixmap(TQPixmap(locate("data", "kcontrol/pics/energybig.png")));
    logo->setTipText(i18n("Learn more about the Energy Star program"));
    logo->setUseTips(true); 
connect(logo, TQ_SIGNAL(leftClickedURL(const TQString&)), TQ_SLOT(openURL(const TQString &)));

    hbox->addStretch();
    hbox->addWidget(logo);

    // Sliders
    if (!m_bKPowersave && !m_bTDEPowersave) {
    m_pStandbySlider = new KIntNumInput(m_Standby, this);
    m_pStandbySlider->setLabel(i18n("&Standby after:"));
    m_pStandbySlider->setRange(0, 120, 10);
    m_pStandbySlider->setSuffix(i18n(" min"));
    m_pStandbySlider->setSpecialValueText(i18n("Disabled"));
    connect(m_pStandbySlider, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotChangeStandby(int)));
    top->addWidget(m_pStandbySlider);
    TQWhatsThis::add( m_pStandbySlider, i18n("Choose the period of inactivity"
       " after which the display should enter \"standby\" mode. This is the"
       " first level of power saving.") );

    m_pSuspendSlider = new KIntNumInput(m_pStandbySlider, m_Suspend, this);
    m_pSuspendSlider->setLabel(i18n("S&uspend after:"));
    m_pSuspendSlider->setRange(0, 120, 10);
    m_pSuspendSlider->setSuffix(i18n(" min"));
    m_pSuspendSlider->setSpecialValueText(i18n("Disabled"));
    connect(m_pSuspendSlider, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotChangeSuspend(int)));
    top->addWidget(m_pSuspendSlider);
    TQWhatsThis::add( m_pSuspendSlider, i18n("Choose the period of inactivity"
       " after which the display should enter \"suspend\" mode. This is the"
       " second level of power saving, but may not be different from the first"
       " level for some displays.") );

    m_pOffSlider = new KIntNumInput(m_pSuspendSlider, m_Off, this);
    m_pOffSlider->setLabel(i18n("&Power off after:"));
    m_pOffSlider->setRange(0, 120, 10);
    m_pOffSlider->setSuffix(i18n(" min"));
    m_pOffSlider->setSpecialValueText(i18n("Disabled"));
    connect(m_pOffSlider, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotChangeOff(int)));
    top->addWidget(m_pOffSlider);
    TQWhatsThis::add( m_pOffSlider, i18n("Choose the period of inactivity"
       " after which the display should be powered off. This is the"
       " greatest level of power saving that can be achieved while the"
       " display is still physically turned on.") );

    } else {
        m_pStandbySlider = 0;
        m_pSuspendSlider = 0;
        m_pOffSlider = 0;
        if(m_bKPowersave) {
            TQPushButton* btnKPowersave = new TQPushButton(this);
            btnKPowersave->setText(i18n("Configure KPowersave..."));
            connect(btnKPowersave, TQ_SIGNAL(clicked()), TQ_SLOT(slotLaunchKPowersave()));
            top->addWidget(btnKPowersave);
        }
        if(m_bTDEPowersave) {
            TQPushButton* btnTDEPowersave = new TQPushButton(this);
            btnTDEPowersave->setText(i18n("Configure TDEPowersave..."));
            connect(btnTDEPowersave, TQ_SIGNAL(clicked()), TQ_SLOT(slotLaunchTDEPowersave()));
            top->addWidget(btnTDEPowersave);
		}
    }

    top->addStretch();

    if (m_bDPMS)
       setButtons( TDECModule::Help | TDECModule::Default | TDECModule::Apply );
    else
       setButtons( TDECModule::Help );

    m_pConfig = new TDEConfig("kcmdisplayrc", false /*readwrite*/, false /*no globals*/);
    m_pConfig->setGroup("DisplayEnergy");

    load();
}


KEnergy::~KEnergy()
{
    delete m_pConfig;
}


void KEnergy::load()
{
    load( false );
}

void KEnergy::load( bool useDefaults )
{
    m_pConfig->setReadDefaults( useDefaults );
    readSettings();
    showSettings();

    emit changed( useDefaults );
}


void KEnergy::save()
{
    writeSettings();
    applySettings(m_bEnabled, m_Standby, m_Suspend, m_Off);

    emit changed(false);
}


void KEnergy::defaults()
{
    load( true );
}


void KEnergy::readSettings()
{
    if (m_bDPMS) {
        m_bEnabled = m_pConfig->readBoolEntry("displayEnergySaving", false);
    }
    m_Standby = m_pConfig->readNumEntry("displayStandby", DFLT_STANDBY);
    m_Suspend = m_pConfig->readNumEntry("displaySuspend", DFLT_SUSPEND);
    m_Off = m_pConfig->readNumEntry("displayPowerOff", DFLT_OFF);

    m_StandbyDesired = m_Standby;
    m_SuspendDesired = m_Suspend;
    m_OffDesired = m_Off;

    m_bChanged = false;
}


void KEnergy::writeSettings()
{
    if (!m_bChanged)
     return;

    m_pConfig->writeEntry( "displayEnergySaving", m_bEnabled);
    m_pConfig->writeEntry("displayStandby", m_Standby);
    m_pConfig->writeEntry("displaySuspend", m_Suspend);
    m_pConfig->writeEntry("displayPowerOff", m_Off);

    m_pConfig->sync();
    m_bChanged = false;
}


void KEnergy::slotLaunchKPowersave()
{
    DCOPRef r("kpowersave", "KPowersaveIface");
    r.send("openConfigureDialog()");
}

void KEnergy::slotLaunchTDEPowersave()
{
    DCOPRef r("tdepowersave", "tdepowersaveIface");
    r.send("openConfigureDialog()");
}

void KEnergy::showSettings()
{
    m_bMaintainSanity = false;

    if (m_bDPMS) {
        m_pCBEnable->setChecked(m_bEnabled);
    }

    if (!m_bKPowersave && !m_bTDEPowersave) {
        m_pStandbySlider->setEnabled(m_bEnabled);
        m_pStandbySlider->setValue(m_Standby);
        m_pSuspendSlider->setEnabled(m_bEnabled);
        m_pSuspendSlider->setValue(m_Suspend);
        m_pOffSlider->setEnabled(m_bEnabled);
        m_pOffSlider->setValue(m_Off);
    }

    m_bMaintainSanity = true;
}


extern "C" {
  int dropError(Display *, XErrorEvent *);
  typedef int (*XErrFunc) (Display *, XErrorEvent *);
}

int dropError(Display *, XErrorEvent *)
{
    return 0;
}

/* static */
void KEnergy::applySettings(bool enable, int standby, int suspend, int off)
{
#ifdef HAVE_DPMS
    XErrFunc defaultHandler;
    defaultHandler = XSetErrorHandler(dropError);

    Display *dpy = tqt_xdisplay();

    int dummy;
    bool hasDPMS = DPMSQueryExtension(dpy, &dummy, &dummy);
    if (hasDPMS) {
        if (enable) {
            DPMSEnable(dpy);
            DPMSSetTimeouts(dpy, 60*standby, 60*suspend, 60*off);
        } else
            DPMSDisable(dpy);
    } else
        tqWarning("Server has no DPMS extension");

    XFlush(dpy);
    XSetErrorHandler(defaultHandler);
#else
    /* keep gcc silent */
    if (enable | standby | suspend | off)
    /* nothing */ ;
#endif
}


void KEnergy::slotChangeEnable(bool ena)
{
    m_bEnabled = ena;
    m_bChanged = true;

    m_pStandbySlider->setEnabled(ena);
    m_pSuspendSlider->setEnabled(ena);
    m_pOffSlider->setEnabled(ena);

    emit changed(true);
}


void KEnergy::slotChangeStandby(int value)
{
    m_Standby = value;

    if ( m_bMaintainSanity ) {
	m_bMaintainSanity = false;
	m_StandbyDesired = value;
        if ((m_Suspend > 0 && m_Standby > m_Suspend) ||
	    (m_SuspendDesired && m_Standby >= m_SuspendDesired) )
            m_pSuspendSlider->setValue(m_Standby);
        if ((m_Off > 0 && m_Standby > m_Off) ||
	    (m_OffDesired && m_Standby >= m_OffDesired) )
            m_pOffSlider->setValue(m_Standby);
	m_bMaintainSanity = true;
    }

    m_bChanged = true;
    emit changed(true);
}


void KEnergy::slotChangeSuspend(int value)
{
    m_Suspend = value;

    if ( m_bMaintainSanity ) {
	m_bMaintainSanity = false;
	m_SuspendDesired = value;
	if (m_Suspend == 0 && m_StandbyDesired > 0)
	    m_pStandbySlider->setValue( m_StandbyDesired );
        else if (m_Suspend < m_Standby || m_Suspend <= m_StandbyDesired )
            m_pStandbySlider->setValue(m_Suspend);
        if ((m_Off > 0 && m_Suspend > m_Off) ||
	    (m_OffDesired && m_Suspend >= m_OffDesired) )
            m_pOffSlider->setValue(m_Suspend);
	m_bMaintainSanity = true;
    }

    m_bChanged = true;
    emit changed(true);
}


void KEnergy::slotChangeOff(int value)
{
    m_Off = value;

    if ( m_bMaintainSanity ) {
	m_bMaintainSanity = false;
	m_OffDesired = value;
	if (m_Off == 0 && m_StandbyDesired > 0)
	    m_pStandbySlider->setValue( m_StandbyDesired );
        else if (m_Off < m_Standby || m_Off <= m_StandbyDesired )
            m_pStandbySlider->setValue(m_Off);
	if (m_Off == 0 && m_SuspendDesired > 0)
	    m_pSuspendSlider->setValue( m_SuspendDesired );
        else if (m_Off < m_Suspend || m_Off <= m_SuspendDesired )
            m_pSuspendSlider->setValue(m_Off);
	m_bMaintainSanity = true;
    }

    m_bChanged = true;
    emit changed(true);
}

void KEnergy::openURL(const TQString &URL)
{
      new KRun(KURL( URL ));
}

#include "energy.moc"
