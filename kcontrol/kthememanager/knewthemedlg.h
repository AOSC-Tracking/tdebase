/*  Copyright (C) 2003 Lukas Tinkl <lukas@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef KNEWTHEMEDLG_H
#define KNEWTHEMEDLG_H

#include <tqstring.h>

#include <kdialogbase.h>

class NewThemeWidget;

/**
 * Dialog for creating new themes, contains just
 * getters and setters used for the theme general properties
 * @author Lukas Tinkl <lukas@kde.org>
 */
class KNewThemeDlg: public KDialogBase
{
    Q_OBJECT
public:
    KNewThemeDlg( TQWidget * parent = 0, const char * name = 0);
    ~KNewThemeDlg();

    TQString getName() const;
    TQString getAuthor() const;
    TQString getEmail() const;
    TQString getHomepage() const;
    TQString getComment() const;
    TQString getVersion() const;

    void setName( const TQString & name );
    void setAuthor( const TQString & author );
    void setEmail( const TQString & email );
    void setVersion( const TQString & version );
private:
    NewThemeWidget * m_base;

private slots:
    void slotThemeNameChanged( const TQString &_text );
};

#endif
