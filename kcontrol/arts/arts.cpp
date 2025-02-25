/*

    Copyright (C) 2000 Stefan Westerfeld
                       stefan@space.twc.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    Permission is also granted to link this program with the Qt
    library, treating Qt like a library that normally accompanies the
    operating system kernel, whether or not that is in fact the case.

*/

#include <unistd.h>

#include <tqcombobox.h>
#include <tqdir.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqregexp.h>
#include <tqslider.h>
#include <tqtabwidget.h>
#include <tqwhatsthis.h>

#include <dcopref.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdecmoduleloader.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klineedit.h>
#include <tdemessagebox.h>
#include <tdeprocess.h>
#include <krichtextlabel.h>
#include <ksimpleconfig.h>
#include <tdestandarddirs.h>
#include <kurlrequester.h>
#include <libtdemid/deviceman.h>

#include "arts.h"

extern "C" {
	TDE_EXPORT void init_arts();

    TDE_EXPORT TDECModule *create_arts(TQWidget *parent, const char* /*name*/)
	{
		TDEGlobal::locale()->insertCatalogue("kcmarts");
		return new KArtsModule(parent, "kcmarts" );
	}
}

static bool startArts()
{
	TDEConfig *config = new TDEConfig("kcmartsrc", true, false);

	config->setGroup("Arts");
	bool startServer = config->readBoolEntry("StartServer",true);
	bool startRealtime = config->readBoolEntry("StartRealtime",true);
	TQString args = config->readEntry("Arguments","-F 10 -S 4096 -s 60 -m artsmessage -c drkonqi -l 3 -f");

	delete config;

	if (startServer)
		tdeApp->tdeinitExec(startRealtime?"artswrapper":"artsd",
		                  TQStringList::split(" ",args));
	return startServer;
}

/*
 * This function uses artsd -A to init audioIOList with the possible audioIO
 * methods. Here is a sample output of artsd -A (note the two spaces before
 * each "interesting" line are used in parsing:
 *
 * # artsd -A
 * possible choices for the audio i/o method:
 *
 *   toss      Threaded Open Sound System
 *   esd       Enlightened Sound Daemon
 *   null      No audio input/output
 *   alsa      Advanced Linux Sound Architecture
 *   oss       Open Sound System
 *
 */
void KArtsModule::initAudioIOList()
{
	TDEProcess* artsd = new TDEProcess();
	*artsd << "artsd";
	*artsd << "-A";

	connect(artsd, TQ_SIGNAL(processExited(TDEProcess*)),
	        this, TQ_SLOT(slotArtsdExited(TDEProcess*)));
	connect(artsd, TQ_SIGNAL(receivedStderr(TDEProcess*, char*, int)),
	        this, TQ_SLOT(slotProcessArtsdOutput(TDEProcess*, char*, int)));

	if (!artsd->start(TDEProcess::Block, TDEProcess::Stderr)) {
		KMessageBox::error(0, i18n("Unable to start the sound server to "
		                           "retrieve possible sound I/O methods.\n"
		                           "Only automatic detection will be "
		                           "available."));
                delete artsd;
	}
}

void KArtsModule::slotArtsdExited(TDEProcess* proc)
{
	latestProcessStatus = proc->exitStatus();
	delete proc;
}

void KArtsModule::slotProcessArtsdOutput(TDEProcess*, char* buf, int len)
{
	// XXX(gioele): I suppose this will be called with full lines, am I wrong?

	TQStringList availableIOs = TQStringList::split("\n", TQCString(buf, len));
	// valid entries have two leading spaces
	availableIOs = availableIOs.grep(TQRegExp("^ {2}"));
	availableIOs.sort();

	TQString name, fullName;
	TQStringList::Iterator it;
	for (it = availableIOs.begin(); it != availableIOs.end(); ++it) {
		name = (*it).left(12).stripWhiteSpace();
		fullName = (*it).mid(12).stripWhiteSpace();
		audioIOList.append(new AudioIOElement(name, fullName));
	}
}

KArtsModule::KArtsModule(TQWidget *parent, const char *name)
  : TDECModule(parent, name), configChanged(false)
{
	setButtons(Default|Apply|Help);

	setQuickHelp( i18n("<h1>Sound System</h1> Here you can configure aRts, TDE's sound server."
		    " This program not only allows you to hear your system sounds while simultaneously"
		    " listening to an MP3 file or playing a game with background music. It also allows you"
		    " to apply different effects to your system sounds and provides programmers with"
		    " an easy way to achieve sound support."));

	initAudioIOList();

	TQVBoxLayout *layout = new TQVBoxLayout(this, 0, KDialog::spacingHint());
	tab = new TQTabWidget(this);
	layout->addWidget(tab);

	general = new generalTab(tab);
	hardware = new hardwareTab(tab);
	//mixer = TDECModuleLoader::loadModule("kmixcfg", tab);
	//midi = new KMidConfig(tab, "tdemidconfig");

	general->layout()->setMargin( KDialog::marginHint() );
	hardware->layout()->setMargin( KDialog::marginHint() );
	general->latencyLabel->setFixedHeight(general->latencyLabel->fontMetrics().lineSpacing());

	tab->addTab(general, i18n("&General"));
	tab->addTab(hardware, i18n("&Hardware"));

	startServer = general->startServer;
	networkTransparent = general->networkTransparent;
	startRealtime = general->startRealtime;
	autoSuspend = general->autoSuspend;
	suspendTime = general->suspendTime;

	fullDuplex = hardware->fullDuplex;
	customDevice = hardware->customDevice;
	deviceName = hardware->deviceName;
	customRate = hardware->customRate;
	samplingRate = hardware->samplingRate;

   	TQString deviceHint = i18n("Normally, the sound server defaults to using the device called <b>/dev/dsp</b> for sound output. That should work in most cases. On some systems where devfs is used, however, you may need to use <b>/dev/sound/dsp</b> instead. Other alternatives are things like <b>/dev/dsp0</b> or <b>/dev/dsp1</b>, if you have a soundcard that supports multiple outputs, or you have multiple soundcards.");

	TQString rateHint = i18n("Normally, the sound server defaults to using a sampling rate of 44100 Hz (CD quality), which is supported on almost any hardware. If you are using certain <b>Yamaha soundcards</b>, you might need to configure this to 48000 Hz here, if you are using <b>old SoundBlaster cards</b>, like SoundBlaster Pro, you might need to change this to 22050 Hz. All other values are possible, too, and may make sense in certain contexts (i.e. professional studio equipment).");

	TQString optionsHint = i18n("This configuration module is intended to cover almost every aspect of the aRts sound server that you can configure. However, there are some things which may not be available here, so you can add <b>command line options</b> here which will be passed directly to <b>artsd</b>. The command line options will override the choices made in the GUI. To see the possible choices, open a Konsole window, and type <b>artsd -h</b>.");

	TQWhatsThis::add(customDevice, deviceHint);
	TQWhatsThis::add(deviceName, deviceHint);
	TQWhatsThis::add(customRate, rateHint);
	TQWhatsThis::add(samplingRate, rateHint);
	TQWhatsThis::add(hardware->customOptions, optionsHint);
	TQWhatsThis::add(hardware->addOptions, optionsHint);

	hardware->audioIO->insertItem( i18n( "Autodetect" ) );
	for (AudioIOElement *a = audioIOList.first(); a != 0; a = audioIOList.next())
		hardware->audioIO->insertItem(i18n(a->fullName.utf8()));

	deviceManager = new DeviceManager();
	deviceManager->initManager();

	TQString s;
	for ( int i = 0; i < deviceManager->midiPorts()+deviceManager->synthDevices(); i++)
	{
		if ( strcmp( deviceManager->type( i ), "" ) != 0 )
			s.sprintf( "%s - %s", deviceManager->name( i ), deviceManager->type( i ) );
		else
			s.sprintf( "%s", deviceManager->name( i ) );

		hardware->midiDevice->insertItem( s, i );

	};

	config = new TDEConfig("kcmartsrc");
	load();

	suspendTime->setRange( 1, 999, 1, true );

	connect(startServer,TQ_SIGNAL(clicked()),this,TQ_SLOT(slotChanged()));
	connect(networkTransparent,TQ_SIGNAL(clicked()),this,TQ_SLOT(slotChanged()));
	connect(startRealtime,TQ_SIGNAL(clicked()),this,TQ_SLOT(slotChanged()));
	connect(fullDuplex,TQ_SIGNAL(clicked()),this,TQ_SLOT(slotChanged()));
	connect(customDevice, TQ_SIGNAL(clicked()), TQ_SLOT(slotChanged()));
	connect(deviceName, TQ_SIGNAL(textChanged(const TQString&)), TQ_SLOT(slotChanged()));
	connect(customRate, TQ_SIGNAL(clicked()), TQ_SLOT(slotChanged()));
	connect(samplingRate, TQ_SIGNAL(valueChanged(const TQString&)), TQ_SLOT(slotChanged()));
//	connect(general->volumeSystray, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotChanged()) );

	connect(hardware->audioIO,TQ_SIGNAL(highlighted(int)),TQ_SLOT(slotChanged()));
	connect(hardware->audioIO,TQ_SIGNAL(activated(int)),TQ_SLOT(slotChanged()));
	connect(hardware->customOptions,TQ_SIGNAL(clicked()),TQ_SLOT(slotChanged()));
	connect(hardware->addOptions,TQ_SIGNAL(textChanged(const TQString&)),TQ_SLOT(slotChanged()));
	connect(hardware->soundQuality,TQ_SIGNAL(highlighted(int)),TQ_SLOT(slotChanged()));
	connect(hardware->soundQuality,TQ_SIGNAL(activated(int)),TQ_SLOT(slotChanged()));
	connect(general->latencySlider,TQ_SIGNAL(valueChanged(int)),TQ_SLOT(slotChanged()));
	connect(autoSuspend,TQ_SIGNAL(clicked()),TQ_SLOT(slotChanged()));
	connect(suspendTime,TQ_SIGNAL(valueChanged(int)),TQ_SLOT(slotChanged()));
	connect(general->testSound,TQ_SIGNAL(clicked()),TQ_SLOT(slotTestSound()));
	connect(hardware->midiDevice, TQ_SIGNAL( highlighted(int) ), this, TQ_SLOT( slotChanged() ) );
	connect(hardware->midiDevice, TQ_SIGNAL( activated(int) ), this, TQ_SLOT( slotChanged() ) );
	connect(hardware->midiUseMapper, TQ_SIGNAL( clicked() ), this, TQ_SLOT( slotChanged() ) );
	connect(hardware->midiMapper, TQ_SIGNAL( textChanged( const TQString& ) ),
			this, TQ_SLOT( slotChanged() ) );

	TDEAboutData *about =  new TDEAboutData(I18N_NOOP("kcmarts"),
                  I18N_NOOP("The Sound Server Control Module"),
                  0, 0, TDEAboutData::License_GPL,
                  I18N_NOOP("(c) 1999 - 2001, Stefan Westerfeld"));
	about->addAuthor("Stefan Westerfeld",I18N_NOOP("aRts Author") , "stw@kde.org");
	setAboutData(about);
}

void KArtsModule::load( bool useDefaults )
{
   config->setReadDefaults( useDefaults );
	config->setGroup("Arts");
	startServer->setChecked(config->readBoolEntry("StartServer",true));
	startRealtime->setChecked(config->readBoolEntry("StartRealtime",true) &&
	                          realtimeIsPossible());
	networkTransparent->setChecked(config->readBoolEntry("NetworkTransparent",false));
	fullDuplex->setChecked(config->readBoolEntry("FullDuplex",false));
	autoSuspend->setChecked(config->readBoolEntry("AutoSuspend",true));
	suspendTime->setValue(config->readNumEntry("SuspendTime",60));
	deviceName->setText(config->readEntry("DeviceName",TQString::null));
	customDevice->setChecked(!deviceName->text().isEmpty());
	hardware->addOptions->setText(config->readEntry("AddOptions",TQString::null));
	hardware->customOptions->setChecked(!hardware->addOptions->text().isEmpty());
	general->latencySlider->setValue(config->readNumEntry("Latency",250));

	int rate = config->readNumEntry("SamplingRate",0);
	if(rate)
	{
		customRate->setChecked(true);
		samplingRate->setValue(rate);
	}
	else
	{
		customRate->setChecked(false);
		samplingRate->setValue(44100);
	}

	switch (config->readNumEntry("Bits", 0)) {
	case 0:
		hardware->soundQuality->setCurrentItem(0);
		break;
	case 16:
		hardware->soundQuality->setCurrentItem(1);
		break;
	case 8:
		hardware->soundQuality->setCurrentItem(2);
		break;
	}

	TQString audioIO = config->readEntry("AudioIO", TQString::null);
	hardware->audioIO->setCurrentItem(0);
	for(AudioIOElement *a = audioIOList.first(); a != 0; a = audioIOList.next())
	{
		if(a->name == audioIO)		// first item: "autodetect"
		  {
			hardware->audioIO->setCurrentItem(audioIOList.at() + 1);
			break;
		  }

	}

//	config->setGroup( "Mixer" );
//	general->volumeSystray->setChecked( config->readBoolEntry( "VolumeControlOnSystray", true ) );

	TDEConfig *midiConfig = new TDEConfig( "kcmmidirc", true );

	midiConfig->setGroup( "Configuration" );
	hardware->midiDevice->setCurrentItem( midiConfig->readNumEntry( "midiDevice", 0 ) );
	TQString mapurl( midiConfig->readPathEntry( "mapFilename" ) );
	hardware->midiMapper->setURL( mapurl );
	hardware->midiUseMapper->setChecked( midiConfig->readBoolEntry( "useMidiMapper", false ) );
	hardware->midiMapper->setEnabled( hardware->midiUseMapper->isChecked() );

	delete midiConfig;

	updateWidgets();
   emit changed( useDefaults );
}

KArtsModule::~KArtsModule() {
        delete config;
        audioIOList.setAutoDelete(true);
        audioIOList.clear();
}

void KArtsModule::saveParams( void )
{
	TQString audioIO;

	int item = hardware->audioIO->currentItem() - 1;	// first item: "default"

	if (item >= 0) {
		audioIO = audioIOList.at(item)->name;
	}

	TQString dev = customDevice->isChecked() ? deviceName->text() : TQString::null;
	int rate = customRate->isChecked()?samplingRate->value() : 0;
	TQString addOptions;
	if(hardware->customOptions->isChecked())
		addOptions = hardware->addOptions->text();

	int latency = general->latencySlider->value();
	int bits = 0;

	if (hardware->soundQuality->currentItem() == 1)
		bits = 16;
	else if (hardware->soundQuality->currentItem() == 2)
		bits = 8;

	config->setGroup("Arts");
	config->writeEntry("StartServer",startServer->isChecked());
	config->writeEntry("StartRealtime",startRealtime->isChecked());
	config->writeEntry("NetworkTransparent",networkTransparent->isChecked());
	config->writeEntry("FullDuplex",fullDuplex->isChecked());
	config->writeEntry("DeviceName",dev);
	config->writeEntry("SamplingRate",rate);
	config->writeEntry("AudioIO",audioIO);
	config->writeEntry("AddOptions",addOptions);
	config->writeEntry("Latency",latency);
	config->writeEntry("Bits",bits);
	config->writeEntry("AutoSuspend", autoSuspend->isChecked());
	config->writeEntry("SuspendTime", suspendTime->value());
	calculateLatency();
	// Save arguments string in case any other process wants to restart artsd.

	config->writeEntry("Arguments",
		createArgs(networkTransparent->isChecked(), fullDuplex->isChecked(),
					fragmentCount, fragmentSize, dev, rate, bits,
					audioIO, addOptions, autoSuspend->isChecked(),
					suspendTime->value() ));

//	config->setGroup( "Mixer" );
//	config->writeEntry( "VolumeControlOnSystray", general->volumeSystray->isChecked() );

	TDEConfig *midiConfig = new TDEConfig( "kcmmidirc", false );

	midiConfig->setGroup( "Configuration" );
	midiConfig->writeEntry( "midiDevice", hardware->midiDevice->currentItem() );
	midiConfig->writeEntry( "useMidiMapper", hardware->midiUseMapper->isChecked() );
	midiConfig->writePathEntry( "mapFilename", hardware->midiMapper->url() );

	delete midiConfig;
    
    TDEConfig *knotifyConfig = new TDEConfig(  "knotifyrc", false );
    
    knotifyConfig->setGroup(  "StartProgress" );
    knotifyConfig->writeEntry(  "Arts Init", startServer->isChecked() );
    knotifyConfig->writeEntry(  "Use Arts", startServer->isChecked() );
    
    delete knotifyConfig;

	config->sync();
}

void KArtsModule::load()
{
   load( false );
}

void KArtsModule::save()
{
	if (configChanged) {
		configChanged = false;
		saveParams();
		restartServer();
		updateWidgets();
	}
	emit changed( false );
}

TQString KArtsModule::handbookSection() const
{
 	int index = tab->currentPageIndex();
 	if (index == 0)
		return "sndserver-general";
	else if (index == 1)
		return "sndserver-soundio";
 	else
 		return TQString::null;
}

int KArtsModule::userSavedChanges()
{
	int reply;

	if (!configChanged)
		return KMessageBox::Yes;

	TQString question = i18n("The settings have changed since the last time "
                            "you restarted the sound server.\n"
                            "Do you want to save them?");
	TQString caption = i18n("Save Sound Server Settings?");
	reply = KMessageBox::questionYesNo(this, question, caption,KStdGuiItem::save(),KStdGuiItem::discard());
	if ( reply == KMessageBox::Yes)
	{
        configChanged = false;
        saveParams();
	}

    return reply;
}

void KArtsModule::slotTestSound()
{
	if (configChanged && (userSavedChanges() == KMessageBox::Yes) || !artsdIsRunning() )
		restartServer();

	TDEProcess test;
	test << "artsplay";
	test << locate("sound", "KDE_Startup_1.ogg");
	test.start(TDEProcess::DontCare);
}

void KArtsModule::defaults()
{
   load( true );
}

void KArtsModule::calculateLatency()
{
	int latencyInBytes, latencyInMs;

	if(general->latencySlider->value() < 490)
	{
		int rate = customRate->isChecked() ? samplingRate->text().toLong() : 44100;

		if (rate < 4000 || rate > 200000) {
			rate = 44100;
		}

		int sampleSize = (hardware->soundQuality->currentItem() == 2) ? 2 : 4;

		latencyInBytes = general->latencySlider->value()*rate*sampleSize/1000;

		fragmentSize = 2;
		do {
			fragmentSize *= 2;
			fragmentCount = latencyInBytes / fragmentSize;
		} while (fragmentCount > 8 && fragmentSize != 4096);

		latencyInMs = (fragmentSize*fragmentCount*1000) / rate / sampleSize;
		general->latencyLabel->setText(
						  i18n("%1 milliseconds (%2 fragments with %3 bytes)")
						  .arg(latencyInMs).arg(fragmentCount).arg(fragmentSize));
	}
	else
	{
		fragmentCount = 128;
		fragmentSize = 8192;
		general->latencyLabel->setText(i18n("as large as possible"));
	}
}

void KArtsModule::updateWidgets()
{
	bool startServerIsChecked = startServer->isChecked();
	if (startRealtime->isChecked() && !realtimeIsPossible()) {
		startRealtime->setChecked(false);
		KMessageBox::error(this, i18n("Impossible to start aRts with realtime "
		                              "priority because artswrapper is "
		                              "missing or disabled"));
	}
	deviceName->setEnabled(customDevice->isChecked());
	TQString audioIO;
	int item = hardware->audioIO->currentItem() - 1;	// first item: "default"
	if (item >= 0)
	{
		audioIO = audioIOList.at(item)->name;
		bool jack = (audioIO == TQString::fromLatin1("jack"));
		if(jack)
		{
			customRate->setChecked(false);
			hardware->soundQuality->setCurrentItem(0);
			autoSuspend->setChecked(false);
		}
		customRate->setEnabled(!jack);
		hardware->soundQuality->setEnabled(!jack);
		autoSuspend->setEnabled(!jack);
	}
	samplingRate->setEnabled(customRate->isChecked());
	hardware->addOptions->setEnabled(hardware->customOptions->isChecked());
	suspendTime->setEnabled(autoSuspend->isChecked());
	calculateLatency();

	general->testSound->setEnabled(startServerIsChecked);

//	general->volumeSystray->setEnabled(startServerIsChecked);
	general->networkedSoundGroupBox->setEnabled(startServerIsChecked);
	general->realtimeGroupBox->setEnabled(startServerIsChecked);
	general->autoSuspendGroupBox->setEnabled(startServerIsChecked);
	hardware->setEnabled(startServerIsChecked);
	hardware->midiMapper->setEnabled( hardware->midiUseMapper->isChecked() );
}

void KArtsModule::slotChanged()
{
	updateWidgets();
	configChanged = true;
	emit changed(true);
}

/* check if starting realtime would be possible */
bool KArtsModule::realtimeIsPossible()
{
	static bool checked = false;
	if (!checked)
	{
	TDEProcess* checkProcess = new TDEProcess();
	*checkProcess << "artswrapper";
	*checkProcess << "check";

	connect(checkProcess, TQ_SIGNAL(processExited(TDEProcess*)),
	        this, TQ_SLOT(slotArtsdExited(TDEProcess*)));
	if (!checkProcess->start(TDEProcess::Block))
	{
		delete checkProcess;
		realtimePossible =  false;
	}
	else if (latestProcessStatus == 0)
	{
		realtimePossible =  true;
	}
	else
	{
		realtimePossible =  false;
	}

	checked = true;

	}
	return realtimePossible;
}

void KArtsModule::restartServer()
{
	config->setGroup("Arts");
        bool starting = config->readBoolEntry("StartServer", true);
	bool restarting = artsdIsRunning();

	// Shut down knotify
	DCOPRef("knotify", "qt/knotify").send("quit");

	// Shut down artsd
	TDEProcess terminateArts;
	terminateArts << "artsshell";
	terminateArts << "terminate";
	terminateArts.start(TDEProcess::Block);

	if (starting)
	{
		// Wait for artsd to shutdown completely and then (re)start artsd again
		KStartArtsProgressDialog dlg(this, "start_arts_progress",
	                       restarting ? i18n("Restarting Sound System") : i18n("Starting Sound System"),
	                       restarting ? i18n("Restarting sound system.") : i18n("Starting sound system."));
	        dlg.exec();
	}

	// Restart knotify
	tdeApp->startServiceByDesktopName("knotify");
}

bool KArtsModule::artsdIsRunning()
{
	TDEProcess check;
	check << "artsshell";
	check << "status";
	check.start(TDEProcess::Block);

	return (check.exitStatus() == 0);
}


void init_arts()
{
	startArts();
}

TQString KArtsModule::createArgs(bool netTrans,
                                bool duplex, int fragmentCount,
                                int fragmentSize,
                                const TQString &deviceName,
                                int rate, int bits, const TQString &audioIO,
                                const TQString &addOptions, bool autoSuspend,
                                int suspendTime
                                )
{
	TQString args;

	if(fragmentCount)
		args += TQString::fromLatin1(" -F %1").arg(fragmentCount);

	if(fragmentSize)
		args += TQString::fromLatin1(" -S %1").arg(fragmentSize);

	if (!audioIO.isEmpty())
		args += TQString::fromLatin1(" -a %1").arg(audioIO);

	if (duplex)
		args += TQString::fromLatin1(" -d");

	if (netTrans)
		args += TQString::fromLatin1(" -n");

	if (!deviceName.isEmpty())
		args += TQString::fromLatin1(" -D ") + deviceName;

	if (rate)
		args += TQString::fromLatin1(" -r %1").arg(rate);

	if (bits)
		args += TQString::fromLatin1(" -b %1").arg(bits);

	if (autoSuspend && suspendTime)
		args += TQString::fromLatin1(" -s %1").arg(suspendTime);

	if (!addOptions.isEmpty())
		args += TQChar(' ') + addOptions;

	args += TQString::fromLatin1(" -m artsmessage");
	args += TQString::fromLatin1(" -c drkonqi");
	args += TQString::fromLatin1(" -l 3");
	args += TQString::fromLatin1(" -f");

	return args;
}

KStartArtsProgressDialog::KStartArtsProgressDialog(KArtsModule *parent, const char *name,
                          const TQString &caption, const TQString &text)
 : KProgressDialog(parent, name, caption, text, true), m_module(parent), m_shutdown(false)
{
  connect(&m_timer, TQ_SIGNAL(timeout()), this, TQ_SLOT(slotProgress()));
  progressBar()->setTotalSteps(20);
  m_timeStep = 700;
  m_timer.start(m_timeStep);
  setAutoClose(false);
}

void
KStartArtsProgressDialog::slotProgress()
{
  int p = progressBar()->progress();
  if (p == 18)
  {
     progressBar()->reset();
     progressBar()->setProgress(1);
     m_timeStep = m_timeStep * 2;
     m_timer.start(m_timeStep);
  }
  else
  {
     progressBar()->setProgress(p+1);
  }

  if (!m_shutdown)
  {
     // Wait for arts to shutdown
     if (!m_module->artsdIsRunning())
     {
     	// Shutdown complete, restart
     	if (!startArts())
     		slotFinished(); // Strange, it didn't start
     	else
	     	m_shutdown = true;
     }
  }
  
  // Shut down completed? Wait for artsd to come up again
  if (m_shutdown && m_module->artsdIsRunning())
     slotFinished(); // Restart complete
}

void
KStartArtsProgressDialog::slotFinished()
{
  progressBar()->setProgress(20);
  m_timer.stop();
  TQTimer::singleShot(1000, this, TQ_SLOT(close()));
}


#ifdef I18N_ONLY
	//lukas: these are hacks to allow translation of the following
	I18N_NOOP("No Audio Input/Output");
	I18N_NOOP("Advanced Linux Sound Architecture");
	I18N_NOOP("Open Sound System");
	I18N_NOOP("Threaded Open Sound System");
	I18N_NOOP("Network Audio System");
	I18N_NOOP("Personal Audio Device");
	I18N_NOOP("SGI dmedia Audio I/O");
	I18N_NOOP("Sun Audio Input/Output");
	I18N_NOOP("Portable Audio Library");
	I18N_NOOP("Enlightened Sound Daemon");
	I18N_NOOP("MAS Audio Input/Output");
	I18N_NOOP("Jack Audio Connection Kit");
#endif

#include "arts.moc"
