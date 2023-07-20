//-----------------------------------------------------------------------------
//
// kblankscrn - Basic screen saver for KDE
//
// Copyright (c)  Martin R. Jones 1996
//

#ifndef __BLANKSCRN_H__
#define __BLANKSCRN_H__

#include <tqcolor.h>
#include <kdialogbase.h>
#include <tdescreensaver.h>

class KColorButton;


class KBlankSaver : public KScreenSaver
{
	TQ_OBJECT
public:
	KBlankSaver( WId drawable );
	virtual ~KBlankSaver();

	void setColor( const TQColor &col );

private:
	void readSettings();
	void blank();

private:
	TQColor color;
};

class KBlankSetup : public KDialogBase
{
	TQ_OBJECT
public:
	KBlankSetup( TQWidget *parent = NULL, const char *name = NULL );

protected:
	void readSettings();

private slots:
	void slotColor( const TQColor & );
	void slotOk();

private:
	TQWidget *preview;
	KBlankSaver *saver;

	TQColor color;
};

#endif

