/*
 * Copyright (C) 2003 Fredrik Höglund <fredrik@kde.org>
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

#ifndef __THEMEPAGE_H
#define __THEMEPAGE_H

#include <tqdict.h>


class TDEListView;
class TQString;
class PreviewWidget;
class TQStringList;
class TQListViewItem;
class TQPushButton;

struct ThemeInfo;


class ThemePage : public TQWidget
{
	TQ_OBJECT

	public:
		ThemePage( TQWidget* parent = 0, const char* name = 0 );
		~ThemePage();

		// Called by the KCM
		void save();
		void load();
		void load( bool useDefaults );
		void defaults();

	signals:
		void changed( bool );

	private slots:
		void selectionChanged( TQListViewItem * );
		void installClicked();
		void removeClicked();

	private:
		bool installThemes( const TQString &file );
		void insertTheme( const TQString & );
		const TQStringList getThemeBaseDirs() const;
		bool isCursorTheme( const TQString &theme, const int depth = 0 ) const;
		void insertThemes();
		TQPixmap createIcon( const TQString &, const TQString & ) const;

		TDEListView *listview;
		PreviewWidget *preview;
		TQPushButton *installButton, *removeButton;
		TQString selectedTheme;
		TQString currentTheme;
		TQStringList themeDirs;
		TQDict<ThemeInfo> themeInfo;
};

#endif // __THEMEPAGE_H
