#ifndef __KCM_FONT_INST_H__
#define __KCM_FONT_INST_H__

////////////////////////////////////////////////////////////////////////////////
//
// Class Name    : KFI::CKCmFontInst
// Author        : Craig Drummond
// Project       : K Font Installer
// Creation Date : 26/04/2003
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
// (C) Craig Drummond, 2003, 2004
////////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tqstringlist.h>
#include <tdecmodule.h>
#include <kurl.h>
#include <tdeconfig.h>
#include <tdeio/job.h>
#ifdef HAVE_XFT
#include <tdeparts/part.h>
#endif

class KDirOperator;
class TDEAction;
class TDERadioAction;
class TDEActionMenu;
class TDEToggleAction;
class KFileItem;
class TQLabel;
class TQSplitter;
class TQDropEvent;
class KFileItem;

namespace KFI
{

class CKCmFontInst : public TDECModule
{
    TQ_OBJECT

    public:

    CKCmFontInst(TQWidget *parent=NULL, const char *name=NULL, const TQStringList &list=TQStringList());
    virtual ~CKCmFontInst();

    void    setMimeTypes(bool showBitmap);

    public slots:

    void    filterFonts();
    TQString quickHelp() const;
    void    listView();
    void    iconView();
    void    setupMenu();
    void    setupViewMenu();
    void    fileHighlighted(const KFileItem *item);
    void    loadingFinished();
    void    addFonts();
    void    removeFonts();
    void    configure();
    void    print();
    void    dropped(const KFileItem *i, TQDropEvent *e, const KURL::List &urls);
    void    infoMessage(const TQString &msg);
    void    updateInformation(int dirs, int fonts);
    void    delResult(TDEIO::Job *job);
    void    jobResult(TDEIO::Job *job);

    private:

    void    addFonts(const KURL::List &src, const KURL &dest);

    private:

    KDirOperator         *itsDirOp;
    KURL                 itsTop;
    TDEToggleAction        *itsShowBitmapAct;
    TDEAction              *itsSepDirsAct,
                         *itsShowHiddenAct,
                         *itsDeleteAct;
    TDERadioAction         *itsListAct,
                         *itsIconAct;
    TDEActionMenu          *itsViewMenuAct;
#ifdef HAVE_XFT
    KParts::ReadOnlyPart *itsPreview;
#endif
    TQSplitter            *itsSplitter;
    TDEConfig              itsConfig;
    bool                 itsEmbeddedAdmin;
    TQLabel               *itsStatusLabel;
};

}

#endif
