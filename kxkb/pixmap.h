#ifndef __PIXMAP_H__
#define __PIXMAP_H__


#include <tqpixmap.h>
#include <tqdict.h>
#include <tqstring.h>

#include "kxkbconfig.h"

enum PixmapStyle {
  PIXMAP_STYLE_NORMAL = 0,
  PIXMAP_STYLE_INDICATOR = 1,
  PIXMAP_STYLE_CONTEXTMENU = 2
};

class LayoutIcon {

private:
  static LayoutIcon* instance;
  static const TQString flagTemplate;

  KxkbConfig m_kxkbConfig;
  bool m_showFlag;
  bool m_showLabel;
  TQColor m_bgColor;
  bool m_bgTransparent;
  TQColor m_fgColor;
  TQFont m_labelFont;
  bool m_labelShadow;
  TQColor m_shColor;
  bool m_fitToBox;

  TQDict<TQPixmap> m_pixmapCache;

  LayoutIcon();
  TQPixmap* createErrorPixmap();
  void dimPixmap(TQPixmap& pixmap);
  TQString getCountryFromLayoutName(const TQString& layoutName);

public:
  static const TQString& ERROR_CODE;

  static LayoutIcon& getInstance();
  const TQPixmap& findPixmap(const TQString& code, int pixmapStyle, const TQString& displayName="");
};

#endif
