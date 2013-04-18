#include <tqfile.h>
#include <tqmap.h>
#include <tqregexp.h>
#include <tqstring.h>
#include <tqtextstream.h>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kprocess.h>
#include <ktempfile.h>

struct AppletInfo
{
    double freeSpace;
    TQString configFile;
    TQString desktopFile;
};
typedef TQMap<TQString, AppletInfo> AppletInfoMap;

int main(int argc, char** argv)
{
    // We must disguise as Kicker in order to obtain the correct reverseLayout setting.
    KCmdLineArgs::init(argc, argv, "kicker", "", "", "", false);
    KApplication app(false, false);

    TQStringList stretchableApplets;
    stretchableApplets << "taskbarapplet.desktop";

    TQTextStream in (stdin,  IO_ReadOnly);
    TQTextStream out(stdout, IO_WriteOnly);

    TQStringList appletIds;
    AppletInfoMap applets;

    TQRegExp rxGroup("^\\[(.+)\\]$");
    TQRegExp rxKeyValue("([^=]+)=[ \t]*([^\n]+)");
    TQString currentGroup;

    TQString line;
    while (!(line = in.readLine()).isNull())
    {
        if (rxGroup.search(line) != -1)
        {
            currentGroup = rxGroup.cap(1);
            continue;
        }

        if (rxKeyValue.search(line) != -1)
        {
            TQString key   = rxKeyValue.cap(1);
            TQString value = rxKeyValue.cap(2);

            if (key == "Applets")
            {
                appletIds = TQStringList::split(",", value);
            }
            else if (key == "FreeSpace")
            {
                applets[currentGroup].freeSpace = value.toDouble();
            }
            else if (key == "ConfigFile")
            {
                applets[currentGroup].configFile = value;
            }
            else if (key == "DesktopFile")
            {
                applets[currentGroup].desktopFile = value;
            }
        }
    }

    if (TQApplication::reverseLayout())
    {
        // Reverse appletIds
        TQStringList appletIdsRev;
        TQStringList::ConstIterator it;
        for (it = appletIds.begin(); it != appletIds.end(); ++it)
        {
            appletIdsRev.prepend(*it);
        }
        appletIds = appletIdsRev;

        // Adjust the FreeSpace values
        for (it = appletIds.begin(); it != appletIds.end(); ++it)
        {
            applets[*it].freeSpace = 1 - applets[*it].freeSpace;

            // Take care of stretchable applets.
            if (stretchableApplets.contains(applets[*it].desktopFile))
            {
                if (it != appletIds.begin())
                {
                    applets[*it].freeSpace = applets[*(--it)].freeSpace; 
                    ++it;
                }
                else
                {
                    applets[*it].freeSpace = 0;
                }
            }
        }
    }

    // Write the changed entries to stdout.
    if (!appletIds.empty())
    {
        out << "[General]" << endl;
        out << "Applets2=" << appletIds.join(",") << endl;
        TQStringList::ConstIterator it;
        for (it = appletIds.begin(); it != appletIds.end(); ++it)
        {
            out << "[" << *it << "]" << endl;
            out << "FreeSpace2=" << applets[*it].freeSpace << endl;
        }
    }

    // Build a list of childpanel config files.
    TQStringList childPanelConfigFiles;
    AppletInfoMap::ConstIterator it2;
    TQStringList::ConstIterator it;
    for (it2 = applets.begin(); it2 != applets.end(); ++it2)
    {
        if (it2.data().desktopFile == "childpanelextension.desktop")
        {
            // Prevent infinite recursion
            if (it2.data().configFile != "kickerrc")
            {
                childPanelConfigFiles << it2.data().configFile;
            }
        }
    }

    if (!childPanelConfigFiles.isEmpty())
    {
        // Create a temporary kconf_update .upd file for updating the childpanels
        KTempFile tempFile(TQString::null, ".upd");
        TQTextStream* upd = tempFile.textStream();
        for (it = childPanelConfigFiles.begin(); it != childPanelConfigFiles.end(); ++it)
        {
            *upd << "Id=kde_3.4_reverseLayout" << endl;
            *upd << "File=" << *it << endl;
            *upd << "Script=kicker-3.4-reverseLayout" << endl;
            *upd << endl;
        }
        tempFile.close();

        // Run kconf_update on the childpanel config files.
        KProcess kconf_update;
        kconf_update << "kconf_update" << tempFile.name();
        kconf_update.start(KProcess::Block);

        tempFile.unlink();
    }
}
