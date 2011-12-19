/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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

*/

#include <unistd.h> // for getuid()

#include <kpushbutton.h>
#include <tqlayout.h>
#include <klocale.h>
#include <kapplication.h>
#include <kcmodule.h>
#include <kseparator.h>
#include <kdialog.h>
#include <kstdguiitem.h>
#include <dcopclient.h>

#include <tqwhatsthis.h>
#include <tqlabel.h>

#include "global.h"
#include "proxywidget.h"
#include "proxywidget.moc"

#include <kdebug.h>
#include <tqtimer.h>

class WhatsThis : public TQWhatsThis
{
public:
    WhatsThis( ProxyWidget* parent )
    : TQWhatsThis( parent ), proxy( parent ) {}
    ~WhatsThis(){};


    TQString text( const TQPoint &  ) {
    if ( !proxy->quickHelp().isEmpty() )
        return proxy->quickHelp();
    else
        return i18n("The currently loaded configuration module.");
    }

private:
    ProxyWidget* proxy;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

static void trinity_setVisible(TQPushButton *btn, bool vis)
{
  if (vis)
    btn->show();
  else
    btn->hide();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////


class RootInfoWidget : public TQLabel
{
public:
    RootInfoWidget(TQWidget *parent, const char *name);
    void setRootMsg(const TQString& s) { setText(s); }
};

RootInfoWidget::RootInfoWidget(TQWidget *parent, const char *name = 0)
    : TQLabel(parent, name)
{
    setFrameShape(TQFrame::Box);
    setFrameShadow(TQFrame::Raised);

    setText(i18n("<b>Changes in this module require root access.</b><br>"
                      "Click the \"Administrator Mode\" button to "
                      "allow modifications in this module."));

	TQWhatsThis::add(this, i18n("This module requires special permissions, probably "
                              "for system-wide modifications; therefore, it is "
                              "required that you provide the root password to be "
                              "able to change the module's properties.  If you "
                              "do not provide the password, the module will be "
                              "disabled."));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

class ProxyView : public TQScrollView
{
public:
    ProxyView(KCModule *client, const TQString& title, TQWidget *parent, bool run_as_root, const char *name);

private:
    virtual void resizeEvent(TQResizeEvent *);

    TQWidget *contentWidget;
    KCModule    *client;
    bool scroll;
};

class ProxyContentWidget : public TQWidget
{
public:
    ProxyContentWidget( TQWidget* parent ) : TQWidget( parent ) {}
    ~ProxyContentWidget(){}

    // this should be really done by qscrollview in AutoOneFit mode!
    TQSize sizeHint() const { return minimumSizeHint(); }
};


ProxyView::ProxyView(KCModule *_client, const TQString&, TQWidget *parent, bool run_as_root, const char *name)
    : TQScrollView(parent, name), client(_client)
{
  setResizePolicy(TQScrollView::AutoOneFit);
  setFrameStyle( NoFrame );
  contentWidget = new ProxyContentWidget( viewport() );

  TQVBoxLayout* vbox = new TQVBoxLayout( contentWidget );

  if (run_as_root && _client->useRootOnlyMsg()) // notify the user
  {
      RootInfoWidget *infoBox = new RootInfoWidget(contentWidget);
      vbox->addWidget( infoBox );
      TQString msg = _client->rootOnlyMsg();
      if (!msg.isEmpty())
	      infoBox->setRootMsg(msg);
      vbox->setSpacing(KDialog::spacingHint());
  }
  client->reparent(contentWidget,0,TQPoint(0,0),true);
  vbox->addWidget( client );
  vbox->activate(); // make sure we have a proper minimumSizeHint
  addChild(contentWidget);
}

void ProxyView::resizeEvent(TQResizeEvent *e)
{
    TQScrollView::resizeEvent(e);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////


ProxyWidget::ProxyWidget(KCModule *client, TQString title, const char *name,
             bool run_as_root)
  : TQWidget(0, name)
  , _client(client)
{
 setCaption(title);

 if (getuid()==0 ) {
	 // Make root modules look as similar as possible...
	 TQCString replyType;
	 TQByteArray replyData;
	 
	 if (kapp->dcopClient()->call("kcontrol", "moduleIface", "getPalette()", TQByteArray(),
				 replyType, replyData))
		 if ( replyType == "TQPalette") {
			 TQDataStream reply( replyData, IO_ReadOnly );
			 TQPalette pal;
			 reply >> pal;
			 setPalette(pal);
		 }
/* // Doesn't work ...
	 if (kapp->dcopClient()->call("kcontrol", "moduleIface", "getStyle()", TQByteArray(),
				 replyType, replyData))
		 if ( replyType == "TQString") {
			 TQDataStream reply( replyData, IO_ReadOnly );
			 TQString style; 
			 reply >> style;
			 setStyle(style);
		 }
*/	 
	 if (kapp->dcopClient()->call("kcontrol", "moduleIface", "getFont()", TQByteArray(),
				 replyType, replyData))
		 if ( replyType == "TQFont") {
			 TQDataStream reply( replyData, IO_ReadOnly );
			 TQFont font;
			 reply >> font;
			 setFont(font);
		 }
 }
	 
  view = new ProxyView(client, title, this, run_as_root, "proxyview");
  (void) new WhatsThis( this );

  connect(_client, TQT_SIGNAL(changed(bool)), TQT_SLOT(clientChanged(bool)));
  connect(_client, TQT_SIGNAL(quickHelpChanged()), TQT_SIGNAL(quickHelpChanged()));

  _sep = new KSeparator(KSeparator::HLine, this);

  _handbook= new KPushButton( KGuiItem(KStdGuiItem::help().text(),"contents"), this );
  _default = new KPushButton( KStdGuiItem::defaults(), this );
  _apply =   new KPushButton( KStdGuiItem::apply(), this );
  _reset =   new KPushButton( KGuiItem( i18n( "&Reset" ), "undo" ), this );
  _root =    new KPushButton( KGuiItem(i18n( "&Administrator Mode" )), this );

  bool mayModify = (!run_as_root || !_client->useRootOnlyMsg()) && !KCGlobal::isInfoCenter();

  // only enable the requested buttons
  int b = _client->buttons();
  trinity_setVisible(_handbook, (b & KCModule::Help));
  trinity_setVisible(_default, mayModify && (b & KCModule::Default));
  trinity_setVisible(_apply, mayModify && (b & KCModule::Apply));
  trinity_setVisible(_reset, mayModify && (b & KCModule::Apply));
  trinity_setVisible(_root, run_as_root);

  // disable initial buttons
  _apply->setEnabled( false );
  _reset->setEnabled( false );

  connect(_handbook, TQT_SIGNAL(clicked()), TQT_SLOT(handbookClicked()));
  connect(_default, TQT_SIGNAL(clicked()), TQT_SLOT(defaultClicked()));
  connect(_apply, TQT_SIGNAL(clicked()), TQT_SLOT(applyClicked()));
  connect(_reset, TQT_SIGNAL(clicked()), TQT_SLOT(resetClicked()));
  connect(_root, TQT_SIGNAL(clicked()), TQT_SLOT(rootClicked()));

  TQVBoxLayout *top = new TQVBoxLayout(this, KDialog::marginHint(), 
      KDialog::spacingHint());
  top->addWidget(view);
  top->addWidget(_sep);

  TQHBoxLayout *buttons = new TQHBoxLayout(top, 4);
  buttons->addWidget(_handbook);
  buttons->addWidget(_default);
  if (run_as_root) 
  {
    buttons->addWidget(_root);
  }

  buttons->addStretch(1);
  if (mayModify)
  {
    buttons->addWidget(_apply);
    buttons->addWidget(_reset);
  }

  top->activate();
}

ProxyWidget::~ProxyWidget()
{
#ifdef USE_QT4
  #warning Possible memory leak in ProxyWidget::~ProxyWidget()
#else // USE_QT4
  if (_client) delete _client;
  _client = 0;
#endif // USE_QT4
}

TQString ProxyWidget::quickHelp() const
{
  if (_client)
    return _client->quickHelp();
  else
    return "";
}

void ProxyWidget::handbookClicked()
{
  if (getuid()!=0)
	  emit handbookRequest();
  else
     kapp->dcopClient()->send("kcontrol", "moduleIface", "invokeHandbook()", TQByteArray());
}

void ProxyWidget::helpClicked()
{
  if (getuid()!=0)
         emit helpRequest();
  else
     kapp->dcopClient()->send("kcontrol", "moduleIface", "invokeHelp()", TQByteArray());
}

void ProxyWidget::defaultClicked()
{
  clientChanged(true);
  _client->defaults();
}

void ProxyWidget::applyClicked()
{
  _client->save();
  clientChanged(false);
}

void ProxyWidget::resetClicked()
{
  _client->load();
  clientChanged(false);
}

void ProxyWidget::rootClicked()
{
  emit runAsRoot();
}

void ProxyWidget::clientChanged(bool state)
{
  _apply->setEnabled(state);
  _reset->setEnabled(state);

  // forward the signal
  emit changed(state);
}

const KAboutData *ProxyWidget::aboutData() const
{
  return _client->aboutData();
}

// vim: sw=2 sts=2 et
