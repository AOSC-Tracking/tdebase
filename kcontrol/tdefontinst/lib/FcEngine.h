#ifndef __FC_ENGINE_H__
#define __FC_ENGINE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tqstring.h>
#include <tqvaluevector.h>
#include <tqfont.h>
#include <kurl.h>
#include <fontconfig/fontconfig.h>

#if (FC_VERSION<20200)

#define KFI_FC_NO_WIDTHS
#define KFI_FC_LIMITED_WEIGHTS

#endif

#ifdef KFI_FC_LIMITED_WEIGHTS

#undef FC_WEIGHT_LIGHT
#define FC_WEIGHT_THIN              0
#define FC_WEIGHT_EXTRALIGHT        40
#define FC_WEIGHT_ULTRALIGHT        FC_WEIGHT_EXTRALIGHT
#define FC_WEIGHT_LIGHT             50
#define FC_WEIGHT_BOOK              75
#define FC_WEIGHT_REGULAR           80
#define FC_WEIGHT_NORMAL            FC_WEIGHT_REGULAR
#define FC_WEIGHT_SEMIBOLD          FC_WEIGHT_DEMIBOLD
#define FC_WEIGHT_EXTRABOLD         205
#define FC_WEIGHT_ULTRABOLD         FC_WEIGHT_EXTRABOLD
#define FC_WEIGHT_HEAVY             FC_WEIGHT_BLACK

#endif

class TQPixmap;

#ifdef HAVE_XFT
typedef struct _XftFont  XftFont;
#endif

namespace KFI
{

class TDE_EXPORT CFcEngine
{
    public:

    CFcEngine();
    ~CFcEngine();

#ifdef HAVE_XFT
    bool    draw(const KURL &url, int w, int h, TQPixmap &pix, int faceNo, bool thumb);
#endif
    int     getNumIndexes() { return itsIndexCount; } // Only valid after draw has been called!
    TQString getName(const KURL &url, int faceNo=0);
    bool    getInfo(const KURL &url, int faceNo, TQString &full, TQString &family, TQString &foundry, TQString &weight,
#ifndef KFI_FC_NO_WIDTHS
                    TQString &width,
#endif
                    TQString &spacing, TQString &slant);
    TQFont   getQFont(const TQString &name, int size);

    const TQValueVector<int> & sizes() const { return itsSizes; }
    int                       alphaSize() const { return itsAlphaSize; }

    static TQString getPreviewString();
    static void    setPreviewString(const TQString &str);
    static TQString getUppercaseLetters();
    static TQString getLowercaseLetters();
    static TQString getPunctuation();
    static TQString getFcString(FcPattern *pat, const char *val, int faceNo=0);
    static TQString createName(FcPattern *pat, int faceNo=0);
    static TQString weightStr(int weight, bool emptyNormal=true);
#ifndef KFI_FC_NO_WIDTHS
    static TQString widthStr(int width, bool emptyNormal=true);
#endif
    static TQString slantStr(int slant, bool emptyNormal=true);
    static TQString spacingStr(int spacing);

    static const int constScalableSizes[];
    static const int constDefaultAlphaSize;

    private:

    bool      parseUrl(const KURL &url, int faceNo, bool all=false);
    void      parseName(const TQString &name, int faceNo, bool all=false);
#ifdef HAVE_XFT
    XftFont * getFont(int size, TQPixmap *pix=NULL);
    void      getSizes(TQPixmap *pix=NULL);
#endif

    private:

    bool              itsInstalled;
    TQString           itsName,
                      itsDescriptiveName,
                      itsFoundry;
    int               itsIndex,
                      itsIndexCount,
                      itsWeight,
#ifndef KFI_FC_NO_WIDTHS
                      itsWidth,
#endif
                      itsSlant,
                      itsSpacing,
                      itsAlphaSize;
    TQValueVector<int> itsSizes;
    KURL              itsLastUrl;
    FcBool            itsScalable;
};

}

#endif
