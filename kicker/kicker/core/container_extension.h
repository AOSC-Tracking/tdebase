/*****************************************************************

Copyright (c) 2000 Matthias Elter <elter@kde.org>

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

#ifndef __container_extension_h__
#define __container_extension_h__

#include <tqframe.h>
#include <tqptrlist.h>

#include <kpanelextension.h>
#include <dcopobject.h>
#include <netwm_def.h>

#include "global.h"
#include "appletinfo.h"
#include "unhidetrigger.h"
#include "extensionSettings.h"

class TQBoxLayout;
class TQGridLayout;
class TQPopupMenu;
class TQTimer;
class TQVBox;
class QXEmbed;
class HideButton;
class KConfig;
class KWinModule;
class PopupWidgetFilter;
class PanelExtensionOpMenu;
class TQColor;

class ExtensionContainer : public TQFrame
{
    Q_OBJECT

public:
    enum UserHidden { Unhidden, LeftTop, RightBottom };
    enum HideMode { ManualHide, AutomaticHide, BackgroundHide };

    ExtensionContainer(const AppletInfo& info,
                       const TQString& extensionId,
                       TQWidget *parent = 0);
    ExtensionContainer(KPanelExtension* extension,
                       const AppletInfo& info,
                       const TQString& extensionId,
                       TQWidget *parent = 0);
    virtual ~ExtensionContainer();

    virtual TQSize sizeHint(KPanelExtension::Position, const TQSize &maxSize) const;

    const AppletInfo& info() const { return _info; }

    TQString extensionId() const { return _id; }

    void readConfig();
    void writeConfig();

    virtual TQString panelId() const { return extensionId(); }

    virtual void about();
    virtual void help();
    virtual void preferences();
    virtual void reportBug();

    void removeSessionConfigFile();

    KPanelExtension::Orientation orientation() const;
    KPanelExtension::Position position() const;
    void setPosition(KPanelExtension::Position p) { arrange( p, tqalignment(), xineramaScreen() ); }

    int xineramaScreen() const;
    void setXineramaScreen(int screen);

    void setResizeableHandle( bool resizeablehandle=true );
    void setHideButtons(bool showLeft, bool showRight);
    void setSize(KPanelExtension::Size size, int custom);
    KPanelExtension::Size size() const;
    int customSize() const { return m_settings.customSize(); }
    HideMode hideMode() const;
    void unhideIfHidden(int showForHowManyMS = 0);
    bool reserveStrut() const;

    KPanelExtension::Alignment tqalignment() const;
    void setAlignment(KPanelExtension::Alignment a) { arrange( position(), a, xineramaScreen() ); }

    TQRect currentGeometry() const;
    TQRect initialGeometry(KPanelExtension::Position p, KPanelExtension::Alignment a,
                          int XineramaScreen, bool autoHidden = false,
                          UserHidden userHidden = Unhidden) const;

    bool eventFilter( TQObject *, TQEvent * );

    int panelOrder() const { return m_panelOrder; }
    void setPanelOrder(int order) { m_panelOrder = order; }

signals:
    void removeme(ExtensionContainer*);

protected slots:
    virtual void showPanelMenu( const TQPoint& pos );
    void moveMe();
    void updateLayout();
    void actuallyUpdateLayout();
    void enableMouseOverEffects();
    void updateHighlightColor();

protected:
    bool event(TQEvent*);
    void closeEvent( TQCloseEvent* e );
    void paintEvent(TQPaintEvent*);
    void leaveEvent(TQEvent*);

    void arrange(KPanelExtension::Position p, KPanelExtension::Alignment a, int XineramaScreen);
    bool autoHidden() const { return _autoHidden; };
    UserHidden userHidden() const { return _userHidden; };
    void resetLayout();
    bool needsBorder() const;

private slots:
    void unhideTriggered( UnhideTrigger::Trigger t, int XineramaScreen );
    void autoHideTimeout();
    void hideLeft();
    void hideRight();
    void autoHide(bool hide);
    void animatedHide(bool left);
    void updateWindowManager();
    void currentDesktopChanged(int);
    void strutChanged();
    void blockUserInput( bool block );
    void maybeStartAutoHideTimer();
    void stopAutoHideTimer();
    void maintainFocus(bool);

private:
    bool shouldUnhideForTrigger(UnhideTrigger::Trigger t) const;
    void init();
    TQSize initialSize(KPanelExtension::Position p, TQRect workArea) const;
    TQPoint initialLocation(KPanelExtension::Position p, KPanelExtension::Alignment a,
                           int XineramaScreen, const TQSize &s, TQRect workArea,
                           bool autohidden = false, UserHidden userHidden = Unhidden) const;
    void positionChange(KPanelExtension::Position p);
    void alignmentChange(KPanelExtension::Alignment a);
    void xineramaScreenChange(int /*XineramaScreen*/) {}
    int arrangeHideButtons();
    int setupBorderSpace();

    ExtensionSettings m_settings;
    ExtensionContainer::HideMode m_hideMode;
    UnhideTrigger::Trigger m_unhideTriggeredAt;

    // State variables
    bool             _autoHidden;
    UserHidden       _userHidden;
    bool             _block_user_input;
    TQPoint           _last_lmb_press;
    bool             _is_lmb_down;
    bool             _in_autohide;

    // Misc objects
    TQTimer               *_autohideTimer;
    TQTimer               *_updateLayoutTimer;
    NETExtendedStrut      _strut;
    PopupWidgetFilter    *_popupWidgetFilter;

    TQString               _id;
    PanelExtensionOpMenu *_opMnu;
    AppletInfo            _info;
    KPanelExtension::Type _type;

    // Widgets
    HideButton     *_ltHB; // Left Hide Button
    HideButton     *_rbHB; // Right Hide Button
    TQGridLayout   *_layout;
    TQWidget       *_resizeHandle; 

    KPanelExtension *m_extension;
    int m_maintainFocus;
    int m_panelOrder;
    TQColor m_highlightColor;
};

class PopupWidgetFilter : public TQObject
{
  Q_OBJECT

  public:
    PopupWidgetFilter( TQObject *parent );
    ~PopupWidgetFilter() {}
    bool eventFilter( TQObject *obj, TQEvent* e );
  signals:
    void popupWidgetHiding();
};

typedef TQValueList<ExtensionContainer*> ExtensionList;

#endif
