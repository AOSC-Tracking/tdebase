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

#ifndef KDMTHEMER_H
#define KDMTHEMER_H

#include <tqobject.h>
#include <tqdom.h>
#include <tqimage.h>

class KdmThemer;
class KdmItem;
class KdmPixmap;
class KdmRect;
class KdmBox;

class TQRect;
class TQWidget;
class TQEvent;

/**
* @author Unai Garro
*/



/*
* The themer widget. Whatever drawn here is just themed
* according to a XML file set by the user.
*/


class KdmThemer : public TQObject {
	Q_OBJECT

public:
	/*
	 * Construct and destruct the interface
	 */

	KdmThemer( const TQString &path, const TQString &mode, TQWidget *parent );
	~KdmThemer();

	bool isOK() { return rootItem != 0; }
	/*
	 * Gives a sizeHint to the widget (parent size)
	 */
	//TQSize sizeHint() const{ return parentWidget()->size(); }

	/*
	 * Takes a shot of the current widget
	 */
//	void pixmap( const TQRect &r, TQPixmap *px );

	virtual // just to put the reference in the vmt
	KdmItem *findNode( const TQString & ) const;

	void updateGeometry( bool force ); // force = true for external calls

	// must be called by parent widget
	void widgetEvent( TQEvent *e );

signals:
	void activated( const TQString &id );

protected slots:
	void slotActivated( const TQString &id );
	void slotPaintRoot();

private:
	/*
	 * Our display mode (e.g. console, remote, ...)
	 */
	TQString m_currentMode;

	/*
	 * The config file being used
	 */
	TQDomDocument domTree;

	/*
	 * Stores the root of the theme
	 */
	KdmItem *rootItem;

	/*
	 * The backbuffer
	 */
	TQPixmap *backBuffer;

	// methods

	/*
	 * Test whether item needs to be displayed
	 */
	bool willDisplay( const TQDomNode &node );

	/*
	 * Parses the XML file looking for the
	 * item list and adds those to the themer
	 */
	void generateItems( KdmItem *parent = 0, const TQDomNode &node = TQDomNode() );

	void showStructure( TQObject *obj );

	TQWidget *widget();
};


#endif
