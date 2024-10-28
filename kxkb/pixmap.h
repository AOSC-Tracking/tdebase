#ifndef __PIXMAP_H__
#define __PIXMAP_H__


#include <tqpixmap.h>
#include <tqdict.h>
#include <tqstring.h>

#include "kxkbconfig.h"

#define ERROR_CODE "error"
#define ERROR_LABEL "!"

#define FLAG_MAX_DIM 24

enum PixmapStyle {
    PIXMAP_STYLE_NORMAL = 0,
    PIXMAP_STYLE_INDICATOR = 1,
    PIXMAP_STYLE_CONTEXTMENU = 2
};

class LayoutIconManager {
    public:
        LayoutIconManager(KxkbConfig *kxkbConfig);
        const TQPixmap& find(const TQString& code, int pixmapStyle, const TQString& displayName = TQString::null);

    private:
        TQPixmap* createErrorPixmap();
        TQString getCountryFromLayoutName(const TQString& layoutName);

    private:
        KxkbConfig *m_kxkbConfig;
        static const TQString flagTemplate;
        bool m_showFlag, m_showLabel, m_bgTransparent, m_labelShadow, m_fitToBox, m_dimFlag, m_bevel;
        TQColor m_bgColor, m_fgColor, m_shColor;
        TQFont m_labelFont;

        TQDict<TQPixmap> m_pixmapCache;
};

#endif
