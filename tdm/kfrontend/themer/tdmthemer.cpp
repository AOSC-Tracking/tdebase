/*
 *  Copyright (C) 2003 by Unai Garro <ugarro@users.sourceforge.net>
 *  Copyright (C) 2004 by Enrico Ros <rosenric@dei.unipd.it>
 *  Copyright (C) 2004 by Stephan Kulow <coolo@kde.org>
 *  Copyright (C) 2004 by Oswald Buddenhagen <ossi@kde.org>
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
 */

#include "tdmthemer.h"
#include "tdmitem.h"
#include "tdmpixmap.h"
#include "tdmrect.h"
#include "tdmlabel.h"

#include <tdmconfig.h>
#include <kfdialog.h>

#include <kiconloader.h>
#include <kimageeffect.h>
#include <tdelocale.h>
#include <ksimpleconfig.h>
#include <kdebug.h>

#include <tqfile.h>
#include <tqfileinfo.h>
#include <tqtimer.h>		// animation timer - TODO
#include <tqobjectlist.h>
#include <tqpainter.h>
#include <tqwidget.h>
#include <tqregion.h>
#include <tqlineedit.h>
#include <tqapplication.h>

#include <unistd.h>

extern bool argb_visual_available;

/*
 * KdmThemer. The main theming interface
 */
KdmThemer::KdmThemer( const TQString &_filename, const TQString &mode, TQWidget *parent )
    : TQObject( parent )
    , rootItem( 0 )
    , backBuffer( 0 )
{
	// Set the mode we're working in
	m_currentMode = mode;

	// read the XML file and create DOM tree
	TQString filename = _filename;
	if (!::access( TQFile::encodeName( filename + "/GdmGreeterTheme.desktop" ), R_OK )) {
		KSimpleConfig cfg( filename + "/GdmGreeterTheme.desktop" );
		cfg.setGroup( "GdmGreeterTheme" );
		filename += '/' + cfg.readEntry( "Greeter" );
	}
	TQFile opmlFile( filename );
	if (!opmlFile.open( IO_ReadOnly )) {
		FDialog::box( widget(), errorbox, i18n( "Cannot open theme file %1" ).arg(filename) );
		return;
	}
	if (!domTree.setContent( &opmlFile )) {
		FDialog::box( widget(), errorbox, i18n( "Cannot parse theme file %1" ).arg(filename) );
		return;
	}
	// Set the root (screen) item
	rootItem = new KdmRect( parent, TQDomNode(), "tdm root" );

	connect( rootItem, TQT_SIGNAL(needUpdate( int, int, int, int )),
	         widget(), TQT_SLOT(update( int, int, int, int )) );

	rootItem->setBaseDir( TQFileInfo( filename ).dirPath( true ) );

	// generate all the items defined in the theme
	generateItems( rootItem );

	connect( rootItem, TQT_SIGNAL(activated( const TQString & )), TQT_SIGNAL(activated( const TQString & )) );
	connect( rootItem, TQT_SIGNAL(activated( const TQString & )), TQT_SLOT(slotActivated( const TQString & )) );

	TQTimer::singleShot(800, this, TQT_SLOT(slotPaintRoot()));

/*	*TODO*
	// Animation timer
	TQTimer *time = new TQTimer( this );
	time->start( 500 );
	connect( time, TQT_SIGNAL(timeout()), TQT_SLOT(update()) )
*/
}

KdmThemer::~KdmThemer()
{
	delete rootItem;
	delete backBuffer;
}

inline TQWidget *
KdmThemer::widget()
{
	return static_cast<TQWidget *>(parent());
}

KdmItem *
KdmThemer::findNode( const TQString &item ) const
{
	return rootItem->findNode( item );
}

KdmItem *
KdmThemer::findNodeByType( const TQString &item ) const
{
	return rootItem->findNodeByType( item );
}

void
KdmThemer::updateGeometry( bool force )
{
	rootItem->setGeometry( TQRect( TQPoint(), widget()->size() ), force );
}

// BEGIN other functions

void
KdmThemer::widgetEvent( TQEvent *e )
{
	if (!rootItem)
		return;
	switch (e->type()) {
	case TQEvent::MouseMove:
		{
			TQMouseEvent *me = static_cast<TQMouseEvent*>(e);
			rootItem->mouseEvent( me->x(), me->y() );
		}
		break;
	case TQEvent::MouseButtonPress:
		{
			TQMouseEvent *me = static_cast<TQMouseEvent*>(e);
			rootItem->mouseEvent( me->x(), me->y(), true );
		}
		break;
	case TQEvent::MouseButtonRelease:
		{
			TQMouseEvent *me = static_cast<TQMouseEvent*>(e);
			rootItem->mouseEvent( me->x(), me->y(), false, true );
		}
		break;
	case TQEvent::Show:
		rootItem->show();
		break;
	case TQEvent::Resize:
		updateGeometry( false );
		showStructure( rootItem );
		break;
	case TQEvent::Paint:
		{
			TQRect paintRect = static_cast<TQPaintEvent*>(e)->rect();
			kdDebug() << timestamp() << " paint on: " << paintRect << endl;

			if (!argb_visual_available) {
				// Software blend only (no compositing support)
				if (!backBuffer) {
					backBuffer = new TQPixmap( widget()->size() );
				}
				if (backBuffer->size() != widget()->size()) {
					backBuffer->resize( widget()->size() );
				}
	
				TQPainter p;
				p.begin( backBuffer );
				rootItem->paint( &p, paintRect );
				p.end();
	
				bitBlt( widget(), paintRect.topLeft(), backBuffer, paintRect );
			}
			else {
				// We have compositing support!
				if (!backBuffer) {
					backBuffer = new TQPixmap( widget()->size(), 32 );
				}
				if (backBuffer->size() != widget()->size()) {
					backBuffer->resize( widget()->size() );
				}

				TQRgb blend_color = tqRgba(0, 0, 0, 0);   // RGBA
				float alpha = tqAlpha(blend_color) / 255.;
				int pixel = tqAlpha(blend_color) << 24 |
						int(tqRed(blend_color) * alpha) << 16 |
						int(tqGreen(blend_color) * alpha) << 8  |
						int(tqBlue(blend_color) * alpha);

				TQPainter p;
				p.begin( backBuffer );
				p.fillRect( paintRect, TQColor(blend_color, pixel) );
				rootItem->paint( &p, paintRect );
				p.end();

				bitBlt( widget(), paintRect.topLeft(), backBuffer, paintRect );
			}

		}
		break;
	default:
		break;
	}
}

/*
void
KdmThemer::pixmap( const TQRect &r, TQPixmap *px )
{
	bitBlt( px, TQPoint( 0, 0 ), widget(), r );
}
*/

void
KdmThemer::generateItems( KdmItem *parent, const TQDomNode &node )
{
	if (!parent)
		return;

	TQDomNodeList subnodeList;	//List of subnodes of this node

	/*
	 * Get the child nodes
	 */
	if (node.isNull()) {	// It's the first node, get its child nodes
		TQDomElement theme = domTree.documentElement();

		// Get its tag, and check it's correct ("greeter")
		if (theme.tagName() != "greeter") {
			kdDebug() << timestamp() << " This does not seem to be a correct theme file." << endl;
			return;
		}
		// Get the list of child nodes
		subnodeList = theme.childNodes();
	} else
		subnodeList = node.childNodes();

	/*
	 * Go through each of the child nodes
	 */
	for (uint nod = 0; nod < subnodeList.count(); nod++) {
		TQDomNode subnode = subnodeList.item( nod );
		TQDomElement el = subnode.toElement();
		TQString tagName = el.tagName();

		if (tagName == "item") {
			if (!willDisplay( subnode )) {
				continue;
			}
			TQString id = el.attribute("id");
			if (id.startsWith("plugin-specific-")) {
				id = id.mid(strlen("plugin-specific-"));
				if (!_pluginsLogin.contains(id)) {
					continue;
				}
			}

			// It's a new item. Draw it
			TQString type = el.attribute( "type" );

			KdmItem *newItem = 0;

			if (type == "label") {
				newItem = new KdmLabel( parent, subnode );
			}
			else if (type == "pixmap") {
				newItem = new KdmPixmap( parent, subnode );
			}
			else if (type == "rect") {
				newItem = new KdmRect( parent, subnode );
			}
			else if (type == "entry" || type == "list") {
				newItem = new KdmRect( parent, subnode );
				newItem->setType( type );
			}
			//	newItem = new KdmEntry( parent, subnode );
			else if (type == "svg") {
				newItem = new KdmPixmap( parent, subnode );
			}
			if (newItem) {
				generateItems( newItem, subnode );
				if (el.attribute( "button", "false" ) == "true") {
					newItem->inheritFromButton( newItem );
				}
			}
		} else if (tagName == "box") {
			if (!willDisplay( subnode )) {
				continue;
			}
			// It's a new box. Draw it
			parent->setBoxLayout( subnode );
			generateItems( parent, subnode );
		} else if (tagName == "fixed") {
			if (!willDisplay( subnode )) {
				continue;
			}
			// It's a new box. Draw it
			parent->setFixedLayout( subnode );
			generateItems( parent, subnode );
		}
	}
}

bool KdmThemer::willDisplay( const TQDomNode &node )
{
	TQDomNode showNode = node.namedItem( "show" );

	// No "show" node means this item can be displayed at all times
	if (showNode.isNull())
		return true;

	TQDomElement el = showNode.toElement();

	TQString modes = el.attribute( "modes" );
	if (!modes.isNull()) {
		TQStringList modeList = TQStringList::split( ",", modes );

		// If current mode isn't in this list, do not display item
		if (modeList.find( m_currentMode ) == modeList.end())
			return false;
	}

	TQString type = el.attribute( "type" );
	if (type == "config" || type == "suspend")
		return false;	// not implemented (yet)
	if (type == "timed")
		return _autoLoginDelay != 0;
	if (type == "chooser")
#ifdef XDMCP
		return _loginMode != LOGIN_LOCAL_ONLY;
#else
		return false;
#endif
	if (type == "halt" || type == "reboot")
		return _allowShutdown != SHUT_NONE;
        else if (type == "userlist")
            return _userList;
        else if ( type == "!userlist" )
            return !_userList;

//	if (type == "system")
//		return true;

	// All tests passed, item will be displayed
	return true;
}

void
KdmThemer::showStructure( TQObject *obj )
{

	const TQObjectList wlist = obj->childrenListObject();
	static int counter = 0;
	if (counter == 0) {
		kdDebug() << timestamp() << " \n\n<=======  Widget tree =================" << endl;
	}
	if (!wlist.isEmpty()) {
		counter++;
		TQObjectListIterator it( wlist );
		TQObject *object;

		while ((object = it.current()) != 0) {
			++it;
			TQString node;
			for (int i = 1; i < counter; i++)
				node += "-";

			if (object->inherits( "KdmItem" )) {
				KdmItem *widget = (KdmItem *)object;
				kdDebug() << node << "|" << widget->type() << " me=" << widget->id << " " << widget->area << endl;
			}

			showStructure( object );
		}
		counter--;
	}
	if (counter == 0) {
		kdDebug() << timestamp() << " \n\n<=======  Widget tree =================\n\n" << endl;
	}
}

void
KdmThemer::slotActivated( const TQString &id )
{
	TQString toactivate;
	if (id == "username-label") {
		toactivate = "user-entry";
	}
	else if (id == "password-label") {
		toactivate = "pw-entry";
	}
	else {
		return;
	}
	
	KdmItem *item = findNode(toactivate);
	if (!item || !item->widget()) {
		return;
	}
	
	item->widget()->setFocus();
	TQLineEdit *le = (TQLineEdit*)item->widget()->tqt_cast("TQLineEdit");
	if (le) {
		le->selectAll();
	}
}

void
KdmThemer::slotPaintRoot()
{
	KdmItem *back_item = findNode("background");
	
	TQRect screen = TQApplication::desktop()->screenGeometry(0);
	TQPixmap pm(screen.size());
	
	if (back_item) {
		TQPainter painter( &pm, true );
		back_item->paint( &painter, back_item->rect());
		painter.end();
	}
	else return;
	
	TQApplication::desktop()->screen()->setErasePixmap(pm);
	TQApplication::desktop()->screen()->erase();
}

#include "tdmthemer.moc"
