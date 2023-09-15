/****************************************************************************

 Copyright (C) 2003 Lubos Lunak        <l.lunak@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

#include <tdecmdlineargs.h>
#include <tdeapplication.h>
#include <tdemessagebox.h>
#include <kprocess.h>
#include <tdelocale.h>
#include <unistd.h>
#include <twin.h>
#include <X11/Xlib.h>

static const TDECmdLineOptions options[] =
    {
    // no need for I18N_NOOP(), this is not supposed to be used directly
        { "pid <pid>", "PID of the application to resume.", 0 },
        { "hostname <hostname>", "Hostname on which the application is running.", 0 },
        { "windowname <caption>", "Caption of the window to be resumed.", 0 },
        { "applicationname <name>", "Name of the application to be resumed.", 0 },
        { "wid <id>", "ID of resource belonging to the application.", 0 },
        TDECmdLineLastOption
    };

int main( int argc, char* argv[] )
    {
    TDELocale::setMainCatalogue( "twin" ); // the messages are in twin's .po file
    TDECmdLineArgs::init( argc, argv, "twin_resume_helper", I18N_NOOP( "TWin" ),
	I18N_NOOP( "TWin helper utility" ), "1.0" );
    TDECmdLineArgs::addCmdLineOptions( options );
    TDEApplication app;
    TDECmdLineArgs* args = TDECmdLineArgs::parsedArgs();
    TQCString hostname = args->getOption( "hostname" );
    bool pid_ok = false;
    pid_t pid = args->getOption( "pid" ).toULong( &pid_ok );
    TQString caption = TQString::fromUtf8( args->getOption( "windowname" ));
    TQString appname = TQString::fromLatin1( args->getOption( "applicationname" ));
    bool id_ok = false;
    Window id = args->getOption( "wid" ).toULong( &id_ok );
    args->clear();
    if( !pid_ok || pid == 0 || !id_ok || id == None
	|| hostname.isEmpty() || caption.isEmpty())
        {
        printf("%s\n", i18n( "This helper utility is not supposed to be called directly." ).ascii());
	return 2;
        }
    TQString question = i18n(
	"<qt>The application \"<b>%1</b>\" has been suspended.<p>"
	"Do you wish to resume this application?</qt>" )
	.arg( caption );
    if( KMessageBox::questionYesNo( 0, question, i18n("Resume suspended application?"), i18n("Resume"), i18n("Keep Suspended") ) == KMessageBox::Yes )
        {
        return 0;
        }
    else
        {
        return 1;
        }
    }
