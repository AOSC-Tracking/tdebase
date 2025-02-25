/*****************************************************************

Copyright (c) 2006 Will Stephenson <wstephenson@novell.com>

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

/*
 * Flip scroll menu
 * Each level of the menu is a separate TQListView
 * Child items are added to their own TQListView.
 * When a parent is clicked, we look up its child menu and insert
 * that in a TQScrollView, then scroll to it.
 *
 * Need to intercept TQListViewItems' parent param and instead of
 * inserting directly into parent, insert into parent item's listview
 *
 * So need
 * - adapted QLVI
 * - wrap QLV and offer same interface
 */

#ifndef FLIPSCROLLVIEW_H
#define FLIPSCROLLVIEW_H

#include <tqscrollview.h>
#include <tqlistview.h>
#include <tqframe.h>
#include <tqtimer.h>
#include <tqpainter.h>
#include <tdestandarddirs.h>
#include "service_mnu.h"

class ItemView;

class BackFrame : public TQFrame
{
    TQ_OBJECT

public:
    BackFrame( TQWidget *parent );
    virtual void drawContents( TQPainter *p );

    void enterEvent ( TQEvent * );
    void leaveEvent( TQEvent * );
    void mousePressEvent ( TQMouseEvent * e );

signals:
    void clicked();

private:
    TQPixmap left_triangle;
    bool mouse_inside;
};

class FlipScrollView : public TQScrollView
{
    TQ_OBJECT
public:
    enum State{ StoppedLeft, StoppedRight, ScrollingLeft, ScrollingRight };
    FlipScrollView( TQWidget * parent = 0, const char * name = 0 );
    ~FlipScrollView();

    ItemView *currentView() const;
    ItemView *leftView() const;
    ItemView *rightView() const;
    ItemView *prepareLeftMove(bool clear=true);
    ItemView *prepareRightMove();

    void flipScroll(const TQString& selectMenuPath = TQString::null);
    void showBackButton(bool enable);
    bool showsBackButton() const {return mShowBack;}

protected slots:
    void slotScrollTimer();

signals:
    void startService(KService::Ptr kservice);
    void startURL(const TQString& u);
    void rightButtonPressed(TQListViewItem*,const TQPoint&,int);
    void backButtonClicked();

protected:
    void viewportResizeEvent ( TQResizeEvent * );

private:
    ItemView * mLeftView;
    ItemView * mRightView;
//  ItemView * mCurrentView;
    int mStepsRemaining;
    State mState;
    TQTimer * mTimer;
    BackFrame *mBackrow;
    TQString mSelectMenuPath;
    int mScrollDirection;
    bool mShowBack;
};




#endif
