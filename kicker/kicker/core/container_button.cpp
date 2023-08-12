/*****************************************************************

Copyright (c) 1996-2003 the kicker authors. See file AUTHORS.

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

#include <algorithm>

#include <tqlayout.h>

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdeglobal.h>
#include <tdemultipledrag.h>
#include <kpanelapplet.h>
#include <kurldrag.h>

#include "global.h"
#include "appletop_mnu.h"

#include "containerarea.h"
#include "panelbutton.h"
#include "bookmarksbutton.h"
#include "browserbutton.h"
#include "desktopbutton.h"
#include "extensionbutton.h"
#include "kbutton.h"
#include "knewbutton.h"
#include "kicker.h"
#include "kickerSettings.h"
#include "kickertip.h"
#include "nontdeappbutton.h"
#include "paneldrag.h"
#include "servicebutton.h"
#include "servicemenubutton.h"
#include "urlbutton.h"
#include "windowlistbutton.h"

#include "container_button.h"
#include "container_button.moc"

ButtonContainer::ButtonContainer(TQPopupMenu* opMenu, TQWidget* parent)
  : BaseContainer(opMenu, parent)
  , _button(0)
  , _layout(0)
  , _oldpos(0,0)
{
     setBackgroundOrigin(AncestorOrigin);
}

bool ButtonContainer::isValid() const
{
   if (_button)
   {
       return _button->isValid();
   }

   return false; // Can this happen?
}

// Buttons Shouldn't be square when larger than a certain size.
int ButtonContainer::widthForHeight(int height) const
{
    if (isValid())
    {
        return _button->widthForHeight(height);
    }

    return height;
}

int ButtonContainer::heightForWidth(int width)  const
{
    if (isValid())
    {
        return _button->heightForWidth(width);
    }

    return width;
}

void ButtonContainer::setBackground()
{
    PanelButton* b = button();
    if (!b)
        return;

    b->unsetPalette();
}

void ButtonContainer::configure()
{
    if (_button)
    {
        _button->configure();
    }
}

void ButtonContainer::doSaveConfiguration(TDEConfigGroup& config, bool layoutOnly) const
{
    // immutability is checked by ContainerBase
    if (_button && !layoutOnly)
    {
        _button->saveConfig(config);
    }
}

void ButtonContainer::setPopupDirection(KPanelApplet::Direction d)
{
    BaseContainer::setPopupDirection(d);

    if (_button)
    {
        _button->setPopupDirection(d);
    }
}

void ButtonContainer::setOrientation(Orientation o)
{
    BaseContainer::setOrientation(o);

    if(_button)
        _button->setOrientation(o);
}

void ButtonContainer::embedButton(PanelButton* b)
{
    if (!b) return;

    if (_layout) delete _layout;

    _button = b;
    _button->installEventFilter(this);

    TQVBoxLayout* vbox = new TQVBoxLayout(this);
    if (!b->centerButtonInContainer()) {
        b->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
    }
    vbox->addWidget(_button, 1);
    if (!b->centerButtonInContainer()) {
        TQSpacerItem* spacer = new TQSpacerItem(0, 0, TQSizePolicy::Minimum, TQSizePolicy::MinimumExpanding);
        vbox->addItem(spacer);
    }
    _layout = vbox;

    connect(_button, TQT_SIGNAL(requestSave()), TQT_SIGNAL(requestSave()));
    connect(_button, TQT_SIGNAL(hideme(bool)), TQT_SLOT(hideRequested(bool)));
    connect(_button, TQT_SIGNAL(removeme()), TQT_SLOT(removeRequested()));
    connect(_button, TQT_SIGNAL(dragme(const TQPixmap)),
            TQT_SLOT(dragButton(const TQPixmap)));
    connect(_button, TQT_SIGNAL(dragme(const KURL::List, const TQPixmap)),
            TQT_SLOT(dragButton(const KURL::List, const TQPixmap)));
}

TQPopupMenu* ButtonContainer::createOpMenu()
{
    return new PanelAppletOpMenu(_actions, appletOpMenu(), 0, _button->title(),
                                 _button->icon(), this);
}

void ButtonContainer::removeRequested()
{
    if (isImmutable())
    {
        return;
    }

    emit removeme(this);
}

void ButtonContainer::hideRequested(bool shouldHide)
{
    if (shouldHide)
    {
        hide();
    }
    else
    {
        show();
    }
}

void ButtonContainer::dragButton(const KURL::List urls, const TQPixmap icon)
{
    if (isImmutable())
    {
        return;
    }

    KMultipleDrag* dd = new KMultipleDrag(this);
    dd->addDragObject(new KURLDrag(urls, 0));
    dd->addDragObject(new PanelDrag(this, 0));
    dd->setPixmap(icon);
    grabKeyboard();
    dd->dragMove();
    releaseKeyboard();
}

void ButtonContainer::dragButton(const TQPixmap icon)
{
    PanelDrag* dd = new PanelDrag(this, this);
    dd->setPixmap(icon);
    grabKeyboard();
    dd->drag();
    releaseKeyboard();
}

bool ButtonContainer::eventFilter(TQObject *o, TQEvent *e)
{
    if (o == _button && e->type() == TQEvent::MouseButtonPress)
    {
        static bool sentinal = false;

        if (sentinal)
        {
            return false;
        }

        sentinal = true;
        TQMouseEvent* me = TQT_TQMOUSEEVENT(e);
        switch (me->button())
        {
        case Qt::MidButton:
        {
            if (isImmutable())
            {
                break;
            }

            _button->setDown(true);
            _moveOffset = me->pos();
            emit moveme(this);
            sentinal = false;
            return true;
        }

        case Qt::RightButton:
        {
            if (!kapp->authorizeTDEAction("kicker_rmb") ||
                isImmutable())
            {
                break;
            }

            TQPopupMenu* menu = opMenu();
            connect( menu, TQT_SIGNAL( aboutToHide() ), this, TQT_SLOT( slotMenuClosed() ) );
            TQPoint pos = KickerLib::popupPosition(popupDirection(), menu, TQT_TQWIDGET(this),
                                                  (orientation() == Qt::Horizontal) ?
                                                   TQPoint(0, 0) : me->pos());

            Kicker::the()->setInsertionPoint(me->globalPos());

            KickerTip::enableTipping(false);
            switch (menu->exec(pos))
            {
            case PanelAppletOpMenu::Move:
                _moveOffset = rect().center();
                emit moveme(this);
                break;
            case PanelAppletOpMenu::Remove:
                emit removeme(this);
                break;
            case PanelAppletOpMenu::Help:
                help();
                break;
            case PanelAppletOpMenu::About:
                about();
                break;
            case PanelAppletOpMenu::Preferences:
                if (_button)
                {
                    _button->properties();
                }
                break;
            default:
                break;
            }
            KickerTip::enableTipping(true);

            Kicker::the()->setInsertionPoint(TQPoint());
            clearOpMenu();
            sentinal = false;
            return true;
        }

        default:
            break;
        }

        sentinal = false;
    }
    return false;
}

void ButtonContainer::completeMoveOperation()
{
    if (_button)
    {
        _button->setDown(false);
        setBackground();
    }
}

void ButtonContainer::slotMenuClosed()
{
    if (_button)
        _button->setDown(false);
}

void ButtonContainer::checkImmutability(const TDEConfigGroup& config)
{
    m_immutable = config.groupIsImmutable() ||
                  config.entryIsImmutable("ConfigFile") ||
                  config.entryIsImmutable("FreeSpace2");
}

// KMenuButton containerpan
KMenuButtonContainer::KMenuButtonContainer(const TDEConfigGroup& config, TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    if(KickerSettings::legacyKMenu())
	embedButton( new KButton(this) );
    else
	embedButton( new KNewButton(this) );
    _actions = PanelAppletOpMenu::KMenuEditor;
}

KMenuButtonContainer::KMenuButtonContainer(TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    if(KickerSettings::legacyKMenu())
	embedButton( new KButton(this) );
    else
	embedButton( new KNewButton(this) );
    _actions = PanelAppletOpMenu::KMenuEditor;
}

int KMenuButtonContainer::heightForWidth( int width ) const
{
    if ( width < 32 )
        return width + 10;
    else
        return ButtonContainer::heightForWidth(width);
}

// DesktopButton container
DesktopButtonContainer::DesktopButtonContainer(TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new DesktopButton(this) );
}

DesktopButtonContainer::DesktopButtonContainer(const TDEConfigGroup& config,
                                               TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new DesktopButton(this) );
}

// ServiceButton container
ServiceButtonContainer::ServiceButtonContainer( const TQString& desktopFile,
                                                TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new ServiceButton( desktopFile, this ) );
    _actions = KPanelApplet::Preferences;
}

ServiceButtonContainer::ServiceButtonContainer( const KService::Ptr &service,
                                                TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new ServiceButton( service, this ) );
    _actions = KPanelApplet::Preferences;
}

ServiceButtonContainer::ServiceButtonContainer( const TDEConfigGroup& config,
                                                TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new ServiceButton( config, this ) );
    _actions = KPanelApplet::Preferences;
}

TQString ServiceButtonContainer::icon() const
{
    return button()->icon();
}

TQString ServiceButtonContainer::visibleName() const
{
    return button()->title();
}

// URLButton container
URLButtonContainer::URLButtonContainer( const TQString& url, TQPopupMenu* opMenu, TQWidget* parent )
  : ButtonContainer(opMenu, parent)
{
    embedButton( new URLButton( url, this ) );
    _actions = KPanelApplet::Preferences;
}

URLButtonContainer::URLButtonContainer( const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new URLButton( config, this ) );
    _actions = KPanelApplet::Preferences;
}

TQString URLButtonContainer::icon() const
{
    return button()->icon();
}

TQString URLButtonContainer::visibleName() const
{
    return button()->title();
}

// BrowserButton container
BrowserButtonContainer::BrowserButtonContainer(const TQString &startDir, TQPopupMenu* opMenu, const TQString& icon, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new BrowserButton(icon, startDir, this) );
    _actions = KPanelApplet::Preferences;
}

BrowserButtonContainer::BrowserButtonContainer( const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new BrowserButton(config, this) );
    _actions = KPanelApplet::Preferences;
}

// ServiceMenuButton container
ServiceMenuButtonContainer::ServiceMenuButtonContainer(const TQString& relPath, TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new ServiceMenuButton(relPath, this) );
}

ServiceMenuButtonContainer::ServiceMenuButtonContainer( const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new ServiceMenuButton(config, this) );
}

TQString ServiceMenuButtonContainer::icon() const
{
    return button()->icon();
}

TQString ServiceMenuButtonContainer::visibleName() const
{
    return button()->title();
}

// WindowListButton container
WindowListButtonContainer::WindowListButtonContainer(const TDEConfigGroup& config, TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new WindowListButton(this) );
}

WindowListButtonContainer::WindowListButtonContainer(TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new WindowListButton(this) );
}

// BookmarkButton container
BookmarksButtonContainer::BookmarksButtonContainer(const TDEConfigGroup& config, TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new BookmarksButton(this) );
    _actions = PanelAppletOpMenu::BookmarkEditor;
}

BookmarksButtonContainer::BookmarksButtonContainer(TQPopupMenu *opMenu, TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new BookmarksButton(this) );
    _actions = PanelAppletOpMenu::BookmarkEditor;
}

// NonKDEAppButton container
NonKDEAppButtonContainer::NonKDEAppButtonContainer(const TQString &name,
                                                   const TQString &description,
                                                   const TQString &filePath,
                                                   const TQString &icon,
                                                   const TQString &cmdLine,
                                                   bool inTerm,
                                                   TQPopupMenu* opMenu,
                                                   TQWidget* parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton(new NonKDEAppButton(name, description, filePath, icon, cmdLine,
                                    inTerm, this));
    _actions = KPanelApplet::Preferences;
}

NonKDEAppButtonContainer::NonKDEAppButtonContainer( const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget *parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new NonKDEAppButton(config, this) );
    _actions = KPanelApplet::Preferences;
}

// ExtensionButton container
ExtensionButtonContainer::ExtensionButtonContainer(const TQString& df, TQPopupMenu* opMenu, TQWidget *parent)
  : ButtonContainer(opMenu, parent)
{
    embedButton( new ExtensionButton(df, this) );
}

ExtensionButtonContainer::ExtensionButtonContainer( const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget *parent)
  : ButtonContainer(opMenu, parent)
{
    checkImmutability(config);
    embedButton( new ExtensionButton(config, this) );
}

TQString ExtensionButtonContainer::icon() const
{
    return button()->icon();
}

TQString ExtensionButtonContainer::visibleName() const
{
    return button()->title();
}

