/**
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
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

#include <tqlabel.h>
#include <tqgroupbox.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqcheckbox.h>
#include <tqslider.h>

#include <tdeapplication.h>
#include <tdeglobal.h>
#include <dcopclient.h>
#include <tdelocale.h>
#include <kdialog.h>
#include <klineedit.h>
#include <knuminput.h>
#include <tdeconfig.h>

#include <netwm.h>

#include "desktop.h"
#include "desktop.moc"

extern "C"
{
  TDE_EXPORT TDECModule *create_virtualdesktops(TQWidget *parent, const char * /*name*/)
  {
    return new KDesktopConfig(parent, "kcmkonq");
  }
}

// I'm using lineedits by intention as it makes sence to be able
// to see all desktop names at the same time. It also makes sense to
// be able to TAB through those line edits fast. So don't send me mails
// asking why I did not implement a more intelligent/smaller GUI.

KDesktopConfig::KDesktopConfig(TQWidget *parent, const char * /*name*/)
  : TDECModule(parent, "kcmkonq")
{

  setQuickHelp( i18n("<h1>Multiple Desktops</h1>In this module, you can configure how many virtual desktops you want and how these should be labeled."));

  Q_ASSERT(maxDesktops % 2 == 0);

  TQVBoxLayout *layout = new TQVBoxLayout(this, 0, KDialog::spacingHint());

  // number group
  TQGroupBox *number_group = new TQGroupBox(this);

  TQHBoxLayout *lay = new TQHBoxLayout(number_group,
                     KDialog::marginHint(),
                     KDialog::spacingHint());

  TQLabel *label = new TQLabel(i18n("N&umber of desktops: "), number_group);
  _numInput = new KIntNumInput(4, number_group);
  _numInput->setRange(1, maxDesktops, 1, true);
  connect(_numInput, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotValueChanged(int)));
  connect(_numInput, TQ_SIGNAL(valueChanged(int)),  TQ_SLOT( changed() ));
  label->setBuddy( _numInput );
  TQString wtstr = i18n( "Here you can set how many virtual desktops you want on your TDE desktop. Move the slider to change the value." );
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( _numInput, wtstr );

  lay->addWidget(label);
  lay->addWidget(_numInput);
  lay->setStretchFactor( _numInput, 2 );

  layout->addWidget(number_group);

  // name group
  TQGroupBox *name_group = new TQGroupBox(i18n("Desktop &Names"), this);

  name_group->setColumnLayout(4, TQt::Horizontal);

  for(int i = 0; i < (maxDesktops/2); i++)
    {
      _nameLabel[i] = new TQLabel(i18n("Desktop %1:").arg(i+1), name_group);
      _nameInput[i] = new KLineEdit(name_group);
      _nameLabel[i+(maxDesktops/2)] = new TQLabel(i18n("Desktop %1:").arg(i+(maxDesktops/2)+1), name_group);
      _nameInput[i+(maxDesktops/2)] = new KLineEdit(name_group);
      TQWhatsThis::add( _nameLabel[i], i18n( "Here you can enter the name for desktop %1" ).arg( i+1 ) );
      TQWhatsThis::add( _nameInput[i], i18n( "Here you can enter the name for desktop %1" ).arg( i+1 ) );
      TQWhatsThis::add( _nameLabel[i+(maxDesktops/2)], i18n( "Here you can enter the name for desktop %1" ).arg( i+(maxDesktops/2)+1 ) );
      TQWhatsThis::add( _nameInput[i+(maxDesktops/2)], i18n( "Here you can enter the name for desktop %1" ).arg( i+(maxDesktops/2)+1 ) );

      connect(_nameInput[i], TQ_SIGNAL(textChanged(const TQString&)),
           TQ_SLOT( changed() ));
      connect(_nameInput[i+(maxDesktops/2)], TQ_SIGNAL(textChanged(const TQString&)),
           TQ_SLOT( changed() ));
    }

  for(int i = 1; i < maxDesktops; i++)
      setTabOrder( _nameInput[i-1], _nameInput[i] );

  layout->addWidget(name_group);

  _wheelOption = new TQCheckBox(i18n("Mouse wheel over desktop background switches desktop"), this);
  connect(_wheelOption,TQ_SIGNAL(toggled(bool)), TQ_SLOT( changed() ));

  layout->addWidget(_wheelOption);
  layout->addStretch(1);

  load();
}

void KDesktopConfig::load()
{
	load( false );
}
	
void KDesktopConfig::load( bool useDefaults )
{
  // get number of desktops
  NETRootInfo info( tqt_xdisplay(), NET::NumberOfDesktops | NET::DesktopNames );
  int n = info.numberOfDesktops();

  int konq_screen_number = 0;
  if (tqt_xdisplay())
     konq_screen_number = DefaultScreen(tqt_xdisplay());

  TQCString groupname;
  if (konq_screen_number == 0)
     groupname = "Desktops";
  else
     groupname.sprintf("Desktops-screen-%d", konq_screen_number);

  TDEConfig * twinconfig = new TDEConfig("twinrc");

  twinconfig->setReadDefaults( useDefaults );

  twinconfig->setGroup("Desktops");
  for(int i = 1; i <= maxDesktops; i++)
  {
    TQString key_name(TQString("Name_") + TQString::number(i));
    TQString name = TQString::fromUtf8(info.desktopName(i));
    if (name.isEmpty()) // Get name from configuration if none is set in the WM.
    {
        name = twinconfig->readEntry(key_name, i18n("Desktop %1").arg(i));
    }
    _nameInput[i-1]->setText(name);

    // Is this entry immutable or not in the range of configured desktops?
    _labelImmutable[i - 1] = twinconfig->entryIsImmutable(key_name);
    _nameInput[i-1]->setEnabled(i <= n && !_labelImmutable[i - 1]);
  }

  _numInput->setEnabled(!twinconfig->entryIsImmutable("Number"));

  delete twinconfig;
  twinconfig = 0;

  TQString configfile;
  if (konq_screen_number == 0)
      configfile = "kdesktoprc";
  else
      configfile.sprintf("kdesktop-screen-%drc", konq_screen_number);

  TDEConfig *config = new TDEConfig(configfile, false, false);

  config->setReadDefaults( useDefaults );

  config->setGroup("Mouse Buttons");
  _wheelOption->setChecked(config->readBoolEntry("WheelSwitchesWorkspace",false));

  _wheelOptionImmutable = config->entryIsImmutable("WheelSwitchesWorkspace");

  if (_wheelOptionImmutable || n<2)
     _wheelOption->setEnabled( false );

  delete config;
  config = 0;

  _numInput->setValue(n);
  emit changed( useDefaults );
}

void KDesktopConfig::save()
{
  NETRootInfo info( tqt_xdisplay(), NET::NumberOfDesktops | NET::DesktopNames );
  // set desktop names
  for(int i = 1; i <= maxDesktops; i++)
  {
    info.setDesktopName(i, (_nameInput[i-1]->text()).utf8());
    info.activate();
  }
  // set number of desktops
  info.setNumberOfDesktops(_numInput->value());
  info.activate();

  XSync(tqt_xdisplay(), false);

  int konq_screen_number = 0;
  if (tqt_xdisplay())
     konq_screen_number = DefaultScreen(tqt_xdisplay());

  TQCString appname;
  if (konq_screen_number == 0)
      appname = "kdesktop";
  else
      appname.sprintf("kdesktop-screen-%d", konq_screen_number);

  TDEConfig *config = new TDEConfig(appname + "rc");
  config->setGroup("Mouse Buttons");
  config->writeEntry("WheelSwitchesWorkspace", _wheelOption->isChecked());
  delete config;

  // Tell kdesktop about the new config file
  if ( !kapp->dcopClient()->isAttached() )
     kapp->dcopClient()->attach();
  TQByteArray data;

  kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );

  emit changed(false);
}

void KDesktopConfig::defaults()
{
	load( true );
}

void KDesktopConfig::slotValueChanged(int n)
{
  for(int i = 0; i < maxDesktops; i++)
  { _nameInput[i]->setEnabled(i < n && !_labelImmutable[i]); }
  if (!_wheelOptionImmutable)
  { _wheelOption->setEnabled(n>1); }
  emit changed(true);
}
