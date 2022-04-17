/***************************************************************************
                          kcmkonsole.h 
                             -------------------
    begin                : mar apr 17 16:44:59 CEST 2001
    copyright            : (C) 2001 by Andrea Rizzi
    email                : rizzi@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KCMKONSOLE_H
#define KCMKONSOLE_H

#include <tdecmodule.h>

#include "kcmkonsoledialog.h"

class TQFont;
class TQStringList;

class KCMKonsole
	: public TDECModule
{
	Q_OBJECT

public:
	KCMKonsole (TQWidget *parent = 0, const char *name = 0, const TQStringList& = 0);
	
	void load();
	void load(bool useDefaults);
	void save();
	void defaults();
private:
	KCMKonsoleDialog *dialog;
	bool xonXoffOrig;
	bool realTransparencyOrig;
	bool bidiOrig;
};

#endif
