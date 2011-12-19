/*
  'Web' kwin client

  Copyright (C) 2005 Sandro Giessl <sandro@giessl.com>
  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include <tqpainter.h>

#include <kconfig.h>

#include "Web.h"
#include "WebButton.h"

extern "C"
{
  KDE_EXPORT KDecorationFactory *create_factory()
  {
    return new Web::WebFactory();
  }
}

namespace Web {

WebClient::WebClient(KDecorationBridge* bridge, KDecorationFactory* factory)
  : KCommonDecoration(bridge, factory)
{
  // Empty.
}

WebClient::~WebClient()
{
  // Empty.
}

TQString WebClient::visibleName() const
{
    return i18n("Web");
}

TQString WebClient::defaultButtonsLeft() const
{
    return "S";
}

TQString WebClient::defaultButtonsRight() const
{
    return "HIAX";
}

bool WebClient::decorationBehaviour(DecorationBehaviour behaviour) const
{
    switch (behaviour) {
        case DB_MenuClose:
            return false;

        case DB_WindowMask:
            return true;

        case DB_ButtonHide:
            return true;

        default:
            return KCommonDecoration::decorationBehaviour(behaviour);
    }
}

int WebClient::layoutMetric(LayoutMetric lm, bool respectWindowState, const KCommonDecorationButton *btn) const
{
//   bool maximized = maximizeMode()==MaximizeFull && !options()->moveResizeMaximizedWindows();

  switch (lm) {
    case LM_BorderLeft:
    case LM_BorderRight:
    case LM_BorderBottom:
      return borderSize_;

    case LM_TitleEdgeLeft:
    case LM_TitleEdgeRight:
    case LM_TitleEdgeTop:
    case LM_TitleEdgeBottom:
      return 0;

    case LM_TitleBorderLeft:
    case LM_TitleBorderRight:
      return 0;

    case LM_TitleHeight:
    case LM_ButtonWidth:
    case LM_ButtonHeight:
      return titleHeight_;

    case LM_ButtonSpacing:
      return 0;

    case LM_ExplicitButtonSpacer:
      return 0;

    default:
      return KCommonDecoration::layoutMetric(lm, respectWindowState, btn);
  }
}

KCommonDecorationButton *WebClient::createButton(ButtonType type)
{
    switch (type) {
        case MenuButton:
            return new WebButton(MenuButton, this, "menu", tqshape_);

        case OnAllDesktopsButton:
            return new WebButton(OnAllDesktopsButton, this, "on_all_desktops", tqshape_);

        case HelpButton:
            return new WebButton(HelpButton, this, "help", tqshape_);

        case MinButton:
            return new WebButton(MinButton, this, "minimize", tqshape_);

        case MaxButton:
            return new WebButton(MaxButton, this, "maximize", tqshape_);

        case CloseButton:
            return new WebButton(CloseButton, this, "close", tqshape_);

        case AboveButton:
            return new WebButton(AboveButton, this, "above", tqshape_);

        case BelowButton:
            return new WebButton(BelowButton, this, "below", tqshape_);

        case ShadeButton:
            return new WebButton(ShadeButton, this, "shade", tqshape_);

        default:
            return 0;
    }
}

  void
WebClient::init()
{
  // title height
  const int textVMargin   = 2;
  TQFontMetrics fm(options()->font(isActive(), isToolWindow()));

  // border size
  switch(options()->preferredBorderSize( factory())) {
    case BorderLarge:
      borderSize_ = 8;
      break;
    case BorderVeryLarge:
      borderSize_ = 12;
      break;
    case BorderHuge:
      borderSize_ = 18;
      break;
    case BorderVeryHuge:
      borderSize_ = 27;
      break;
    case BorderOversized:
      borderSize_ = 40;
      break;
    case BorderNormal:
    default:
      borderSize_ = 4;
  }
  titleHeight_ = QMAX(QMAX(14, fm.height() + textVMargin * 2), borderSize_);
  if (0 != titleHeight_ % 2)
    titleHeight_ += 1;

  KConfig c("kwinwebrc");
  c.setGroup("General");
  tqshape_ = c.readBoolEntry("Shape", true);

  KCommonDecoration::init();
}

  void
WebClient::reset( unsigned long changed )
{
  if (changed & SettingColors)
  {
    // tqrepaint the whole thing
    widget()->tqrepaint(false);
  } else if (changed & SettingFont) {
    // font has changed -- update title height
    // title height
    const int textVMargin   = 2;
    TQFontMetrics fm(options()->font(isActive(), isToolWindow()));
    titleHeight_ = QMAX(QMAX(14, fm.height() + textVMargin * 2), borderSize_);
    if (0 != titleHeight_ % 2)
      titleHeight_ += 1;

    widget()->tqrepaint(false);
  }

  KCommonDecoration::reset(changed);
}

  void
WebClient::paintEvent(TQPaintEvent * pe)
{
  int r_x, r_y, r_x2, r_y2;
  TQT_TQRECT_OBJECT(widget()->rect()).coords(&r_x, &r_y, &r_x2, &r_y2);
  const int titleEdgeLeft = layoutMetric(LM_TitleEdgeLeft);
  const int titleEdgeTop = layoutMetric(LM_TitleEdgeTop);
  const int titleEdgeRight = layoutMetric(LM_TitleEdgeRight);
  const int titleEdgeBottom = layoutMetric(LM_TitleEdgeBottom);
  const int ttlHeight = layoutMetric(LM_TitleHeight);
  const int titleEdgeBottomBottom = r_y+titleEdgeTop+ttlHeight+titleEdgeBottom-1;
  TQRect titleRect = TQRect(r_x+titleEdgeLeft+buttonsLeftWidth(), r_y+titleEdgeTop,
            r_x2-titleEdgeRight-buttonsRightWidth()-(r_x+titleEdgeLeft+buttonsLeftWidth()),
            titleEdgeBottomBottom-(r_y+titleEdgeTop) );
  titleRect.setTop(1);

  TQPainter p(widget());

  p.setPen(Qt::black);
  p.setBrush(options()->colorGroup(ColorFrame, isActive()).background());

  p.setClipRegion(pe->region() - titleRect);

  p.drawRect(widget()->rect());

  p.setClipRegion(pe->region());

  p.fillRect(titleRect, options()->color(ColorTitleBar, isActive()));

  if (tqshape_)
  {
    int r(width());
    int b(height());

    // Draw edge of top-left corner inside the area removed by the mask.

    p.drawPoint(3, 1);
    p.drawPoint(4, 1);
    p.drawPoint(2, 2);
    p.drawPoint(1, 3);
    p.drawPoint(1, 4);

    // Draw edge of top-right corner inside the area removed by the mask.

    p.drawPoint(r - 5, 1);
    p.drawPoint(r - 4, 1);
    p.drawPoint(r - 3, 2);
    p.drawPoint(r - 2, 3);
    p.drawPoint(r - 2, 4);

    // Draw edge of bottom-left corner inside the area removed by the mask.

    p.drawPoint(1, b - 5);
    p.drawPoint(1, b - 4);
    p.drawPoint(2, b - 3);
    p.drawPoint(3, b - 2);
    p.drawPoint(4, b - 2);

    // Draw edge of bottom-right corner inside the area removed by the mask.

    p.drawPoint(r - 2, b - 5);
    p.drawPoint(r - 2, b - 4);
    p.drawPoint(r - 3, b - 3);
    p.drawPoint(r - 4, b - 2);
    p.drawPoint(r - 5, b - 2);
  }

  p.setFont(options()->font(isActive(), isToolWindow()));

  p.setPen(options()->color(ColorFont, isActive()));

  p.drawText(titleRect, AlignCenter, caption());
}

void WebClient::updateWindowShape()
{
  if (!tqshape_)
    return;

  TQRegion mask(0, 0, width(), height());

  int r(width());
  int b(height());

  // Remove top-left corner.

  mask -= TQRegion(0, 0, 5, 1);
  mask -= TQRegion(0, 1, 3, 1);
  mask -= TQRegion(0, 2, 2, 1);
  mask -= TQRegion(0, 3, 1, 2);

  // Remove top-right corner.

  mask -= TQRegion(r - 5, 0, 5, 1);
  mask -= TQRegion(r - 3, 1, 3, 1);
  mask -= TQRegion(r - 2, 2, 2, 1);
  mask -= TQRegion(r - 1, 3, 1, 2);

  // Remove bottom-left corner.

  mask -= TQRegion(0, b - 5, 1, 3);
  mask -= TQRegion(0, b - 3, 2, 1);
  mask -= TQRegion(0, b - 2, 3, 1);
  mask -= TQRegion(0, b - 1, 5, 1);

  // Remove bottom-right corner.

  mask -= TQRegion(r - 5, b - 1, 5, 1);
  mask -= TQRegion(r - 3, b - 2, 3, 1);
  mask -= TQRegion(r - 2, b - 3, 2, 1);
  mask -= TQRegion(r - 1, b - 5, 1, 2);

  setMask(mask);
}

KDecoration* WebFactory::createDecoration( KDecorationBridge* b )
{
  return(new WebClient(b, this));
}

bool WebFactory::reset(unsigned long changed)
{
  // Do we need to "hit the wooden hammer" ?
  bool needHardReset = true;
  if (changed & SettingColors || changed & SettingFont)
  {
    needHardReset = false;
  } else if (changed & SettingButtons) {
    // handled by KCommonDecoration
    needHardReset = false;
  }

  if (needHardReset) {
    return true;
  } else {
    resetDecorations(changed);
    return false;
  }
}

bool WebFactory::supports( Ability ability )
{
    switch( ability )
    {
        case AbilityAnnounceButtons:
        case AbilityButtonOnAllDesktops:
        case AbilityButtonHelp:
        case AbilityButtonMinimize:
        case AbilityButtonMaximize:
        case AbilityButtonClose:
        case AbilityButtonMenu:
        case AbilityButtonAboveOthers:
        case AbilityButtonBelowOthers:
        case AbilityButtonShade:
            return true;
        default:
            return false;
    };
}

TQValueList< WebFactory::BorderSize > WebFactory::borderSizes() const
{ // the list must be sorted
  return TQValueList< BorderSize >() << BorderNormal << BorderLarge <<
      BorderVeryLarge <<  BorderHuge << BorderVeryHuge << BorderOversized;
}

}

#include "Web.moc"
// vim:ts=2:sw=2:tw=78:set et:
// kate: indent-width 2; replace-tabs on; tab-width 2; space-indent on;
