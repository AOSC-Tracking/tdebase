//
// A special widget which draws a sample of KDE widgets
// It is used to preview color schemes
//
// Copyright (c)  Mark Donohoe 1998
//

#ifndef __WIDGETCANVAS_H__
#define __WIDGETCANVAS_H__

#include <tqmap.h>
#undef None	// Qt4
#include <tdeapplication.h>

#define MAX_HOTSPOTS   28
#define SCROLLBAR_SIZE 16

// These defines define the order of the colors in the combo box.
#define CSM_Standard_background		0
#define CSM_Standard_text		1
#define CSM_Select_background		2
#define CSM_Select_text			3
#define CSM_Link			4
#define CSM_Followed_Link		5
#define CSM_Background			6
#define CSM_Text			7
#define CSM_Button_background		8
#define CSM_Button_text			9
#define CSM_Active_title_bar		10
#define CSM_Active_title_text		11
#define CSM_Active_title_blend		12
#define CSM_Active_title_button		13
#define CSM_Inactive_title_bar		14
#define CSM_Inactive_title_text		15
#define CSM_Inactive_title_blend	16
#define CSM_Inactive_title_button	17
#define CSM_Active_frame		18
#define CSM_Active_handle		19
#define CSM_Inactive_frame		20
#define CSM_Inactive_handle		21
#define CSM_Alternate_background        22
#define CSM_LAST			23

class TQPixmap;
class TQColor;
class TQPainter;
class TQEvent;

class KPixmap;

class HotSpot
{
public:
    HotSpot() {}
    HotSpot( const TQRect &r, int num )
	: rect(r), number(num) {}

    TQRect rect;
    int number;
};

class WidgetCanvas : public TQWidget
{
    TQ_OBJECT

public:
    WidgetCanvas( TQWidget *parent=0, const char *name=0 );
    void drawSampleWidgets();
    void resetTitlebarPixmaps(const TQColor &active,
			      const TQColor &inactive);
    void addToolTip( int area, const TQString & );
    TQPixmap smplw;
    
    TQColor iaTitle;
    TQColor iaTxt;
    TQColor iaBlend;
    TQColor iaFrame;
    TQColor iaHandle;
    TQColor aTitle;
    TQColor aTxt;
    TQColor aBlend;
    TQColor aFrame;
    TQColor aHandle;
    TQColor back;
    TQColor txt;
    TQColor select;
    TQColor selectTxt;
    TQColor window;
    TQColor windowTxt;
    TQColor button;
    TQColor buttonTxt;
    TQColor aTitleBtn;
    TQColor iTitleBtn;
    TQColor link;
    TQColor visitedLink;
    TQColor alternateBackground;

    int contrast;
    bool shadeSortColumn;

signals:
    void widgetSelected( int );
    void colorDropped( int, const TQColor&);
	
protected:
	void redrawPopup(const TQColorGroup &cg);
	
    virtual void paintEvent( TQPaintEvent * );
    virtual void mousePressEvent( TQMouseEvent * );
    virtual void mouseMoveEvent( TQMouseEvent * );
    virtual void resizeEvent( TQResizeEvent * );
    virtual void showEvent( TQShowEvent * );
    virtual void dropEvent( TQDropEvent *);
    virtual void dragEnterEvent( TQDragEnterEvent *);
    void paletteChange( const TQPalette & );

    TQMap<int,TQString> tips;
    HotSpot hotspots[MAX_HOTSPOTS];
    int currentHotspot;
};

#endif
