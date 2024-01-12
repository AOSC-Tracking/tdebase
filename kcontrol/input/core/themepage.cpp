/*
 * Copyright (C) 2003 Fredrik H�glund <fredrik@kde.org>
 *
 * Based on the large cursor code written by Rik Hemsley,
 * Copyright (c) 2000 Rik Hemsley <rik@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <tdeglobal.h>
#include <kstandarddirs.h>
#include <kurl.h>
#include <kprocess.h>
#include <tdeio/job.h>
#include <tdeio/netaccess.h>
#include <tdemessagebox.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <tdelistview.h>
#include <kdialog.h>

#include <tqlayout.h>
#include <tqdir.h>
#include <tqpixmap.h>
#include <tqimage.h>
#include <tqlabel.h>

#include "themepage.h"
#include "themepage.moc"

#include "bitmaps.h"


namespace {
	// Listview columns
	enum Columns { NameColumn = 0, DescColumn, /* hidden */ DirColumn };
}


ThemePage::ThemePage( TQWidget* parent, const char* name )
	: TQWidget( parent, name )
{
	TQBoxLayout *layout = new TQVBoxLayout( this );
	layout->setAutoAdd( true );
	layout->setMargin( KDialog::marginHint() );
	layout->setSpacing( KDialog::spacingHint() );

	new TQLabel( i18n("Select the cursor theme you want to use:"), this );

	// Create the theme list view
	listview = new TDEListView( this );
	listview->setFullWidth( true );
	listview->setAllColumnsShowFocus( true );
	listview->addColumn( i18n("Name") );
	listview->addColumn( i18n("Description") );

	connect( listview, TQ_SIGNAL(selectionChanged(TQListViewItem*)),
			TQ_SLOT(selectionChanged(TQListViewItem*)) );

	insertThemes();
}


ThemePage::~ThemePage()
{
}


void ThemePage::selectionChanged( TQListViewItem *item )
{
	selectedTheme = item->text( DirColumn );
	emit changed( selectedTheme != currentTheme );
}


void ThemePage::save()
{
	if ( currentTheme == selectedTheme )
		return;

	bool whiteCursor = selectedTheme.right( 5 ) == "White";
	bool largeCursor = selectedTheme.left( 5 ) == "Large";

	TDEConfig c( "kcminputrc" );
	c.setGroup( "Mouse" );
	c.writeEntry( "LargeCursor", largeCursor );
	c.writeEntry( "WhiteCursor", whiteCursor );

	currentTheme = selectedTheme;

	fixCursorFile();

	KMessageBox::information( this, i18n("You have to restart TDE for these "
				"changes to take effect."), i18n("Cursor Settings Changed"),
				"CursorSettingsChanged" );
}

void ThemePage::load()
{
	load( false );
}

void ThemePage::load( bool useDefaults )
{
	bool largeCursor, whiteCursor;

	TDEConfig c( "kcminputrc" );

	c.setReadDefaults( useDefaults );

	c.setGroup( "Mouse" );
	largeCursor = c.readBoolEntry( "LargeCursor", false );
	whiteCursor = c.readBoolEntry( "WhiteCursor", false );

	if ( largeCursor )
		currentTheme = whiteCursor ? "LargeWhite" : "LargeBlack";
	else
		currentTheme = whiteCursor ? "SmallWhite" : "SmallBlack";

	selectedTheme = currentTheme;
	TQListViewItem *item = listview->findItem( currentTheme, DirColumn );
	item->setSelected( true );
}


void ThemePage::defaults()
{
	load( true );
}


void ThemePage::insertThemes()
{
	TDEListViewItem *item;

	item = new TDEListViewItem( listview, i18n("Small black"),
			i18n("Small black cursors"), "SmallBlack" );
	item->setPixmap( 0, TQPixmap( arrow_small_black_xpm ) );
	listview->insertItem( item );

	item = new TDEListViewItem( listview, i18n("Large black"),
			i18n("Large black cursors"), "LargeBlack" );
	item->setPixmap( 0, TQPixmap( arrow_large_black_xpm ) );
	listview->insertItem( item );

	item = new TDEListViewItem( listview, i18n("Small white"),
			i18n("Small white cursors"), "SmallWhite" );
	item->setPixmap( 0, TQPixmap( arrow_small_white_xpm ) );
	listview->insertItem( item );

	item = new TDEListViewItem( listview, i18n("Large white"),
			i18n("Large white cursors"), "LargeWhite" );
	item->setPixmap( 0, TQPixmap( arrow_large_white_xpm ) );
	listview->insertItem( item );
}


void ThemePage::fixCursorFile()
{
	// Make sure we have the 'font' resource dir registered and can find the
	// override dir.
	//
	// Next, if the user wants large cursors, copy the font
	// cursor_large.pcf.gz to (localtdedir)/share/fonts/override/cursor.pcf.gz.
	// Else remove the font cursor.pcf.gz from (localtdedir)/share/fonts/override.
	//
	// Run mkfontdir to update fonts.dir in that dir.

	TDEGlobal::dirs()->addResourceType( "font", "share/fonts/" );
	TDEIO::mkdir( KURL::fromPathOrURL(TQDir::homeDirPath() + "/.fonts/kde-override") );
	TQString overrideDir = TQDir::homeDirPath() + "/.fonts/kde-override/";

	KURL installedFont;
	installedFont.setPath( overrideDir + "cursor.pcf.gz" );

	if ( currentTheme == "SmallBlack" )
		TDEIO::NetAccess::del( installedFont, this );
	else {
		KURL source;

		if ( currentTheme == "LargeBlack" )
			source.setPath( locate("data", "kcminput/cursor_large_black.pcf.gz") );
		else if ( currentTheme == "LargeWhite" )
			source.setPath( locate("data", "kcminput/cursor_large_white.pcf.gz") );
		else if ( currentTheme == "SmallWhite" )
			source.setPath( locate("data", "kcminput/cursor_small_white.pcf.gz") );

		TDEIO::NetAccess::file_copy( source, installedFont, -1, true );
	}

	TQString cmd = TDEGlobal::dirs()->findExe( "mkfontdir" );
	if ( !cmd.isEmpty() )
	{
		TDEProcess p;
		p << cmd << overrideDir;
		p.start(TDEProcess::Block);
	}
}
