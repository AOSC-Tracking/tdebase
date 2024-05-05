/**
 * smartcard.cpp
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
 * Copyright (c) 2001 Fernando Llobregat <fernando.llobregat@free.fr>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqpushbutton.h>

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <kcarddb.h>
#include <kcardfactory.h>
#include <kcardgsm_impl.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>

#include "smartcard.h"

KSmartcardConfig::KSmartcardConfig(TQWidget *parent, const char *name)
  : TDECModule(parent, name),DCOPObject(name)
{

  TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
  config = new TDEConfig("ksmartcardrc", false, false);

  DCOPClient *dc = TDEApplication::kApplication()->dcopClient();

  _ok = false;
  dc->remoteInterfaces("kded", "kardsvc", &_ok);

  TDEAboutData *about =
  new TDEAboutData(I18N_NOOP("kcmsmartcard"), I18N_NOOP("TDE Smartcard Control Module"),
                0, 0, TDEAboutData::License_GPL,
                I18N_NOOP("(c) 2001 George Staikos"));

  about->addAuthor("George Staikos", 0, "staikos@kde.org");
  setAboutData( about );

  if (_ok) {


     base = new SmartcardBase(this);
     layout->add(base);

     _popUpKardChooser = new TDEPopupMenu(this,"KpopupKardChooser");
     _popUpKardChooser->insertItem(i18n("Change Module..."),
				   this,
				   TQ_SLOT(slotLaunchChooser()));
     // The config backend

     connect(base->launchManager, TQ_SIGNAL(clicked()), TQ_SLOT( changed() ));
     connect(base->beepOnInsert,  TQ_SIGNAL(clicked()), TQ_SLOT( changed() ));
     connect(base->enableSupport, TQ_SIGNAL(clicked()), TQ_SLOT( changed() ));


     connect(base->enablePolling, TQ_SIGNAL(clicked()), TQ_SLOT( changed() ));
     connect(base->_readerHostsListView,
	     TQ_SIGNAL(rightButtonPressed(TQListViewItem *,const TQPoint &,int)),
	     this,
	     TQ_SLOT(slotShowPopup(TQListViewItem *,const TQPoint &,int)));



     if (!connectDCOPSignal("",
			    "",
			    "signalReaderListChanged(TQStringList)",
			    "loadReadersTab(TQStringList)",
			    FALSE))

       kdDebug()<<"Error connecting to DCOP server" <<endl;


     if (!connectDCOPSignal("",
			    "",
			    "signalCardStateChanged(TQString,bool,TQString)",
			    "updateReadersState (TQString,bool,TQString) ",
			    FALSE))

       kdDebug()<<"Error connecting to DCOP server" <<endl;
     _cardDB= new KCardDB();
     load();
  } else {
     layout->add(new NoSmartcardBase(this));
  }
}




KSmartcardConfig::~KSmartcardConfig()
{
    delete config;
    delete _cardDB;
}

void KSmartcardConfig::slotLaunchChooser(){


  if ( KCardDB::launchSelector(base->_readerHostsListView->currentItem()->parent()->text(0))){

    KMessageBox::sorry(this,i18n("Unable to launch KCardChooser"));
  }


}

void KSmartcardConfig::slotShowPopup(TQListViewItem * item ,const TQPoint & _point,int i)
{

  //The popup only appears in cards, not in the slots1
  if (item->isSelectable()) return;
  _popUpKardChooser->exec(_point);

}


void KSmartcardConfig::updateReadersState (TQString readerName,
                                           bool isCardPresent,
                                           TQString atr) {

    TDEListViewItem * tID=(TDEListViewItem *) base->_readerHostsListView->findItem(readerName, 0);
    if (tID==0) return;

    TDEListViewItem * tIDChild=(TDEListViewItem*) tID->firstChild();
    if (tIDChild==NULL) return;

    delete tIDChild;

    if (!isCardPresent)
                (void) new TDEListViewItem(tID,i18n("No card inserted"));
    else{

        getSupportingModule(tID,atr);
    }


}



void KSmartcardConfig::loadReadersTab( TQStringList lr){

  //Prepare data for dcop calls
  TQByteArray data, retval;
  TQCString rettype;
  TQDataStream arg(data, IO_WriteOnly);
  TQCString modName = "kardsvc";
  arg << modName;

  //  New view items
  TDEListViewItem * temp;

  //If the smartcard support is disabled we unload the kardsvc KDED module
  //  and return

  base->_readerHostsListView->clear();

  if (!config->readBoolEntry("Enable Support", false)){




    //  New view items
    TDEListViewItem * temp;
    kapp->dcopClient()->call("kded", "kded", "unloadModule(TQCString)",
			     data, rettype, retval);

    (void) new TDEListViewItem(base->_readerHostsListView,
			     i18n("Smart card support disabled"));


    return;

  }

  if (lr.isEmpty()){


    (void) new TDEListViewItem(base->_readerHostsListView,
			     i18n("No readers found. Check 'pcscd' is running"));
    return;
  }

  for (TQStringList::Iterator _slot=lr.begin();_slot!=lr.end();++_slot){

   temp= new TDEListViewItem(base->_readerHostsListView,*_slot);


   TQByteArray dataATR;
   TQDataStream argATR(dataATR,IO_WriteOnly);
   argATR << *_slot;

   kapp->dcopClient()->call("kded", "kardsvc", "getCardATR(TQString)",
			   dataATR, rettype, retval);


   TQString cardATR;
   TQDataStream retReaderATR(retval, IO_ReadOnly);
   retReaderATR>>cardATR;

   if (cardATR.isNull()){

     (void) new TDEListViewItem(temp,i18n("NO ATR or no card inserted"));
     continue;
   }

   getSupportingModule(temp,cardATR);




  }

}


void KSmartcardConfig::getSupportingModule( TDEListViewItem * ant,
                                            TQString & cardATR) const{


    if (cardATR.isNull()){

        (void) new TDEListViewItem(ant,i18n("NO ATR or no card inserted"));
        return;
    }


    TQString modName=_cardDB->getModuleName(cardATR);
    if (!modName.isNull()){
        TQStringList mng= TQStringList::split(",",modName);
        TQString type=mng[0];
        TQString subType=mng[1];
        TQString subSubType=mng[2];
        TDEListViewItem * hil =new TDEListViewItem(ant,
                                               i18n("Managed by: "),
                                               type,
                                               subType,
                                               subSubType);
        hil->setSelectable(FALSE);
    }
    else{


        TDEListViewItem * hil =new TDEListViewItem(ant,
                                               i18n("No module managing this card"));
        hil->setSelectable(FALSE);
    }

  }
void KSmartcardConfig::load()
{
	load( false );

void KSmartcardConfig::load(bool useDefaults )
{

  //Prepare data for dcop calls
  TQByteArray data, retval;
  TQCString rettype;
  TQDataStream arg(data, IO_WriteOnly);
  TQCString modName = "kardsvc";
  arg << modName;

  //Update the toggle buttons with the current configuration

  config->setReadDefaults( useDefaults );

  if (_ok) {
  base->enableSupport->setChecked(config->readBoolEntry("Enable Support",
							false));
  base->enablePolling->setChecked(config->readBoolEntry("Enable Polling",
							true));
  base->beepOnInsert->setChecked(config->readBoolEntry("Beep on Insert",
						       true));
  base->launchManager->setChecked(config->readBoolEntry("Launch Manager",
							true));
  }

  // We call kardsvc to retrieve the current readers
  kapp->dcopClient()->call("kded", "kardsvc", "getSlotList ()",
			   data, rettype, retval);
  TQStringList readers;
  readers.clear();
  TQDataStream retReader(retval, IO_ReadOnly);
  retReader>>readers;

  //And we update the panel
  loadReadersTab(readers);

  emit changed(useDefaults);

}


void KSmartcardConfig::save()
{
if (_ok) {
  config->writeEntry("Enable Support", base->enableSupport->isChecked());
  config->writeEntry("Enable Polling", base->enablePolling->isChecked());
  config->writeEntry("Beep on Insert", base->beepOnInsert->isChecked());
  config->writeEntry("Launch Manager", base->launchManager->isChecked());


  TQByteArray data, retval;
  TQCString rettype;
  TQDataStream arg(data, IO_WriteOnly);
  TQCString modName = "kardsvc";
  arg << modName;

  // Start or stop the server as needed
  if (base->enableSupport->isChecked()) {

    kapp->dcopClient()->call("kded", "kded", "loadModule(TQCString)",
			     data, rettype, retval);
    config->sync();

    kapp->dcopClient()->call("kded", "kardsvc", "reconfigure()",
			     data, rettype, retval);
  } else {



    kapp->dcopClient()->call("kded", "kded", "unloadModule(TQCString)",
			     data, rettype, retval);
  }


}
  emit changed(false);
}

void KSmartcardConfig::defaults()
{
	load( true );
}



TQString KSmartcardConfig::quickHelp() const
{
  return i18n("<h1>smartcard</h1> This module allows you to configure TDE support"
     " for smartcards. These can be used for various tasks such as storing"
     " SSL certificates and logging in to the system.");
}

extern "C"
{
  TDE_EXPORT TDECModule *create_smartcard(TQWidget *parent, const char *)
  {
    return new KSmartcardConfig(parent, "kcmsmartcard");
  }

  TDE_EXPORT void init_smartcard()
  {
    TDEConfig *config = new TDEConfig("ksmartcardrc", false, false);
    bool start = config->readBoolEntry("Enable Support", false);
    delete config;

    if (start) {
	TQByteArray data, retval;
	TQCString rettype;
	TQDataStream arg(data, IO_WriteOnly);
	TQCString modName = "kardsvc";
	arg << modName;
	kapp->dcopClient()->call("kded", "kded", "loadModule(TQCString)",
			         data, rettype, retval);
    }
  }
}


#include "smartcard.moc"

