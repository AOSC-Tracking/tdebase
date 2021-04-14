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

#ifndef __container_button_h__
#define __container_button_h__

#include <tdelocale.h>
#include <kservice.h>
#include <kurl.h>

#include "container_base.h"

class TQLayout;
class PanelButton;
class TDEConfigGroup;

class ButtonContainer : public BaseContainer
{
    Q_OBJECT

public:
    ButtonContainer(TQPopupMenu* opMenu, TQWidget* parent = 0);

    virtual bool isValid() const;
    virtual bool isAMenu() const { return false; }

    virtual int widthForHeight(int height) const;
    virtual int heightForWidth(int width)  const;

    virtual void setBackground();

    virtual void configure();

    bool eventFilter (TQObject *, TQEvent *);
    virtual void completeMoveOperation();

    PanelButton* button() const { return _button; }

public slots:
    void setPopupDirection(KPanelApplet::Direction d);
    void setOrientation(KPanelExtension::Orientation o);

protected slots:
    void slotMenuClosed();
    void removeRequested();
    void hideRequested(bool);
    void dragButton(const KURL::List urls, const TQPixmap icon);
    void dragButton(const TQPixmap icon);

protected:
    virtual void doSaveConfiguration( TDEConfigGroup&, bool layoutOnly ) const;
    void embedButton(PanelButton* p);
    TQPopupMenu* createOpMenu();
    void checkImmutability(const TDEConfigGroup&);

protected:
    PanelButton  *_button;
    TQLayout      *_layout;
    TQPoint        _oldpos;
};

class KMenuButtonContainer : public ButtonContainer
{
public:
    KMenuButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    KMenuButtonContainer(TQPopupMenu* opMenu, TQWidget* parent = 0);
    virtual TQString appletType() const { return "KMenuButton"; }
    virtual TQString icon() const { return "kmenu"; }
    virtual TQString visibleName() const { return i18n("TDE Menu"); }

    virtual int heightForWidth( int width )  const;
    bool isAMenu() const { return true; }
};

class DesktopButtonContainer : public ButtonContainer
{
public:
    DesktopButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    DesktopButtonContainer(TQPopupMenu* opMenu, TQWidget* parent = 0);
    TQString appletType() const { return "DesktopButton"; }
    virtual TQString icon() const { return "desktop"; }
    virtual TQString visibleName() const { return i18n("Desktop Access"); }
};

class ServiceButtonContainer : public ButtonContainer
{
public:
    ServiceButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    ServiceButtonContainer(const KService::Ptr & service,  TQPopupMenu* opMenu,TQWidget* parent = 0);
    ServiceButtonContainer(const TQString& desktopFile,  TQPopupMenu* opMenu,TQWidget* parent = 0);
    TQString appletType() const { return "ServiceButton"; }
    virtual TQString icon() const;
    virtual TQString visibleName() const;
};

class URLButtonContainer : public ButtonContainer
{
public:
    URLButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    URLButtonContainer(const TQString& url, TQPopupMenu* opMenu, TQWidget* parent = 0);
    TQString appletType() const { return "URLButton"; }
    virtual TQString icon() const;
    virtual TQString visibleName() const;
};

class BrowserButtonContainer : public ButtonContainer
{
public:
    BrowserButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    BrowserButtonContainer(const TQString& startDir, TQPopupMenu* opMenu, const TQString& icon = "kdisknav", TQWidget* parent = 0);
    TQString appletType() const { return "BrowserButton"; }
    virtual TQString icon() const { return "kdisknav"; }
    virtual TQString visibleName() const { return i18n("Quick Browser"); }
    bool isAMenu() const { return true; }
};

class ServiceMenuButtonContainer : public ButtonContainer
{
public:
    ServiceMenuButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    ServiceMenuButtonContainer(const TQString& relPath, TQPopupMenu* opMenu, TQWidget* parent = 0);
    TQString appletType() const { return "ServiceMenuButton"; }
    virtual TQString icon() const;
    virtual TQString visibleName() const;
    bool isAMenu() const { return true; }
};

class WindowListButtonContainer : public ButtonContainer
{
public:
    WindowListButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    WindowListButtonContainer(TQPopupMenu* opMenu, TQWidget* parent = 0);
    TQString appletType() const { return "WindowListButton"; }
    virtual TQString icon() const { return "window_duplicate"; }
    virtual TQString visibleName() const { return i18n("Windowlist"); }
    bool isAMenu() const { return true; }
};

class BookmarksButtonContainer : public ButtonContainer
{
public:
    BookmarksButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget* parent = 0);
    BookmarksButtonContainer(TQPopupMenu* opMenu, TQWidget* parent = 0);
    TQString appletType() const { return "BookmarksButton"; }
    virtual TQString icon() const { return "bookmark"; }
    virtual TQString visibleName() const { return i18n("Bookmarks"); }
    bool isAMenu() const { return true; }
};

class NonKDEAppButtonContainer : public ButtonContainer
{
public:
    NonKDEAppButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget *parent=0);
    NonKDEAppButtonContainer(const TQString &name, const TQString &description,
                             const TQString &filePath, const TQString &icon,
                             const TQString &cmdLine, bool inTerm,
                             TQPopupMenu* opMenu, TQWidget* parent = 0);
    TQString appletType() const { return "ExecButton"; }
    virtual TQString icon() const { return "application-x-executable"; }
    virtual TQString visibleName() const { return i18n("Non-TDE Application"); }
};

class ExtensionButtonContainer : public ButtonContainer
{
public:
    ExtensionButtonContainer(const TDEConfigGroup& config, TQPopupMenu* opMenu, TQWidget *parent=0);
    ExtensionButtonContainer(const TQString& desktopFile, TQPopupMenu* opMenu, TQWidget *parent= 0);
    TQString appletType() const { return "ExtensionButton"; }
    virtual TQString icon() const;
    virtual TQString visibleName() const;
    bool isAMenu() const { return true; }
};

#endif

