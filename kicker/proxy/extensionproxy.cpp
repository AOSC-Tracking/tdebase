/*****************************************************************

Copyright (c) 2000 Matthias Elter <elter@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTDHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <stdlib.h>

#include <tqstring.h>
#include <tqfile.h>
#include <qxembed.h>

#include <kapplication.h>
#include <kglobal.h>
#include <klibloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kpanelextension.h>
#include <kaboutdata.h>
#include <tqfileinfo.h>
#include <dcopclient.h>

#include "appletinfo.h"
#include "extensionproxy.h"
#include "extensionproxy.moc"

#include <X11/Xlib.h>


static KCmdLineOptions options[] =
{
  { "+desktopfile", I18N_NOOP("The extension's desktop file"), 0 },
  { "configfile <file>", I18N_NOOP("The config file to be used"), 0 },
  { "callbackid <id>", I18N_NOOP("DCOP callback id of the extension container"), 0 },
  KCmdLineLastOption
};

extern "C" KDE_EXPORT int kdemain( int argc, char ** argv )
{
    KAboutData aboutData( "extensionproxy", I18N_NOOP("Panel Extension Proxy")
                          , "v0.1.0"
                          ,I18N_NOOP("Panel extension proxy")
                          , KAboutData::License_BSD
                          , "(c) 2000, The KDE Developers");
    KCmdLineArgs::init(argc, argv, &aboutData );
    aboutData.addAuthor("Matthias Elter",0, "elter@kde.org");
    aboutData.addAuthor("Matthias Ettrich",0, "ettrich@kde.org");
    KApplication::addCmdLineOptions();
    KCmdLineArgs::addCmdLineOptions(options); // Add our own options.

    KApplication a;
    a.disableSessionManagement();

    KGlobal::dirs()->addResourceType("extensions", KStandardDirs::kde_default("data") +
				     "kicker/extensions");

    // setup proxy object
    ExtensionProxy proxy(0, "extensionproxywidget");

    // parse cmdline args
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    // sanity check
    if ( args->count() == 0 )
        KCmdLineArgs::usage(i18n("No desktop file specified") );

    // do we have a callback id?
    if (args->getOption("callbackid").isNull()) {
	kdError() << "Callback ID is null. " << endl;
	exit(0);
    }

    // Perhaps we should use a konsole-like solution here (shell, list of args...)
    TQCString desktopfile = TQCString( args->arg(0) );

    // load extension DSO
    proxy.loadExtension( desktopfile, args->getOption("configfile"));

    // dock into our extension container
    proxy.dock(args->getOption("callbackid"));

    return a.exec();
}

ExtensionProxy::ExtensionProxy(TQObject* parent, const char* name)
  : TQObject(parent, name)
  , DCOPObject("ExtensionProxy")
  , _info(0)
  , _extension(0)
{
    // try to attach to DCOP server
    if (!kapp->dcopClient()->attach()) {
	kdError() << "Failed to attach to DCOP server." << endl;
	exit(0);
    }

    if (kapp->dcopClient()->registerAs("extension_proxy", true) == 0) {
	kdError() << "Failed to register at DCOP server." << endl;
	exit(0);
    }
}

ExtensionProxy::~ExtensionProxy()
{
    kapp->dcopClient()->detach();
}

void ExtensionProxy::loadExtension(const TQCString& desktopFile, const TQCString& configFile)
{
    TQString df;

    // try simple path first
    TQFileInfo finfo( desktopFile );
    if ( finfo.exists() ) {
	df = finfo.absFilePath();
    } else {
	// locate desktop file
	df = KGlobal::dirs()->findResource("extensions", TQString(desktopFile));
    }

    TQFile file(df);
    // does the config file exist?
    if (df.isNull() || !file.exists()) {
	kdError() << "Failed to locate extension desktop file: " << desktopFile << endl;
	exit(0);
    }

    // create AppletInfo instance
    _info = new AppletInfo(df);

    // set the config file
    if (!configFile.isNull())
	_info->setConfigFile(configFile);

    // load extension DSO
    _extension = loadExtension(*_info);

    // sanity check
    if (!_extension) {
	kdError() << "Failed to load extension: " << _info->library() << endl;
	exit(0);
    }

    // connect updateLayout signal
    connect(_extension, TQT_SIGNAL(updateLayout()), TQT_SLOT(slotUpdateLayout()));
}

KPanelExtension* ExtensionProxy::loadExtension(const AppletInfo& info)
{
    KLibLoader* loader = KLibLoader::self();
    KLibrary* lib = loader->library(TQFile::encodeName(info.library()));

    if (!lib)
    {
        kdWarning() << "cannot open extension: " << info.library()
                    << " because of " << loader->lastErrorMessage() << endl;
        return 0;
    }

    KPanelExtension* (*init_ptr)(TQWidget *, const TQString&);
    init_ptr = (KPanelExtension* (*)(TQWidget *, const TQString&))lib->symbol( "init" );

    if (!init_ptr)
    {
        kdWarning() << info.library() << " is not a kicker extension!" << endl;
        return 0;
    }

    return init_ptr(0, info.configFile());
}

void ExtensionProxy::dock(const TQCString& callbackID)
{
    kdDebug(1210) << "Callback ID: " << callbackID << endl;

    _callbackID = callbackID;

    // try to attach to DCOP server
    DCOPClient* dcop = kapp->dcopClient();

    dcop->setNotifications(true);
    connect(dcop, TQT_SIGNAL(applicationRemoved(const TQCString&)),
	    TQT_SLOT(slotApplicationRemoved(const TQCString&)));

    WId win;

    // get docked
    {
	TQCString replyType;
	TQByteArray data, replyData;
	TQDataStream dataStream( data, IO_WriteOnly );

	int actions = 0;
	if(_extension) actions = _extension->actions();
	dataStream << actions;

	int type = 0;
	if (_extension) type = static_cast<int>(_extension->type());
	dataStream << type;

	// we use "call" to know whether it was sucessful

	int screen_number = 0;
	if (qt_xdisplay())
	    screen_number = DefaultScreen(qt_xdisplay());
	TQCString appname;
	if (screen_number == 0)
	    appname = "kicker";
	else
	    appname.sprintf("kicker-screen-%d", screen_number);

	if ( !dcop->call(appname, _callbackID, "dockRequest(int,int)",
			 data, replyType, replyData ) )
        {
            kdError() << "Failed to dock into the panel." << endl;
            exit(0);
        }

	TQDataStream reply( replyData, IO_ReadOnly );
	reply >> win;

    }

    if (win) {
        if (_extension)
        {
            _extension->hide();
        }
        QXEmbed::initialize();
        QXEmbed::embedClientIntoWindow( _extension, win );
    }
    else {
        kdError() << "Failed to dock into the panel." << endl;
        if(_extension) delete _extension;
        exit(0);
    }
}

bool ExtensionProxy::process(const TQCString &fun, const TQByteArray &data,
                          TQCString& replyType, TQByteArray &replyData)
{
    if ( fun == "sizeHint(int,TQSize)" )
	{
	    TQDataStream dataStream( data, IO_ReadOnly );
	    int pos;
	    TQSize maxSize;
	    dataStream >> pos;
	    dataStream >> maxSize;

	    TQDataStream reply( replyData, IO_WriteOnly );
	    replyType = "TQSize";

	    if(!_extension)
		reply << maxSize;
	    else
		reply << _extension->sizeHint((KPanelExtension::Position)pos, maxSize);

	    return true;
	}
    else if ( fun == "setPosition(int)" )
	{
	    TQDataStream dataStream( data, IO_ReadOnly );
	    int pos;
	    dataStream >> pos;

	    if(_extension) {
		_extension->setPosition( (KPanelExtension::Position)pos );
	    }
	    return true;
	}
    else if ( fun == "setAlignment(int)" )
	{
	    TQDataStream dataStream( data, IO_ReadOnly );
	    int tqalignment;
	    dataStream >> tqalignment;

	    if(_extension) {
		_extension->setAlignment( (KPanelExtension::Alignment)tqalignment );
	    }
	    return true;
	}
	else if ( fun == "setSize(int,int)" )
	{
	    TQDataStream dataStream( data, IO_ReadOnly );
		int serializedSize;
		int custom;
		dataStream >> serializedSize;
	    dataStream >> custom;

		if (_extension)
			_extension->setSize(KPanelExtension::Size(serializedSize), custom);
		return true;
	}
    else if ( fun == "removedFromPanel()" )
    {
        if(_extension) delete _extension;
        exit(0);
        return true;
    }
    else if ( fun == "about()" )
	{
	    if(_extension) _extension->action( KPanelExtension::About );
	    return true;
	}
    else if ( fun == "help()" )
	{
	    if(_extension) _extension->action( KPanelExtension::Help );
	    return true;
	}
    else if ( fun == "preferences()" )
    {
        if(_extension) _extension->action( KPanelExtension::Preferences );
        return true;
    }
    else if ( fun == "reportBug()" )
    {
        if(_extension) _extension->action( KPanelExtension::ReportBug );
        return true;
    }
    else if ( fun == "actions()" )
	{
	    TQDataStream reply( replyData, IO_WriteOnly );
	    int actions = 0;
	    if(_extension) actions = _extension->actions();
	    reply << actions;
	    replyType = "int";
	    return true;
	}
    else if ( fun == "preferedPosition()" )
	{
	    TQDataStream reply( replyData, IO_WriteOnly );
	    int pos = static_cast<int>(KPanelExtension::Bottom);
	    if(_extension) pos = static_cast<int>(_extension->preferedPosition());
	    reply << pos;
	    replyType = "int";
	    return true;
	}
    else if ( fun == "type()" )
	{
	    TQDataStream reply( replyData, IO_WriteOnly );
	    int type = 0;
	    if (_extension) type = static_cast<int>(_extension->type());
	    reply << type;
	    replyType = "int";
	    return true;
	}
    return false;
}

void ExtensionProxy::slotUpdateLayout()
{
    if(_callbackID.isNull()) return;

    TQByteArray data;
    int screen_number = 0;
    if (qt_xdisplay())
	screen_number = DefaultScreen(qt_xdisplay());
    TQCString appname;
    if (screen_number == 0)
	appname = "kicker";
    else
	appname.sprintf("kicker-screen-%d", screen_number);

    kapp->dcopClient()->send(appname, _callbackID, "updateLayout()", data);
}

void ExtensionProxy::slotApplicationRemoved(const TQCString& appId)
{
    int screen_number = 0;
    if (qt_xdisplay())
	screen_number = DefaultScreen(qt_xdisplay());
    TQCString appname;
    if (screen_number == 0)
	appname = "kicker";
    else
	appname.sprintf("kicker-screen-%d", screen_number);

    if(appId == appname) {
	kdDebug(1210) << "Connection to kicker lost, shutting down" << endl;
	kapp->quit();
    }
}
