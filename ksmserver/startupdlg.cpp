/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2010-2011 Timothy Pearson <kb9vqf@pearsoncomputing.net>
Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#include "startupdlg.h"
#include <tqapplication.h>
#include <tqlayout.h>
#include <tqgroupbox.h>
#include <tqvbuttongroup.h>
#include <tqlabel.h>
#include <tqvbox.h>
#include <tqtimer.h>
#include <tqstyle.h>
#include <tqcombobox.h>
#include <tqcursor.h>
#include <tqmessagebox.h>
#include <tqbuttongroup.h>
#include <tqiconset.h>
#include <tqpixmap.h>
#include <tqpopupmenu.h>
#include <tqtooltip.h>
#include <tqimage.h>
#include <tqpainter.h>
#include <tqfontmetrics.h>
#include <tqregexp.h>
#include <tqeventloop.h>

#include <tdelocale.h>
#include <tdeconfig.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kiconloader.h>
#include <tdeglobalsettings.h>
#include <twin.h>
#include <kuser.h>
#include <kpixmap.h>
#include <kimageeffect.h>
#include <kdialog.h>
#include <kseparator.h>
#include <tdeconfig.h>

#include <dcopclient.h>
#include <dcopref.h>

#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <dmctl.h>
#include <tdeaction.h>
#include <netwm.h>

#include <X11/Xlib.h>

#include "startupdlg.moc"

TQWidget* KSMStartupIPDlg::showStartupIP()
{
    tdeApp->enableStyles();
    KSMStartupIPDlg* l = new KSMStartupIPDlg( 0 );

    tdeApp->disableStyles();

    return l;
}

KSMStartupIPDlg::KSMStartupIPDlg(TQWidget* parent)
  : KSMModalDialog( parent )

{
	setStatusMessage(i18n("Loading your settings").append("..."));

	show();
	setActiveWindow();
}

KSMStartupIPDlg::~KSMStartupIPDlg()
{
}
