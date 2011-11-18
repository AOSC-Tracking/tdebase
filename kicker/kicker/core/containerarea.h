/*****************************************************************

Copyright (c) 1996-2004 the kicker authors. See file AUTHORS.

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

#ifndef __containerarea_h__
#define __containerarea_h__

#include <tqpixmap.h>
#include <tqptrlist.h>
#include <tqtimer.h>

#include <appletinfo.h>

#include "global.h"
#include "panner.h"
#include "container_base.h"

class KConfig;
class DragIndicator;
class PanelContainer;
class KRootPixmap;

class AppletContainer;
class ContainerAreaLayout;
class AddAppletDialog;

class ContainerArea : public Panner
{
    Q_OBJECT

public:
    ContainerArea( KConfig* config, TQWidget* parent, TQPopupMenu* opMenu, const char* name = 0 );
    ~ContainerArea();

    void initialize(bool useDefaultConfig);
    int position() const;
    KPanelApplet::Direction popupDirection() const;
    bool isImmutable() const;

    const TQWidget* addButton(const AppletInfo& info);
    const TQWidget* addKMenuButton();
    const TQWidget* addDesktopButton();
    const TQWidget* addWindowListButton();
    const TQWidget* addBookmarksButton();
    const TQWidget* addServiceButton(const TQString& desktopFile);
    const TQWidget* addURLButton(const TQString &url);
    const TQWidget* addBrowserButton();
    const TQWidget* addBrowserButton(const TQString &startDir,
                            const TQString& icon = TQString("kdisknav"));
    const TQWidget* addServiceMenuButton(const TQString& relPath);
    const TQWidget* addNonKDEAppButton();
    const TQWidget* addNonKDEAppButton(const TQString &name,
                                      const TQString &description,
                                      const TQString &filePath,
                                      const TQString &icon,
                                      const TQString &cmdLine, bool inTerm);
    const TQWidget* addExtensionButton(const TQString& desktopFile);
    AppletContainer* addApplet(const AppletInfo& info,
                               bool isImmutable = false,
                               int insertionIndex = -1);

    void configure();

    bool inMoveOperation() const { return (_moveAC != 0); }
    int widthForHeight(int height) const;
    int heightForWidth(int width) const;

    const TQPixmap* completeBackgroundPixmap() const;

    BaseContainer::List containers(const TQString& type) const;
    int containerCount(const TQString& type) const;
    TQStringList listContainers() const;
    bool canAddContainers() const;

signals:
    void maintainFocus(bool);

public slots:
    void resizeContents(int w, int h);
    bool removeContainer(BaseContainer* a);
    bool removeContainer(int index);
    void removeContainers(BaseContainer::List containers);
    void takeContainer(BaseContainer* a);
    void setPosition(KPanelExtension::Position p);
    void tqsetAlignment(KPanelExtension::Alignment a);
    void slotSaveContainerConfig();
    void tqrepaint();
    void showAddAppletDialog();
    void addAppletDialogDone();

protected:
    TQString createUniqueId(const TQString& appletType) const;
    void completeContainerAddition(BaseContainer* container,
                                   int insertionIndex = -1);

    bool eventFilter(TQObject*, TQEvent*);
    void mouseMoveEvent(TQMouseEvent*);
    void mouseReleaseEvent(TQMouseEvent *);
    void dragEnterEvent(TQDragEnterEvent*);
    void dragMoveEvent(TQDragMoveEvent*);
    void dragLeaveEvent(TQDragLeaveEvent*);
    void dropEvent(TQDropEvent*);
    void resizeEvent(TQResizeEvent*);
    void viewportResizeEvent(TQResizeEvent*);

    void defaultContainerConfig();
    void loadContainers(const TQStringList& containers);
    void saveContainerConfig(bool layoutOnly = false);

    TQRect availableSpaceFollowing(BaseContainer*);
    void moveDragIndicator(int pos);

    void scrollTo(BaseContainer*);

    void addContainer(BaseContainer* a,
                      bool arrange = false,
                      int insertionIndex = -1);
    void removeAllContainers();

protected slots:
    void autoScroll();
    void updateBackground(const TQPixmap&);
    void setBackground();
    void immutabilityChanged(bool);
    void updateContainersBackground();
    void startContainerMove(BaseContainer*);
    void resizeContents();
    void destroyCachedGeometry();

private:
    BaseContainer::List   m_containers;
    BaseContainer*  _moveAC;
    KPanelExtension::Position	    _pos;
    KConfig*	    _config;
    DragIndicator*  _dragIndicator;
    BaseContainer*  _dragMoveAC;
    TQPoint	    _dragMoveOffset;
    TQPopupMenu*     m_opMenu;
    KRootPixmap*    _rootPixmap;
    bool            _transparent;
    bool            _useBgTheme;
    bool            _bgSet;
    TQPixmap         _completeBg;
    TQTimer          _autoScrollTimer;
    bool            m_canAddContainers;
    bool            m_immutable;
    bool            m_updateBackgroundsCalled;

    TQWidget*             m_contents;
    ContainerAreaLayout* m_layout;
    AddAppletDialog*     m_addAppletDialog;
    TQMap< TQWidget*, TQRect > m_cachedGeometry;
};


class DragIndicator : public TQWidget
{
    Q_OBJECT

public:
    DragIndicator(TQWidget* parent = 0, const char* name = 0);
    ~DragIndicator() {}

    TQSize preferredSize() const { return _preferredSize; }
    void setPreferredSize(const TQSize& size) { _preferredSize = size; }

protected:
    void paintEvent(TQPaintEvent*);
    void mousePressEvent(TQMouseEvent*);

private:
    TQSize _preferredSize;
};

#endif

