////////////////////////////////////////////////////////////////////////////////
//
// Namespace     : KFI::Misc
// Author        : Craig Drummond
// Project       : K Font Installer
// Creation Date : 01/05/2001
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
// (C) Craig Drummond, 2001, 2002, 2003, 2004
////////////////////////////////////////////////////////////////////////////////

#include "Misc.h"
#include <tqfile.h>
#include <tdeprocess.h> 
#include <tdestandarddirs.h>
#include <klargefile.h>
#include <tdeio/netaccess.h>
#include <unistd.h>

namespace KFI
{

namespace Misc
{

TQString linkedTo(const TQString &i)
{
    TQString d;

    if(isLink(i))
    {
        char buffer[1000];
        int  n=readlink(TQFile::encodeName(i), buffer, 1000);

        if(n!=-1)
        {
            buffer[n]='\0';
            d=buffer;
        }
    }

    return d;
}

TQString dirSyntax(const TQString &d)
{
    if(!d.isEmpty())
    {
        TQString ds(d);

        ds.replace("//", "/");

        int slashPos=ds.findRev('/');

        if(slashPos!=(((int)ds.length())-1))
            ds.append('/');

        return ds;
    }

    return d;
}

TQString xDirSyntax(const TQString &d)
{
    if(!d.isEmpty())
    {
        TQString ds(d);

        ds.replace("//", "/");

        int slashPos=ds.findRev('/');
 
        if(slashPos==(((int)ds.length())-1))
            ds.remove(slashPos, 1);
        return ds;
    }

    return d;
}

TQString getDir(const TQString &f)
{
    TQString d(f);

    int slashPos=d.findRev('/');
 
    if(slashPos!=-1)
        d.remove(slashPos+1, d.length());

    return dirSyntax(d);
}

TQString getFile(const TQString &f)
{
    TQString d(f);

    int slashPos=d.findRev('/');
 
    if(slashPos!=-1)
        d.remove(0, slashPos+1);

    return d;
}

bool createDir(const TQString &dir)
{
    //
    // Clear any umask before dir is created
    mode_t oldMask=umask(0000);
    bool   status=TDEStandardDirs::makeDir(dir, DIR_PERMS);
    // Reset umask
    ::umask(oldMask);
    return status;
}

bool doCmd(const TQString &cmd, const TQString &p1, const TQString &p2, const TQString &p3)
{
    TDEProcess proc;

    proc << cmd;

    if(!p1.isEmpty())
        proc << p1;
    if(!p2.isEmpty())
        proc << p2;
    if(!p3.isEmpty())
        proc << p3;

    proc.start(TDEProcess::Block);

    return proc.normalExit() && proc.exitStatus()==0;
}

TQString changeExt(const TQString &f, const TQString &newExt)
{
    TQString newStr(f);
    int     dotPos=newStr.findRev('.');

    if(-1==dotPos)
        newStr+=TQChar('.')+newExt;
    else
    {
        newStr.remove(dotPos+1, newStr.length());
        newStr+=newExt;
    }
    return newStr;
}

void createBackup(const TQString &f)
{
    const TQString constExt(".bak");

    if(!fExists(f+constExt) && fExists(f))
        doCmd("cp", "-f", f, f+constExt);
}

//
// Get a list of files associated with a file, e.g.:
//
//    File: /home/a/courier.pfa
//
//    Associated: /home/a/courier.afm /home/a/courier.pfm
//
void getAssociatedUrls(const KURL &url, KURL::List &list, bool afmAndPfm, TQWidget *widget)
{
    const char *afm[]={"afm", "AFM", "Afm", "AFm", "AfM", "aFM", "aFm", "afM", NULL},
               *pfm[]={"pfm", "PFM", "Pfm", "PFm", "PfM", "pFM", "pFm", "pfM", NULL};
    bool       gotAfm=false,
               localFile=url.isLocalFile();
    int        e;

    for(e=0; afm[e]; ++e)
    {
        KURL statUrl(url);
        TDEIO::UDSEntry uds;

        statUrl.setPath(changeExt(url.path(), afm[e]));

        if(localFile ? fExists(statUrl.path()) : TDEIO::NetAccess::stat(statUrl, uds, widget))
        {
            list.append(statUrl);
            gotAfm=true;
            break;
        }
    }

    if(afmAndPfm || !gotAfm)
        for(e=0; pfm[e]; ++e)
        {
            KURL          statUrl(url);
            TDEIO::UDSEntry uds;

            statUrl.setPath(changeExt(url.path(), pfm[e]));
            if(localFile ? fExists(statUrl.path()) : TDEIO::NetAccess::stat(statUrl, uds, widget))
            {
                list.append(statUrl);
                break;
            }
        }
}

time_t getTimeStamp(const TQString &item)
{
    KDE_struct_stat info;

    return !item.isEmpty() && 0==KDE_lstat(TQFile::encodeName(item), &info) ? info.st_mtime : 0;
}


bool check(const TQString &path, unsigned int fmt, bool checkW)
{ 
    KDE_struct_stat info;
    TQCString        pathC(TQFile::encodeName(path));

    return 0==KDE_lstat(pathC, &info) && (info.st_mode&S_IFMT)==fmt && (!checkW || 0==::access(pathC, W_OK));
}

}

}
