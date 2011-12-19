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

#ifndef __panner_h__
#define __panner_h__

#include <tqwidget.h>

#include "simplebutton.h"

class TQBoxLayout;
class TQTimer;

class KDE_EXPORT Panner : public TQWidget
{
    Q_OBJECT

public:
    Panner( TQWidget* parent, const char* name = 0 );
    ~Panner();

    TQSize minimumSizeHint() const { return TQWidget::minimumSizeHint(); }

    Qt::Orientation orientation() const { return _orient; }
    virtual void setOrientation(Qt::Orientation orientation);
    
    TQWidget *viewport() const { return _viewport; }
    
    TQRect contentsRect() const { return TQRect(0, 0, width(), height()); }
    
    int contentsX() const { return _viewport ? -_viewport->x() : 0; }
    int contentsY() const { return _viewport ? -_viewport->y() : 0; }
    int contentsWidth() const { return _viewport ? _viewport->width() : 0; }
    int contentsHeight() const { return _viewport ? _viewport->height() : 0; }
    void setContentsPos(int x, int y);
    
    int visibleWidth() const { return _clipper->width(); }
    int visibleHeight() const { return _clipper->height(); }
    
    void	contentsToViewport( int x, int y, int& vx, int& vy ) const;
    void	viewportToContents( int vx, int vy, int& x, int& y ) const;
    TQPoint	contentsToViewport( const TQPoint& ) const;
    TQPoint	viewportToContents( const TQPoint& ) const;
    
    void addChild(TQWidget *child) { child->show(); }
    void removeChild(TQWidget *child) { child->hide(); }
    int childX(TQWidget *child) const { return child->x(); }
    int childY(TQWidget *child) const { return child->y(); }
    void moveChild(TQWidget *child, int x, int y) { child->move(x, y); }

    void ensureVisible( int x, int y );
    void ensureVisible( int x, int y, int xmargin, int ymargin );

public slots:
    virtual void resizeContents( int w, int h );
    void startScrollRightDown();
    void startScrollLeftUp();
    void stopScroll();
    void scrollRightDown();
    void scrollLeftUp();
    void reallyUpdateScrollButtons();
    void scrollBy(int dx, int dy);

signals:
    void contentsMoving(int x, int y);

protected:
    virtual bool eventFilter( TQObject *obj, TQEvent *e );
    virtual void resizeEvent(TQResizeEvent *ev);
    virtual void viewportResizeEvent( TQResizeEvent* );
    virtual void viewportMousePressEvent( TQMouseEvent* );
    virtual void viewportMouseReleaseEvent( TQMouseEvent* );
    virtual void viewportMouseDoubleClickEvent( TQMouseEvent* );
    virtual void viewportMouseMoveEvent( TQMouseEvent* );

private:
    void setupButtons();
    void createScrollButtons();
    void updateScrollButtons();

    Orientation       _orient;
    TQBoxLayout       *_layout;
    SimpleArrowButton *_luSB; // Left Scroll Button
    SimpleArrowButton *_rdSB; // Right Scroll Button
    TQTimer *_updateScrollButtonsTimer;
    TQTimer *_scrollTimer;
    
    TQWidget *_clipper;
    TQWidget *_viewport;
    int _cwidth, _cheight;
    int _cx, _cy;
    int _step;
};

#endif
