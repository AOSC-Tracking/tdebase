/*
    This file is part of KHelpcenter.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef KHC_FORMATTER_H
#define KHC_FORMATTER_H

#include <tqstring.h>
#include <tqmap.h>

namespace KHC {

class Formatter
{
  public:
    Formatter();
    virtual ~Formatter();

    bool readTemplates();

    virtual TQString header( const TQString &title );
    virtual TQString footer();
    virtual TQString separator();
    virtual TQString docTitle( const TQString & );
    virtual TQString sectionHeader( const TQString & );
    virtual TQString paragraph( const TQString & );
    virtual TQString title( const TQString & );
    
    virtual TQString processResult( const TQString & );

  private:
    bool mHasTemplate;
    TQMap<TQString,TQString> mSymbols;
};

}

#endif
