/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef __pglobal_h__
#define __pglobal_h__

#include <tqiconset.h>
#include <tqmap.h>

#include <kpanelapplet.h>
#include <kpanelextension.h>
#include <kurl.h>

namespace KickerLib
{

/*
 * Functions to convert between various enums
 */
KDE_EXPORT KPanelExtension::Position directionToPosition(KPanelApplet::Direction d);
KDE_EXPORT KPanelExtension::Position directionToPopupPosition(KPanelApplet::Direction d);
KDE_EXPORT KPanelApplet::Direction positionToDirection(KPanelExtension::Position p);
KDE_EXPORT KPanelApplet::Direction arrowToDirection(TQt::ArrowType p);
KDE_EXPORT int sizeValue(KPanelExtension::Size s);

/**
 * Pixel sizes for but sizes and margins
 */
KDE_EXPORT int maxButtonDim();

/**
 * Tint the image to reflect the current color scheme
 * Used, for instance, by KMenu side bar
 */
KDE_EXPORT void colorize(TQImage& image);

/**
 * Blend a color rectangle on a painter
 */
KDE_EXPORT void drawBlendedRect(TQPainter *p, const TQRect &r, const TQColor &color = TQt::black, int alpha = 0x40);

/**
 * Blend two colours together to get a colour halfway in between
 */
KDE_EXPORT TQColor blendColors(const TQColor& c1, const TQColor& c2);

/**
 * Create or copy .desktop files for use in kicker safely and easily
 */
KDE_EXPORT TQString copyDesktopFile(const KURL&url);
KDE_EXPORT TQString newDesktopFile(const KURL&url);


/**
 * Reduces a popup menu
 *
 * When a popup menu contains only 1 sub-menu, it makes no sense to
 * show this popup-menu but we better show the sub-menu directly.
 *
 * This function checks whether that is the case and returns either the
 * original menu or the sub-menu when appropriate.
 */
KDE_EXPORT TQPopupMenu *reduceMenu(TQPopupMenu *);


/**
 * Calculate the appropriate position for a popup menu based on the
 * direction, the size of the menu, the widget geometry, and a optional
 * point in the local coordinates of the widget.
 */
KDE_EXPORT TQPoint popupPosition(KPanelApplet::Direction d,
                                const TQWidget* popup,
                                const TQWidget* source,
                                const TQPoint& offset = TQPoint(0, 0));

/**
 * Calculate an acceptable inverse of the given color wich will be used
 * as the shadow color.
 */
KDE_EXPORT TQColor shadowColor(const TQColor& c);

/**
 * Get an appropriate for a menu in Plasma. As the user may set this size
 * globally, it is important to always use this method.
 * @param icon the name of icon requested
 * @return the icon set for the requested icon
 */
KDE_EXPORT TQIconSet menuIconSet(const TQString& icon);

}

#endif // __pglobal_h__
