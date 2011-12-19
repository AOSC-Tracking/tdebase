/*****************************************************************

Copyright (c) 2000 Matthias Elter
              2004 Aaron J. Seigo <aseigo@kde.org>

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

#ifndef _panelextension_h_
#define _panelextension_h_

#include <kpanelextension.h>
#include <dcopobject.h>

#include "appletinfo.h"

class AppletContainer;
class ContainerArea;
class TQPopupMenu;
class TQGridLayout;

// This is the KPanelExtension responsible for the main kicker panel
// Prior to KDE 3.4 it was the ChildPanelExtension

class PanelExtension : public KPanelExtension, virtual public  DCOPObject
{
    Q_OBJECT
    K_DCOP

public:
    PanelExtension(const TQString& configFile, TQWidget *parent = 0, const char *name = 0);
    virtual ~PanelExtension();

    TQPopupMenu* opMenu();

k_dcop:
    int panelSize() { return sizeInPixels(); }
    int panelOrientation() { return static_cast<int>(orientation()); }
    int panelPosition() { return static_cast<int>(position()); }

    void setPanelSize(int size);
    void addKMenuButton();
    void addDesktopButton();
    void addWindowListButton();
    void addURLButton(const TQString &url);
    void addBrowserButton(const TQString &startDir);
    void addServiceButton(const TQString &desktopEntry);
    void addServiceMenuButton(const TQString &name, const TQString& relPath);
    void addNonKDEAppButton(const TQString &filePath, const TQString &icon,
                            const TQString &cmdLine, bool inTerm);
    void addNonKDEAppButton(const TQString &title, const TQString &description,
                            const TQString &filePath, const TQString &icon,
                            const TQString &cmdLine, bool inTerm);

    void addApplet(const TQString &desktopFile);
    void addAppletContainer(const TQString &desktopFile); // KDE4: remove, useless

    bool insertApplet(const TQString& desktopFile, int index);
    bool insertImmutableApplet(const TQString& desktopFile, int index);
    TQStringList listApplets();
    bool removeApplet(int index);

    void restart(); // KDE4: remove, moved to Kicker
    void configure(); // KDE4: remove, moved to Kikcker

public:
    TQSize sizeHint(Position, TQSize maxSize) const;
    Position preferedPosition() const { return Bottom; }
    bool eventFilter( TQObject *, TQEvent * );

protected:
    void positionChange(Position);

    ContainerArea    *_containerArea;

protected slots:
    void configurationChanged();
    void immutabilityChanged(bool);
    void slotBuildOpMenu();
    void showConfig();
    void showProcessManager();
    virtual void populateContainerArea();

private:
    TQPopupMenu* _opMnu;
    TQPopupMenu* m_panelAddMenu;
    TQPopupMenu* m_removeMnu;
    TQPopupMenu* m_addExtensionMenu;
    TQPopupMenu* m_removeExtensionMenu;
    TQString _configFile;
    bool m_opMenuBuilt;
};

class MenubarExtension : public PanelExtension
{
    Q_OBJECT

    public:
        MenubarExtension(const AppletInfo& info);
        virtual ~MenubarExtension();

    protected slots:
        virtual void populateContainerArea();

    private:
        MenubarExtension();

        AppletContainer* m_menubar;
};

#endif
