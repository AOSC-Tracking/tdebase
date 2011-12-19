/*****************************************************************

Copyright (c) 2006 Dirk Mueller <mueller@kde.org>

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

#ifndef __k_mnu_stub_h__
#define __k_mnu_stub_h__

#include <tqstring.h>
#include <tqpoint.h>

class KickerClientMenu;
class KMenu;
class PanelKMenu;




class KMenuStub
{
public:
    KMenuStub(KMenu* _kmenu) 
      : m_type(t_KMenu)  { m_w.kmenu = _kmenu; }
    KMenuStub(PanelKMenu* _panelkmenu) 
      : m_type(t_PanelKMenu) { m_w.panelkmenu = _panelkmenu; }
    ~KMenuStub() {} 

    void removeClientMenu(int id);
    int insertClientMenu(KickerClientMenu *p);
    void adjustSize();
    void hide();
    void show();
    void showMenu();
    void resize();
    void popup(const TQPoint &pos, int indexAtPoint = -1);
    void selectFirstItem();
    void resize(int, int);
    TQSize sizeHint() const;
    bool highlightMenuItem( const TQString &menuId );
    void clearRecentMenuItems();
    void initialize();

    TQWidget* widget();

    bool isVisible() const;
private:
     enum {t_PanelKMenu, t_KMenu} m_type;
     union {
        KMenu* kmenu;
        PanelKMenu* panelkmenu;
     } m_w;
};

#endif
