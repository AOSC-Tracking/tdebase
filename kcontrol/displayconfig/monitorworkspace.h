/**
 * monitorworkspace.h
 *
 * Copyright (c) 2011 Timothy Pearson <kb9vqf@pearsoncomputing.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MONITORWORKSPACE_H
#define MONITORWORKSPACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tqlabel.h>
#include <tqptrlist.h>
#include <tqslider.h>
#include <tqworkspace.h>
#include <tqobjectlist.h>
#include <tqwidgetlist.h>

class MonitorRegion: public TQt
{
public:
	MonitorRegion();
	MonitorRegion(TQRect rect);
	MonitorRegion(TQMemArray<TQRect> newrects);
	~MonitorRegion();

	TQMemArray<TQRect> rects();
	MonitorRegion unite(MonitorRegion region);
	bool contains(TQRect);

private:
	TQMemArray<TQRect> rectangles;
};

class DraggableMonitor: public TQLabel
{
	TQ_OBJECT
	
public:
	DraggableMonitor( TQWidget* parent, const char* name, int wflags );
	~DraggableMonitor();

protected:
// 	void closeEvent( TQCloseEvent * );
	void mousePressEvent(TQMouseEvent *event);
	void mouseReleaseEvent(TQMouseEvent *event);
	void mouseMoveEvent(TQMouseEvent *event);

signals:
	void workspaceRelayoutNeeded();
	void monitorDragComplete(int);
	void monitorSelected(int);

public:
	int screen_id;
	bool is_primary;

private:
	TQPoint lastMousePosition;
};

class MonitorWorkspace : public TQWorkspace
{
	TQ_OBJECT
	
public:
	MonitorWorkspace( TQWidget* parent, const char* name );
	~MonitorWorkspace();

protected:
	virtual void resizeEvent( TQResizeEvent* re );

signals:
	void workspaceRelayoutNeeded();

public:
	float resize_factor;
};

#endif // MONITORWORKSPACE_H

