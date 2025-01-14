////////////////////////////////////////////////////////////////////////////////
//
// Class Names   : KFI::CFontViewerApp, KFI::CFontViewerAppMainWindow
// Author        : Craig Drummond
// Project       : K Font Installer (tdefontinst-kcontrol)
// Creation Date : 30/04/2004
// Version       : $Revision$ $Date$
//
////////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
////////////////////////////////////////////////////////////////////////////////
// (C) Craig Drummond, 2004
////////////////////////////////////////////////////////////////////////////////

#include "FontViewerApp.h"
#include "KfiConstants.h"
#include <tdeaboutdata.h>
#include <tdecmdlineargs.h>
#include <klibloader.h>
#include <tdelocale.h>
#include <tdeglobal.h>
#include <tdefiledialog.h>
#include <tdeconfig.h>

#define CFG_GROUP    "FontViewer Settings"
#define CFG_SIZE_KEY "Window Size"

namespace KFI
{

CFontViewerAppMainWindow::CFontViewerAppMainWindow()
                        : KParts::MainWindow((TQWidget *)0L)
{
    KLibFactory *factory=KLibLoader::self()->factory("libtdefontviewpart");

    if(factory)
    {
        KStdAction::open(this, TQ_SLOT(fileOpen()), actionCollection());
        KStdAction::quit(tdeApp, TQ_SLOT(quit()), actionCollection());

        itsPreview=(KParts::ReadOnlyPart *)factory->create(this, "fontvier", "KParts::ReadOnlyPart");

        TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
        KURL         openURL;

        if(args->count() > 0)
        {
            KURL url(args->url(args->count() - 1));

            if(url.isValid())
                openURL = url;
        }

        setCentralWidget(itsPreview->widget());
        createGUI(itsPreview);

        if(!openURL.isEmpty())
            itsPreview->openURL(openURL);

        TQSize             defSize(450, 380);
        TDEConfigGroupSaver saver(tdeApp->config(), CFG_GROUP);

        resize(tdeApp->config()->readSizeEntry(CFG_SIZE_KEY, &defSize));
        show();
    }
    else
        exit(0);
}

CFontViewerAppMainWindow::~CFontViewerAppMainWindow()
{
    TDEConfigGroupSaver saver(tdeApp->config(), CFG_GROUP);
    tdeApp->config()->writeEntry(CFG_SIZE_KEY, size());
    tdeApp->config()->sync();
}

void CFontViewerAppMainWindow::fileOpen()
{
    KURL url(KFileDialog::getOpenURL(TQString::null, "application/x-font-ttf application/x-font-otf "
                                                    "application/x-font-ttc application/x-font-type1 "
                                                    "application/x-font-bdf application/x-font-pcf ",
                                     this, i18n("Select Font to View")));
    if(url.isValid())
        itsPreview->openURL(url);
}

CFontViewerApp::CFontViewerApp()
{
    TDEGlobal::locale()->insertCatalogue(KFI_CATALOGUE);
    setMainWidget(new CFontViewerAppMainWindow());
}

}

static TDECmdLineOptions options[] =
{
    { "+[URL]", I18N_NOOP("URL to open"), 0 },
    TDECmdLineLastOption
};

static TDEAboutData aboutData("tdefontview", I18N_NOOP("Font Viewer"), 0, I18N_NOOP("Simple font viewer"),
                            TDEAboutData::License_GPL,
                            I18N_NOOP("(c) Craig Drummond, 2004"));

int main(int argc, char **argv)
{
    TDECmdLineArgs::init(argc, argv, &aboutData);
    TDECmdLineArgs::addCmdLineOptions(options);
    KFI::CFontViewerApp::addCmdLineOptions();

    KFI::CFontViewerApp app;

    return app.exec();
}

#include "FontViewerApp.moc"
