#include <tqpainter.h>
#include <tqbitmap.h>
#include <tqdatetime.h>
#include <tqdrawutil.h>
#include <tqlcdnumber.h>
#include <tqtimer.h>

#include <kdatepicker.h>
#include <kglobal.h>
#include <kwin.h>
#include <kiconloader.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kpopupmenu.h>

#include <taskmanager.h>

#include "kaspopup.h"
#include "kastasker.h"

#include "kasclockitem.h"
#include "kasclockitem.moc"

class LCD : public TQLCDNumber
{
public:
    LCD( TQWidget *parent, const char *name=0 )
	: TQLCDNumber(parent,name) {}
    ~LCD() {}

    void draw( TQPainter *p ) { drawContents(p); }
};

KasClockItem::KasClockItem( KasBar *parent )
    : KasItem( parent )
{
    setCustomPopup( true );

    TQTimer *t = new TQTimer( this, "t" );
    connect( t, TQT_SIGNAL( timeout() ), TQT_SLOT( updateTime() ) );
    t->start( 1000 );

    lcd = new LCD( parent );
    lcd->hide();

    lcd->setSizePolicy( TQSizePolicy::Minimum, TQSizePolicy::Minimum );
    lcd->setBackgroundMode( NoBackground );
    lcd->setFrameStyle( TQFrame::NoFrame );
    lcd->setSegmentStyle( TQLCDNumber::Flat );
    lcd->setNumDigits( 5 );
    lcd->setAutoMask( true );
    updateTime();

    connect( this, TQT_SIGNAL(leftButtonClicked(TQMouseEvent *)), TQT_SLOT(togglePopup()) );
    connect( this, TQT_SIGNAL(rightButtonClicked(TQMouseEvent *)), TQT_SLOT(showMenuAt(TQMouseEvent *) ) );
}

KasClockItem::~KasClockItem()
{
    delete lcd;
}

KasPopup *KasClockItem::createPopup()
{
    KasPopup *pop = new KasPopup( this );
    setPopup( pop );

    (void) new KDatePicker( pop );
    pop->adjustSize();

    return pop;
}

void KasClockItem::updateTime()
{
    setText( KGlobal::locale()->formatDate( TQDate::currentDate(), true /* shortFormat */ ) );
    lcd->display( KGlobal::locale()->formatTime( TQTime::currentTime(), false /* includeSecs */, false /* isDuration */) );
    
    update();
}

void KasClockItem::paint( TQPainter *p )
{
    KasItem::paint( p );

    lcd->setGeometry( TQRect( 0, 0, extent(), extent()-15 ) );

    p->save();
    p->translate( 3, 15 );
    lcd->setPaletteForegroundColor( kasbar()->colorGroup().mid() );
    lcd->draw( p );
    p->restore();

    p->save();
    p->translate( 1, 13 );
    lcd->setPaletteForegroundColor( resources()->activePenColor() );
    lcd->draw( p );
    p->restore();
}

void KasClockItem::showMenuAt( TQMouseEvent *ev )
{
    hidePopup();
    showMenuAt( ev->globalPos() );
}

void KasClockItem::showMenuAt( TQPoint p )
{
    mouseLeave();
    kasbar()->updateMouseOver();

    KasTasker *bar = dynamic_cast<KasTasker *> (KasItem::kasbar());
    if ( !bar )
	return;

    KPopupMenu *menu = bar->contextMenu();
    menu->exec( p );
}
