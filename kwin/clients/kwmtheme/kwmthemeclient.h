#ifndef __KWMTHEMECLIENT_H
#define __KWMTHEMECLIENT_H

#include <tqbutton.h>
#include <tqtoolbutton.h>
#include <kpixmap.h>
#include <kdecoration.h>
#include <kdecorationfactory.h>

class TQLabel;
class TQSpacerItem;
class TQGridLayout;

namespace KWMTheme {

class MyButton : public TQToolButton
{
public:
    MyButton(TQWidget *parent=0, const char *name=0)
        : TQToolButton(parent, name){setAutoRaise(true);setCursor( arrowCursor ); }
protected:
    void drawButtonLabel(TQPainter *p);
};

class KWMThemeClient : public KDecoration
{
    Q_OBJECT
public:
    KWMThemeClient( KDecorationBridge* b, KDecorationFactory* f );
    ~KWMThemeClient(){;}
    void init();
    void resize( const TQSize& s );
    TQSize minimumSize() const;
    void borders( int& left, int& right, int& top, int& bottom ) const;
protected:
    void doShape();
    void drawTitle(TQPainter &p);
    void resizeEvent( TQResizeEvent* );
    void paintEvent( TQPaintEvent* );
    void showEvent( TQShowEvent* );
    void mouseDoubleClickEvent( TQMouseEvent * );
    bool eventFilter( TQObject* o, TQEvent* e );
    void captionChange();
    void desktopChange();
    void maximizeChange();
    void iconChange();
    void activeChange();
    void shadeChange() {};
    Position mousePosition(const TQPoint &) const;
protected slots:
    //void slotReset();
    void menuButtonPressed();
    void slotMaximize();
private:
    TQPixmap buffer;
    KPixmap *aGradient, *iGradient;
    MyButton *maxBtn, *stickyBtn, *mnuBtn;
    TQSpacerItem *titlebar;
    TQGridLayout* layout;
};

class KWMThemeFactory : public KDecorationFactory
{
public:
    KWMThemeFactory();
    ~KWMThemeFactory();
    KDecoration* createDecoration( KDecorationBridge* b );
    bool reset( unsigned long mask );
};

}

#endif

