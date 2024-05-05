////////////////////////////////////////////////////////////////////////////////
//
// Class Name    : KFI::CFontViewPartFactory
// Author        : Craig Drummond
// Project       : K Font Installer
// Creation Date : 03/08/2002
// Version       : $Revision$ $Date$
//
////////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
////////////////////////////////////////////////////////////////////////////////
// (C) Craig Drummond, 2002, 2003, 2004
////////////////////////////////////////////////////////////////////////////////

#include "FontViewPartFactory.h"
#include "FontViewPart.h"
#include <kdebug.h>
#include <tdeaboutdata.h>
#include <kinstance.h>
#include <assert.h>

extern "C"
{
    TDE_EXPORT void* init_libtdefontviewpart()
    {
        TDEGlobal::locale()->insertCatalogue("tdefontinst");
        return new KFI::CFontViewPartFactory;
    }
}

namespace KFI
{

TDEInstance * CFontViewPartFactory::theirInstance=NULL;
TDEAboutData * CFontViewPartFactory::theirAbout=NULL;

CFontViewPartFactory::CFontViewPartFactory()
{
}

CFontViewPartFactory::~CFontViewPartFactory()
{
    delete theirAbout;
    theirAbout=0L;
    delete theirInstance;
    theirInstance=0L;
}

TQObject * CFontViewPartFactory::createObject(TQObject *parent, const char *name, const char *, const TQStringList &)
{
    if(parent && !parent->isWidgetType())
    {
        kdDebug() << "CFontViewPartFactory: parent does not inherit TQWidget" << endl;
        return 0L;
    }

    return new CFontViewPart((TQWidget*) parent, name);
}

TDEInstance* CFontViewPartFactory::instance()
{
    if(!theirInstance)
    {
        theirAbout = new TDEAboutData("fontviewpart", I18N_NOOP("CFontViewPart"), "0.1");
        theirInstance = new TDEInstance(theirAbout);
    }
    return theirInstance;
}

}

#include "FontViewPartFactory.moc"
