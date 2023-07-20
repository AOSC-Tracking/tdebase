#ifndef KONSOLEBOOKMARKMENU_H
#define KONSOLEBOOKMARKMENU_H

#include <tqptrlist.h>
#include <tqptrstack.h>
#include <tqobject.h>
#include <sys/types.h>
#include <kbookmark.h>
#include <kbookmarkmenu.h>

#include "konsolebookmarkhandler.h"


class TQString;
class KBookmark;
class TDEAction;
class TDEActionMenu;
class TDEActionCollection;
class KBookmarkOwner;
class KBookmarkMenu;
class TDEPopupMenu;
class KonsoleBookmarkMenu;

class KonsoleBookmarkMenu : public KBookmarkMenu
{
    TQ_OBJECT

public:
    KonsoleBookmarkMenu( KBookmarkManager* mgr,
                         KonsoleBookmarkHandler * _owner, TDEPopupMenu * _parentMenu,
                         TDEActionCollection *collec, bool _isRoot,
                         bool _add = true, const TQString & parentAddress = "");

    void fillBookmarkMenu();

public slots:

signals:

private slots:

private:
    KonsoleBookmarkHandler * m_kOwner;

protected slots:
    void slotAboutToShow2();
    void slotBookmarkSelected();
    void slotNSBookmarkSelected();

protected:
    void refill();

private:
    class KonsoleBookmarkMenuPrivate;
    KonsoleBookmarkMenuPrivate *d;
};

#endif // KONSOLEBOOKMARKMENU_H
