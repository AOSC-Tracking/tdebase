#ifndef __FONT_VIEW_PART_FACTORY_H__
#define __FONT_VIEW_PART_FACTORY_H__

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

#include <klibloader.h>

class TDEInstance;
class TDEAboutData;

namespace KFI
{

class CFontViewPartFactory : public KLibFactory
{
    Q_OBJECT

    public:

    CFontViewPartFactory();
    virtual ~CFontViewPartFactory();
    virtual TQObject *createObject(TQObject *parent = 0, const char *name = 0, const char *classname = "TQObject", const TQStringList &args = TQStringList());

    static TDEInstance * instance();

    private:

    static TDEInstance  *theirInstance;
    static TDEAboutData *theirAbout;
};

}

#endif
