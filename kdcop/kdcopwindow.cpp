/*
 * Copyright (C) 2000 by Matthias Kalle Dalheimer <kalle@kde.org>
 *               2004 by Olivier Goffart  <ogoffart @ tiscalinet.be>
 *
 * Licensed under the Artistic License.
 */

#include "kdcopwindow.h"
#include "kdcoplistview.h"

#include <dcopclient.h>
#include <klocale.h>
#include <kdatastream.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <keditlistbox.h>
#include <klistbox.h>
#include <kdialogbase.h>
#include <kstdaccel.h>
#include <kcolorbutton.h>
#include <klistviewsearchline.h>

#include <tqtimer.h>
#include <tqwidgetstack.h>
#include <tqlabel.h>
#include <tqsplitter.h>
#include <tqlayout.h>
#include <tqcheckbox.h>
#include <tqlineedit.h>
#include <tqvalidator.h>
#include <tqpushbutton.h>
#include <tqkeycode.h>
#include <tqpixmap.h>
#include <tqcursor.h>
#include <tqsize.h>
#include <tqrect.h>
#include <tqclipboard.h>
#include <tqdatetime.h>
#include <dcopref.h>
#include <tqvbox.h>
#include <tqimage.h>
#include <tqheader.h>

#include <kdebug.h>
#include <kkeydialog.h>
#include <stdio.h>
#include <kstatusbar.h>
#include <kurl.h>
#include <kurlrequester.h>

class DCOPBrowserApplicationItem;
class DCOPBrowserInterfaceItem;
class DCOPBrowserFunctionItem;

//------------------------------

class KMultiIntEdit : public TQVBox
{
public:
	KMultiIntEdit(TQWidget *parent , const char * name=0) : TQVBox(parent,name) {}
	void addField(int key, const TQString & caption  )
	{
		TQHBox *l=new TQHBox(this);
		new TQLabel(caption + ": ", l);
		KLineEdit* e = new KLineEdit( l );
		m_widgets.insert(key, e ) ;
        	e->setValidator( new TQIntValidator( TQT_TQOBJECT(e) ) );
	}
	int field(int key)
	{
		KLineEdit *e=m_widgets[key];
		if(!e) return 0;
		return e->text().toInt();
	}

private:
	TQMap<int,KLineEdit*> m_widgets;
};

//------------------------------

DCOPBrowserItem::DCOPBrowserItem
(TQListView * parent, DCOPBrowserItem::Type type)
  : TQListViewItem(parent),
    type_(type)
{
}

DCOPBrowserItem::DCOPBrowserItem
(TQListViewItem * parent, DCOPBrowserItem::Type type)
  :  TQListViewItem(parent),
     type_(type)
{
}

  DCOPBrowserItem::Type
DCOPBrowserItem::type() const
{
  return type_;
}

// ------------------------------------------------------------------------

DCOPBrowserApplicationItem::DCOPBrowserApplicationItem
(TQListView * parent, const TQCString & app)
  : DCOPBrowserItem(parent, Application),
    app_(app)
{
  setExpandable(true);
  setText(0, TQString::fromUtf8(app_));
  setPixmap(0,  KGlobal::iconLoader()->loadIcon( TQString::fromLatin1( "exec" ), KIcon::Small ));


	/* Get the icon:  we use the icon from a mainwindow in that class.
	    a lot of applications has a app-mainwindow#1 object, but others can still have
	    a main window with another name. In that case, we search for a main window with the qt object.
	 * Why don't search with qt dirrectly?
	    simply because some application that have a 'mainwindow#1' doesn't have a qt object.  And, for
	    reason i don't know, some application return stanges result. Some konqueror instance are returning
	    konqueror-mainwindow#3 while only the #1 exists,  I already seen the same problem with konsole
	 * All calls are async to don't block the GUI if the clients does not reply immediatly
	 */

	TQRegExp rx( "([^\\-]+)");  // remove the possible processus id
	rx.search(app_);           //   konqueror-123  => konqueror-mainwindow#1
	TQString mainWindowName= rx.cap(1) + "-mainwindow#1" ;

	TQByteArray data;
	int callId=kapp->dcopClient()->callAsync( app_, mainWindowName.utf8(), "icon()", data, this, TQT_SLOT(retreiveIcon(int, const TQCString&, const TQByteArray&)));

	if(!callId)
	{
		//maybe there is another mainwindow registered with another name.
		TQByteArray data;
		TQDataStream arg(data, IO_WriteOnly);
		arg << TQCString( "MainWindow" );

		kapp->dcopClient()->callAsync( app_, "qt", "find(TQCString)", data, this, TQT_SLOT(slotGotWindowName(int, const TQCString&, const TQByteArray& )));
	}
}

  void
DCOPBrowserApplicationItem::setOpen(bool o)
{
  DCOPBrowserItem::setOpen(o);

  if (0 == firstChild())
    populate();
}

  void
DCOPBrowserApplicationItem::populate()
{
  KApplication::setOverrideCursor(tqwaitCursor);

  bool ok = false;
  bool isDefault = false;

  QCStringList objs = kapp->dcopClient()->remoteObjects(app_, &ok);

  for (QCStringList::ConstIterator it = objs.begin(); it != objs.end(); ++it)
  {
    if (*it == "default")
    {
      isDefault = true;
      continue;
    }
    new DCOPBrowserInterfaceItem(this, app_, *it, isDefault);
    isDefault = false;
  }

  KApplication::restoreOverrideCursor();
}

void DCOPBrowserApplicationItem::slotGotWindowName(int /*callId*/, const TQCString& /*replyType*/, const TQByteArray &replyData)
{
	TQDataStream reply(replyData, IO_ReadOnly);
	QCStringList mainswindows;
	reply >> mainswindows;
	TQStringList sl=TQStringList::split("/",mainswindows.first() );
	if(sl.count() >= 1)
	{
		TQString mainWindowName=sl[1];
		if(!mainWindowName.isEmpty())
		{
			TQByteArray data;
			kapp->dcopClient()->callAsync( app_, mainWindowName.utf8(), "icon()", data,
				this, TQT_SLOT(retreiveIcon(int, const TQCString&, const TQByteArray&)));
		}
	}
}

void DCOPBrowserApplicationItem::retreiveIcon(int /*callId*/,const TQCString& /*replyType*/, const TQByteArray &replyData)
{
	TQDataStream reply(replyData, IO_ReadOnly);
	TQPixmap returnQPixmap;
	reply >> returnQPixmap;
	if(!returnQPixmap.isNull())
		setPixmap(0, TQPixmap(TQImage(returnQPixmap.convertToImage()).smoothScale(16,16)) );
	else
		kdDebug() << "Unable to retreive the icon" << endl;
}

// ------------------------------------------------------------------------

DCOPBrowserInterfaceItem::DCOPBrowserInterfaceItem
(
 DCOPBrowserApplicationItem * parent,
 const TQCString & app,
 const TQCString & object,
 bool def
)
  : DCOPBrowserItem(parent, Interface),
    app_(app),
    object_(object)
{
  setExpandable(true);

  if (def)
    setText(0, i18n("%1 (default)").arg(TQString::fromUtf8(object_)));
  else
    setText(0, TQString::fromUtf8(object_));
}

  void
DCOPBrowserInterfaceItem::setOpen(bool o)
{
  DCOPBrowserItem::setOpen(o);

  if (0 == firstChild())
    populate();
}

  void
DCOPBrowserInterfaceItem::populate()
{
  KApplication::setOverrideCursor(tqwaitCursor);

  bool ok = false;

  QCStringList funcs = kapp->dcopClient()->remoteFunctions(app_, object_, &ok);

  for (QCStringList::ConstIterator it = funcs.begin(); it != funcs.end(); ++it)
    if ((*it) != "QCStringList functions()")
      new DCOPBrowserFunctionItem(this, app_, object_, *it);

  KApplication::restoreOverrideCursor();
}

// ------------------------------------------------------------------------

DCOPBrowserFunctionItem::DCOPBrowserFunctionItem
(
 DCOPBrowserInterfaceItem * parent,
 const TQCString & app,
 const TQCString & object,
 const TQCString & function
)
  : DCOPBrowserItem(parent, Function),
    app_(app),
    object_(object),
    function_(function)
{
  setExpandable(false);
  setText(0, TQString::fromUtf8(function_));
}

  void
DCOPBrowserFunctionItem::setOpen(bool o)
{
  DCOPBrowserItem::setOpen(o);
}

// ------------------------------------------------------------------------

KDCOPWindow::KDCOPWindow(TQWidget *parent, const char * name)
  : KMainWindow(parent, name)
{
  dcopClient = kapp->dcopClient();
  dcopClient->attach();
  resize( 377, 480 );
  statusBar()->message(i18n("Welcome to the KDE DCOP browser"));

	mainView = new kdcopview(this, "KDCOP");
        mainView->kListViewSearchLine1->setListView( mainView->lv );
	setCentralWidget(mainView);
	mainView->lv->addColumn(i18n("Application"));
	mainView->lv->header()->setStretchEnabled(true, 0);
//	mainView->lv->addColumn(i18n("Interface"));
//	mainView->lv->addColumn(i18n("Function"));
	mainView->lv->setDragAutoScroll( FALSE );
	mainView->lv->setRootIsDecorated( TRUE );
  connect
    (
     mainView->lv,
     TQT_SIGNAL(doubleClicked(TQListViewItem *)),
     TQT_SLOT(slotCallFunction(TQListViewItem *))
    );

  connect
    (
     mainView->lv,
     TQT_SIGNAL(currentChanged(TQListViewItem *)),
     TQT_SLOT(slotCurrentChanged(TQListViewItem *))
    );


  // set up the actions
  KStdAction::quit( TQT_TQOBJECT(this), TQT_SLOT( close() ), actionCollection() );
  KStdAction::copy( TQT_TQOBJECT(this), TQT_SLOT( slotCopy()), actionCollection() );
  KStdAction::keyBindings( guiFactory(), TQT_SLOT( configureShortcuts() ), actionCollection() );


  (void) new KAction( i18n( "&Reload" ), "reload", KStdAccel::shortcut(KStdAccel::Reload), TQT_TQOBJECT(this), TQT_SLOT( slotReload() ), actionCollection(), "reload" );

  exeaction =
    new KAction
    (
     i18n("&Execute"),
      "exec",
     CTRL + Key_E,
     TQT_TQOBJECT(this),
     TQT_SLOT(slotCallFunction()),
     actionCollection(),
     "execute"
    );

  exeaction->setEnabled(false);
  exeaction->setToolTip(i18n("Execute the selected DCOP call."));

  langmode = new KSelectAction ( i18n("Language Mode"),
  		CTRL + Key_M,
		TQT_TQOBJECT(this),
		TQT_SLOT(slotMode()),
		actionCollection(),
		"langmode");
  langmode->setEditable(false);
  langmode->setItems(TQStringList::split(",", "Shell,C++,Python"));
  langmode->setToolTip(i18n("Set the current language export."));
  langmode->setCurrentItem(0);
  slotMode();
  connect
    (
     dcopClient,
     TQT_SIGNAL(applicationRegistered(const TQCString &)),
     TQT_SLOT(slotApplicationRegistered(const TQCString &))
    );

  connect
    (
     dcopClient,
     TQT_SIGNAL(applicationRemoved(const TQCString &)),
     TQT_SLOT(slotApplicationUnregistered(const TQCString &))
    );

  dcopClient->setNotifications(true);
  createGUI();
  setCaption(i18n("DCOP Browser"));
	mainView->lb_replyData->hide();
  TQTimer::singleShot(0, this, TQT_SLOT(slotFillApplications()));
}


void KDCOPWindow::slotCurrentChanged( TQListViewItem* i )
{
  DCOPBrowserItem* item = (DCOPBrowserItem*)i;

  if( item->type() == DCOPBrowserItem::Function )
    exeaction->setEnabled( true );
  else
    exeaction->setEnabled( false );
}


void KDCOPWindow::slotCallFunction()
{
  slotCallFunction( mainView->lv->currentItem() );
}

void KDCOPWindow::slotReload()
{
  slotFillApplications();
}

void KDCOPWindow::slotCallFunction( TQListViewItem* it )
{
  if(it == 0)
    return;
  DCOPBrowserItem * item = static_cast<DCOPBrowserItem *>(it);

  if (item->type() != DCOPBrowserItem::Function)
    return;

  DCOPBrowserFunctionItem * fitem =
    static_cast<DCOPBrowserFunctionItem *>(item);

  TQString unNormalisedSignature = TQString::fromUtf8(fitem->function());
  TQString normalisedSignature;
  TQStringList types;
  TQStringList names;

  if (!getParameters(unNormalisedSignature, normalisedSignature, types, names))
  {
    KMessageBox::error
      (this, i18n("No parameters found."), i18n("DCOP Browser Error"));

    return;
  }

  TQByteArray data;
  TQByteArray replyData;

  TQCString replyType;

  TQDataStream arg(data, IO_WriteOnly);

  KDialogBase mydialog( this, "KDCOP Parameter Entry", true,
    TQString::null, KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, true );

  mydialog.setCaption
    ( i18n("Call Function %1").arg( static_cast<const char *>(fitem->function()) ) );

  TQFrame *frame = mydialog.makeMainWidget();

  TQLabel* h1 = new TQLabel( i18n( "Name" ), frame );
  TQLabel* h2 = new TQLabel( i18n( "Type" ), frame );
  TQLabel* h3 = new TQLabel( i18n( "Value" ), frame );

  TQGridLayout* grid = new TQGridLayout( frame, types.count() + 2, 3,
    0, KDialog::spacingHint() );

  grid->addWidget( h1, 0, 0 );
  grid->addWidget( h2, 0, 1 );
  grid->addWidget( h3, 0, 2 );

  // Build up a dialog for parameter entry if there are any parameters.

  if (types.count())
  {
    int i = 0;

    TQPtrList<TQWidget> wl;

    for (TQStringList::ConstIterator it = types.begin(); it != types.end(); ++it)
    {
      i++;

      const TQString type = *it;

      const TQString name = i-1 < (int)names.count() ? names[i-1] : TQString::null;

      if( type == "int" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "int", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQIntValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "unsigned"  || type == "uint" || type == "unsigned int"
             || type == "TQ_UINT32" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "unsigned int", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );

        TQIntValidator* iv = new TQIntValidator( TQT_TQOBJECT(e) );
        iv->setBottom( 0 );
        e->setValidator( iv );
      }
      else if ( type == "long" || type == "long int" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "long", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQIntValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "ulong" || type == "unsigned long" || type == "unsigned long int"
             || type == "TQ_UINT64" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "unsigned long", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQIntValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "short" || type == "short int" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "long", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQIntValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "ushort" || type == "unsigned short" || type == "unsigned short int"  )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "unsigned short", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQIntValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "float" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "float", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQDoubleValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "double" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "double", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
        e->setValidator( new TQDoubleValidator( TQT_TQOBJECT(e) ) );
      }
      else if ( type == "bool" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "bool", frame );
        grid->addWidget( l, i, 1 );
        TQCheckBox* c = new TQCheckBox( frame );
        grid->addWidget( c, i, 2 );
        wl.append( c );
      }
      else if ( type == "TQString" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQString", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQCString" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQString", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQStringList" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQStringList", frame );
        grid->addWidget( l, i, 1 );
        KEditListBox* e = new KEditListBox ( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQValueList<TQCString>" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQValueList<TQCString>", frame );
        grid->addWidget( l, i, 1 );
        KEditListBox* e = new KEditListBox ( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "KURL" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "KURL", frame );
        grid->addWidget( l, i, 1 );
        KLineEdit* e = new KLineEdit( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQColor" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQColor", frame );
        grid->addWidget( l, i, 1 );
        KColorButton* e = new KColorButton( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQSize" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQSize", frame );
        grid->addWidget( l, i, 1 );
        KMultiIntEdit* e = new KMultiIntEdit( frame );
        e->addField( 1, i18n("Width") );
        e->addField( 2, i18n("Height") );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQPoint" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQPoint", frame );
        grid->addWidget( l, i, 1 );
        KMultiIntEdit* e = new KMultiIntEdit( frame );
        e->addField( 1, i18n("X") );
        e->addField( 2, i18n("Y") );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if ( type == "TQRect" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQRect", frame );
        grid->addWidget( l, i, 1 );
        KMultiIntEdit* e = new KMultiIntEdit( frame );
        e->addField( 1, i18n("Left") );
        e->addField( 2, i18n("Top") );
        e->addField( 3, i18n("Width") );
        e->addField( 4, i18n("Height") );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else if( type == "TQPixmap" )
      {
        TQLabel* n = new TQLabel( name, frame );
        grid->addWidget( n, i, 0 );
        TQLabel* l = new TQLabel( "TQPixmap", frame );
        grid->addWidget( l, i, 1 );
        KURLRequester* e = new KURLRequester( frame );
        grid->addWidget( e, i, 2 );
        wl.append( e );
      }
      else
      {
        KMessageBox::sorry(this, i18n("Cannot handle datatype %1").arg(type));
        return;
      }
    }

    if (!wl.isEmpty())
      wl.at(0)->setFocus();

    i++;

    int ret = mydialog.exec();

    if (TQDialog::Accepted != ret)
      return;

    // extract the arguments

    i = 0;

    for (TQStringList::ConstIterator it = types.begin(); it != types.end(); ++it)
    {
      TQString type = *it;

      if ( type == "int" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toInt();
      }
      else if ( type == "unsigned" || type == "uint" || type == "unsigned int"
             || type == "TQ_UINT32" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toUInt();
      }
      else if( type == "long" || type == "long int" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toLong();
      }
      else if( type == "ulong" || type == "unsigned long" || type == "unsigned long int" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toULong();
      }
      else if( type == "short" || type == "short int" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toShort();
      }
      else if( type == "ushort" || type == "unsigned short" || type == "unsigned short int" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toUShort();
      }
      else if ( type == "TQ_UINT64" )
      {
        KLineEdit* e = ( KLineEdit* )wl.at( i );
        arg << e->text().toULongLong();
      }
      else if( type == "float" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toFloat();
      }
      else if( type == "double" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text().toDouble();
      }
      else if( type == "bool" )
      {
        TQCheckBox* c = (TQCheckBox*)wl.at( i );
        arg << c->isChecked();
      }
      else if( type == "TQCString" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << TQCString( e->text().local8Bit() );
      }
      else if( type == "TQString" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << e->text();
      }
      else if( type == "TQStringList" )
      {
        KEditListBox* e = (KEditListBox*)wl.at( i );
        arg << e->items();
      }
      else if( type == "TQValueList<TQCString>" )
      {
        KEditListBox* e = (KEditListBox*)wl.at( i );
        for (int i = 0; i < e->count(); i++)
          arg << TQCString( e->text(i).local8Bit() );
      }
      else if( type == "KURL" )
      {
        KLineEdit* e = (KLineEdit*)wl.at( i );
        arg << KURL( e->text() );
      }
      else if( type == "TQColor" )
      {
       KColorButton* e = (KColorButton*)wl.at( i );
       arg << e->color();
      }
      else if( type == "TQSize" )
      {
       KMultiIntEdit* e = (KMultiIntEdit*)wl.at( i );
       arg << TQSize(e->field(1) , e->field(2)) ;
      }
      else if( type == "TQPoint" )
      {
       KMultiIntEdit* e = (KMultiIntEdit*)wl.at( i );
       arg << TQPoint(e->field(1) , e->field(2)) ;
      }
      else if( type == "TQRect" )
      {
       KMultiIntEdit* e = (KMultiIntEdit*)wl.at( i );
       arg << TQRect(e->field(1) , e->field(2) , e->field(3) , e->field(4)) ;
      }
      else if( type == "TQPixmap" )
      {
       KURLRequester* e= (KURLRequester*)wl.at( i );
       arg << TQPixmap(e->url());
      }
      else
      {
        KMessageBox::sorry(this, i18n("Cannot handle datatype %1").arg(type));
        return;
      }

      i++;
    }
  }

  DCOPRef( fitem->app(), "MainApplication-Interface" ).call( "updateUserTimestamp", kapp->userTimestamp());

  // Now do the DCOP call

  bool callOk =
    dcopClient->call
    (
     fitem->app(),
     fitem->object(),
     normalisedSignature.utf8(),
     data,
     replyType,
     replyData
    );

  if (!callOk)
  {
    kdDebug()
      << "call failed( "
      << fitem->app().data()
      << ", "
      << fitem->object().data()
      << ", "
      << normalisedSignature
      << " )"
      << endl;

    statusBar()->message(i18n("DCOP call failed"));

    TQString msg = i18n("<p>DCOP call failed.</p>%1");

    bool appRegistered = dcopClient->isApplicationRegistered(fitem->app());

    if (appRegistered)
    {
      msg =
        msg.arg
        (
         i18n
         (
          "<p>Application is still registered with DCOP;"
          " I do not know why this call failed.</p>"
         )
        );
    }
    else
    {
      msg =
        msg.arg
        (
         i18n
         (
          "<p>The application appears to have unregistered with DCOP.</p>"
         )
        );
    }

    KMessageBox::information(this, msg);
  }
  else
  {
    TQString coolSignature =
      TQString::fromUtf8(fitem->app())
      + "."
      + TQString::fromUtf8(fitem->object())
      + "."
      + normalisedSignature ;

    statusBar()->message(i18n("DCOP call %1 executed").arg(coolSignature));

    if (replyType != "void" && replyType != "ASYNC" && !replyType.isEmpty() )
    {
      TQDataStream reply(replyData, IO_ReadOnly);
      if (demarshal(replyType, reply, mainView->lb_replyData))
	{
      mainView->l_replyType->setText
        (
         i18n("<strong>%1</strong>")
         .arg(TQString::fromUtf8(replyType))
        );
	mainView->lb_replyData->show();
	}
	else
	{
	        mainView->l_replyType->setText(i18n("Unknown type %1.").arg(TQString::fromUtf8(replyType)));
      		mainView->lb_replyData->hide();
	}
    }
    else
    {
      mainView->l_replyType->setText(i18n("No returned values"));
      mainView->lb_replyData->hide();
    }
  }
}


void KDCOPWindow::slotFillApplications()
{
  KApplication::setOverrideCursor(tqwaitCursor);

  QCStringList apps = dcopClient->registeredApplications();
  TQCString appId = dcopClient->appId();

  mainView->lv->clear();

  for (QCStringList::ConstIterator it = apps.begin(); it != apps.end(); ++it)
  {
    if ((*it) != appId && (*it).left(9) != "anonymous")
    {
      new DCOPBrowserApplicationItem(mainView->lv, *it);
    }
  }

  KApplication::restoreOverrideCursor();
}

bool KDCOPWindow::demarshal
(
 TQCString &   replyType,
 TQDataStream & reply,
 TQListBox	*theList
)
{
  TQStringList ret;
  TQPixmap pret;
  bool isValid = true;
  theList->clear();
  ret.clear();

  if ( replyType == "TQVariant" )
  {
    // read data type from stream
    TQ_INT32 type;
    reply >> type;

    // change replyType to real typename
    replyType = TQVariant::typeToName( (TQVariant::Type)type );

    // demarshal data with a recursive call
    return demarshal(replyType, reply, theList);
  }
  else if ( replyType == "int" )
  {
    int i;
    reply >> i;
    ret << TQString::number(i);
  }
  else if ( replyType == "uint" || replyType == "unsigned int"
         || replyType == "TQ_UINT32" )
  {
    uint i;
    reply >> i;
    ret << TQString::number(i);
  }
  else if ( replyType == "long" || replyType == "long int" )
  {
    long l;
    reply >> l;
    ret << TQString::number(l);
  }
  else if ( replyType == "ulong" || replyType == "unsigned long" || replyType == "unsigned long int" )
  {
    ulong l;
    reply >> l;
    ret << TQString::number(l);
  }
  else if ( replyType == "TQ_UINT64" )
  {
    TQ_UINT64 i;
    reply >> i;
    ret << TQString::number(i);
  }
  else if ( replyType == "float" )
  {
    float f;
    reply >> f;
    ret << TQString::number(f);
  }
  else if ( replyType == "double" )
  {
    double d;
    reply >> d;
    ret << TQString::number(d);
  }
  else if (replyType == "bool")
  {
    bool b;
    reply >> b;
    ret << (b ? TQString::fromUtf8("true") : TQString::fromUtf8("false"));
  }
  else if (replyType == "TQString")
  {
    TQString s;
    reply >> s;
    ret << s;
  }
  else if (replyType == "TQStringList")
  {
    reply >> ret;
  }
  else if (replyType == "TQCString")
  {
    TQCString r;
    reply >> r;
    ret << TQString::fromUtf8(r);
  }
  else if (replyType == "QCStringList")
  {
    QCStringList lst;
    reply >> lst;

    for (QCStringList::ConstIterator it(lst.begin()); it != lst.end(); ++it)
      ret << *it;
  }
  else if (replyType == "KURL")
  {
    KURL r;
    reply >> r;
    ret << r.prettyURL();
  }
  else if (replyType == "TQSize")
  {
    TQSize r;
    reply >> r;
    ret << TQString::number(r.width()) + "x" + TQString::number(r.height());
  }
  else if (replyType == "TQPoint")
  {
    TQPoint r;
    reply >> r;
    ret << "(" + TQString::number(r.x()) + "," + TQString::number(r.y()) + ")";
  }
  else if (replyType == "TQRect")
  {
    TQRect r;
    reply >> r;
    ret << TQString::number(r.x()) + "x" + TQString::number(r.y()) + "+" + TQString::number(r.height()) + "+" + TQString::number(r.width());
  }
  else if (replyType == "TQFont")
  {
	TQFont r;
	reply >> r;
	ret << r.rawName();
  }
  else if (replyType == "TQCursor")
  {
	TQCursor r;
	reply >> r;
	//theList->insertItem(r, 1);
	ret << "Cursor #" + TQString::number(r.shape());
  }
  else if (replyType == "TQPixmap")
  {
	TQPixmap r;
	reply >> r;
	theList->insertItem(r, 1);
  }
  else if (replyType == "TQColor")
  {
	TQColor r;
	reply >> r;
	TQString color = r.name();
	TQPixmap p(15,15);
	p.fill(r);
	theList->insertItem(p,color, 1);
  }
  else if (replyType == "TQDateTime")
  {
  	TQDateTime r;
	reply >> r;
	ret << r.toString();
  }
  else if (replyType == "TQDate")
  {
  	TQDate r;
	reply >> r;
	ret << r.toString();
  }
  else if (replyType == "TQTime")
  {
  	TQTime r;
	reply >> r;
	ret << r.toString();
  }
  else if (replyType == "DCOPRef")
  {
	DCOPRef r;
	reply >> r;
	if (!r.app().isEmpty() && !r.obj().isEmpty())
	  ret << TQString("DCOPRef(%1, %2)").arg(static_cast<const char *>(r.app()), static_cast<const char *>(r.obj()));
  }
  else
  {
    ret <<
      i18n("Do not know how to demarshal %1").arg(TQString::fromUtf8(replyType));
	isValid = false;
  }

      if (!ret.isEmpty())
      	theList->insertStringList(ret);
	return isValid;
}

  void
KDCOPWindow::slotApplicationRegistered(const TQCString & appName)
{
  TQListViewItemIterator it(mainView->lv);

  for (; it.current(); ++it)
  {
    DCOPBrowserApplicationItem * item =
      static_cast<DCOPBrowserApplicationItem *>(it.current());

    if (item->app() == appName)
      return;
  }

  TQCString appId = dcopClient->appId();

  if (appName != appId && appName.left(9) != "anonymous")
  {
    new DCOPBrowserApplicationItem(mainView->lv, appName);
  }
}

  void
KDCOPWindow::slotApplicationUnregistered(const TQCString & appName)
{
  TQListViewItemIterator it(mainView->lv);

  for (; it.current(); ++it)
  {
    DCOPBrowserApplicationItem * item =
      static_cast<DCOPBrowserApplicationItem *>(it.current());

    if (item->app() == appName)
    {
      delete item;
      return;
    }
  }
}

  bool
KDCOPWindow::getParameters
(
 const TQString  & _unNormalisedSignature,
 TQString        & normalisedSignature,
 TQStringList    & types,
 TQStringList    & names
)
{
  TQString unNormalisedSignature(_unNormalisedSignature);

  int s = unNormalisedSignature.find(' ');

  if ( s < 0 )
    s = 0;
  else
    s++;

  unNormalisedSignature = unNormalisedSignature.mid(s);

  int left  = unNormalisedSignature.find('(');
  int right = unNormalisedSignature.findRev(')');

  if (-1 == left)
  {
    // Fucked up function signature.
    return false;
  }

  TQStringList intTypes;
  intTypes << "int" << "unsigned" << "long" << "bool" ;

  if (left > 0 && left + 1 < right - 1)
  {
    types =
      TQStringList::split
      (',', unNormalisedSignature.mid(left + 1, right - left - 1));

    for (TQStringList::Iterator it = types.begin(); it != types.end(); ++it)
    {
      (*it) = (*it).simplifyWhiteSpace();

      int s = (*it).findRev(' ');

      if (-1 != s && !intTypes.contains((*it).mid(s + 1)))
      {
        names.append((*it).mid(s + 1));

        (*it) = (*it).left(s);
      }
    }
  }

  normalisedSignature =
    unNormalisedSignature.left(left) + "(" + types.join(",") + ")";

  return true;
}
void KDCOPWindow::slotCopy()
{
	// Copy pixmap and text to the clipboard from the
	// below list view.  If there is nothing selected from
	// the below menu then tell the tree to copy its current
	// selection as text.
	TQClipboard *clipboard = TQApplication::tqclipboard();
	if (mainView->lb_replyData->count()!= 0)
	{

		//if (!mainView->lb_replyData->pixmap(mainView->lb_replyData->currentItem())->isNull())
		//{
			kdDebug() << "Is pixmap" << endl;
		//	TQPixmap p;
		// 	p = *mainView->lb_replyData->pixmap(mainView->lb_replyData->currentItem());
		//	clipboard->setPixmap(p);
		//}
		TQString t = mainView->lb_replyData->text(mainView->lb_replyData->currentItem());
		if (!t.isNull())
			clipboard->setText(t);
	}
}

void KDCOPWindow::slotMode()
{
	kdDebug () << "Going to mode " << langmode->currentText() << endl;
	// Tell lv what the current mode is here...
	mainView->lv->setMode(langmode->currentText());
}

#include "kdcopwindow.moc"
