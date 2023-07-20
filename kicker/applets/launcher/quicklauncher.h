/*****************************************************************

Copyright (c) 2000 Bill Nagel

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

#ifndef __quicklauncher_h__
#define __quicklauncher_h__

#include <dcopobject.h>
#include <tqimage.h>
#include <tqstring.h>
#include <tqvaluevector.h>
#include <kpanelapplet.h>
#include <map>

#include "flowgridmanager.h"
#include "prefs.h"
#include "quickbutton.h"

class ConfigDlg;
class TQPopupMenu;
class QuickButtonGroup;
class PopularityStatistics;
class TDEAction;

typedef QuickButtonGroup ButtonGroup;

class QuickLauncher: public KPanelApplet, public DCOPObject
{
    TQ_OBJECT
    K_DCOP

k_dcop:
    void serviceStartedByStorageId(TQString starter, TQString storageId);

public:
    enum {DEFAULT_ICON_DIM=QuickButton::DEFAULT_ICON_DIM};
    enum {SIZE_AUTO=0};
    
    struct PopularityInfo {
        float popularity;
    };
    
    QuickLauncher(const TQString& configFile, Type t = Normal, int actions = 0,
                  TQWidget *parent = 0, const char *name = 0);
    ~QuickLauncher();
    int widthForHeight(int height) const;
    int heightForWidth(int width) const;
    void addApp(TQString url, int index, bool manuallyAdded);
    virtual void action(Action a);

public slots:
    void addApp(TQString url, bool manuallyAdded);
    void addAppBeforeManually(TQString url, TQString sender);
    void removeAppManually(QuickButton *button);
    void removeApp(TQString url, bool manuallyRemoved);
    void removeApp(int index, bool manuallyRemoved);
    void removeAppManually(int index);
    void saveConfig();
    void about();
    
protected:
    int findApp(TQString url);
    int findApp(QuickButton *button);
    
    void mousePressEvent(TQMouseEvent *e);
    void resizeEvent(TQResizeEvent*);
    void dragEnterEvent(TQDragEnterEvent *e);
    void dragLeaveEvent(TQDragLeaveEvent *e);
    void dragMoveEvent(TQDragMoveEvent *e);
    void dropEvent(TQDropEvent *e);
    void refreshContents();
    void setRefreshEnabled(bool enable);
    void setConserveSpace(bool conserve_space);
    void setDragEnabled(bool conserve_space);
    
    bool conserveSpace() const { return m_manager->conserveSpace(); }
    bool isDragEnabled() const { return m_settings->dragEnabled(); }
    
    void buildPopupMenu();
    void loadConfig();
    
    void mergeButtons(int index);
    void clearTempButtons();
    int  dimension() const;

protected slots:
    void slotConfigure();
    void slotSettingsDialogChanged();
    void fillRemoveAppsMenu();
    void slotOwnServiceExecuted(TQString serviceMenuId);
    void slotAdjustToCurrentPopularity();
    void slotStickyToggled();

protected:
    void updateInsertionPosToStatusQuo();
    void updateStickyHighlightLayer();
    QuickButton* createButton(TQString url);
    virtual void paintEvent(TQPaintEvent* e);
    virtual void positionChange(Position);
    
    TQPopupMenu *m_popup;
    TQPopupMenu *m_appletPopup;
    TQPopupMenu *m_removeAppsMenu;
    QuickButtonGroup *m_buttons, *m_newButtons, *m_oldButtons, *m_dragButtons;
    int m_space, m_border;
    TQSize m_buttonSize;
    FlowGridManager *m_manager;
    int m_dropLen, m_dropPos, m_minPanelDim;
    bool m_dragAccepted, m_refreshEnabled, m_needsSave, m_needsRefresh;
    std::map<TQString, int> m_appOrdering;
    Prefs* m_settings;
    TDEAction *m_configAction;
    ConfigDlg *m_configDialog;
    PopularityStatistics* m_popularity;
    TQImage m_stickyHighlightLayer;
    TQTimer *m_saveTimer;
};

#endif
