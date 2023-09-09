/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#include <unistd.h>

#include <tqlayout.h>
#include <tqlabel.h>

#include <dcopclient.h>
#include <tdelocale.h>

#include "filteropts.h"

FilterOptions::FilterOptions(TQWidget *parent, const char *name)
    		  :TDECModule(parent, name)
{

    TQGridLayout *lay = new TQGridLayout(this, 1, 1, 10, 5);

    lay->addWidget(new TQLabel(i18n("Under construction…"), this), 0, 0);
    lay->activate();

}

void FilterOptions::load( bool useDefaults )
{
	
}

void FilterOptions::save()
{
}

void FilterOptions::defaults()
{
    load( true );
}

void FilterOptions::moduleChanged(bool state)
{
    emit changed(state);
}

#include "filteropts.moc"
