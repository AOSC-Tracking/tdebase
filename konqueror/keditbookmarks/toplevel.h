/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __toplevel_h
#define __toplevel_h

#include <tdemainwindow.h>
#include <kcommand.h>
#include <kbookmark.h>

class KBookmarkManager;
class TDEToggleAction;
class KBookmarkEditorIface;
class ImportCommand;
class BookmarkInfoWidget;
class IKEBCommand;

struct SelcAbilities {
    bool itemSelected:1;
    bool group:1;
    bool root:1;
    bool separator:1;
    bool urlIsEmpty:1;
    bool multiSelect:1;
    bool singleSelect:1;
    bool notEmpty:1;
    bool tbShowState:1;
};

class CmdHistory : public TQObject {
    Q_OBJECT
public:
    CmdHistory(TDEActionCollection *collection);
    virtual ~CmdHistory() { ; }

    void notifyDocSaved();

    void clearHistory();
    void addCommand(KCommand *);
    void didCommand(KCommand *);
    
    //For an explanation see bookmarkInfo::commitChanges()
    void addInFlightCommand(KCommand *);

    static CmdHistory *self();

protected slots:
    void slotCommandExecuted(KCommand *k);

private:
    KCommandHistory m_commandHistory;
    static CmdHistory *s_self;
};

class KBookmark;
class KBookmarkManager;

class CurrentMgr : public TQObject {
    Q_OBJECT
public:
    typedef enum {HTMLExport, OperaExport, IEExport, MozillaExport, NetscapeExport} ExportType;

    static CurrentMgr* self() { if (!s_mgr) { s_mgr = new CurrentMgr(); } return s_mgr; }
    static KBookmark bookmarkAt(const TQString & a);

    KBookmarkManager* mgr() const { return m_mgr; }
    bool showNSBookmarks() const;
    TQString path() const;

    void createManager(const TQString &filename);
    void notifyManagers(KBookmarkGroup grp);
    void notifyManagers();
    bool managerSave();
    void saveAs(const TQString &fileName);
    void doExport(ExportType type, const TQString & path = TQString::null);
    void setUpdate(bool update);

    void reloadConfig();

    static TQString makeTimeStr(const TQString &);
    static TQString makeTimeStr(int);

protected slots:
    void slotBookmarksChanged(const TQString &, const TQString &);

private:
    CurrentMgr() : m_mgr(0), ignorenext(0) { ; }
    KBookmarkManager *m_mgr;
    static CurrentMgr *s_mgr;
    uint ignorenext;
};

class KEBApp : public TDEMainWindow {
    Q_OBJECT
public:
    static KEBApp* self() { return s_topLevel; }

    KEBApp(const TQString & bookmarksFile, bool readonly, const TQString &address, bool browser, const TQString &caption);
    virtual ~KEBApp();

    void updateStatus(TQString url);
    void setActionsEnabled(SelcAbilities);

    void setCancelFavIconUpdatesEnabled(bool);
    void setCancelTestsEnabled(bool);

    void notifyCommandExecuted();
    void findURL(TQString url);

    TQWidget* popupMenuFactory(const char *type) { 
        return factory()->container(type, this); 
    }

    TDEToggleAction* getToggleAction(const char *) const;

    TQString caption() const { return m_caption; }
    bool readonly() const { return m_readOnly; }
    bool browser() const { return m_browser; } 
    bool nsShown() const;

    BookmarkInfoWidget *bkInfo() { return m_bkinfo; }

public slots:
    void updateActions();
    void slotConfigureToolbars();

protected slots:
    void slotClipboardDataChanged();
    void slotNewToolbarConfig();

private:
    static KBookmarkManager* bookmarkManager();

    void resetActions();
    void createActions();

    void updateListView();

    static KEBApp *s_topLevel;
    KBookmarkEditorIface *m_dcopIface;

public: // only temporary
    CmdHistory *m_cmdHistory;
    TQString m_bookmarksFilename;
    TQString m_caption;

    void construct();

private:
    BookmarkInfoWidget *m_bkinfo;

    bool m_canPaste:1;
    bool m_readOnly:1;
    bool m_browser:1;
};

#endif
