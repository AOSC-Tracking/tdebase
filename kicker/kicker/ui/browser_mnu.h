/*****************************************************************

Copyright (c) 2001 Matthias Elter <elter@kde.org>

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

#ifndef __browser_mnu_h__
#define __browser_mnu_h__

#include <tqmap.h>
#include <tqvaluevector.h>
#include <kpanelmenu.h>
#include <kdirwatch.h>

class PanelBrowserMenu : public KPanelMenu
{
    TQ_OBJECT

public:
    PanelBrowserMenu(TQString path, TQWidget *parent = 0, const char *name = 0, int startid = 0);
  ~PanelBrowserMenu();

    void append(const TQPixmap &pixmap, const TQString &title, const TQString &filename, bool mimecheck);
    void append(const TQPixmap &pixmap, const TQString &title, PanelBrowserMenu *subMenu);

public slots:
    void initialize();

protected slots:
    void slotExec(int id);
    void slotOpenTerminal();
    void slotOpenFileManager();
    void slotMimeCheck();
    void slotClearIfNeeded(const TQString&);
    void slotClear();
    void slotDragObjectDestroyed();

protected:
    void mousePressEvent(TQMouseEvent *);
    void mouseMoveEvent(TQMouseEvent *);
    void dropEvent(TQDropEvent *ev);
    void dragEnterEvent(TQDragEnterEvent *ev);
    void dragMoveEvent(TQDragMoveEvent *);
    void initIconMap();

    TQPoint             _lastpress;
    TQMap<int, TQString> _filemap;
    TQMap<int, bool>    _mimemap;
    TQTimer            *_mimecheckTimer;
    KDirWatch          _dirWatch;
    TQValueVector<PanelBrowserMenu*> _subMenus;

    int                _startid;
    bool               _dirty;

    // With this flag set to 'true' the menu only displays files and
    // directories. i.e. the "Open in File Manager" and "Open in Terminal"
    // entries are not inserted in the menu and its submenus.
    bool               _filesOnly;

    static TQMap<TQString, TQPixmap> *_icons;
};

#endif
