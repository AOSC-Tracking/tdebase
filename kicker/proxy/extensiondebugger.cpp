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

#include <tqfile.h>
#include <tqlayout.h>
#include <tqpushbutton.h>

#include <tdeapplication.h>
#include <tdeglobal.h>
#include <klibloader.h>
#include <tdelocale.h>
#include <kstandarddirs.h>
#include <tdecmdlineargs.h>
#include <kdebug.h>
#include <kpanelextension.h>
#include <tdeaboutdata.h>
#include <tqfileinfo.h>

#include "appletinfo.h"
#include "extensiondebugger.h"
#include "extensiondebugger.moc"



static TDECmdLineOptions options[] =
{
  { "+desktopfile", I18N_NOOP("The extensions desktop file"), 0 },
  TDECmdLineLastOption
};

KPanelExtension* loadExtension(const AppletInfo& info)
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

int main( int argc, char ** argv )
{
    TDEAboutData aboutData( "extensionproxy", I18N_NOOP("Panel extension proxy.")
                          , "v0.1.0"
                          ,I18N_NOOP("Panel extension proxy.")
                          , TDEAboutData::License_BSD
                          , "(c) 2000, The KDE Developers");
    TDECmdLineArgs::init(argc, argv, &aboutData );
    aboutData.addAuthor("Matthias Elter",0, "elter@kde.org");
    aboutData.addAuthor("Matthias Ettrich",0, "ettrich@kde.org");
    TDEApplication::addCmdLineOptions();
    TDECmdLineArgs::addCmdLineOptions(options); // Add our own options.

    TDEApplication a;
    a.disableSessionManagement();

    TDEGlobal::dirs()->addResourceType("extensions", TDEStandardDirs::kde_default("data") +
				     "kicker/extensions");

    TQString df;

    // parse cmdline args
    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

    // sanity check
    if ( args->count() == 0 )
        TDECmdLineArgs::usage(i18n("No desktop file specified") );


    TQCString desktopFile = TQCString( args->arg(0) );

    // try simple path first
    TQFileInfo finfo( desktopFile );
    if ( finfo.exists() ) {
	df = finfo.absFilePath();
    } else {
	// locate desktop file
	df = TDEGlobal::dirs()->findResource("extensions", TQString(desktopFile));
    }

    // does the config file exist?
    if (!TQFile::exists(df)) {
	kdError() << "Failed to locate extension desktop file: " << desktopFile << endl;
	return 1;
    }

    AppletInfo info( df );

    KPanelExtension *extension = loadExtension(info);
    if ( !extension ) {
	kdError() << "Failed to load extension: " << info.library() << endl;
	return 1;
    }

    ExtensionContainer *container = new ExtensionContainer( extension );
    container->show();

    TQObject::connect( &a, TQ_SIGNAL( lastWindowClosed() ), &a, TQ_SLOT( quit() ) );

    int result = a.exec();

    delete extension;
    return result;
}

ExtensionContainer::ExtensionContainer( KPanelExtension *extension, TQWidget *parent, const char *name )
    : TQWidget( parent, name ), m_extension( extension )
{
    ( new TQVBoxLayout( this ) )->setAutoAdd( true );

    TQPushButton *configButton = new TQPushButton( i18n( "Configure..." ), this );
    connect( configButton, TQ_SIGNAL( clicked() ),
             this, TQ_SLOT( showPreferences() ) );

    m_extension->reparent( this, TQPoint( 0, 0 ) );
}

void ExtensionContainer::resizeEvent( TQResizeEvent * )
{
    m_extension->setGeometry( 0, 0, width(), height() );
}

void ExtensionContainer::showPreferences()
{
    m_extension->action( KPanelExtension::Preferences );
}
