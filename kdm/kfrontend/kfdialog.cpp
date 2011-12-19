/*

Dialog class that handles input focus in absence of a wm

Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
Copyright (C) 2000-2003 Oswald Buddenhagen <ossi@kde.org>


This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "kfdialog.h"
#include "kdmconfig.h"

#include <klocale.h>
#include <kpushbutton.h>
#include <kstdguiitem.h>
#include <kglobalsettings.h>

#include <tqlabel.h>
#include <tqlayout.h>
#include <tqapplication.h>
#include <tqcursor.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

extern bool has_kwin;
extern bool is_themed;

FDialog::FDialog( TQWidget *parent, bool framed )
	: inherited( parent, 0, true, (framed&&has_kwin)?0:WX11BypassWM ), winFrame(NULL), m_wmTitle(has_kwin)
{
	if (framed) {
		// Signal that we do not want any window controls to be shown at all
		Atom kde_wm_system_modal_notification;
		kde_wm_system_modal_notification = XInternAtom(qt_xdisplay(), "_KDE_WM_MODAL_SYS_NOTIFICATION", False);
		XChangeProperty(qt_xdisplay(), winId(), kde_wm_system_modal_notification, XA_INTEGER, 32, PropModeReplace, (unsigned char *) "TRUE", 1L);
	}

	if (framed) {
		winFrame = new TQFrame( this, 0, TQt::WNoAutoErase );
		if (m_wmTitle)
			winFrame->setFrameStyle( TQFrame::NoFrame );
		else
			winFrame->setFrameStyle( TQFrame::WinPanel | TQFrame::Raised );
		winFrame->setLineWidth( 2 );
	} else
		winFrame = 0;

	setCaption(KDM_LOGIN_SCREEN_BASE_TITLE);

	if (framed) {
		if (m_wmTitle) setFixedSize(sizeHint());
	}
}

void
FDialog::resizeEvent( TQResizeEvent *e )
{
	inherited::resizeEvent( e );
	if (winFrame) {
		winFrame->resize( size() );
		winFrame->erase();
		if (m_wmTitle) setFixedSize(sizeHint());
	}
}

void
FDialog::adjustGeometry()
{
	TQDesktopWidget *dsk = tqApp->desktop();

	if (_greeterScreen < 0)
		_greeterScreen = _greeterScreen == -2 ?
			dsk->screenNumber( TQPoint( dsk->width() - 1, 0 ) ) :
			dsk->screenNumber( TQPoint( 0, 0 ) );

	TQRect scr = dsk->screenGeometry( _greeterScreen );
	if (!winFrame)
		setFixedSize( scr.size() );
	else {
		setMaximumSize( scr.size() * .9 );
		adjustSize();
	}

	if (parentWidget())
		return;

	TQRect grt( rect() );
	if (winFrame) {
		unsigned x = 50, y = 50;
		sscanf( _greeterPos, "%u,%u", &x, &y );
		grt.moveCenter( TQPoint( scr.x() + scr.width() * x / 100,
		                        scr.y() + scr.height() * y / 100 ) );
		int di;
		if ((di = scr.right() - grt.right()) < 0)
			grt.moveBy( di, 0 );
		if ((di = scr.left() - grt.left()) > 0)
			grt.moveBy( di, 0 );
		if ((di = scr.bottom() - grt.bottom()) < 0)
			grt.moveBy( 0, di );
		if ((di = scr.top() - grt.top()) > 0)
			grt.moveBy( 0, di );
		setGeometry( grt );
	}

	if (dsk->screenNumber( TQCursor::pos() ) != _greeterScreen)
		TQCursor::setPos( grt.center() );
}

struct WinList {
	struct WinList *next;
	TQWidget *win;
};

int
FDialog::exec()
{
	static WinList *wins;
	WinList *win;

	win = new WinList;
	win->win = this;
	win->next = wins;
	wins = win;
	show();
	setActiveWindow();
	inherited::exec();
	hide();
	wins = win->next;
	delete win;
	if (wins)
		wins->win->setActiveWindow();
	return result();
}

void
FDialog::box( TQWidget *parent, TQMessageBox::Icon type, const TQString &text )
{
	KFMsgBox dlg( parent, type, text.stripWhiteSpace() );
	dlg.exec();
}

KFMsgBox::KFMsgBox( TQWidget *parent, TQMessageBox::Icon type, const TQString &text )
	: inherited( parent, !is_themed )
{
	if (type == TQMessageBox::NoIcon) setCaption(KDM_LOGIN_SCREEN_BASE_TITLE);
	if (type == TQMessageBox::Question) setCaption(KDM_LOGIN_SCREEN_BASE_TITLE + " - " + i18n("Question"));
	if (type == TQMessageBox::Information) setCaption(KDM_LOGIN_SCREEN_BASE_TITLE + " - " + i18n("Information"));
	if (type == TQMessageBox::Warning) setCaption(KDM_LOGIN_SCREEN_BASE_TITLE + " - " + i18n("Warning"));
	if (type == TQMessageBox::Critical) setCaption(KDM_LOGIN_SCREEN_BASE_TITLE + " - " + i18n("Error"));

	TQLabel *label1 = new TQLabel( this );
	label1->setPixmap( TQMessageBox::standardIcon( type ) );
	TQLabel *label2 = new TQLabel( text, this );
	TQRect d = KGlobalSettings::desktopGeometry(this);
	if ( label2->fontMetrics().size( 0, text).width() > d.width() * 3 / 5) 
		label2->setAlignment(TQt::WordBreak | TQt::AlignAuto );
	KPushButton *button = new KPushButton( KStdGuiItem::ok(), this );
	button->setDefault( true );
	button->setSizePolicy( TQSizePolicy( TQSizePolicy::Preferred, TQSizePolicy::Preferred ) );
	connect( button, TQT_SIGNAL(clicked()), TQT_SLOT(accept()) );

	TQGridLayout *grid = new TQGridLayout( this, 2, 2, 10 );
	grid->addWidget( label1, 0, 0, Qt::AlignCenter );
	grid->addWidget( label2, 0, 1, Qt::AlignCenter );
	grid->addMultiCellWidget( button, 1,1, 0,1, Qt::AlignCenter );
}
