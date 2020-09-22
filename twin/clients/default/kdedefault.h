/*
 *
 *	KDE2 Default KWin client
 *
 *	Copyright (C) 1999, 2001 Daniel Duley <mosfet@kde.org>
 *	Matthias Ettrich <ettrich@kde.org>
 *	Karol Szwed <gallium@kde.org>
 *
 *	Draws mini titlebars for tool windows.
 *	Many features are now customizable.
 */

#ifndef _KDE_DEFAULT_H
#define _KDE_DEFAULT_H

#include <tqbutton.h>
#include <tqbitmap.h>
#include <tqdatetime.h>
#include <kpixmap.h>
#include <kcommondecoration.h>
#include <kdecorationfactory.h>

class TQSpacerItem;
class TQBoxLayout;
class TQGridLayout;

namespace Default {

class KDEDefaultClient;

class KDEDefaultHandler: public KDecorationFactory
{
	public:
		KDEDefaultHandler();
		~KDEDefaultHandler();
                KDecoration* createDecoration( KDecorationBridge* b );
		bool reset( unsigned long changed );
		virtual TQValueList< BorderSize > borderSizes() const;
		virtual bool supports( Ability ability );

	private:
		unsigned long readConfig( bool update );
		void createPixmaps();
		void freePixmaps();
		void drawButtonBackground(KPixmap *pix,
				const TQColorGroup &g, bool sunken);
};


// class KDEDefaultButton : public TQButton, public KDecorationDefines
class KDEDefaultButton : public KCommonDecorationButton
{
	public:
		KDEDefaultButton(ButtonType type, KDEDefaultClient *parent, const char *name);
		~KDEDefaultButton();

		void reset(unsigned long changed);

		void setBitmap(const unsigned char *bitmap);

	protected:
		void enterEvent(TQEvent *);
		void leaveEvent(TQEvent *);
		void drawButton(TQPainter *p);
		void drawButtonLabel(TQPainter*) {;}

		TQBitmap* deco;
		bool    large;
		bool	isMouseOver;
};


class KDEDefaultClient : public KCommonDecoration
{
	public:
		KDEDefaultClient( KDecorationBridge* b, KDecorationFactory* f );
		~KDEDefaultClient() {;}

		virtual TQString visibleName() const;
		virtual TQString defaultButtonsLeft() const;
		virtual TQString defaultButtonsRight() const;
		virtual bool decorationBehaviour(DecorationBehaviour behaviour) const;
		virtual int layoutMetric(LayoutMetric lm, bool respectWindowState = true, const KCommonDecorationButton * = 0) const;
		virtual KCommonDecorationButton *createButton(ButtonType type);

		virtual TQRegion cornerShape(WindowCorner corner);

		void init();
		void reset( unsigned long changed );

	protected:
		void paintEvent( TQPaintEvent* );

	private:
		bool mustDrawHandle() const;
		int           titleHeight;
};

}

#endif
