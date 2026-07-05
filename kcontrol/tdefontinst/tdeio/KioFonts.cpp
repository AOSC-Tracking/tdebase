////////////////////////////////////////////////////////////////////////////////
//
// Class Name    : KFI::CKioFonts
// Author        : Craig Drummond
// Project       : K Font Installer
// Creation Date : 05/03/2003
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

/***************************************************************************

    NOTE: Large sections of this code are copied from tdeio_file
          -- can't just inherit from tdeio_file as tdeio_file uses "error(...);
             return;" So there is no way to know if an error occured!

 ***************************************************************************/

#include "KioFonts.h"
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <tdeio/global.h>
#include <tdeio/ioslave_defaults.h>
#include <tdeio/netaccess.h>
#include <tdeio/slaveinterface.h>
#include <tdeio/connection.h>
#include <tqtextstream.h>
#include <kmimetype.h>
#include <tdemessagebox.h>
#include <tdeprocess.h>
#include <tqdir.h>
#include <tqdatastream.h>
#include <tqregexp.h>
#include <tdeinstance.h>
#include <klargefile.h>
#include <tdetempfile.h>
#include <tdesu/su.h>
#include <tdeprocess.h>
#include <kdebug.h>
#include <ktar.h>
#include <kxftconfig.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>
#include "KfiConstants.h"
#include "FcEngine.h"
#include "Misc.h"
#include <X11/Xlib.h>
#include <fixx11h.h>
#include <ctype.h>

//#define KFI_FORCE_DEBUG_TO_STDERR

#ifdef KFI_FORCE_DEBUG_TO_STDERR

#include <tqtextstream.h>
TQTextOStream ostr(stderr);
#define KFI_DBUG ostr << "[" << (int)(getpid()) << "] "

#else

#define KFI_DBUG kdDebug() << "[" << (int)(getpid()) << "] "

#endif

#define MAX_IPC_SIZE   (1024*32)
#define TIMEOUT        2         // Time between last mod and writing files...
#define MAX_NEW_FONTS  50        // #fonts that can be installed before automatically configuring (related to above)
#define FC_CACHE_CMD  "fc-cache"

static const char * constMultipleExtension=".fonts.tar.gz";  // Fonts that have multiple files are returned as a .tar.gz!
static const int    constMaxLastDestTime=5;
static const int    constMaxFcCheckTime=10;

extern "C"
{
    TDE_EXPORT int kdemain(int argc, char **argv);
}

int kdemain(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: tdeio_" KFI_TDEIO_FONTS_PROTOCOL " protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    TDELocale::setMainCatalogue(KFI_CATALOGUE);

    TDEInstance instance("tdeio_" KFI_TDEIO_FONTS_PROTOCOL);
    KFI::CKioFonts slave(argv[2], argv[3]);

    slave.dispatchLoop();

    return 0;
}

namespace KFI
{

inline bool isSysFolder(const TQString &sect)
{
    return i18n(KFI_TDEIO_FONTS_SYS)==sect || KFI_TDEIO_FONTS_SYS==sect;
}

inline bool isUserFolder(const TQString &sect)
{
    return i18n(KFI_TDEIO_FONTS_USER)==sect || KFI_TDEIO_FONTS_USER==sect;
}

static TQString removeMultipleExtension(const KURL &url)
{
    TQString fname(url.fileName());
    int     pos;

    if(-1!=(pos=fname.findRev(TQString::fromLatin1(constMultipleExtension))))
        fname=fname.left(pos);

    return fname;
}

static TQString modifyName(const TQString &fname)
{
    static const char constSymbols[]={ '-', ' ', ':', 0 };

    TQString rv(fname);
    int     dotPos=rv.findRev('.');

    if(-1!=dotPos)
    {
        unsigned int rvLen=rv.length();

        for(unsigned int i=dotPos+1; i<rvLen; ++i)
            rv[i]=rv[i].lower();
    }

    for(int s=0; constSymbols[s]; ++s)
        rv=rv.replace(constSymbols[s], '_');

    return rv;
}

static int getSize(const TQCString &file)
{
    KDE_struct_stat buff;

    if(-1!=KDE_lstat(file, &buff))
    {
        if (S_ISLNK(buff.st_mode))
        {
            char buffer2[1000];
            int n=readlink(file, buffer2, 1000);
            if(n!= -1)
                buffer2[n]='\0';

            if(-1==KDE_stat(file, &buff))
                return -1;
        }
        return buff.st_size;
    }

    return -1;
}

static int getFontSize(const TQString &file)
{
    int size=0;

    KURL::List  urls;
    TQStringList files;

    Misc::getAssociatedUrls(KURL(file), urls);

    files.append(file);

    if(urls.count())
    {
        KURL::List::Iterator uIt,
                             uEnd=urls.end();

        for(uIt=urls.begin(); uIt!=uEnd; ++uIt)
            files.append((*uIt).path());
    }

    TQStringList::Iterator it(files.begin()),
                          end(files.end());

    for(; it!=end; ++it)
    {
        int s=getSize(TQFile::encodeName(*it));

        if(s>-1)
            size+=s;
    }

    return size;
}

static int getSize(TQValueList<FcPattern *> &patterns)
{
    TQValueList<FcPattern *>::Iterator it,
                                      end=patterns.end();
    int                               size=0;

    for(it=patterns.begin(); it!=end; ++it)
        size+=getFontSize(CFcEngine::getFcString(*it, FC_FILE));

    return size;
}

static void addAtom(TDEIO::UDSEntry &entry, unsigned int ID, long l, const TQString &s=TQString::null)
{
    TDEIO::UDSAtom atom;
    atom.m_uds = ID;
    atom.m_long = l;
    atom.m_str = s;
    entry.append(atom);
}

static bool createFolderUDSEntry(TDEIO::UDSEntry &entry, const TQString &name, const TQString &path, bool sys)
{
    KFI_DBUG << "createFolderUDSEntry " << name << ' ' << path << ' ' << sys << ' ' << endl;

    KDE_struct_stat buff;
    TQCString        cPath(TQFile::encodeName(path));

    entry.clear();

    if(-1!=KDE_lstat(cPath, &buff))
    {
        addAtom(entry, TDEIO::UDS_NAME, 0, name);

        if (S_ISLNK(buff.st_mode))
        {
            KFI_DBUG << path << " is a link" << endl;

            char buffer2[1000];
            int n=readlink(cPath, buffer2, 1000);
            if(n!= -1)
                buffer2[n]='\0';

            addAtom(entry, TDEIO::UDS_LINK_DEST, 0, TQString::fromLocal8Bit(buffer2));

            if(-1==KDE_stat(cPath, &buff))
            {
                // It is a link pointing to nowhere
                addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFMT - 1);
                addAtom(entry, TDEIO::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO);
                addAtom(entry, TDEIO::UDS_SIZE, 0);
                goto notype;
            }
        }

        addAtom(entry, TDEIO::UDS_FILE_TYPE, buff.st_mode&S_IFMT);
        addAtom(entry, TDEIO::UDS_ACCESS, buff.st_mode&07777);
        addAtom(entry, TDEIO::UDS_SIZE, buff.st_size);

        notype:
        addAtom(entry, TDEIO::UDS_MODIFICATION_TIME, buff.st_mtime);

        struct passwd *user = getpwuid(buff.st_uid);
        addAtom(entry, TDEIO::UDS_USER, 0, user ? user->pw_name : TQString::number(buff.st_uid).latin1());

        struct group *grp = getgrgid(buff.st_gid);
        addAtom(entry, TDEIO::UDS_GROUP, 0, grp ? grp->gr_name : TQString::number(buff.st_gid).latin1());

        addAtom(entry, TDEIO::UDS_ACCESS_TIME, buff.st_atime);
        addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, sys
                                                ? KFI_TDEIO_FONTS_PROTOCOL"/system-folder" 
                                                : KFI_TDEIO_FONTS_PROTOCOL"/folder");
        addAtom(entry, TDEIO::UDS_GUESSED_MIME_TYPE, 0, "application/octet-stream");
        TQString url(KFI_TDEIO_FONTS_PROTOCOL+TQString::fromLatin1(":/"));
        return true;
    }
    else if (sys && !Misc::root())   // Default system fonts folder does not actually exist yet!
    {
        KFI_DBUG << "Default system folder (" << path << ") does not yet exist, so create dummy entry" << endl;
        addAtom(entry, TDEIO::UDS_NAME, 0, name);
        addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFDIR);
        addAtom(entry, TDEIO::UDS_ACCESS, 0744);
        addAtom(entry, TDEIO::UDS_USER, 0, "root");
        addAtom(entry, TDEIO::UDS_GROUP, 0, "root");
        addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, KFI_TDEIO_FONTS_PROTOCOL"/system-folder");
        addAtom(entry, TDEIO::UDS_GUESSED_MIME_TYPE, 0, "application/octet-stream");

        return true;
    }


    return false;
}

static bool createFontUDSEntry(TDEIO::UDSEntry &entry, const TQString &name, TQValueList<FcPattern *> &patterns, bool sys)
{
    KFI_DBUG << "createFontUDSEntry " << name << ' ' << patterns.count() << endl;

    bool multiple=true;

    if(1==patterns.count()) // Only one font file, but are there any .pfm or .afm files?
    {
        KURL::List urls;

        Misc::getAssociatedUrls(KURL(CFcEngine::getFcString(patterns.first(), FC_FILE)), urls);

        if(0==urls.count())
            multiple=false;
    }

    //
    // In case of mixed bitmap/scalable - prefer scalable
    TQValueList<FcPattern *>           sortedPatterns;
    TQValueList<FcPattern *>::Iterator it,
                                      end(patterns.end());
    FcBool                            b=FcFalse;

    for(it=patterns.begin(); it!=end; ++it)
        if(FcResultMatch==FcPatternGetBool(*it, FC_SCALABLE, 0, &b) && b)
            sortedPatterns.prepend(*it);
        else
            sortedPatterns.append(*it);

    end=sortedPatterns.end();
    entry.clear();
    addAtom(entry, TDEIO::UDS_SIZE, getSize(patterns));

    for(it=sortedPatterns.begin(); it!=end; ++it)
    {
        TQString         path(CFcEngine::getFcString(*it, FC_FILE));
        TQCString        cPath(TQFile::encodeName(path));
        KDE_struct_stat buff;

        if(-1!=KDE_lstat(cPath, &buff))
        {
            addAtom(entry, TDEIO::UDS_NAME, 0, name);

            if (S_ISLNK(buff.st_mode))
            {
                KFI_DBUG << path << " is a link" << endl;

                char buffer2[1000];
                int  n=readlink(cPath, buffer2, 1000);

                if(n!= -1)
                    buffer2[n]='\0';

                addAtom(entry, TDEIO::UDS_LINK_DEST, 0, TQString::fromLocal8Bit(buffer2));

                if(-1==KDE_stat(cPath, &buff))
                {
                    // It is a link pointing to nowhere
                    addAtom(entry, TDEIO::UDS_FILE_TYPE, S_IFMT - 1);
                    addAtom(entry, TDEIO::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO);
                    goto notype;
                }
            }

            addAtom(entry, TDEIO::UDS_FILE_TYPE, buff.st_mode&S_IFMT);
            addAtom(entry, TDEIO::UDS_ACCESS, buff.st_mode&07777);

            notype:
            addAtom(entry, TDEIO::UDS_MODIFICATION_TIME, buff.st_mtime);

            struct passwd *user = getpwuid(buff.st_uid);
            addAtom(entry, TDEIO::UDS_USER, 0, user ? user->pw_name : TQString::number(buff.st_uid).latin1());

            struct group *grp = getgrgid(buff.st_gid);
            addAtom(entry, TDEIO::UDS_GROUP, 0, grp ? grp->gr_name : TQString::number(buff.st_gid).latin1());

            addAtom(entry, TDEIO::UDS_ACCESS_TIME, buff.st_atime);
            addAtom(entry, TDEIO::UDS_MIME_TYPE, 0, KMimeType::findByPath(path, 0, true)->name());
            addAtom(entry, TDEIO::UDS_GUESSED_MIME_TYPE, 0, "application/octet-stream");

            TQString url(KFI_TDEIO_FONTS_PROTOCOL+TQString::fromLatin1(":/"));

            if(!Misc::root())
            {
                url+=sys ? i18n(KFI_TDEIO_FONTS_SYS) : i18n(KFI_TDEIO_FONTS_USER);
                url+=TQString::fromLatin1("/");
            }
            if(multiple)
                url+=name+TQString::fromLatin1(constMultipleExtension);
            else
                url+=Misc::getFile(path);
            addAtom(entry, TDEIO::UDS_URL, 0, url);
            return true;  // This file was OK, so use its values...
        }
    }
    return false;
}

enum EUrlStatus
{
    BAD_URL,
    URL_OK,
    REDIRECT_URL
};

static KURL getRedirect(const KURL &u)
{
    // Go from fonts:/System to fonts:/

    KURL    redirect(u);
    TQString path(u.path()),
            sect(CKioFonts::getSect(path));

    path.remove(sect);
    path.replace("//", "/");
    redirect.setPath(path);

    KFI_DBUG << "Redirect from " << u.path() << " to " << redirect.path() << endl;
    return redirect;
}

static bool nonRootSys(const KURL &u)
{
    return !Misc::root() && isSysFolder(CKioFonts::getSect(u.path()));
}

static TQString getFontFolder(const TQString &defaultDir, const TQString &root, TQStringList &dirs)
{
    if(dirs.contains(defaultDir))
        return defaultDir;
    else
    {
        TQStringList::Iterator it,
                              end=dirs.end();
        bool                  found=false;

        for(it=dirs.begin(); it!=end && !found; ++it)
            if(0==(*it).find(root))
                return *it;
    }

    return TQString::null;
}

static bool writeAll(int fd, const char *buf, size_t len)
{
   while(len>0)
   {
      ssize_t written=write(fd, buf, len);
      if (written<0 && EINTR!=errno)
          return false;
      buf+=written;
      len-=written;
   }
   return true;
}

static bool checkExt(const char *fname, const char *ext)
{
    unsigned int len=strlen(fname);

    return len>4 ? (fname[len-4]=='.' && tolower(fname[len-3])==ext[0] && tolower(fname[len-2])==ext[1] &&
                    tolower(fname[len-1])==ext[2])
                 : false;
}

static bool isAAfm(const TQString &fname)
{
    if(checkExt(TQFile::encodeName(fname), "afm"))   // CPD? Is this a necessary check?
    {
        TQFile file(fname);

        if(file.open(IO_ReadOnly))
        {
            TQTextStream stream(&file);
            TQString     line;

            for(int lc=0; lc<30 && !stream.atEnd(); ++lc)
            {
                line=stream.readLine();

                if(line.contains("StartFontMetrics"))
                {
                    file.close();
                    return true;
                }
            }

            file.close();
        }
    }

    return false;
}

static bool isAPfm(const TQString &fname)
{
    bool ok=false;

    // I know extension checking is bad, but Ghostscript's pf2afm requires the pfm file to
    // have the .pfm extension...
    if(checkExt(TQFile::encodeName(fname), "pfm"))
    {
        //
        // OK, the extension matches, so perform a little contents checking...
        FILE *f=fopen(TQFile::encodeName(fname).data(), "r");

        if(f)
        {
            static const unsigned long constCopyrightLen =  60;
            static const unsigned long constTypeToExt    =  49;
            static const unsigned long constExtToFname   =  20;
            static const unsigned long constExtLen       =  30;
            static const unsigned long constFontnameMin  =  75;
            static const unsigned long constFontnameMax  = 512;

            unsigned short version=0,
                           type=0,
                           extlen=0;
            unsigned long  length=0,
                           fontname=0,
                           fLength=0;

            fseek(f, 0, SEEK_END);
            fLength=ftell(f);
            fseek(f, 0, SEEK_SET);

            if(2==fread(&version, 1, 2, f) && // Read version
               4==fread(&length, 1, 4, f) &&  // length...
               length==fLength &&
               0==fseek(f, constCopyrightLen, SEEK_CUR) &&   // Skip copyright notice...
               2==fread(&type, 1, 2, f) &&
               0==fseek(f, constTypeToExt, SEEK_CUR) &&
               2==fread(&extlen, 1, 2, f) &&
               extlen==constExtLen &&
               0==fseek(f, constExtToFname, SEEK_CUR) &&
               4==fread(&fontname, 1, 4, f) &&
               fontname>constFontnameMin && fontname<constFontnameMax)
                ok=true;
            fclose(f);
        }
    }

    return ok;
}

//
// This function is *only* used for the generation of AFMs from PFMs.
static bool isAType1(const TQString &fname)
{
    static const char *       constStr="%!PS-AdobeFont-";
    static const unsigned int constStrLen=15;
    static const unsigned int constPfbOffset=6;
    static const unsigned int constPfbLen=constStrLen+constPfbOffset;

    TQCString name(TQFile::encodeName(fname));
    char     buffer[constPfbLen];
    bool     match=false;

    if(checkExt(name, "pfa"))
    {
        FILE *f=fopen(name.data(), "r");

        if(f)
        {
            if(constStrLen==fread(buffer, 1, constStrLen, f))
                match=0==memcmp(buffer, constStr, constStrLen);
            fclose(f);
        }
    }
    else if(checkExt(name, "pfb"))
    {
        static const char constPfbMarker=0x80;

        FILE *f=fopen(name.data(), "r");

        if(f)
        {
            if(constPfbLen==fread(buffer, 1, constPfbLen, f))
                match=buffer[0]==constPfbMarker && 0==memcmp(&buffer[constPfbOffset], constStr, constStrLen);
            fclose(f);
        }
    }

    return match;
}

static TQString getMatch(const TQString &file, const char *extension)
{
    TQString f(Misc::changeExt(file, extension));

    return Misc::fExists(f) ? f : TQString::null;
}

inline bool isHidden(const KURL &u)
{
    return TQChar('.')==u.fileName()[0];
}

struct FontList
{
    struct Path
    {
        Path(const TQString &p=TQString::null) : orig(p) { }

        TQString orig,
                modified;

        bool operator==(const Path &p) const { return p.orig==orig; }
    };

    FontList(const TQString &n=TQString::null, const TQString &p=TQString::null) : name(n) { if(!p.isEmpty()) paths.append(Path(p)); }

    TQString          name;
    TQValueList<Path> paths;

    bool operator==(const FontList &f) const { return f.name==name; }
};

//
// This function returns a set of maping of from -> to for copy/move operations
static bool getFontList(const TQStringList &files, TQMap<TQString, TQString> &map)
{
    //
    // First of all create a list of font files, and their paths
    TQStringList::ConstIterator it=files.begin(),
                               end=files.end();
    TQValueList<FontList>       list;

    for(;it!=end; ++it)
    {
        TQString                        name(Misc::getFile(*it)),
                                       path(Misc::getDir(*it));
        TQValueList<FontList>::Iterator entry=list.find(FontList(name));

        if(entry!=list.end())
        {
            if(!(*entry).paths.contains(path))
                (*entry).paths.append(path);
        }
        else
            list.append(FontList(name, path));
    }

    TQValueList<FontList>::Iterator fIt(list.begin()),
                                   fEnd(list.end());

    for(; fIt!=fEnd; ++fIt)
    {
        TQValueList<FontList::Path>::Iterator pBegin((*fIt).paths.begin()),
                                             pIt(++pBegin),
                                             pEnd((*fIt).paths.end());
        --pBegin;

        if((*fIt).paths.count()>1)
        {
            // There's more than 1 file with the same name, but in a different locations
            // therefore, take the unique part of the path, and replace / with _
            // e.g.
            //     /usr/X11R6/lib/X11/fonts/75dpi/times.pcf.gz
            //     /usr/X11R6/lib/X11/fonts/100dpi/times.pcf.gz
            //
            //   Will produce:
            //     75dpi_times.pcf.gz
            //     100dpi_times.pcf.gz
            unsigned int beginLen((*pBegin).orig.length());

            for(; pIt!=pEnd; ++pIt)
            {
                unsigned int len=TQMIN((*pIt).orig.length(), beginLen);

                for(unsigned int i=0; i<len; ++i)
                    if((*pIt).orig[i]!=(*pBegin).orig[i])
                    {
                        (*pIt).modified=(*pIt).orig.mid(i);
                        (*pIt).modified=(*pIt).modified.replace('/', '_');
                        if((*pBegin).modified.isEmpty())
                        {
                            (*pBegin).modified=(*pBegin).orig.mid(i);
                            (*pBegin).modified=(*pBegin).modified.replace('/', '_');
                        }
                        break;
                    }
            }
        }
        for(pIt=(*fIt).paths.begin(); pIt!=pEnd; ++pIt)
            map[(*pIt).orig+(*fIt).name]=(*pIt).modified+(*fIt).name;
    }

    return list.count() ? true : false;
}

CKioFonts::CKioFonts(const TQCString &pool, const TQCString &app)
         : TDEIO::SlaveBase(KFI_TDEIO_FONTS_PROTOCOL, pool, app),
           itsRoot(Misc::root()),
           itsUsingFcFpe(false),
           itsUsingXfsFpe(false),
           itsHasSys(false),
           itsAddToSysFc(false),
           itsFontChanges(0),
           itsLastDest(DEST_UNCHANGED),
           itsLastDestTime(0),
           itsLastFcCheckTime(0),
           itsFontList(NULL)
{
    KFI_DBUG << "Constructor" << endl;

    // Set core dump size to 0 because we will have
    // root's password in memory.
    struct rlimit rlim;
    rlim.rlim_cur=rlim.rlim_max=0;
    itsCanStorePasswd=setrlimit(RLIMIT_CORE, &rlim) ? false : true;

    //
    // Check with fontconfig for folder locations...
    //
    // 1. Get list of fontconfig dirs
    // 2. For user, look for any starting with $HOME - but prefer $HOME/.fonts
    // 3. For system, look for any starting with /usr/local/share - but prefer /usr/local/share/fonts
    // 4. If either are not found, then add to local.conf / .fonts.conf

    FcStrList   *list=FcConfigGetFontDirs(FcInitLoadConfigAndFonts());
    TQStringList dirs;
    FcChar8     *dir;

    while((dir=FcStrListNext(list)))
        dirs.append(Misc::dirSyntax((const char *)dir));

    EFolder mainFolder=FOLDER_SYS;

    if(!itsRoot)
    {
        TQString home(Misc::dirSyntax(TQDir::homeDirPath())),
                defaultDir(Misc::dirSyntax(TQDir::homeDirPath()+"/.fonts/")),
                dir(getFontFolder(defaultDir, home, dirs));

        if(dir.isEmpty())  // Then no $HOME/ was found in fontconfigs dirs!
        {
            KXftConfig xft(KXftConfig::Dirs, false);
            xft.addDir(defaultDir);
            xft.apply();
            dir=defaultDir;
        }
        mainFolder=FOLDER_USER;
        itsFolders[FOLDER_USER].location=dir;
    }

    TQString sysDefault("/usr/local/share/fonts/"),
            sysDir(getFontFolder(sysDefault, "/usr/local/share/", dirs));

    if(sysDir.isEmpty())
    {
        if(itsRoot)
        {
            KXftConfig xft(KXftConfig::Dirs, true);
            xft.addDir(sysDefault);
            xft.apply();
        }
        else
            itsAddToSysFc=true;

        sysDir=sysDefault;
    }

    itsFolders[FOLDER_SYS].location=sysDir;

    //
    // Ensure exists
    if(!Misc::dExists(itsFolders[mainFolder].location))
        Misc::createDir(itsFolders[mainFolder].location);

    //
    // Work out best params to send to tdefontinst

    // ...determine if X already knows about the system font path...
    Display *xDisplay=XOpenDisplay(NULL);

    if(xDisplay)
    {
        int  numPaths=0;
        char **paths=XGetFontPath(xDisplay, &numPaths);

        if(numPaths>0)
            for(int path=0; path<numPaths && !itsUsingFcFpe; ++path)
                if(paths[path][0]=='/')
                {
                    if(Misc::dirSyntax(paths[path])==itsFolders[FOLDER_SYS].location)
                        itsHasSys=true;
                }
                else
                {
                    TQString str(paths[path]);

                    str.replace(TQRegExp("\\s*"), "");

                    if(0==str.find("unix/:"))
                        itsUsingXfsFpe=true;
                    else if("fontconfig"==str)
                        itsUsingFcFpe=true;
                }
        XFreeFontPath(paths);
        XCloseDisplay(xDisplay);
    }
}

CKioFonts::~CKioFonts()
{
    KFI_DBUG << "Destructor" << endl;
    doModified();
}

void CKioFonts::listDir(const KURL &url)
{
    KFI_DBUG << "listDir " << url.path() << endl;

    if(updateFontList() && checkUrl(url, true))
    {
        TDEIO::UDSEntry entry;
        int           size=0;

        if(itsRoot || TQStringList::split('/', url.path(), false).count()!=0)
        {
            EFolder folder=getFolder(url);

            totalSize(itsFolders[folder].fontMap.count());
            if(itsFolders[folder].fontMap.count())
            {
                TQMap<TQString, TQValueList<FcPattern *> >::Iterator it=itsFolders[folder].fontMap.begin(),
                                                                  end=itsFolders[folder].fontMap.end();

                for ( ; it != end; ++it)
                {
                    entry.clear();
                    if(createFontUDSEntry(entry, it.key(), it.data(), FOLDER_SYS==folder))
                        listEntry(entry, false);
                }
            }
        }
        else
        {
            size=2;
            totalSize(size);
            createFolderUDSEntry(entry, i18n(KFI_TDEIO_FONTS_USER), itsFolders[FOLDER_USER].location, false);
            listEntry(entry, false);
            createFolderUDSEntry(entry, i18n(KFI_TDEIO_FONTS_SYS), itsFolders[FOLDER_SYS].location, true);
            listEntry(entry, false);
        }

        listEntry(size ? entry : TDEIO::UDSEntry(), true);
        finished();
    }

    KFI_DBUG << "listDir - finished!" << endl;
}

void CKioFonts::stat(const KURL &url)
{
    KFI_DBUG << "stat " << url.prettyURL() << endl;

    if(updateFontList() && checkUrl(url, true))
    {
        TQString path(url.path(-1));

        if(path.isEmpty())
        {
            error(TDEIO::ERR_COULD_NOT_STAT, url.prettyURL());
            return;
        }

        TQStringList   pathList(TQStringList::split('/', path, false));
        TDEIO::UDSEntry entry;
        bool          err=false;

        switch(pathList.count())
        {
            case 0:
                err=!createFolderUDSEntry(entry, i18n("Fonts"), itsFolders[itsRoot ? FOLDER_SYS : FOLDER_USER].location, false);
                break;
            case 1:
                if(itsRoot)
                    err=!createStatEntry(entry, url, FOLDER_SYS);
                else
                    if(isUserFolder(pathList[0]))
                        err=!createFolderUDSEntry(entry, i18n(KFI_TDEIO_FONTS_USER), itsFolders[FOLDER_USER].location, false);
                    else if(isSysFolder(pathList[0]))
                        err=!createFolderUDSEntry(entry, i18n(KFI_TDEIO_FONTS_SYS), itsFolders[FOLDER_USER].location, true);
                    else
                    {
                        error(TDEIO::ERR_SLAVE_DEFINED,
                              i18n("Please specify \"%1\" or \"%2\".").arg(i18n(KFI_TDEIO_FONTS_USER)).arg(i18n(KFI_TDEIO_FONTS_SYS)));
                        return;
                    }
                break;
            default:
                err=!createStatEntry(entry, url, getFolder(url));
        }

        if(err)
        {
            error(TDEIO::ERR_DOES_NOT_EXIST, url.prettyURL());
            return;
        }

        statEntry(entry);
        finished();
    }
}

bool CKioFonts::createStatEntry(TDEIO::UDSEntry &entry, const KURL &url, EFolder folder)
{
    KFI_DBUG << "createStatEntry " << url.path() << endl;

    TQMap<TQString, TQValueList<FcPattern *> >::Iterator it=getMap(url);

    if(it!=itsFolders[folder].fontMap.end())
        return createFontUDSEntry(entry, it.key(), it.data(), FOLDER_SYS==folder);
    return false;
}

void CKioFonts::get(const KURL &url)
{
    KFI_DBUG << "get " << url.path() << " query:" << url.query() << endl;

    bool        thumb="1"==metaData("thumbnail");
    TQStringList srcFiles;

    if(updateFontList() && checkUrl(url) && getSourceFiles(url, srcFiles))  // Any error will be logged in getSourceFiles
    {
        //
        // The thumbnail job always donwloads non-local files to /tmp/... and passes this file name to the thumbnail
        // creator. However, in the case of fonts which are split among many files, this wont work. Therefore, when the
        // thumbnail code asks for the font to donwload, just return the URL used. This way the font-thumbnail creator can
        // read this and just ask Xft/fontconfig for the font data.
        if(thumb)
        {
            TQByteArray   array;
            TQTextOStream stream(array);

            emit mimeType("text/plain");

            KFI_DBUG << "hasMetaData(\"thumbnail\"), so return: " << url.prettyURL() << endl;

            stream << url.prettyURL();
            totalSize(array.size());
            data(array);
            processedSize(array.size());
            data(TQByteArray());
            processedSize(array.size());
            finished();
            return;
        }

        TQString         realPath,
                        useMime;
        KDE_struct_stat buff;
        bool            multiple=false;

        if(1==srcFiles.count())
            realPath=srcFiles.first();
        else   // Font is made up of multiple files - so create .tar.gz of them all!
        {
            KTempFile tmpFile;
            KTar      tar(tmpFile.name(), "application/x-gzip");

            tmpFile.setAutoDelete(false);
            realPath=tmpFile.name();

            if(tar.open(IO_WriteOnly))
            {
                TQMap<TQString, TQString> map;

                getFontList(srcFiles, map);

                TQMap<TQString, TQString>::Iterator fIt(map.begin()),
                                                 fEnd(map.end());

                //
                // Iterate through created list, and add to tar archive
                for(; fIt!=fEnd; ++fIt)
                    tar.addLocalFile(fIt.key(), fIt.data());

                multiple=true;
                tar.close();
            }
        }

        TQCString realPathC(TQFile::encodeName(realPath));
        KFI_DBUG << "real: " << realPathC << endl;
    
        if (-2==KDE_stat(realPathC.data(), &buff))
            error(EACCES==errno ? TDEIO::ERR_ACCESS_DENIED : TDEIO::ERR_DOES_NOT_EXIST, url.prettyURL());
        else if (S_ISDIR(buff.st_mode))
            error(TDEIO::ERR_IS_DIRECTORY, url.prettyURL());
        else if (!S_ISREG(buff.st_mode))
            error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
        else
        {
            int fd = KDE_open(realPathC.data(), O_RDONLY);

            if (fd < 0)
                error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
            else
            {
                // Determine the mimetype of the file to be retrieved, and emit it.
                // This is mandatory in all slaves (for KRun/BrowserRun to work).
                emit mimeType(useMime.isEmpty() ? KMimeType::findByPath(realPathC, buff.st_mode, true)->name() : useMime);
    
                totalSize(buff.st_size);
    
                TDEIO::filesize_t processed=0;
                char            buffer[MAX_IPC_SIZE];
                TQByteArray      array;
    
                while(1)
                {
                    int n=::read(fd, buffer, MAX_IPC_SIZE);
                    if (-1==n)
                    {
			if (errno == EINTR)
				continue;
                        error(TDEIO::ERR_COULD_NOT_READ, url.prettyURL());
                        close(fd);
                        if(multiple)
                            ::unlink(realPathC);
                        return;
                    }
                    if (0==n)
                        break; // Finished
    
                    array.setRawData(buffer, n);
                    data(array);
                    array.resetRawData(buffer, n);
    
                    processed+=n;
                    processedSize(processed);
                }
    
                data(TQByteArray());
                close(fd);
    
                processedSize(buff.st_size);
                finished();
            }
        }
        if(multiple)
            ::unlink(realPathC);
    }
}

void CKioFonts::put(const KURL &u, int mode, bool overwrite, bool resume)
{
    KFI_DBUG << "put " << u.path() << endl;

    if(isHidden(u))
    {
        error(TDEIO::ERR_WRITE_ACCESS_DENIED, u.prettyURL());
        return;
    }

    // updateFontList(); // CPD: dont update font list upon a put - is too slow. Just stat on filename!

    //checkUrl(u) // CPD: Don't need to check URL, as the call to "confirmUrl()" below will sort out any probs!

    KURL            url(u);
    bool            changed=confirmUrl(url),
                    nrs=nonRootSys(url);
    EFolder         destFolder(getFolder(url));
    TQString         dest=itsFolders[destFolder].location+modifyName(url.fileName()),
                    passwd;
    TQCString        destC=TQFile::encodeName(dest);
    KDE_struct_stat buffDest;
    bool            destExists=(KDE_lstat(destC.data(), &buffDest)!= -1);

    if (destExists && !overwrite && !resume)
    {
        error(TDEIO::ERR_FILE_ALREADY_EXIST, url.prettyURL());
        return;
    }

    if(nrs) // Need to check can get root passwd before start download...
    { 
        passwd=getRootPasswd(); 
 
        if(passwd.isEmpty())
        {
            error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\" folder.").arg(i18n(KFI_TDEIO_FONTS_SYS)));
            return;
        }
    }

    //
    // As we don't get passed a mime-type the following needs to happen:
    //
    //    1. Download to a temporary file
    //    2. Check with FreeType that the file is a font, or that it is
    //       an AFM or PFM file
    //    3. If its OK, then get the fonts "name" from 
    KTempFile tmpFile;
    TQCString  tmpFileC(TQFile::encodeName(tmpFile.name()));

    tmpFile.setAutoDelete(true);

    if(putReal(tmpFile.name(), tmpFileC, destExists, mode, resume))
    {
        if(!checkFile(tmpFile.name()))  // error logged in checkFile
            return;
   
        if(nrs)  // Ask root to copy the font...
        {
            TQCString cmd;

            if(!Misc::dExists(itsFolders[destFolder].location))
            {
                cmd+="mkdir ";
                cmd+=TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location));
                cmd+=" && chmod 0755 ";
                cmd+=TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location));
                cmd+=" && ";
            }
            cmd+="cp -f ";
            cmd+=TQFile::encodeName(TDEProcess::quote(tmpFileC));
            cmd+=" ";
            cmd+=TQFile::encodeName(TDEProcess::quote(destC));
            cmd+=" && chmod 0644 ";
            cmd+=destC;

            if(!itsCanStorePasswd)
                createRootRefreshCmd(cmd);

            // Get root to move this to fonts folder...
            if(doRootCmd(cmd, passwd))
            {
                modified(FOLDER_SYS);
                createAfm(dest, true, passwd);
            }
            else
            {
                error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\" folder.").arg(i18n(KFI_TDEIO_FONTS_SYS)));
                return;
            }
        }
        else // Move it to our font folder...
        {
            tmpFile.setAutoDelete(false);
            if(Misc::doCmd("mv", "-f", tmpFileC, destC))
            {
                ::chmod(destC.data(), Misc::FILE_PERMS);
                modified(FOLDER_USER);
                createAfm(dest);
            }
            else
            {
                error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\" folder.").arg(i18n(KFI_TDEIO_FONTS_USER)));
                return;
            }
        }

        finished();

        if(changed)
            itsLastDestTime=time(NULL);
    }
}

bool CKioFonts::putReal(const TQString &destOrig, const TQCString &destOrigC, bool origExists,
                        int mode, bool resume)
{
    bool    markPartial=config()->readBoolEntry("MarkPartial", true);
    TQString dest;

    if (markPartial)
    {
        TQString  destPart(destOrig+TQString::fromLatin1(".part"));
        TQCString destPartC(TQFile::encodeName(destPart));

        dest = destPart;

        KDE_struct_stat buffPart;
        bool            partExists=(-1!=KDE_stat(destPartC.data(), &buffPart));

        if (partExists && !resume && buffPart.st_size>0)
        {
             // Maybe we can use this partial file for resuming
             // Tell about the size we have, and the app will tell us
             // if it's ok to resume or not.
             resume=canResume(buffPart.st_size);

             if (!resume)
                 if (!::remove(destPartC.data()))
                     partExists = false;
                 else
                 {
                     error(TDEIO::ERR_CANNOT_DELETE_PARTIAL, destPart);
                     return false;
                 }
        }
    }
    else
    {
        dest = destOrig;
        if (origExists && !resume)
            ::remove(destOrigC.data());
            // Catch errors when we try to open the file.
    }

    TQCString destC(TQFile::encodeName(dest));

    int fd;

    if (resume)
    {
        fd = KDE_open(destC.data(), O_RDWR);  // append if resuming
        KDE_lseek(fd, 0, SEEK_END); // Seek to end
    }
    else
    {
        // WABA: Make sure that we keep writing permissions ourselves,
        // otherwise we can be in for a surprise on NFS.
        fd = KDE_open(destC.data(), O_CREAT | O_TRUNC | O_WRONLY, -1==mode ? 0666 :  mode | S_IWUSR | S_IRUSR);
    }

    if (fd < 0)
    {
        error(EACCES==errno ? TDEIO::ERR_WRITE_ACCESS_DENIED : TDEIO::ERR_CANNOT_OPEN_FOR_WRITING, dest);
        return false;
    }

    int result;
    // Loop until we got 0 (end of data)
    do
    {
        TQByteArray buffer;

        dataReq(); // Request for data
        result = readData(buffer);
        if(result > 0 && !writeAll(fd, buffer.data(), buffer.size()))
        {
            if(ENOSPC==errno) // disk full
            {
                error(TDEIO::ERR_DISK_FULL, destOrig);
                result = -2; // means: remove dest file
            }
            else
            {
                error(TDEIO::ERR_COULD_NOT_WRITE, destOrig);
                result = -1;
            }
        }
    }
    while(result>0);

    if (result<0)
    {
        close(fd);
        if (-1==result)
           ::remove(destC.data());
        else if (markPartial)
        {
           KDE_struct_stat buff;

           if ((-1==KDE_stat(destC.data(), &buff)) || 
               (buff.st_size<config()->readNumEntry("MinimumKeepSize", DEFAULT_MINIMUM_KEEP_SIZE)))
               ::remove(destC.data());
        }
        ::exit(255);
    }

    if (-1==fd) // we got nothing to write out, so we never opened the file
    {
        finished();
        return false;
    }

    if (close(fd))
    {
        error(TDEIO::ERR_COULD_NOT_WRITE, destOrig);
        return false;
    }

    // after full download rename the file back to original name
    if (markPartial && ::rename(destC.data(), destOrigC.data()))
    {
        error(TDEIO::ERR_CANNOT_RENAME_PARTIAL, destOrig);
        return false;
    }

    return true;
}

void CKioFonts::copy(const KURL &src, const KURL &d, int mode, bool overwrite)
{
    //
    // Support:
    //    Copying to fonts:/
    //    Copying from fonts:/ and file:/
    //
    KFI_DBUG << "copy " << src.prettyURL() << " - " << d.prettyURL() << endl;

    if(isHidden(d))
    {
        error(TDEIO::ERR_WRITE_ACCESS_DENIED, d.prettyURL());
        return;
    }

    bool fromFonts=KFI_TDEIO_FONTS_PROTOCOL==src.protocol();

    if((!fromFonts || updateFontList()) // CPD: dont update font list upon a copy from file - is too slow. Just stat on filename!
        &&  checkUrl(src) && checkAllowed(src))
    {
        //checkUrl(u) // CPD as per comment in ::put()

        TQStringList srcFiles;

        if(getSourceFiles(src, srcFiles))  // Any error will be logged in getSourceFiles
        {
            KURL                   dest(d);
            bool                   changed=confirmUrl(dest);
            EFolder                destFolder(getFolder(dest));
            TQMap<TQString, TQString> map;

            if(!fromFonts)
                map[src.path()]=src.fileName();

            // As above, if copying from file, then only stat on dest filename, but if from fonts to fonts need to
            // get the list of possible source files, etc.
            if(fromFonts ? confirmMultiple(src, srcFiles, FOLDER_SYS==destFolder ? FOLDER_USER : FOLDER_SYS, OP_COPY) &&
                           getFontList(srcFiles, map) &&
                           checkDestFiles(src, map, dest, destFolder, overwrite)
                         : checkDestFile(src, dest, destFolder, overwrite) )
            {
                if(nonRootSys(dest))
                {
                    TQCString cmd;
                    int      size=0;

                    if(!Misc::dExists(itsFolders[destFolder].location))
                    {
                        cmd+="mkdir ";
                        cmd+=TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location));
                        cmd+=" && chmod 0755 ";
                        cmd+=TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location));
                        cmd+=" && ";
                    }

                    TQMap<TQString, TQString>::Iterator fIt(map.begin()),
                                                     fEnd(map.end());

                    for(; fIt!=fEnd; ++fIt)
                    {
                        cmd+="cp -f ";
                        cmd+=TQFile::encodeName(TDEProcess::quote(fIt.key()));
                        cmd+=" ";
                        cmd+=TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location+modifyName(fIt.data())));
                        int s=getSize(TQFile::encodeName(fIt.key()));
                        if(s>0)
                            size+=s;
                        if(++fIt!=fEnd)
                            cmd+=" && ";
                        --fIt;
                    }

                    if(!itsCanStorePasswd)
                        createRootRefreshCmd(cmd);
    
                    totalSize(size);
  
                    TQString passwd=getRootPasswd();

                    if(doRootCmd(cmd, passwd))
                    {
                        modified(destFolder);
                        processedSize(size);
                        if(src.isLocalFile() && 1==srcFiles.count())
                            createAfm(itsFolders[destFolder].location+modifyName(map.begin().data()), true, passwd);
                    }
                    else
                    {
                        error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\" folder.").arg(i18n(KFI_TDEIO_FONTS_SYS)));
                        return;
                    }
                }
                else
                {
                    TQMap<TQString, TQString>::Iterator fIt(map.begin()),
                                                     fEnd(map.end());

                    for(; fIt!=fEnd; ++fIt)
                    {
                        TQCString        realSrc(TQFile::encodeName(fIt.key())),
                                        realDest(TQFile::encodeName(itsFolders[destFolder].location+modifyName(fIt.data())));
                        KDE_struct_stat buffSrc;

                        if(-1==KDE_stat(realSrc.data(), &buffSrc))
                        {
                            error(EACCES==errno ? TDEIO::ERR_ACCESS_DENIED : TDEIO::ERR_DOES_NOT_EXIST, src.prettyURL());
                            return;
                        }

                        int srcFd=KDE_open(realSrc.data(), O_RDONLY);
    
                        if (srcFd<0)
                        {
                            error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, src.prettyURL());
                            return;
                        }
    
                        if(!Misc::dExists(itsFolders[destFolder].location))
                            Misc::createDir(itsFolders[destFolder].location);
    
                        // WABA: Make sure that we keep writing permissions ourselves,
                        // otherwise we can be in for a surprise on NFS.
                        int destFd=KDE_open(realDest.data(), O_CREAT | O_TRUNC | O_WRONLY, -1==mode ? 0666 : mode | S_IWUSR);
    
                        if (destFd<0)
                        {
                            error(EACCES==errno ? TDEIO::ERR_WRITE_ACCESS_DENIED : TDEIO::ERR_CANNOT_OPEN_FOR_WRITING, dest.prettyURL());
                            close(srcFd);
                            return;
                        }
    
                        totalSize(buffSrc.st_size);

                        TDEIO::filesize_t processed = 0;
                        char            buffer[MAX_IPC_SIZE];
                        TQByteArray      array;
    
                        while(1)
                        {
                            int n=::read(srcFd, buffer, MAX_IPC_SIZE);
    
                            if(-1==n)
                            {
				if (errno == EINTR)
					continue;
                                error(TDEIO::ERR_COULD_NOT_READ, src.prettyURL());
                                close(srcFd);
                                close(destFd);
                                return;
                            }
                            if(0==n)
                                break; // Finished
    
                           if(!writeAll(destFd, buffer, n))
                           {
                                close(srcFd);
                                close(destFd);
                                if (ENOSPC==errno) // disk full
                                {
                                    error(TDEIO::ERR_DISK_FULL, dest.prettyURL());
                                    remove(realDest.data());
                                }
                                else
                                    error(TDEIO::ERR_COULD_NOT_WRITE, dest.prettyURL());
                                return;
                            }
    
                            processed += n;
                            processedSize(processed);
                        }
    
                        close(srcFd);
    
                        if(close(destFd))
                        {
                            error(TDEIO::ERR_COULD_NOT_WRITE, dest.prettyURL());
                            return;
                        }
    
                        ::chmod(realDest.data(), Misc::FILE_PERMS);
    
                        // copy access and modification time
                        struct utimbuf ut;
    
                        ut.actime = buffSrc.st_atime;
                        ut.modtime = buffSrc.st_mtime;
                        ::utime(realDest.data(), &ut);
    
                        processedSize(buffSrc.st_size);
                        modified(destFolder);
                    }

                    if(src.isLocalFile() && 1==srcFiles.count())
                        createAfm(itsFolders[destFolder].location+modifyName(map.begin().data()));
                }
    
                finished();
    
                if(changed)
                    itsLastDestTime=time(NULL);
            }
        }
    }
}

void CKioFonts::rename(const KURL &src, const KURL &d, bool overwrite)
{
    KFI_DBUG << "rename " << src.prettyURL() << " - " << d.prettyURL() << ", " << overwrite << endl;

    if(src.directory()==d.directory())
        error(TDEIO::ERR_SLAVE_DEFINED, i18n("Sorry, fonts cannot be renamed."));
    else if(itsRoot) // Should never happen...
        error(TDEIO::ERR_UNSUPPORTED_ACTION, unsupportedActionErrorString(mProtocol, TDEIO::CMD_RENAME));
    else
    {
        //
        // Can't rename from/to file:/ -> therefore rename can only be from fonts:/System to fonts:/Personal,
        // or vice versa.

        TQStringList srcFiles;

        if(getSourceFiles(src, srcFiles))   // Any error will be logged in getSourceFiles
        {
            KURL                   dest(d);
            bool                   changed=confirmUrl(dest);
            EFolder                destFolder(getFolder(dest));
            TQMap<TQString, TQString> map;

            if(confirmMultiple(src, srcFiles, FOLDER_SYS==destFolder ? FOLDER_USER : FOLDER_SYS, OP_MOVE) &&
               getFontList(srcFiles, map) &&
               checkDestFiles(src, map, dest, destFolder, overwrite))
            {
                TQMap<TQString, TQString>::Iterator fIt(map.begin()),
                                                 fEnd(map.end());
                bool                             askPasswd=true,
                                                 toSys=FOLDER_SYS==destFolder;
                TQCString                         userId,
                                                 groupId,
                                                 destDir(TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location)));

                userId.setNum(toSys ? 0 : getuid());
                groupId.setNum(toSys ? 0 : getgid());

                for(; fIt!=fEnd; ++fIt)
                {
                    TQCString cmd,
                             destFile(TQFile::encodeName(TDEProcess::quote(itsFolders[destFolder].location+fIt.data())));

                    if(toSys && !Misc::dExists(itsFolders[destFolder].location))
                    {
                        cmd+="mkdir ";
                        cmd+=destDir;
                        cmd+=" && ";
                    }

                    cmd+="mv -f ";
                    cmd+=TQFile::encodeName(TDEProcess::quote(fIt.key()));
                    cmd+=" ";
                    cmd+=destFile;
                    cmd+=" && chmod -f 0644 ";
                    cmd+=destFile;
                    cmd+=" && chown -f ";
                    cmd+=userId;
                    cmd+=":";
                    cmd+=groupId;
                    cmd+=" ";
                    cmd+=destFile;

                    TQString sysDir,
                            userDir;

                    if(FOLDER_SYS==destFolder)
                    {
                        sysDir=itsFolders[destFolder].location;
                        userDir=Misc::getDir(fIt.key());
                    }
                    else
                    {
                        userDir=itsFolders[destFolder].location;
                        sysDir=Misc::getDir(fIt.key());
                    }

                    if(!itsCanStorePasswd)
                        createRootRefreshCmd(cmd, sysDir);

                    if(doRootCmd(cmd, askPasswd))
                    {
                        modified(FOLDER_SYS, true, sysDir);
                        modified(FOLDER_USER, true, userDir);
                        askPasswd=false;  // Don't keep on asking for password...
                    }
                    else
                    {
                        error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\" folder.").arg(i18n(KFI_TDEIO_FONTS_SYS)));
                        return;
                    }
                }
                if(changed)
                    itsLastDestTime=time(NULL);
            }
        }
    }
}

void CKioFonts::del(const KURL &url, bool)
{
    KFI_DBUG << "del " << url.path() << endl;

    TQValueList<FcPattern *> *entries;

    if(checkUrl(url) && checkAllowed(url) && 
       updateFontList() && (entries=getEntries(url)) && entries->count() &&
       confirmMultiple(url, entries, getFolder(url), OP_DELETE))
    {
        TQValueList<FcPattern *>::Iterator it,
                                          end=entries->end();
        CDirList                          modifiedDirs;
        bool                              clearList=KFI_TDEIO_NO_CLEAR!=url.query();

        if(nonRootSys(url))
        {
            TQCString cmd("rm -f");
 
            for(it=entries->begin(); it!=end; ++it)
            {
                TQString file(CFcEngine::getFcString(*it, FC_FILE));

                modifiedDirs.add(Misc::getDir(file));
                cmd+=" ";
                cmd+=TQFile::encodeName(TDEProcess::quote(file));

                KURL::List urls;

                Misc::getAssociatedUrls(KURL(file), urls);

                if(urls.count())
                {
                    KURL::List::Iterator uIt,
                                         uEnd=urls.end();

                    for(uIt=urls.begin(); uIt!=uEnd; ++uIt)
                    {
                        cmd+=" ";
                        cmd+=TQFile::encodeName(TDEProcess::quote((*uIt).path()));
                    }
                }
            }
    
            if(!itsCanStorePasswd)
                createRootRefreshCmd(cmd, modifiedDirs);
    
            if(doRootCmd(cmd))
                modified(FOLDER_SYS, clearList, modifiedDirs);
            else
                error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\" folder.").arg(i18n(KFI_TDEIO_FONTS_SYS)));
        }
        else
        {
            for(it=entries->begin(); it!=end; ++it)
            {
                TQString file(CFcEngine::getFcString(*it, FC_FILE));

                if (0!=unlink(TQFile::encodeName(file).data()))
                    error(EACCES==errno || EPERM==errno
                            ? TDEIO::ERR_ACCESS_DENIED
                            : EISDIR==errno
                                    ? TDEIO::ERR_IS_DIRECTORY
                                    : TDEIO::ERR_CANNOT_DELETE,
                          file);
                else
                {
                    modifiedDirs.add(Misc::getDir(file));

                    KURL::List urls;

                    Misc::getAssociatedUrls(KURL(file), urls);

                    if(urls.count())
                    {
                        KURL::List::Iterator uIt,
                                             uEnd=urls.end();

                        for(uIt=urls.begin(); uIt!=uEnd; ++uIt)
                            unlink(TQFile::encodeName((*uIt).path()).data());
                    }
                }
            }
            modified(itsRoot ? FOLDER_SYS : FOLDER_USER, clearList, modifiedDirs);
        }
        finished();
    }
}

void CKioFonts::modified(EFolder folder, bool clearList, const CDirList &dirs)
{
    KFI_DBUG << "modified(" << (int)folder << ")\n";

    if(FOLDER_SYS!=folder || itsCanStorePasswd || itsRoot)
    {
        if(dirs.count())
        {
            CDirList::ConstIterator it(dirs.begin()),
                                    end(dirs.end());

            for(; it!=end; ++it)
                itsFolders[folder].modified.add(*it);
        }
        else
            itsFolders[folder].modified.add(itsFolders[folder].location);

        if(++itsFontChanges>MAX_NEW_FONTS)
        {
            setTimeoutSpecialCommand(0); // Cancel timer
            doModified();
        } 
        else
            setTimeoutSpecialCommand(TIMEOUT);
    }

    if(FOLDER_SYS==folder && !itsRoot && !itsCanStorePasswd)
    {
        // If we modified sys, we're not root, and  couldn't store the passwd, then tdefontinst has already been called
        // so no need to ask it to add folder to fontconfig and X's config files...
        itsHasSys=true;
        itsAddToSysFc=false;
    }
    if(clearList)
        clearFontList();  // List of fonts has changed.../
}

void CKioFonts::special(const TQByteArray &a)
{
    KFI_DBUG << "special" << endl;

    if(a.size())
    {
        TQDataStream stream(a, IO_ReadOnly);
        int         cmd;

        stream >> cmd;

        switch (cmd)
        {
            case SPECIAL_RESCAN:
                clearFontList();
                updateFontList();
                finished();
                break;
            case SPECIAL_RECONFIG:  // Only itended to be called from kcmfontinst - when a user has re-enabled doX or doGs
                if(itsRoot && !itsFolders[FOLDER_SYS].modified.contains(itsFolders[FOLDER_SYS].location))
                    itsFolders[FOLDER_SYS].modified.add(itsFolders[FOLDER_SYS].location);
                else if(!itsRoot && !itsFolders[FOLDER_USER].modified.contains(itsFolders[FOLDER_USER].location))
                    itsFolders[FOLDER_USER].modified.add(itsFolders[FOLDER_USER].location);

                doModified();
                finished();
                break;
            default:
                error( TDEIO::ERR_UNSUPPORTED_ACTION, TQString::number(cmd));
        }
    }
    else
        doModified();
}

void CKioFonts::createRootRefreshCmd(TQCString &cmd, const CDirList &dirs, bool reparseCfg)
{
    if(reparseCfg)
        reparseConfig();

    if(!cmd.isEmpty())
        cmd+=" && ";

    cmd+=FC_CACHE_CMD;

    if(dirs.count())
    {
        CDirList::ConstIterator it(dirs.begin()),
                                end(dirs.end());

        for(; it!=end; ++it)
        {
            TQCString tmpCmd;

            if(*it==itsFolders[FOLDER_SYS].location)
            {
                if(0!=itsNrsKfiParams[0])
                    tmpCmd+=itsNrsKfiParams;
            }
            else
                if(0!=itsNrsNonMainKfiParams[0])
                    tmpCmd+=itsNrsNonMainKfiParams;

            if(!tmpCmd.isEmpty())
            {
                cmd+=" && tdefontinst ";
                cmd+=tmpCmd;
                cmd+=" ";
                cmd+=TQFile::encodeName(TDEProcess::quote(*it));
            }
        }
    }
    else if (0!=itsNrsKfiParams[0])
    {
        cmd+=" && tdefontinst ";
        cmd+=itsNrsKfiParams;
        cmd+=" ";
        cmd+=TQFile::encodeName(TDEProcess::quote(itsFolders[FOLDER_SYS].location));
    }
}

void CKioFonts::doModified()
{
    KFI_DBUG << "doModified" << endl;

    if(itsFolders[FOLDER_SYS].modified.count() || itsFolders[FOLDER_USER].modified.count())
        reparseConfig();

    itsFontChanges=0;
    if(itsFolders[FOLDER_SYS].modified.count())
    {
        if(itsRoot)
        {
            Misc::doCmd(FC_CACHE_CMD);
            KFI_DBUG << "RUN(root): " << FC_CACHE_CMD << endl;

            //
            // If a non-default folder has been modified, always configure X
            if(NULL==strchr(itsKfiParams, 'x') && 
               (itsFolders[FOLDER_SYS].modified.count()>1 || !itsFolders[FOLDER_SYS].modified.contains(itsFolders[FOLDER_SYS].location)))
            {
                if(0==itsKfiParams[0])
                    strcpy(itsKfiParams, "-x");
                else
                    strcat(itsKfiParams, "x");
            }

            if(0!=itsKfiParams[0])
            {
                CDirList::ConstIterator it(itsFolders[FOLDER_SYS].modified.begin()),
                                        end(itsFolders[FOLDER_SYS].modified.end());

                for(; it!=end; ++it)
                {
                    Misc::doCmd("tdefontinst", itsKfiParams, TQFile::encodeName(*it));
                    KFI_DBUG << "RUN(root): tdefontinst " << itsKfiParams << ' ' << *it << endl;
                }

                if(itsFolders[FOLDER_SYS].modified.contains(itsFolders[FOLDER_SYS].location))
                {
                    itsHasSys=true;
                    itsAddToSysFc=false;
                }
            }
        }
        else
        {
            TQCString cmd;

            createRootRefreshCmd(cmd, itsFolders[FOLDER_SYS].modified, false);
            if(doRootCmd(cmd, false) && itsFolders[FOLDER_SYS].modified.contains(itsFolders[FOLDER_SYS].location))
            {
                itsHasSys=true;
                itsAddToSysFc=false;
            }
            if(NULL==strstr(itsNrsKfiParams, "s"))
                Misc::doCmd("xset", "fp", "rehash");  // doRootCmd can only refresh if xfs is being used, so try here anyway...
        }
        itsFolders[FOLDER_SYS].modified.clear();
    }

    if(!itsRoot && itsFolders[FOLDER_USER].modified.count())
    {
        Misc::doCmd(FC_CACHE_CMD);
        KFI_DBUG << "RUN(non-root): " << FC_CACHE_CMD << endl;

        if(0!=itsKfiParams[0])
        {
            CDirList::ConstIterator it(itsFolders[FOLDER_USER].modified.begin()),
                                    end(itsFolders[FOLDER_USER].modified.end());

            for(; it!=end; ++it)
            {
                 Misc::doCmd("tdefontinst", itsKfiParams, TQFile::encodeName(*it));
                KFI_DBUG << "RUN(non-root): tdefontinst " << itsKfiParams << ' ' << *it << endl;
            }
        }
        itsFolders[FOLDER_USER].modified.clear();
    }

    KFI_DBUG << "finished ModifiedDirs" << endl;
}

#define SYS_USER "root"
TQString CKioFonts::getRootPasswd(bool askPasswd)
{
    KFI_DBUG << "getRootPasswd" << endl;
    TDEIO::AuthInfo authInfo;
    SuProcess     proc(SYS_USER);
    bool          error=false;
    int           attempts=0;
    TQString       errorMsg;

    authInfo.url=KURL(KFI_TDEIO_FONTS_PROTOCOL ":///");
    authInfo.username=SYS_USER;
    authInfo.keepPassword=true;

    if(!checkCachedAuthentication(authInfo) && !askPasswd)
        authInfo.password=itsPasswd;

    if(askPasswd)
        while(!error && 0!=proc.checkInstall(authInfo.password.local8Bit()))
        {
            KFI_DBUG << "ATTEMPT : " << attempts << endl;
            if(1==attempts)
                errorMsg=i18n("Incorrect password.\n");
            if((!openPassDlg(authInfo, errorMsg) && attempts) || ++attempts>4 || SYS_USER!=authInfo.username)
                error=true;
        }
    else
        error=proc.checkInstall(authInfo.password.local8Bit()) ? true : false;
    return error ? TQString::null : authInfo.password;
}

bool CKioFonts::doRootCmd(const char *cmd, const TQString &passwd)
{
    KFI_DBUG << "doRootCmd " << cmd << endl;

    if(!passwd.isEmpty())
    {
        SuProcess proc(SYS_USER);

        if(itsCanStorePasswd)
            itsPasswd=passwd;

        KFI_DBUG << "Try to run command" << endl;
        proc.setCommand(cmd);
        return proc.exec(passwd.local8Bit()) ? false : true;
    }

    return false;
}

bool CKioFonts::confirmUrl(KURL &url)
{
    KFI_DBUG << "confirmUrl " << url.path() << endl;
    if(!itsRoot)
    {
        TQString sect(getSect(url.path()));

        if(!isSysFolder(sect) && !isUserFolder(sect))
        {
            bool changeToSystem=false;

            if(DEST_UNCHANGED!=itsLastDest && itsLastDestTime && (abs(time(NULL)-itsLastDestTime) < constMaxLastDestTime))
                changeToSystem=DEST_SYS==itsLastDest;
            else
                changeToSystem=KMessageBox::No==messageBox(QuestionYesNo,
                                                           i18n("Do you wish to install the font into \"%1\" (in which "
                                                                "case the font will only be usable by you), or \"%2\" ("
                                                                "the font will be usable by all users - but you will "
                                                                "need to know the administrator's password)?")
                                                               .arg(i18n(KFI_TDEIO_FONTS_USER)).arg(i18n(KFI_TDEIO_FONTS_SYS)),
                                                           i18n("Where to Install"), i18n(KFI_TDEIO_FONTS_USER),
                                                           i18n(KFI_TDEIO_FONTS_SYS));

            if(changeToSystem)
            {
                itsLastDest=DEST_SYS;
                url.setPath(TQChar('/')+i18n(KFI_TDEIO_FONTS_SYS)+TQChar('/')+url.fileName());
            }
            else
            {
                itsLastDest=DEST_USER;
                url.setPath(TQChar('/')+i18n(KFI_TDEIO_FONTS_USER)+TQChar('/')+url.fileName());
            }

            KFI_DBUG << "Changed URL to:" << url.path() << endl;
            return true;
        }
    }

    return false;
}

void CKioFonts::clearFontList()
{
    KFI_DBUG << "clearFontList" << endl;

    if(itsFontList)
        FcFontSetDestroy(itsFontList);

    itsFontList=NULL;
    itsFolders[FOLDER_SYS].fontMap.clear();
    itsFolders[FOLDER_USER].fontMap.clear();
}

bool CKioFonts::updateFontList()
{
    KFI_DBUG << "updateFontList" << endl;

    if(!itsFontList || !FcConfigUptoDate(0)  ||   // For some reason just the "!FcConfigUptoDate(0)" check does not always work :-(
       (abs(time(NULL)-itsLastFcCheckTime)>constMaxFcCheckTime))
    {
        FcInitReinitialize();
        clearFontList();
    }

    if(!itsFontList)
    {
        KFI_DBUG << "updateFontList - update list of fonts " << endl;

        itsLastFcCheckTime=time(NULL);

        FcPattern   *pat = FcPatternCreate();
        FcObjectSet *os  = FcObjectSetBuild(FC_FILE, FC_FAMILY, FC_WEIGHT, FC_SCALABLE,
#ifdef KFI_FC_HAS_WIDTHS
                                            FC_WIDTH,
#endif
                                            FC_SLANT, (void*)0);

        itsFontList=FcFontList(0, pat, os);

        FcPatternDestroy(pat);
        FcObjectSetDestroy(os);

        if (itsFontList)
        {
            TQString home(Misc::dirSyntax(TQDir::homeDirPath()));

            for (int i = 0; i < itsFontList->nfont; i++)
            {
                EFolder folder=FOLDER_SYS;
                TQString file(Misc::fileSyntax(CFcEngine::getFcString(itsFontList->fonts[i], FC_FILE)));

                if(!file.isEmpty())
                {
                    if(!itsRoot && 0==file.find(home))
                        folder=FOLDER_USER;

                    TQValueList<FcPattern *> &patterns=
                                                itsFolders[folder].fontMap[CFcEngine::createName(itsFontList->fonts[i])];
                    bool                    use=true;

                    if(patterns.count()) // Check for duplicates...
                    {
                        TQValueList<FcPattern *>::Iterator it,
                                                          end=patterns.end();

                        for(it=patterns.begin(); use && it!=end; ++it)
                            if(file==(Misc::fileSyntax(CFcEngine::getFcString(*it, FC_FILE))))
                                use=false;
                    }
                    if(use)
                        patterns.append(itsFontList->fonts[i]);
                }
            }
        }
    }

    if(NULL==itsFontList)
    {
        error(TDEIO::ERR_SLAVE_DEFINED, i18n("Internal fontconfig error."));
        return false;
    }

    return true;
}

CKioFonts::EFolder CKioFonts::getFolder(const KURL &url)
{
    return itsRoot || isSysFolder(getSect(url.path())) ? FOLDER_SYS : FOLDER_USER;
}

TQMap<TQString, TQValueList<FcPattern *> >::Iterator CKioFonts::getMap(const KURL &url)
{
    EFolder                                           folder(getFolder(url));
    TQMap<TQString, TQValueList<FcPattern *> >::Iterator it=itsFolders[folder].fontMap.find(removeMultipleExtension(url));

    if(it==itsFolders[folder].fontMap.end()) // Perhaps it was fonts:/System/times.ttf ???
    {
        FcPattern *pat=getEntry(folder, url.fileName(), false);

        if(pat)
            it=itsFolders[folder].fontMap.find(CFcEngine::createName(pat));
    }

    return it;
}

TQValueList<FcPattern *> * CKioFonts::getEntries(const KURL &url)
{
    TQMap<TQString, TQValueList<FcPattern *> >::Iterator it=getMap(url);

    if(it!=itsFolders[getFolder(url)].fontMap.end())
        return &(it.data());

    error(TDEIO::ERR_SLAVE_DEFINED, i18n("Could not access \"%1\".").arg(url.prettyURL()));
    return NULL;
}

FcPattern * CKioFonts::getEntry(EFolder folder, const TQString &file, bool full)
{
    TQMap<TQString, TQValueList<FcPattern *> >::Iterator it,
                                                      end=itsFolders[folder].fontMap.end();

    for(it=itsFolders[folder].fontMap.begin(); it!=end; ++it)
    {
        TQValueList<FcPattern *>::Iterator patIt,
                                          patEnd=it.data().end();

        for(patIt=it.data().begin(); patIt!=patEnd; ++patIt)
            if( (full && CFcEngine::getFcString(*patIt, FC_FILE)==file) ||
                (!full && Misc::getFile(CFcEngine::getFcString(*patIt, FC_FILE))==file))
                return *patIt;
    }

    return NULL;
}

bool CKioFonts::checkFile(const TQString &file)
{
    TQCString cFile(TQFile::encodeName(file));

    //
    // To speed things up, check the files extension 1st...
    if(checkExt(cFile, "ttf") || checkExt(cFile, "otf") || checkExt(cFile, "ttc") || checkExt(cFile, "pfa") || checkExt(cFile, "pfb") ||
       isAAfm(file) || isAPfm(file))
        return true;

    //
    // No exension match, so try querying with FreeType...
    int       count=0;
    FcPattern *pat=FcFreeTypeQuery((const FcChar8 *)(TQFile::encodeName(file).data()), 0, NULL, &count);

    if(pat)
    {
        FcPatternDestroy(pat);
        return true;
    }

    error(TDEIO::ERR_SLAVE_DEFINED, i18n("<p>Only fonts may be installed.</p><p>If installing a fonts package (*%1), then "
                                       "extract the components, and install individually.</p>").arg(constMultipleExtension));
    return false;
}

bool CKioFonts::getSourceFiles(const KURL &src, TQStringList &files)
{
    if(KFI_TDEIO_FONTS_PROTOCOL==src.protocol())
    {
        TQValueList<FcPattern *> *entries=getEntries(src);

        if(entries && entries->count())
        {
            TQValueList<FcPattern *>::Iterator it,
                                              end=entries->end();

            for(it=entries->begin(); it!=end; ++it)
                files.append(CFcEngine::getFcString(*it, FC_FILE));
        }

        if(files.count())
        {
            TQStringList::Iterator sIt,
                                  sEnd=files.end();

            for(sIt=files.begin(); sIt!=sEnd; ++sIt)
            {
                KURL::List urls;

                Misc::getAssociatedUrls(KURL(*sIt), urls);

                if(urls.count())
                {
                    KURL::List::Iterator uIt,
                                         uEnd=urls.end();

                    for(uIt=urls.begin(); uIt!=uEnd; ++uIt)
                        if(-1==files.findIndex((*uIt).path()))
                            files.append((*uIt).path());
                }
           }
        }
    }
    else
        if(src.isLocalFile())
            if(checkFile(src.path()))
                files.append(src.path());
            else
                return false;  // error logged in checkFile...

    if(files.count())
    {
        TQStringList::Iterator it,
                              end=files.end();

        for(it=files.begin(); it!=end; ++it)
        {
            TQCString        realSrc=TQFile::encodeName(*it);
            KDE_struct_stat buffSrc;

            if (-1==KDE_stat(realSrc.data(), &buffSrc))
            {
                error(EACCES==errno ? TDEIO::ERR_ACCESS_DENIED : TDEIO::ERR_DOES_NOT_EXIST, src.prettyURL());
                return false;
            }
            if(S_ISDIR(buffSrc.st_mode))
            {
                error(TDEIO::ERR_IS_DIRECTORY, src.prettyURL());
                return false;
            }
            if(S_ISFIFO(buffSrc.st_mode) || S_ISSOCK(buffSrc.st_mode))
            {
                error(TDEIO::ERR_CANNOT_OPEN_FOR_READING, src.prettyURL());
                return false;
            }
        }
    }
    else
    {
        error(TDEIO::ERR_DOES_NOT_EXIST, src.prettyURL());
        return false;
    }

    return true;
}

bool CKioFonts::checkDestFile(const KURL &src, const KURL &dest, EFolder destFolder, bool overwrite)
{
    if(!overwrite && (Misc::fExists(itsFolders[destFolder].location+src.fileName()) ||
                      Misc::fExists(itsFolders[destFolder].location+modifyName(src.fileName())) ) )
    {
        error(TDEIO::ERR_FILE_ALREADY_EXIST, dest.prettyURL());
        return false;
    }

    return true;
}

bool CKioFonts::checkDestFiles(const KURL &src, TQMap<TQString, TQString> &map, const KURL &dest, EFolder destFolder, bool overwrite)
{
    //
    // Check whether files exist at destination...
    //
    if(dest.protocol()==src.protocol() &&
       dest.directory()==src.directory())  // Check whether confirmUrl changed a "cp fonts:/System fonts:/"
                                           // to "cp fonts:/System fonts:/System"
    {
        error(TDEIO::ERR_FILE_ALREADY_EXIST, dest.prettyURL());
        return false;
    }

    if(!overwrite)
    {
        TQMap<TQString, TQString>::Iterator fIt(map.begin()),
                                         fEnd(map.end());

        for(; fIt!=fEnd; ++fIt)
            if(NULL!=getEntry(destFolder, fIt.data()) || NULL!=getEntry(destFolder, modifyName(fIt.data())))
            {
                error(TDEIO::ERR_FILE_ALREADY_EXIST, dest.prettyURL());
                return false;
            }
    }

    return true;
}

//
// Gather the number and names of the font faces located in "files". If there is more than 1 face
// (such as there would be for a TTC font), then ask the user for confirmation of the action.
bool CKioFonts::confirmMultiple(const KURL &url, const TQStringList &files, EFolder folder, EOp op)
{
    if(KFI_TDEIO_FONTS_PROTOCOL!=url.protocol())
        return true;

    TQStringList::ConstIterator it,
                               end=files.end();
    TQStringList                fonts;

    for(it=files.begin(); it!=files.end(); ++it)
    {
        FcPattern *pat=getEntry(folder, *it, false);

        if(pat)
        {
            TQString name(CFcEngine::createName(pat));

            if(-1==fonts.findIndex(name))
                fonts.append(name);
        }
    }
    
    if(fonts.count()>1)
    {
        TQString               out;
        TQStringList::Iterator it,
                              end=fonts.end();

        for(it=fonts.begin(); it!=end; ++it)
            out+=TQString("<li>")+*it+TQString("</li>");

        if(KMessageBox::No==messageBox(QuestionYesNo,
                                              OP_MOVE==op
                                                ? i18n("<p>This font is located in a file alongside other fonts; in order "
                                                       "to proceed with the moving they will all have to be moved. "
                                                       "The other affected fonts are:</p><ul>%1</ul><p>\n Do you wish to "
                                                       "move all of these?</p>").arg(out)
                                            : OP_COPY==op
                                                ? i18n("<p>This font is located in a file alongside other fonts; in order "
                                                       "to proceed with the copying they will all have to be copied. "
                                                       "The other affected fonts are:</p><ul>%1</ul><p>\n Do you wish to "
                                                       "copy all of these?</p>").arg(out)
                                                : i18n("<p>This font is located in a file alongside other fonts; in order "
                                                       "to proceed with the deleting they will all have to be deleted. "
                                                       "The other affected fonts are:</p><ul>%1</ul><p>\n Do you wish to "
                                                       "delete all of these?</p>").arg(out)))
        {
            error(TDEIO::ERR_USER_CANCELED, url.prettyURL());
            return false;
        }
    }

    return true;
}

bool CKioFonts::confirmMultiple(const KURL &url, TQValueList<FcPattern *> *patterns, EFolder folder, EOp op)
{
    if(KFI_TDEIO_FONTS_PROTOCOL!=url.protocol())
        return true;

    TQStringList files;

    if(patterns && patterns->count())
    {
        TQValueList<FcPattern *>::Iterator it,
                                          end=patterns->end();

        for(it=patterns->begin(); it!=end; ++it)
            files.append(CFcEngine::getFcString(*it, FC_FILE));
    }

    return confirmMultiple(url, files, folder, op);
}

bool CKioFonts::checkUrl(const KURL &u, bool rootOk)
{
    if(KFI_TDEIO_FONTS_PROTOCOL==u.protocol() && (!rootOk || (rootOk && "/"!=u.path())))
    {
        TQString sect(getSect(u.path()));

        if(itsRoot)
        {
            if((isSysFolder(sect) || isUserFolder(sect)) &&
               (itsFolders[FOLDER_SYS].fontMap.end()==itsFolders[FOLDER_SYS].fontMap.find(sect)))
//CPD: TODO: || it has a font specified! e.g. fonts:/System/Times -> even in have a fonts:/System font, redirect
//should still happen
            {
                 redirection(getRedirect(u));
                 finished();
                 return false;
            }
        }
        else
            if(!isSysFolder(sect) && !isUserFolder(sect))
            {
                error(TDEIO::ERR_SLAVE_DEFINED, i18n("Please specify \"%1\" or \"%2\".")
                      .arg(i18n(KFI_TDEIO_FONTS_USER)).arg(i18n(KFI_TDEIO_FONTS_SYS)));
                return false;
            }
    }

    return true;
}

bool CKioFonts::checkAllowed(const KURL &u)
{
    if (KFI_TDEIO_FONTS_PROTOCOL==u.protocol())
    {
        TQString ds(Misc::dirSyntax(u.path()));

        if(ds==TQString(TQChar('/')+i18n(KFI_TDEIO_FONTS_USER)+TQChar('/')) ||
           ds==TQString(TQChar('/')+i18n(KFI_TDEIO_FONTS_SYS)+TQChar('/')) ||
           ds==TQString(TQChar('/')+TQString::fromLatin1(KFI_TDEIO_FONTS_USER)+TQChar('/')) ||
           ds==TQString(TQChar('/')+TQString::fromLatin1(KFI_TDEIO_FONTS_SYS)+TQChar('/')))
        {
            error(TDEIO::ERR_SLAVE_DEFINED, i18n("Sorry, you cannot rename, move, copy, or delete either \"%1\" or \"%2\".")
                  .arg(i18n(KFI_TDEIO_FONTS_USER)).arg(i18n(KFI_TDEIO_FONTS_SYS))); \
            return false;
        }
    }

    return true;
}

//
// Create an AFM from a Type 1 (pfa/pfb) font and its PFM file...
void CKioFonts::createAfm(const TQString &file, bool nrs, const TQString &passwd)
{
    if(nrs && passwd.isEmpty())
        return;

    bool type1=isAType1(file),
         pfm=!type1 && isAPfm(file);  // No point checking if is pfm if its a type1

    if(type1 || pfm)
    {
        TQString afm=getMatch(file, "afm");  // pf2afm wants files with lowercase extension, so just check for lowercase!
                                            // -- when a font is installed, the extensio is converted to lowercase anyway...

        if(afm.isEmpty())  // No point creating if AFM already exists!
        {
            TQString pfm,
                    t1;

            if(type1)      // Its a Type1, so look for existing PFM
            {
                pfm=getMatch(file, "pfm");
                t1=file;
            }
            else           // Its a PFM, so look for existing Type1
            {
                t1=getMatch(file, "pfa");
                if(t1.isEmpty())
                    t1=getMatch(file, "pfb");
                pfm=file;
            }

            if(!t1.isEmpty() && !pfm.isEmpty())         // Do we have both Type1 and PFM?
            {
                TQString name(t1.left(t1.length()-4));   // pf2afm wants name without extension...

                if(nrs)
                {
                    TQCString cmd("pf2afm ");
                    cmd+=TQFile::encodeName(TDEProcess::quote(name));
                    doRootCmd(cmd, passwd);
                }
                else
                    Misc::doCmd("pf2afm", TQFile::encodeName(name));
            }
        }
    }
}

void CKioFonts::reparseConfig()
{
    KFI_DBUG << "reparseConfig" << endl;

    itsKfiParams[0]=0;
    if(!itsRoot)
    {
        itsNrsKfiParams[0]=0;
        itsNrsNonMainKfiParams[0]=0;
    }

    if(itsRoot)
    {
        TDEConfig cfg(KFI_ROOT_CFG_FILE);
        bool    doX=cfg.readBoolEntry(KFI_CFG_X_KEY, KFI_DEFAULT_CFG_X),
                doGs=cfg.readBoolEntry(KFI_CFG_GS_KEY, KFI_DEFAULT_CFG_GS);

        if(doX || !doGs)
        {
            strcpy(itsKfiParams, doGs ? "-g" : "-");
            if(doX)
            {
                if(!itsUsingXfsFpe)
                    strcat(itsKfiParams, "r");

                if(!itsUsingFcFpe)
                {
                    strcat(itsKfiParams, itsUsingXfsFpe ? "sx" : "x");
                    if(!itsHasSys)
                        strcat(itsKfiParams, "a");
                }
            }
        }
    }
    else
    {
        TDEConfig rootCfg(KFI_ROOT_CFG_FILE);
        bool    rootDoX=rootCfg.readBoolEntry(KFI_CFG_X_KEY, KFI_DEFAULT_CFG_X),
                rootDoGs=rootCfg.readBoolEntry(KFI_CFG_GS_KEY, KFI_DEFAULT_CFG_GS);

        strcpy(itsNrsKfiParams, "-");

        if(rootDoX || rootDoGs)
        {
            strcpy(itsNrsKfiParams, "-");
            strcpy(itsNrsNonMainKfiParams, "-");

            if(rootDoGs)
            {
                strcpy(itsNrsKfiParams, "g");
                strcpy(itsNrsNonMainKfiParams, "g");
            }

            if(rootDoX && !itsUsingFcFpe)
            {
                strcat(itsNrsKfiParams, itsUsingXfsFpe ? "sx" : "x");   // Can't get root to refresh X, only xfs!
                strcat(itsNrsNonMainKfiParams, itsUsingXfsFpe ? "sx" : "x");
                if(!itsHasSys)
                     strcat(itsNrsKfiParams, "a");
            }
            if(0==itsNrsNonMainKfiParams[1])
                itsNrsNonMainKfiParams[0]=0;
        }

        if(itsAddToSysFc)
            strcpy(itsNrsKfiParams, "f");

        if(0==itsNrsKfiParams[1])
            itsNrsKfiParams[0]=0;

        TDEConfig cfg(KFI_CFG_FILE);
        bool    doX=cfg.readBoolEntry(KFI_CFG_X_KEY, KFI_DEFAULT_CFG_X),
                doGs=cfg.readBoolEntry(KFI_CFG_GS_KEY, KFI_DEFAULT_CFG_GS);

        strcpy(itsKfiParams, doGs ? "-g" : "-");

        if(doX)
            strcat(itsKfiParams, itsUsingFcFpe ? "r" : "rx");
    }

    if(0==itsKfiParams[1])
        itsKfiParams[0]=0;
}

}
