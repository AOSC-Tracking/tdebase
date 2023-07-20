/**
 * This file is part of KHelpCenter
 *
 * Copyright (c) 2000 Matthias H�lzer-Kl�pfel <hoelzer@kde.org>
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

#ifndef KHC_HTMLSEARCHCONFIG_H
#define KHC_HTMLSEARCHCONFIG_H

#include <tqwidget.h>

class KURLRequester;
class TDEConfig;

namespace KHC {

class HtmlSearchConfig : public TQWidget
{
    TQ_OBJECT
  public:
    HtmlSearchConfig(TQWidget *parent = 0L, const char *name = 0L);
    virtual ~HtmlSearchConfig();

    void load( TDEConfig * );
    void save( TDEConfig * );
    void defaults();
    void makeReadOnly();

  signals:
    void changed();

  protected slots:
    void urlClicked(const TQString&);

  private:
    KURLRequester *mHtsearchUrl;
    KURLRequester *mIndexerBin;
    KURLRequester *mDbDir;
};

}

#endif
