/*
 * This file is part of the KDE project, module kcmbackground.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 * Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
 *
 * Based on old backgnd.cpp:
 *
 * Copyright (c)  Martin R. Jones 1996
 * Converted to a kcc module by Matthias Hoelzer 1997
 * Gradient backgrounds by Mark Donohoe 1997
 * Pattern backgrounds by Stephan Kulow 1998
 * Randomizing & dnd & new display modes by Matej Koss 1998
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#include <tqlayout.h>
#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <dcopclient.h>
#include <kimageio.h>
#include <kgenericfactory.h>

#include "bgdialog.h"

#include "main.h"

/* as late as possible, as it includes some X headers without protecting them */
#include <X11/Xlib.h>

/**** DLL Interface ****/
typedef KGenericFactory<KBackground, TQWidget> KBackGndFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_background, KBackGndFactory("kcmbackground"))

/**** KBackground ****/
KBackground::~KBackground( )
{
    delete m_pConfig;
}

KBackground::KBackground(TQWidget *parent, const char *name, const TQStringList &/* */)
    : TDECModule(KBackGndFactory::instance(), parent, name)
{
    int screen_number = 0;
    if (tqt_xdisplay())
        screen_number = DefaultScreen(tqt_xdisplay());
    TQCString configname;
    if (screen_number == 0)
        configname = "kdesktoprc";
    else
        configname.sprintf("kdesktop-screen-%drc", screen_number);
    m_pConfig = new TDEConfig(configname, false, false);

    TQVBoxLayout *layout = new TQVBoxLayout(this);
    m_base = new BGDialog(this, m_pConfig);
    setQuickHelp( m_base->quickHelp());
    layout->add(m_base);
    layout->addStretch();

    KImageIO::registerFormats();

    // reparenting that is done.
    setAcceptDrops(true);

    connect(m_base, TQ_SIGNAL(changed(bool)), TQ_SIGNAL(changed(bool)));

    TDEAboutData *about =
    new TDEAboutData(I18N_NOOP("kcmbackground"), I18N_NOOP("TDE Background Control Module"),
                  0, 0, TDEAboutData::License_GPL,
                  I18N_NOOP("(c) 2009,2010 Timothy Pearson"));

    about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
    about->addAuthor("Waldo Bastian", 0, "bastian@kde.org");
    about->addAuthor("George Staikos", 0, "staikos@kde.org");
    about->addAuthor("Martin R. Jones", 0, "jones@kde.org");
    about->addAuthor("Matthias Hoelzer-Kluepfel", 0, "mhk@kde.org");
    about->addAuthor("Stephan Kulow", 0, "coolo@kde.org");
    about->addAuthor("Mark Donohoe", 0, 0);
    about->addAuthor("Matej Koss", 0 , 0);

    setAboutData( about );
}

void KBackground::load()
{
    load( false );
}

void KBackground::load( bool useDefaults )
{
    m_base->load( useDefaults );
}


void KBackground::save()
{
    m_base->save();

    // reconfigure kdesktop. kdesktop will notify all clients
    DCOPClient *client = kapp->dcopClient();
    if (!client->isAttached())
        client->attach();

    int screen_number = 0;
    if (tqt_xdisplay())
        screen_number = DefaultScreen(tqt_xdisplay());
    TQCString appname;
    if (screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", screen_number);

    client->send(appname, "KBackgroundIface", "configure()", TQString(""));
}

void KBackground::defaults()
{
    m_base->defaults();
}

#include "main.moc"
