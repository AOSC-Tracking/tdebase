/*
 *
 * Redmond KWin client
 *
 * Copyright 2001-2003
 *   Ported to twin_iii by Chris Lee <clee@kde.org>
 *   Karol Szwed <gallium@kde.org>
 *   http://gallium.n3.net/
 *
 * Based on the default KWin client.
 *
 * Updated to support the new API 9/2003 (CL)
 * Updated to emulate More Accurately 9/2003 (CL)
 * Updated to support toolwindows 3/2001 (KS)
 *
 */

#ifndef __KDE_REDMOND_H
#define __KDE_REDMOND_H

#include <tqbitmap.h>
#include <kpixmap.h>
#include <kcommondecoration.h>
#include <kdecorationfactory.h>

namespace Redmond {

class RedmondDeco;

class RedmondButton : public KCommonDecorationButton
{
	TQ_OBJECT
public:
	RedmondButton(ButtonType type, RedmondDeco *parent, const char *name);
	void setBitmap(const unsigned char *bitmap);
	void setPixmap(const TQPixmap &p);
	void reset(unsigned long changed);

protected:
	virtual void drawButton(TQPainter *p);
	void drawButtonLabel(TQPainter *){;}

	TQBitmap  deco;
	TQPixmap  pix;
	bool     miniBtn;
};


class RedmondDeco : public KCommonDecoration
{
public:
	RedmondDeco(KDecorationBridge *, KDecorationFactory *);
	~RedmondDeco() {;}

	virtual TQString visibleName() const;
	virtual TQString defaultButtonsLeft() const;
	virtual TQString defaultButtonsRight() const;
	virtual bool decorationBehaviour(DecorationBehaviour behaviour) const;
	virtual int layoutMetric(LayoutMetric lm, bool respectWindowState = true, const KCommonDecorationButton * = 0) const;
	virtual KCommonDecorationButton *createButton(ButtonType type);

	void init();

protected:
    virtual void reset( unsigned long changed );

	void paintEvent(TQPaintEvent*);

private:
	int            titleHeight;
};

class RedmondDecoFactory : public TQObject, public KDecorationFactory
{
   TQ_OBJECT
public:
	RedmondDecoFactory();
	virtual ~RedmondDecoFactory();
	virtual KDecoration *createDecoration(KDecorationBridge *);
	virtual bool reset(unsigned long);
	virtual bool supports( Ability ability );
	virtual TQValueList< BorderSize > borderSizes() const;
private:
	void readConfig();
};

}

#endif
