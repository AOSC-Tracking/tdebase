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

#include <tqapplication.h>
#include <tqhbox.h>
#include <tqheader.h>
#include <assert.h>

#include "itemview.h"
#include "flipscrollview.h"
#include "kickerSettings.h"

/* Flip scroll steps, as percentage of itemview width to scroll per
 * step.  Assumes the itemview is scrolled in ten steps */

/* slow start, then fast */
//static const double scrollSteps[] = { 0.05, 0.05, 0.1125, 0.1125, 0.1125, 0.1125, 0.1125, 0.1125, 0.1125, 0.1125 };

/* slow fast slow */
//static const double scrollSteps[] = { 0.05, 0.05, 0.13, 0.13, 0.15, 0.13, 0.13, 0.13, 0.05, 0.05 };

/* slow veryfast slow */
static const double scrollSteps[] = { 0.03, 0.03, 0.147, 0.147, 0.147, 0.147, 0.147, 0.147, 0.03, 0.028 };
;

BackFrame::BackFrame( TQWidget *parent )
    : TQFrame( parent ), mouse_inside( false )
{
    setFrameStyle( TQFrame::NoFrame );
    if ( TQApplication::reverseLayout() )
        left_triangle.load( locate( "data", "kicker/pics/right_triangle.png" ) );
    else
        left_triangle.load( locate( "data", "kicker/pics/left_triangle.png" ) );
}

void BackFrame::drawContents( TQPainter *p )
{
    TQColor gray( 230, 230, 230 );
    if ( mouse_inside )
        p->fillRect( 3, 3, width() - 6, height() - 6, colorGroup().color( TQColorGroup::Highlight ) );
    else
        p->fillRect( 3, 3, width() - 6, height() - 6, gray );
    p->setPen( gray.dark(110) );
    p->drawRect( 3, 3, width() - 6, height() - 6 );

    int pixsize = ( width() - 6 ) * 3 / 5;
    TQImage i = left_triangle.convertToImage().smoothScale( pixsize, pixsize );
    TQPixmap tri;
    tri.convertFromImage( i );

    p->drawPixmap( ( width() - tri.width() ) / 2, ( height() - tri.height() ) / 2, tri );
}

void BackFrame::enterEvent( TQEvent *e )
{
    mouse_inside = true;
    update();
}

void BackFrame::leaveEvent( TQEvent *e )
{
    mouse_inside = false;
    update();
}

void BackFrame::mousePressEvent ( TQMouseEvent * e )
{
    emit clicked();
}

FlipScrollView::FlipScrollView( TQWidget * parent, const char * name )
    : TQScrollView( parent, name ), mState( StoppedLeft ), mScrollDirection( 1 ), mShowBack( false )
{
    setVScrollBarMode( TQScrollView::AlwaysOff );
    setHScrollBarMode( TQScrollView::AlwaysOff );
    setFrameStyle( TQFrame::NoFrame );
    mLeftView = new ItemView( this, "left_view" );
    addChild( mLeftView );

    mRightView = new ItemView( this, "right_view" );
    addChild( mRightView );

    mTimer = new TQTimer( this, "mTimer" );
    connect( mTimer, TQT_SIGNAL( timeout() ), TQT_SLOT( slotScrollTimer() ) );

    connect( mLeftView, TQT_SIGNAL( startService(KService::Ptr) ),
             TQT_SIGNAL( startService(KService::Ptr) ) );
    connect( mLeftView, TQT_SIGNAL( startURL(const TQString& ) ),
             TQT_SIGNAL( startURL(const TQString& ) ) );
    connect( mLeftView, TQT_SIGNAL( rightButtonPressed(TQListViewItem*,const TQPoint&,int) ),
             TQT_SIGNAL( rightButtonPressed(TQListViewItem*,const TQPoint&,int) ) );
    connect( mRightView, TQT_SIGNAL( startService(KService::Ptr) ),
             TQT_SIGNAL( startService(KService::Ptr) ) );
    connect( mRightView, TQT_SIGNAL( startURL(const TQString& ) ),
             TQT_SIGNAL( startURL(const TQString& ) ) );
    connect( mRightView, TQT_SIGNAL( rightButtonPressed(TQListViewItem*,const TQPoint&,int) ),
             TQT_SIGNAL( rightButtonPressed(TQListViewItem*,const TQPoint&,int) ) );

    // wild hack to make sure it has correct width
    mLeftView->setVScrollBarMode( TQScrollView::AlwaysOn );
    mRightView->setVScrollBarMode( TQScrollView::AlwaysOn );
    mLeftView->setVScrollBarMode( TQScrollView::Auto );
    mRightView->setVScrollBarMode( TQScrollView::Auto );

    mBackrow = new BackFrame( this );
    mBackrow->resize( 24, 100 );
    connect( mBackrow, TQT_SIGNAL( clicked() ), TQT_SIGNAL( backButtonClicked() ) );
}

ItemView* FlipScrollView::prepareRightMove()
{
    if ( mState != StoppedLeft )
    {
        mTimer->stop();
        ItemView *swap = mLeftView;
        mLeftView = mRightView;
        mRightView = swap;
        moveChild( mLeftView, 0, 0 );
        moveChild( mRightView, width(), 0 );
        mBackrow->hide();
        mRightView->resize( width(), height() );
        mLeftView->resize( width(), height() );
        setContentsPos( 0, 0 );
    }

    mState = StoppedLeft;
    mRightView->clear();
    return mRightView;
}

void FlipScrollView::showBackButton( bool enable )
{
    kdDebug() << "FlipScrollView::showBackButton " << enable << endl;
    mShowBack = enable;
}

ItemView* FlipScrollView::prepareLeftMove(bool clear)
{
    if ( mState != StoppedRight )
    {
        mTimer->stop();
        ItemView *swap = mLeftView;
        mLeftView = mRightView;
        mRightView = swap;
        moveChild( mLeftView, 0, 0 );
        moveChild( mRightView, width(), 0 );
        mRightView->resize( width(), height() );
        mLeftView->resize( width(), height() );
        mBackrow->hide();
        setContentsPos( width(), 0 );
    }

    mState = StoppedRight;
    if (clear)
        mLeftView->clear();
    return mLeftView;
}

void FlipScrollView::viewportResizeEvent ( TQResizeEvent * )
{
    mLeftView->resize( size() );
    mRightView->resize( width() - mBackrow->width(), height() );
    mBackrow->resize( mBackrow->width(), height() );
    resizeContents( width() * 2, height() );
    moveChild( mBackrow, width(), 0 );
    moveChild( mRightView, width() + mBackrow->width(), 0 );
    setContentsPos( 0, 0 );
}

ItemView *FlipScrollView::currentView() const
{
    if ( mState == StoppedRight )
        return mRightView;
    else
        return mLeftView;
}

ItemView *FlipScrollView::leftView() const
{
    return mLeftView;
}

ItemView *FlipScrollView::rightView() const
{
    return mRightView;
}

FlipScrollView::~FlipScrollView() {}

static const int max_steps = 10;

void FlipScrollView::slotScrollTimer()
{
    mStepsRemaining--;
    assert( mStepsRemaining >= 0 && mStepsRemaining < int(sizeof(  scrollSteps ) / sizeof( double )) );
    if (KickerSettings::scrollFlipView())
      scrollBy( ( int )( mScrollDirection * mLeftView->width() * scrollSteps[ mStepsRemaining ] ), 0 );
    else
      scrollBy( ( int )( mScrollDirection * mLeftView->width()), 0 );

    if ( mStepsRemaining == 0 )
    {
        if ( mState == ScrollingRight )
        {
            mState = StoppedRight;
            setContentsPos( width(), 0 );
        } else {
            mState = StoppedLeft;
            setContentsPos( 0, 0 );
        }

        kdDebug() << "slotScrollTimer " << mShowBack << endl;

        if ( mShowBack )
        {
            mBackrow->show();
            if ( mState == StoppedRight )
            {
            	
                if ( TQApplication::reverseLayout() )
                    moveChild( mRightView, width(), 0 );
                else
                    moveChild( mRightView, width() + mBackrow->width(), 0 );
                mRightView->resize( width() - mBackrow->width(), height() );
                mLeftView->resize( width(), height() );
                if ( TQApplication::reverseLayout() )
                    moveChild( mBackrow, width() + mRightView->width(), 0 );
                else
                    moveChild( mBackrow, width(), 0 );
                moveChild( mLeftView, 0, 0 );
            } else
            {
                moveChild( mRightView, width(), 0 );
                mRightView->resize( width(), height() );
                mLeftView->resize( width() - mBackrow->width(), height() );
                if ( TQApplication::reverseLayout() )
                {
                    moveChild( mBackrow, mLeftView->width(), 0 );
                    moveChild( mLeftView, 0, 0 );
                }
                else
                {
                    moveChild( mBackrow, 0, 0 );
                    moveChild( mLeftView, mBackrow->width(), 0 );
                }
            }
        } else
            mBackrow->hide();

        if (!mSelectMenuPath.isEmpty()) {
            if (mSelectMenuPath=="kicker:/goup/") {
                currentView()->setSelected(currentView()->firstChild(),true);
                currentView()->firstChild()->tqrepaint();
            }
            else {
                TQListViewItem * child = currentView()->firstChild();
                while( child ) {
                    KMenuItem* kitem = dynamic_cast<KMenuItem*>(child);
                    if (kitem && kitem->menuPath()==mSelectMenuPath) {
                        currentView()->setSelected(child,true);
                        kdDebug() << "child tqrepaint\n";
                        child->tqrepaint();
                        break;
                    }
                    child = child->nextSibling();
                }
            }
        }
        mLeftView->setVScrollBarMode( TQScrollView::Auto );
        mRightView->setVScrollBarMode( TQScrollView::Auto );
        mTimer->stop();
	mLeftView->setMouseMoveSelects( true );
	mRightView->setMouseMoveSelects( true );
    }
}

void FlipScrollView::flipScroll(const TQString& selectMenuPath)
{
    if ( mState == StoppedLeft )
    {
        mState = ScrollingRight;
        mScrollDirection = 1;
    }
    else
    {
        mState = ScrollingLeft;
        mScrollDirection = -1;
    }

    mLeftView->setVScrollBarMode( TQScrollView::AlwaysOff );
    mRightView->setVScrollBarMode( TQScrollView::AlwaysOff );
    if (KickerSettings::scrollFlipView())
      mStepsRemaining = max_steps;
    else
      mStepsRemaining = 1;
    mTimer->start( 30 );
    mSelectMenuPath = selectMenuPath;
    if (!mSelectMenuPath.isEmpty()) {
	mLeftView->setMouseMoveSelects( false );
	mRightView->setMouseMoveSelects( false );
    }
}

#include "flipscrollview.moc"
