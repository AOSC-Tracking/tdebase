/*
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 * 
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#ifndef __PixmapServer_h_Included__
#define __PixmapServer_h_Included__

#include <tqwindowdefs.h>

#include <tqwidget.h>
#include <tqmap.h>

#include <X11/X.h>
#include <X11/Xlib.h>

/**
 * Used internally by KPixmapServer.
 */

struct KPixmapInode 
{
    Qt::HANDLE handle;
    Atom selection;
};

struct KPixmapData
{
    TQPixmap *pixmap;
    int usecount;
    int refcount;
};

struct TDESelectionInode
{
    Qt::HANDLE handle;
    TQString name;
};

/**
 * KPixmapServer: Share pixmaps between X clients with deletion and
 * multi-server capabilities. 
 * The sharing is implemented using X11 Selections.
 *
 * @author Geert Jansen <g.t.jansen@stud.tue.nl>
 */
class KPixmapServer: public TQWidget
{
    TQ_OBJECT

public:
    KPixmapServer();
    ~KPixmapServer();

    /**
     * Adds a pixmap to this server. This will make it available to all
     * other X clients on the current display.
     *
     * You must never delete a pixmap that you add()'ed. The pixmap is 
     * deleted when you call remove() and after all clients have stopped 
     * using it.
     *
     * You can add the same pixmap under multiple names.
     *
     * @param name An X11-wide unique identifier for the pixmap.
     * @param pm A pointer to the pixmap.
     * @param overwrite Should an pixmap with the same name be overwritten?
     */
    void add(TQString name, TQPixmap *pm, bool overwrite=true);

    /**
     * Remove a pixmap from the server. This will delete the pixmap after 
     * all clients have stopped using it.
     *
     * @param name The name of the shared pixmap.
     */
    void remove(TQString name);

    /**
     * List all pixmaps currently served by this server.
     * 
     * @return A TQStringList containing all the shared pixmaps.
     */
    TQStringList list();

    /**
     * Re-set ownership of the selection providing the shared pixmap.
     *
     * @param name The name of the shared pixmap.
     */
    void setOwner(TQString name);

signals:
    /** 
     * This signal is emitted when the selection providing the named pixmap
     * is disowned. This means that said pixmap won't be served anymore by 
     * this server, though it can be served by another. You can re-aqcuire
     * the selection by calling setOwner().
     */
    void selectionCleared(TQString name);

protected:
    bool x11Event(XEvent *);

private:
    Atom pixmap;

    TQMap<TQString,KPixmapInode> m_Names;
    TQMap<Atom,TDESelectionInode> m_Selections;
    TQMap<HANDLE,KPixmapData> m_Data;
    TQMap<Atom,HANDLE> m_Active;

    typedef TQMap<TQString,KPixmapInode>::Iterator NameIterator;
    typedef TQMap<Atom,TDESelectionInode>::Iterator SelectionIterator;
    typedef TQMap<HANDLE,KPixmapData>::Iterator DataIterator;
    typedef TQMap<Atom,HANDLE>::Iterator AtomIterator;
};


#endif // __PixmapServer_h_Included__
