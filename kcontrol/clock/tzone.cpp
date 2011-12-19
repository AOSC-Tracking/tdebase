/*
 *  tzone.cpp
 *
 *  Copyright (C) 1998 Luca Montecchiani <m.luca@usa.net>
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <config.h>

#include <tqlabel.h>
#include <tqfile.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <kio/netaccess.h>

//#include "xpm/world.xpm"
#include "tzone.h"
#include "tzone.moc"

#if defined(USE_SOLARIS)
#include <ktempfile.h>
#include <kstandarddirs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

Tzone::Tzone(TQWidget * parent, const char *name)
  : TQVGroupBox(parent, name)
{
    setTitle(i18n("To change the timezone, select your area from the list below"));

    tzonelist = new KTimezoneWidget(this, "ComboBox_1", &m_zoneDb);
    connect( tzonelist, TQT_SIGNAL(selectionChanged()), TQT_SLOT(handleZoneChange()) );

    m_local = new TQLabel(this);

    load();

    tzonelist->setEnabled(getuid() == 0);
}

void Tzone::load()
{
    currentZone();

    // read the currently set time zone
    tzonelist->setSelected(m_zoneDb.local()->name(), true);
}

void Tzone::currentZone()
{
    TQString localZone(i18n("Current local timezone: %1 (%2)"));
    TQCString result(100);

    time_t now = time(0);
    tzset();
    strftime(result.data(), result.size(), "%Z", localtime(&now));
    m_local->setText(localZone.arg(KTimezoneWidget::displayName(m_zoneDb.local())).arg(static_cast<const char *>(result)));
}

// FIXME: Does the logic in this routine actually work correctly? For example,
// on non-Solaris systems which do not use /etc/timezone?
void Tzone::save()
{
    TQStringList selectedZones(tzonelist->selection());

    if (selectedZones.count() > 0)
    {
      // Find untranslated selected zone
      TQString selectedzone(selectedZones[0]);

#if defined(USE_SOLARIS)	// MARCO

        KTempFile tf( locateLocal( "tmp", "kde-tzone" ) );
        tf.setAutoDelete( true );
        TQTextStream *ts = tf.textStream();

# ifndef INITFILE
#  define INITFILE	"/etc/default/init"
# endif

        TQFile fTimezoneFile(INITFILE);
        bool updatedFile = false;

        if (tf.status() == 0 && fTimezoneFile.open(IO_ReadOnly))
        {
            bool found = false;

            TQTextStream is(&fTimezoneFile);

            for (TQString line = is.readLine(); !line.isNull();
                 line = is.readLine())
            {
                if (line.find("TZ=") == 0)
                {
                    *ts << "TZ=" << selectedzone << endl;
                    found = true;
                }
                else
                {
                    *ts << line << endl;
                }
            }

            if (!found)
            {
                *ts << "TZ=" << selectedzone << endl;
            }

            updatedFile = true;
            fTimezoneFile.close();
        }

        if (updatedFile)
        {
            ts->device()->reset();
            fTimezoneFile.remove();

            if (fTimezoneFile.open(IO_WriteOnly | IO_Truncate))
            {
                TQTextStream os(&fTimezoneFile);

                for (TQString line = ts->readLine(); !line.isNull();
                     line = ts->readLine())
                {
                    os << line << endl;
                }

                fchmod(fTimezoneFile.handle(),
                       S_IXUSR | S_IRUSR | S_IRGRP | S_IXGRP |
                       S_IROTH | S_IXOTH);
                fTimezoneFile.close();
            }
        }


        TQString val = selectedzone;
#else
        TQFile fTimezoneFile("/etc/timezone");

        if (fTimezoneFile.open(IO_WriteOnly | IO_Truncate) )
        {
            TQTextStream t(&fTimezoneFile);
            t << selectedzone;
            fTimezoneFile.close();
        }

        TQString tz = "/usr/share/zoneinfo/" + selectedzone;

        kdDebug() << "Set time zone " << tz << endl;

	if (!TQFile::remove("/etc/localtime"))
	{
		//After the KDE 3.2 release, need to add an error message
	}
	else
		if (!KIO::NetAccess::file_copy(KURL(tz),KURL("/etc/localtime")))
			KMessageBox::error( 0,  i18n("Error setting new timezone."),
                        		    i18n("Timezone Error"));

        TQString val = ":" + tz;
#endif // !USE_SOLARIS

        setenv("TZ", val.ascii(), 1);
        tzset();

    } else {
#if !defined(USE_SOLARIS) // Do not update the System!
        unlink( "/etc/timezone" );
        unlink( "/etc/localtime" );

        setenv("TZ", "", 1);
        tzset();
#endif // !USE SOLARIS
    }

    currentZone();
}
