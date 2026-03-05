/*
 *  Copyright (c) 2000 Matthias Hölzer-Klüpfel <hoelzer@kde.org>
                  2000 Carsten Pfeiffer <pfeiffer@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#ifndef __swallow_h__
#define __swallow_h__


#include <tqevent.h>
#include <tqptrlist.h>
#include <qxembed.h>

#include <kpanelapplet.h>

class TQBoxLayout;
class TDEConfig;
class TDEProcess;
class TWinModule;

class SwallowApp;

typedef struct _SwallowCommand {
    TQString cmdline;
    TQString title;
} SwallowCommand;

typedef TQPtrList<SwallowCommand> SwallowCommandList;
typedef TQPtrListIterator<SwallowCommand> SwallowCommandListIterator;
typedef TQPtrList<SwallowApp> SwallowAppList;

class SwallowApplet : public KPanelApplet
{
    TQ_OBJECT

public:
    SwallowApplet( const TQString& configFile, TQWidget *parent,
                          const char *name = 0L );
    ~SwallowApplet();

    // returns 0L if we don't have a SwallowApplet object yet,
    // but who cares
    static TWinModule * winModule() { return wModule; }
    static void removeApplet( SwallowApp * );

public: // for KPanelApplet
    int widthForHeight( int w );
    int heightForWidth( int h );

    void windowAdded(WId win);
    void processExited(TDEProcess *proc);

public slots:
    virtual void preferences();

private slots:
    void embedded( SwallowApp * );

private:
    void layoutApps();
    SwallowCommandList* readConfig();
    void createApps( SwallowCommandList * );


    static SwallowApplet *self;
    static SwallowAppList *appList;
    static SwallowAppList *embeddedList;
    static TWinModule *wModule;

    SwallowCommandList * m_swcList;
    TQBoxLayout *m_layout;

};


class SwallowApp : public QXEmbed
{
    TQ_OBJECT
	
public:
    SwallowApp( const SwallowCommand * swc, TQWidget* parent = 0,
		const char* name = 0);
    ~SwallowApp();

    float sizeRatio() const { return wh_ratio; }

signals:
    void embedded( SwallowApp * );

protected slots:
    void windowAdded(WId win);
    void processExited(TDEProcess *proc);

private:
    TDEProcess   	*process;
    TQString  	winTitle;
    float 	wh_ratio;

};

#endif // __swallow_h__
