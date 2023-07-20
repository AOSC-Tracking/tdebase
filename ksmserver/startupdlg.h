/*****************************************************************
ksmserver - the KDE session management server

Copyright (C) 2000 Matthias Ettrich <ettrich@kde.org>
******************************************************************/

#ifndef STARTUPDLG_H
#define STARTUPDLG_H

#include <tqpixmap.h>
#include <tqimage.h>
#include <tqdatetime.h>
#include <kdialog.h>
#include <kpushbutton.h>
#include <tqpushbutton.h>
#include <tqframe.h>
#include <kguiitem.h>
#include <tqtoolbutton.h>
#include <ksharedpixmap.h>

class TQPushButton;
class TQVButtonGroup;
class TQPopupMenu;
class TQTimer;
class TQPainter;
class TQString;
class TDEAction;

#include "timed.h"
#include <tdeapplication.h>
#include <kpixmapio.h>


// The startup-in-progress dialog
class KSMStartupIPDlg : public KSMModalDialog
{
	TQ_OBJECT

public:
	static TQWidget* showStartupIP();

protected:
	~KSMStartupIPDlg();

private:
	KSMStartupIPDlg( TQWidget* parent );
};

#endif
