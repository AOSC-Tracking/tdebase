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

#ifndef SERVICE_MENU_H
#define SERVICE_MENU_H

#include <tqmap.h>
#include <set>
#include <tqvaluevector.h>

#include <ksycocaentry.h>
#include <kservice.h>
#include <kpanelmenu.h>
#include <kservicegroup.h>
/**
 * PanelServiceMenu is filled with KDE services and service groups. The sycoca
 * database is queried and the hierarchical structure built by creating child
 * menus of type PanelServiceMenu, so the creation is recursive.
 *
 * The entries are sorted alphabetically and groups come before services.
 *
 * @author Rik Hemsley <rik@kde.org>
 */

class KLineEdit;
typedef TQMap<int, KSycocaEntry::Ptr> EntryMap;
typedef TQValueVector<TQPopupMenu*> PopupMenuList;
class PanelServiceMenu;
typedef TQMap<PanelServiceMenu*,int> PanelServiceMenuMap;

class KDE_EXPORT PanelServiceMenu : public KPanelMenu
{
    Q_OBJECT

public:
    PanelServiceMenu(const TQString & label, const TQString & relPath,
                     TQWidget* parent  = 0, const char* name = 0,
                     bool addmenumode = false,
                     const TQString &insertInlineHeader = TQString::null);

    virtual ~PanelServiceMenu();

    TQString relPath() { return relPath_; }

    void setExcludeNoDisplay( bool flag );

    virtual void showMenu();
    bool highlightMenuItem( const TQString &menuId );
    void selectFirstItem();
    void setSearchString(const TQString& searchString);
    bool hasSearchResults();

private:
    void fillMenu( KServiceGroup::Ptr &_root, KServiceGroup::List &_list,
        const TQString &_relPath, int & id );

protected slots:
    virtual void initialize();
    virtual void slotExec(int id);
    virtual void slotClearOnClose();
    virtual void slotClear();
    virtual void configChanged();
    virtual void slotClose();
    void slotSetTooltip(int id);
    void slotDragObjectDestroyed();

    // for use in Add Applicaton To Panel
    virtual void addNonKDEApp() {}

protected:
    void insertMenuItem(KService::Ptr & s, int nId, int nIndex = -1,
                        const TQStringList *suppressGenericNames=0,
                        const TQString &aliasname = TQString::null,
                        const TQString &label = TQString::null, const TQString &categoryIcon = TQString::null);
    virtual PanelServiceMenu * newSubMenu(const TQString & label,
                                          const TQString & relPath,
                                          TQWidget * parent, const char * name,
                                          const TQString & _inlineHeader =
                                                TQString::null);

    virtual void mousePressEvent(TQMouseEvent *);
    virtual void mouseReleaseEvent(TQMouseEvent *);
    virtual void mouseMoveEvent(TQMouseEvent *);
    virtual void dragEnterEvent(TQDragEnterEvent *);
    virtual void dragLeaveEvent(TQDragLeaveEvent *);
    virtual void updateRecentlyUsedApps(KService::Ptr &s);
    void activateParent(const TQString &child);
    int serviceMenuStartId() { return 4242; }
    int serviceMenuEndId() { return 5242; }
    virtual void clearSubmenus();
    void doInitialize();

    TQString relPath_;

    EntryMap entryMap_;

    bool loaded_;
    bool excludeNoDisplay_;
    TQString insertInlineHeader_;
    TQPopupMenu * opPopup_;
    bool clearOnClose_;
    bool addmenumode_;
    TQPoint startPos_;
    PopupMenuList subMenus;
    PanelServiceMenuMap searchSubMenuIDs;
    bool hasSearchResults_;
    std::set<int> searchMenuItems;

private slots:
    void slotContextMenu(int);

private:
    enum ContextMenuEntry { AddItemToPanel, EditItem, AddMenuToPanel, EditMenu,
                            AddItemToDesktop, AddMenuToDesktop, PutIntoRunDialog };
    KPopupMenu* popupMenu_;
    KSycocaEntry* contextKSycocaEntry_;
    void readConfig();
 };

#endif // SERVICE_MENU_H
