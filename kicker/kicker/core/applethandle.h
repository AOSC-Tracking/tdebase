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

#ifndef __applethandle_h__
#define __applethandle_h__

#include <tqwidget.h>
#include <tqpushbutton.h>

#include "container_applet.h"
#include "simplebutton.h"

class TQBoxLayout;
class TQTimer;
class AppletHandleDrag;
class AppletHandleButton;

class AppletHandle : public TQWidget
{
    Q_OBJECT
    
    public:
        AppletHandle(AppletContainer* parent);

        void resetLayout();
        void setFadeOutHandle(bool);

        bool eventFilter (TQObject *, TQEvent *);

        int widthForHeight( int h ) const;
        int heightForWidth( int w ) const;

        void setPopupDirection(KPanelApplet::Direction);
        KPanelApplet::Direction popupDirection() const
        {
            return m_popupDirection;
        }

        KPanelApplet::Orientation orientation() const
        {
            return m_applet->orientation();
        }

        bool onMenuButton(const TQPoint& point) const;

    signals:
        void moveApplet( const TQPoint& moveOffset );
        void showAppletMenu();

    public slots:
        void toggleMenuButtonOff();

    protected slots:
        void menuButtonPressed();
        void checkHandleHover();

    private:
        AppletContainer* m_applet;
        TQBoxLayout* m_layout;
        AppletHandleDrag* m_dragBar;
        AppletHandleButton* m_menuButton;
        bool m_drawHandle;
        KPanelApplet::Direction m_popupDirection;
        TQTimer* m_handleHoverTimer;
        bool m_inside;
};

class AppletHandleDrag : public TQWidget
{
    Q_OBJECT
    
    public:
        AppletHandleDrag(AppletHandle* parent);

        TQSize minimumSizeHint() const;
        TQSize minimumSize() const { return minimumSizeHint(); }
        TQSize sizeHint() const { return minimumSize(); }
        TQSizePolicy sizePolicy() const;

    protected:
        void paintEvent( TQPaintEvent* );
        void enterEvent( TQEvent* );
        void leaveEvent( TQEvent* );
        const AppletHandle* m_parent;
    
    private:
        bool m_inside;
};

class AppletHandleButton : public SimpleArrowButton
{
    Q_OBJECT
    
    public:
        AppletHandleButton(AppletHandle *parent);
        TQSize minimumSizeHint() const;
        TQSize minimumSize() const { return minimumSizeHint(); }
        TQSize sizeHint() const { return minimumSize(); }
        TQSizePolicy sizePolicy() const;

    private:
        bool m_moveMouse;
        const AppletHandle* m_parent;
};

#endif
