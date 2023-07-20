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

#ifndef KARTSCONFIG_H
#define KARTSCONFIG_H

#include <tdeapplication.h>

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlineedit.h>
#include <tqradiobutton.h>
#include <tqtimer.h>

#include <tdecmodule.h>
#include <knuminput.h>
#include <kprogress.h>

#include "generaltab.h"
#include "hardwaretab.h"


class TDEProcess;
class DeviceManager;

class KArtsModule : public TDECModule
{
  TQ_OBJECT

public:

  KArtsModule(TQWidget *parent=0, const char *name=0);
  ~KArtsModule();
  void saveParams( void );

  void load();
  void load( bool useDefaults );
  void save();
  void defaults();
  virtual TQString handbookSection() const;
  
  bool artsdIsRunning();

private slots:

  void slotChanged();
  void slotTestSound();
  void slotArtsdExited(TDEProcess* proc);
  void slotProcessArtsdOutput(TDEProcess* p, char* buf, int len);
  //void slotStartServerChanged();

private:

  void updateWidgets ();
  void calculateLatency();
  TQString createArgs(bool netTrans,bool duplex, int fragmentCount,
                     int fragmentSize,
                     const TQString &deviceName,
                     int rate, int bits, const TQString &audioIO,
                     const TQString &addOptions, bool autoSuspend,
                     int suspendTime);
  int userSavedChanges();

  TQCheckBox *startServer, *startRealtime, *networkTransparent,
  			*fullDuplex, *customDevice, *customRate, *autoSuspend;
  TQLineEdit *deviceName;
  TQSpinBox *samplingRate;
  KIntNumInput *suspendTime;
  TQTabWidget *tab;
  generalTab *general;
  hardwareTab *hardware;
  TDEConfig *config;
  DeviceManager *deviceManager;
  int latestProcessStatus;
  int fragmentCount;
  int fragmentSize;
  bool configChanged;
  bool realtimePossible;

  class AudioIOElement {
  public:
	  AudioIOElement(const TQString &name, const TQString &fullName)
		  : name(name), fullName(fullName) {;}
	  TQString name;
	  TQString fullName;
  };

  void initAudioIOList();
  TQPtrList<AudioIOElement> audioIOList;

  void restartServer();
  bool realtimeIsPossible();
};


class KStartArtsProgressDialog : public KProgressDialog
{
   TQ_OBJECT
public:
   KStartArtsProgressDialog(KArtsModule *parent, const char *name,
                          const TQString &caption, const TQString &text);
public slots:   
   void slotProgress();
   void slotFinished();

private:
   TQTimer m_timer;
   int m_timeStep;
   KArtsModule *m_module;
   bool m_shutdown;
};

#endif

