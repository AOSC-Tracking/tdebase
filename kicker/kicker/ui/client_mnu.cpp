/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <tqpixmap.h>

#include <kapplication.h>
#include <kdebug.h>
#include <dcopclient.h>

#include "client_mnu.h"
#include "client_mnu.moc"

KickerClientMenu::KickerClientMenu( TQWidget * parent, const char *name )
    : TQPopupMenu( parent, name), DCOPObject( name )
{
}

KickerClientMenu::~KickerClientMenu()
{
}

void KickerClientMenu::clear()
{
    TQPopupMenu::clear();
}

void KickerClientMenu::insertItem( TQPixmap icon, TQString text, int id )
{
    int globalid = TQPopupMenu::insertItem( icon, text, this, TQT_SLOT( slotActivated(int) ) );
    setItemParameter( globalid, id );
}

void KickerClientMenu::insertItem( TQString text, int id )
{
    int globalid = TQPopupMenu::insertItem( text, this, TQT_SLOT( slotActivated(int) ) );
    setItemParameter( globalid, id );
}

TQCString KickerClientMenu::insertMenu( TQPixmap icon, TQString text, int id )
{
    TQString subname("%1-submenu%2");
    TQCString subid = subname.arg(static_cast<const char *>(objId())).arg(id).local8Bit();
    KickerClientMenu *sub = new KickerClientMenu(this, subid);
    int globalid = TQPopupMenu::insertItem( icon, text, sub, id);
    setItemParameter( globalid, id );

    return subid;
}

void KickerClientMenu::connectDCOPSignal( TQCString signal, TQCString appId, TQCString objId )
{
    // very primitive right now
    if ( signal == "activated(int)" ) {
	app = appId;
	obj = objId;
    } else {
	kdWarning() << "DCOP: no such signal " << className() << "::" << signal.data() << endl;
    }
}

bool KickerClientMenu::process(const TQCString &fun, const TQByteArray &data,
			       TQCString &replyType, TQByteArray &replyData)
{
    if ( fun == "clear()" ) {
	clear();
	replyType = "void";
	return true;
    }
    else if ( fun == "insertItem(TQPixmap,TQString,int)" ) {
	TQDataStream dataStream( data, IO_ReadOnly );
	TQPixmap icon;
	TQString text;
	int id;
	dataStream >> icon >> text >> id;
	insertItem( icon, text, id );
	replyType = "void";
	return true;
    }
    else if ( fun == "insertMenu(TQPixmap,TQString,int)" ) {
	TQDataStream dataStream( data, IO_ReadOnly );
	TQPixmap icon;
	TQString text;
	int id;
	dataStream >> icon >> text >> id;
	TQCString ref = insertMenu( icon, text, id );
	replyType = "TQCString";
	TQDataStream replyStream( replyData, IO_WriteOnly );
	replyStream << ref;
	return true;
    }
    else if ( fun == "insertItem(TQString,int)" ) {
	TQDataStream dataStream( data, IO_ReadOnly );
	TQString text;
	int id;
	dataStream >> text >> id;
	insertItem( text, id );
	replyType = "void";
	return true;
    }
    else if ( fun == "connectDCOPSignal(TQCString,TQCString,TQCString)" ) {
	TQDataStream dataStream( data, IO_ReadOnly );
	TQCString signal, appId, objId;
	dataStream >> signal >> appId >> objId;
	connectDCOPSignal( signal, appId, objId );
	replyType = "void";
	return true;
    }
    return false;
}

void KickerClientMenu::slotActivated(int id)
{
    if ( !app.isEmpty()  ) {
	TQByteArray data;
	TQDataStream dataStream( data, IO_WriteOnly );
	dataStream << id;
	kapp->dcopClient()->send( app, obj, "activated(int)", data );
    }
}
