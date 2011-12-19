/*
 * B-II KWin Client
 *
 * Changes:
 * 	Customizable button positions by Karol Szwed <gallium@kde.org>
 * 	Ported to the trinity.2 API by Luciano Montanaro <mikelima@cirulla.net>
 */

#ifndef __B2CLIENT_H
#define __B2CLIENT_H

#include <tqvariant.h>
#include <tqdatetime.h>
#include <tqbutton.h>
#include <tqbitmap.h>
#include <kpixmap.h>
#include <kdecoration.h>
#include <kdecorationfactory.h>

class TQSpacerItem;
class TQBoxLayout;
class TQGridLayout;

namespace B2 {

class B2Client;

class B2Button : public TQButton
{
public:
    B2Button(B2Client *_client=0, TQWidget *parent=0, const TQString& tip=NULL, const int realizeBtns = Qt::LeftButton);
    ~B2Button() {};

    void setBg(const TQColor &c){bg = c;}
    void setPixmaps(KPixmap *pix, KPixmap *pixDown, KPixmap *iPix,
                    KPixmap *iPixDown);
    void setPixmaps(int button_id);
    void setToggle(){setToggleType(Toggle);}
    void setActive(bool on){setOn(on);}
    void setUseMiniIcon(){useMiniIcon = true;}
    TQSize sizeHint() const;
    TQSizePolicy sizePolicy() const;
protected:
    virtual void drawButton(TQPainter *p);
    void drawButtonLabel(TQPainter *){;}

    void mousePressEvent( TQMouseEvent* e );
    void mouseReleaseEvent( TQMouseEvent* e );
private:
    void enterEvent(TQEvent *e);
    void leaveEvent(TQEvent *e);
    
    bool useMiniIcon;
    KPixmap *icon[6];
    TQColor bg; //only use one color (the rest is pixmap) so forget TQPalette ;)

public:
    B2Client* client;
    ButtonState last_button;
    int realizeButtons;
    bool hover;
};

class B2Titlebar : public TQWidget
{
    friend class B2Client;
public:
    B2Titlebar(B2Client *parent);
    ~B2Titlebar(){;}
    bool isFullyObscured() const {return isfullyobscured;}
    void recalcBuffer();
    TQSpacerItem *captionSpacer;
protected:
    void paintEvent( TQPaintEvent* );
    bool x11Event(XEvent *e);
    void mouseDoubleClickEvent( TQMouseEvent * );
    void wheelEvent(TQWheelEvent *);
    void mousePressEvent( TQMouseEvent * );
    void mouseReleaseEvent( TQMouseEvent * );
    void mouseMoveEvent(TQMouseEvent *);
    void resizeEvent(TQResizeEvent *ev);
private:
    void drawTitlebar(TQPainter &p, bool state);

    B2Client *client;
    TQString oldTitle;
    KPixmap titleBuffer;
    TQPoint moveOffset;
    bool set_x11mask;
    bool isfullyobscured;
    bool shift_move;
};

class B2Client : public KDecoration
{
    Q_OBJECT
    friend class B2Titlebar;
public:
    B2Client(KDecorationBridge *b, KDecorationFactory *f);
    ~B2Client(){;}
    void init();
    void unobscureTitlebar();
    void titleMoveAbs(int new_ofs);
    void titleMoveRel(int xdiff);
    // transparent stuff
    virtual bool drawbound(const TQRect& geom, bool clear);
protected:
    void resizeEvent( TQResizeEvent* );
    void paintEvent( TQPaintEvent* );
    void showEvent( TQShowEvent* );
    void windowWrapperShowEvent( TQShowEvent* );
    void captionChange();
    void desktopChange();
    void shadeChange();
    void activeChange();
    void maximizeChange();
    void iconChange();
    void doShape();
    Position mousePosition( const TQPoint& p ) const;
    void resize(const TQSize&);
    void borders(int &, int &, int &, int &) const;
    TQSize minimumSize() const;
    bool eventFilter(TQObject *, TQEvent *);
private slots:
    void menuButtonPressed();
    //void slotReset();
    void maxButtonClicked();
    void shadeButtonClicked();
    void resizeButtonPressed();
private:
    void addButtons(const TQString& s, const TQString tips[],
                    B2Titlebar* tb, TQBoxLayout* titleLayout);
    void positionButtons();
    void calcHiddenButtons();
    bool mustDrawHandle() const;
    bool isModalSystemNotification();
    
    enum ButtonType{BtnMenu=0, BtnSticky, BtnIconify, BtnMax, BtnClose,
        BtnHelp, BtnShade, BtnResize, BtnCount};
    B2Button* button[BtnCount];
    TQGridLayout *g;
    // Border spacers
    TQSpacerItem *topSpacer; 
    TQSpacerItem *bottomSpacer; 
    TQSpacerItem *leftSpacer;
    TQSpacerItem *rightSpacer;
    B2Titlebar *titlebar;
    int bar_x_ofs;
    int in_unobs;
    TQTime time;
    bool resizable;
};

class B2ClientFactory : public TQObject, public KDecorationFactory
{
public:
    B2ClientFactory();
    virtual ~B2ClientFactory();
    virtual KDecoration *createDecoration(KDecorationBridge *);
    virtual bool reset(unsigned long changed);
    virtual bool supports( Ability ability );
    TQValueList< B2ClientFactory::BorderSize > borderSizes() const;
};

}

#endif
