/**
 * displayconfig.cpp
 *
 * Copyright (c) 2009-2010 Timothy Pearson <kb9vqf@pearsoncomputing.net>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqpushbutton.h>
#include <tqtabwidget.h>
#include <tqtimer.h>

#include <dcopclient.h>

#include <tdeaboutdata.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <tdeglobal.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <tdepopupmenu.h>
#include <kinputdialog.h>
#include <kurlrequester.h>
#include <tdecmoduleloader.h>
#include <kgenericfactory.h>
#include <kstandarddirs.h>

#include <unistd.h>
#include <ksimpleconfig.h>
#include <string>
#include <stdio.h>
#include <tqstring.h>

#include <math.h>
#define PI 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679

#include "displayconfig.h"

/**** DLL Interface ****/
typedef KGenericFactory<KDisplayConfig, TQWidget> KDisplayCFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_displayconfig, KDisplayCFactory("kcmdisplayconfig") )

KSimpleConfig *systemconfig;

TQPoint moveTQRectOutsideTQRect(TQRect base, TQRect movable, int fallback_level = 0) {
	TQPoint final_result;

	double base_center_x = base.x() + (base.width()/2);
	double base_center_y = base.y() + (base.height()/2);
	double movable_center_x = movable.x() + (movable.width()/2);
	double movable_center_y = movable.y() + (movable.height()/2);

	double max_x_movement = (base.width()/2) + (movable.width()/2);
	double max_y_movement = (base.height()/2) + (movable.height()/2);

	double x_diff = abs(base_center_x-movable_center_x);
	double y_diff = abs(base_center_y-movable_center_y);

	int invert_movement;

	// Calculate the angles of the four corners of the base rectangle
	double angle_1 = atan2((base.height()/2),	(base.width()/2));
	double angle_2 = atan2((base.height()/2),	(base.width()/2)*(-1));
	double angle_3 = atan2((base.height()/2)*(-1),	(base.width()/2)*(-1));
	double angle_4 = atan2((base.height()/2)*(-1),	(base.width()/2));

	// Calculate the angle that the movable rectangle center is on
	double movable_angle = atan2(base_center_y-movable_center_y, movable_center_x-base_center_x);

	// Fix up coordinates
	if (angle_1 < 0) angle_1 = angle_1 + (2*PI);
	if (angle_2 < 0) angle_2 = angle_2 + (2*PI);
	if (angle_3 < 0) angle_3 = angle_3 + (2*PI);
	if (angle_4 < 0) angle_4 = angle_4 + (2*PI);
	if (movable_angle < 0) movable_angle = movable_angle + (2*PI);

	// Now calculate quadrant
	int quadrant;
	if ((movable_angle < angle_2) && (movable_angle >= angle_1)) {
		quadrant = 2;
	}
	else if ((movable_angle < angle_3) && (movable_angle >= angle_2)) {
		quadrant = 3;
	}
	else if ((movable_angle < angle_4) && (movable_angle >= angle_3)) {
		quadrant = 4;
	}
	else {
		quadrant = 1;
	}

	if (fallback_level == 0) {
		if ((quadrant == 2) || (quadrant == 4)) {
			// Move it in the Y direction
			if (movable_center_y < base_center_y)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint(0, (max_y_movement-y_diff)*invert_movement);
		}
		else {
			// Move it in the X direction
			if (movable_center_x < base_center_x)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint((max_x_movement-x_diff)*invert_movement, 0);
		}
	}
	if (fallback_level == 1) {
		if ((quadrant == 1) || (quadrant == 3)) {
			// Move it in the Y direction
			if (movable_center_y < base_center_y)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint(0, (max_y_movement-y_diff)*invert_movement);
		}
		else {
			// Move it in the X direction
			if (movable_center_x < base_center_x)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint((max_x_movement-x_diff)*invert_movement, 0);
		}
	}
	if (fallback_level == 2) {
		// Ooh, nasty, I need to move the rectangle the other (suboptimal) direction
		if ((quadrant == 2) || (quadrant == 4)) {
			// Move it in the Y direction
			if (movable_center_y >= base_center_y)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint(0, (max_y_movement+y_diff)*invert_movement);
		}
		else {
			// Move it in the X direction
			if (movable_center_x >= base_center_x)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint((max_x_movement+x_diff)*invert_movement, 0);
		}
	}
	if (fallback_level == 3) {
		// Ooh, nasty, I need to move the rectangle the other (suboptimal) direction
		if ((quadrant == 1) || (quadrant == 3)) {
			// Move it in the Y direction
			if (movable_center_y >= base_center_y)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint(0, (max_y_movement+y_diff)*invert_movement);
		}
		else {
			// Move it in the X direction
			if (movable_center_x >= base_center_x)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint((max_x_movement+x_diff)*invert_movement, 0);
		}
	}

	// Check for intersection
	TQRect test_rect = movable;
	test_rect.moveBy(final_result.x(), final_result.y());
	if (test_rect.intersects(base)) {
		if (final_result.x() > 0)
			final_result.setX(final_result.x()+1);
		if (final_result.x() < 0)
			final_result.setX(final_result.x()-1);
		if (final_result.y() > 0)
			final_result.setY(final_result.y()+1);
		if (final_result.y() < 0)
			final_result.setY(final_result.y()-1);
	}

	return final_result;
}

TQPoint moveTQRectSoThatItTouchesTQRect(TQRect base, TQRect movable, int fallback_level = 0) {
	TQPoint final_result;

	double base_center_x = base.x() + (base.width()/2);
	double base_center_y = base.y() + (base.height()/2);
	double movable_center_x = movable.x() + (movable.width()/2);
	double movable_center_y = movable.y() + (movable.height()/2);

	double max_x_movement = (base.width()/2) + (movable.width()/2);
	double max_y_movement = (base.height()/2) + (movable.height()/2);

	double x_diff = abs(base_center_x-movable_center_x);
	double y_diff = abs(base_center_y-movable_center_y);

	int invert_movement;

	// Calculate the angles of the four corners of the base rectangle
	double angle_1 = atan2((base.height()/2),	(base.width()/2));
	double angle_2 = atan2((base.height()/2),	(base.width()/2)*(-1));
	double angle_3 = atan2((base.height()/2)*(-1),	(base.width()/2)*(-1));
	double angle_4 = atan2((base.height()/2)*(-1),	(base.width()/2));

	// Calculate the angle that the movable rectangle center is on
	double movable_angle = atan2(base_center_y-movable_center_y, movable_center_x-base_center_x);

	// Fix up coordinates
	if (angle_1 < 0) angle_1 = angle_1 + (2*PI);
	if (angle_2 < 0) angle_2 = angle_2 + (2*PI);
	if (angle_3 < 0) angle_3 = angle_3 + (2*PI);
	if (angle_4 < 0) angle_4 = angle_4 + (2*PI);
	if (movable_angle < 0) movable_angle = movable_angle + (2*PI);

	// Now calculate quadrant
	int quadrant;
	if ((movable_angle < angle_2) && (movable_angle >= angle_1)) {
		quadrant = 2;
	}
	else if ((movable_angle < angle_3) && (movable_angle >= angle_2)) {
		quadrant = 3;
	}
	else if ((movable_angle < angle_4) && (movable_angle >= angle_3)) {
		quadrant = 4;
	}
	else {
		quadrant = 1;
	}

	if (fallback_level == 0) {
		if ((quadrant == 2) || (quadrant == 4)) {
			// Move it in the Y direction
			if (movable_center_y < base_center_y)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint(0, (max_y_movement-y_diff)*invert_movement);
		}
		else {
			// Move it in the X direction
			if (movable_center_x < base_center_x)
				invert_movement = -1;
			else
				invert_movement = 1;
			final_result = TQPoint((max_x_movement-x_diff)*invert_movement, 0);
		}
	}

	// Check for intersection
	TQRect test_rect = movable;
	test_rect.moveBy(final_result.x(), final_result.y());
	if (test_rect.intersects(base)) {
		if (final_result.x() > 0)
			final_result.setX(final_result.x()-1);
		if (final_result.x() < 0)
			final_result.setX(final_result.x()+1);
		if (final_result.y() > 0)
			final_result.setY(final_result.y()-1);
		if (final_result.y() < 0)
			final_result.setY(final_result.y()+1);
	}

	return final_result;
}

TQPoint moveTQRectOutsideMonitorRegion(TQRect rect, MonitorRegion region) {
	// This is a fun little class (not!)
	// It needs to move the TQRect so that it does not overlap any rectangles in the region
	long rect_center_x = rect.x() + (rect.width()/2);
	long rect_center_y = rect.y() + (rect.height()/2);

	// First, see if the rectangle actually overlaps the region
	if (!region.contains(rect))
		return TQPoint(0,0);

	// Then, break the region into the series of source rectangles
	TQMemArray<TQRect> rectangles = region.rects();

	// Next, find which rectangle is closest to the center of the TQRect
	int closest = 0;
	long distance = 16384*16384;
	int fallback_mode = 0;
	long test_distance;
	long test_center_x;
	long test_center_y;
	for ( unsigned int i = 0; i < rectangles.size(); i++ ) {
		test_center_x = rectangles[i].x() + (rectangles[i].width()/2);
		test_center_y = rectangles[i].y() + (rectangles[i].height()/2);
		test_distance = pow(test_center_x-rect_center_x,2) + pow(test_center_y-rect_center_y,2);
		if (test_distance < distance) {
			// Make sure this is an outer rectangle; i,e. there is empty space where
			// we would want to move the TQRect...
			// To do that we will move the TQRect in all four possible directions,
			// and see if any put the TQRect in an empty location
			// If they do, then this current rectangle is considered usable
			// and it is added to the distance checking routine.
			TQPoint test_moveby = moveTQRectOutsideTQRect(rectangles[i], rect, 0);
			TQRect test_rect = rect;
			test_rect.moveBy(test_moveby.x(), test_moveby.y());
			if (!region.contains(test_rect)) {
				closest = i;
				distance = test_distance;
				fallback_mode = 0;
			}
			else {
				test_moveby = moveTQRectOutsideTQRect(rectangles[i], rect, 1);
				test_rect = rect;
				test_rect.moveBy(test_moveby.x(), test_moveby.y());
				if (!region.contains(test_rect)) {
					closest = i;
					distance = test_distance;
					fallback_mode = 1;
				}
				else {
					test_moveby = moveTQRectOutsideTQRect(rectangles[i], rect, 2);
					test_rect = rect;
					test_rect.moveBy(test_moveby.x(), test_moveby.y());
					if (!region.contains(test_rect)) {
						closest = i;
						distance = test_distance;
						fallback_mode = 2;
					}
					else {
						test_moveby = moveTQRectOutsideTQRect(rectangles[i], rect, 3);
						test_rect = rect;
						test_rect.moveBy(test_moveby.x(), test_moveby.y());
						if (!region.contains(test_rect)) {
							closest = i;
							distance = test_distance;
							fallback_mode = 3;
						}
					}
				}
			}
		}
	}

	// Finally, calculate the required translation to move the TQRect outside the MonitorRegion
	// so that it touches the closest line found above
	return moveTQRectOutsideTQRect(rectangles[closest], rect, fallback_mode);
}

TQPoint compressTQRectTouchingMonitorRegion(TQRect rect, MonitorRegion region, TQSize workspace) {
	// This is another fun little class (not!)
	// It needs to move the TQRect so that it touches the closest outside line of the MonitorRegion
	bool should_move;
	long rect_center_x = rect.x() + (rect.width()/2);
	long rect_center_y = rect.y() + (rect.height()/2);

	// First, break the region into the series of source rectangles
	TQMemArray<TQRect> rectangles = region.rects();

	// Next, find which rectangle is closest to the center of the TQRect
	should_move = false;
	int closest = 0;
	long distance = 16384*16384;
	int fallback_mode;
	long test_distance;
	long test_center_x;
	long test_center_y;
	for ( unsigned int i = 0; i < rectangles.size(); i++ ) {
		test_center_x = rectangles[i].x() + (rectangles[i].width()/2);
		test_center_y = rectangles[i].y() + (rectangles[i].height()/2);
		test_distance = pow(test_center_x-rect_center_x,2) + pow(test_center_y-rect_center_y,2);
		if ( (abs(test_center_x-(workspace.width()/2))<2) && (abs(test_center_y-(workspace.height()/2))<2) ) {
			test_distance=0;	// Give the primary monitor "gravity" so it can attract all other monitors to itself
		}
		if (test_distance < distance) {
			// Make sure this is an outer rectangle; i,e. there is empty space where
			// we would want to move the TQRect...
			// To do that we will move the TQRect in all four possible directions,
			// and see if any put the TQRect in an empty location
			// If they do, then this current rectangle is considered usable
			// and it is added to the distance checking routine.
			TQPoint test_moveby = moveTQRectSoThatItTouchesTQRect(rectangles[i], rect, 0);
			TQRect test_rect = rect;
			test_rect.moveBy(test_moveby.x(), test_moveby.y());
			if (!region.contains(test_rect)) {
				closest = i;
				distance = test_distance;
				fallback_mode = 0;
				should_move = true;
			}
		}
	}

	// Finally, calculate the required translation to move the TQRect outside the MonitorRegion
	// so that it touches the closest line found above
	if (should_move)
		return moveTQRectSoThatItTouchesTQRect(rectangles[closest], rect, fallback_mode);
	else
		return TQPoint(0, 0);
}

void KDisplayConfig::updateDraggableMonitorInformation (int monitor_id) {
	updateDraggableMonitorInformationInternal(monitor_id, true);
	changed();
}

void KDisplayConfig::updateDraggableMonitorInformationInternal (int monitor_id, bool recurse) {
	int i;
	int j=0;
	DraggableMonitor *primary_monitor = NULL;
	DraggableMonitor *moved_monitor = NULL;
	SingleScreenData *screendata = NULL;
	TQObjectList monitors;

	// Find the moved draggable monitor object
	monitors = base->monitorPhyArrange->childrenListObject();
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
				if (monitor->screen_id == monitor_id) {
					moved_monitor = monitor;
					screendata = m_screenInfoArray[activeProfileName].at(moved_monitor->screen_id);
				}
			}
		}
	}

	if (!screendata) {
		return;
	}

	bool isvisiblyrotated = (screendata->current_rotation_index == ROTATION_90_DEGREES_INDEX) || (screendata->current_rotation_index == ROTATION_270_DEGREES_INDEX);

	if (screendata->is_extended) {
		moved_monitor->show();
	}
	else {
		moved_monitor->hide();
	}

	// Handle resizing
	if (isvisiblyrotated) {
		moved_monitor->setFixedSize(screendata->current_y_pixel_count*base->monitorPhyArrange->resize_factor, screendata->current_x_pixel_count*base->monitorPhyArrange->resize_factor);
	}
	else {
		moved_monitor->setFixedSize(screendata->current_x_pixel_count*base->monitorPhyArrange->resize_factor, screendata->current_y_pixel_count*base->monitorPhyArrange->resize_factor);
	}

	// Find the primary monitor
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray[activeProfileName].at(i);
		if (screendata->is_primary)
			j=i;
	}
	monitors = base->monitorPhyArrange->childrenListObject();
	primary_monitor = NULL;
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
				if (monitor->screen_id == j) {
					monitor->is_primary = true;	// Prevent dragging of the primary monitor
					primary_monitor = monitor;
				}
				else {
					monitor->is_primary = false;
				}
			}
		}
	}

	if (primary_monitor) {
		if (moved_monitor != primary_monitor) {
			// Run layout rules
			applyMonitorLayoutRules(moved_monitor);

			int toffset_x = moved_monitor->x() - ((base->monitorPhyArrange->width()/2)-(primary_monitor->width()/2));
			int toffset_y = moved_monitor->y() - ((base->monitorPhyArrange->height()/2)-(primary_monitor->height()/2));

			int offset_x = toffset_x / base->monitorPhyArrange->resize_factor;
			int offset_y = toffset_y / base->monitorPhyArrange->resize_factor;

			screendata = m_screenInfoArray[activeProfileName].at(monitor_id);
			screendata->absolute_x_position = offset_x;
			screendata->absolute_y_position = offset_y;
		}
		else {
			// Reset the position of the primary monitor
			moveMonitor(primary_monitor, 0, 0);
		}
	}
	else {
		printf("[WARNING] Display layout broken...\n"); fflush(stdout);
	}

	layoutDragDropDisplay();

// 	// FIXME Yes, this should work.  For some reason it creates big problems instead
// 	// Run layout rules on all monitors
// 	if (recurse == true) {
// 		applyMonitorLayoutRules();
// 	}
}

bool KDisplayConfig::applyMonitorLayoutRules() {
	int i;
	for (i=0;i<numberOfScreens;i++) {
		updateDraggableMonitorInformationInternal(i, false);
	}

	return false;
}

bool KDisplayConfig::applyMonitorLayoutRules(DraggableMonitor* monitor_to_move) {
	int i;
	bool monitor_was_moved;
	TQObjectList monitors;

	// Ensure that the monitors:
	// 1) Do NOT overlap
	// 2) Touch on at least one side
	monitor_was_moved = false;

	// Handle 1)

	// First, create a region from the monitor rectangles
	// The moved monitor cannot exist within that region
	MonitorRegion other_monitors;
	monitors = base->monitorPhyArrange->childrenListObject();
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
				if (monitor != monitor_to_move) {
					other_monitors = other_monitors.unite(MonitorRegion(monitor->geometry()));
				}
			}
		}
	}

	// Now get the required move X/Y direction
	TQPoint req_move = moveTQRectOutsideMonitorRegion(monitor_to_move->geometry(), other_monitors);

	// And move the monitor
	if (!monitor_to_move->isHidden())
		monitor_to_move->move(monitor_to_move->x()+req_move.x(), monitor_to_move->y()+req_move.y());
	else {
		req_move.setX(0);
		req_move.setY(0);
		monitor_to_move->move(base->monitorPhyArrange->width(), base->monitorPhyArrange->height());
	}

	if ((req_move.x() != 0) || (req_move.y() != 0))
		monitor_was_moved = true;

	// Handle 2)
	// Now we need to shrink the monitors so that no gaps appear between then
	// All shrinking must take place towards the nearest extant monitor edge

	// First, determine which rectangles touch the primary monitor, or touch rectangles that then
	// in turn touch the primary monitor
	// FIXME

	// Only run this routine if we don't touch the primary monitor, or touch any rectangles that
	// actually do touch the primary monitor (possible through other rectangles, etc.)...
	// This would be for efficiency
	// FIXME
// 	if () {
		TQPoint req_move2(-1,-1);
		while ((req_move2.x() != 0) || (req_move2.y() != 0)) {
			// First, create a region from the monitor rectangles
			MonitorRegion other_monitors2;
			monitors = base->monitorPhyArrange->childrenListObject();
			if ( monitors.count() ) {
				for ( i = 0; i < int(monitors.count()); ++i ) {
					if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
						DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
						if (monitor != monitor_to_move) {
							other_monitors2 = other_monitors2.unite(MonitorRegion(monitor->geometry()));
						}
					}
				}
			}

			// Now get the required move X/Y direction
			req_move2 = compressTQRectTouchingMonitorRegion(monitor_to_move->geometry(), other_monitors, base->monitorPhyArrange->size());

			// And move the monitor
			if (!monitor_to_move->isHidden())
				monitor_to_move->move(monitor_to_move->x()+req_move2.x(), monitor_to_move->y()+req_move2.y());
			else {
				req_move2.setX(0);
				req_move2.setY(0);
				monitor_to_move->move(base->monitorPhyArrange->width(), base->monitorPhyArrange->height());
			}

			if ((req_move2.x() != 0) || (req_move2.y() != 0))
				monitor_was_moved = true;
		}
// 	}

	return monitor_was_moved;
}

void KDisplayConfig::moveMonitor(DraggableMonitor* monitor, int realx, int realy) {
	int i;
	int j=0;
	bool primary_found;
	DraggableMonitor *primary_monitor;
	SingleScreenData *screendata;

	// Find the primary monitor
	primary_found = false;
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray[activeProfileName].at(i);
		if (screendata->is_primary) {
			j=i;
			primary_found = true;
		}
	}
	TQObjectList monitors = base->monitorPhyArrange->childrenListObject();
	primary_monitor = NULL;
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
				if (monitor->screen_id == j) {
					monitor->is_primary = true;	// Prevent dragging of the primary monitor
					primary_monitor = monitor;
				}
				else {
					monitor->is_primary = false;
				}
			}
		}
	}

	if (primary_found && primary_monitor) {
		int tx = realx * base->monitorPhyArrange->resize_factor;
		int ty = realy * base->monitorPhyArrange->resize_factor;
	
		if (!monitor->isHidden())
			monitor->move((base->monitorPhyArrange->width()/2)-(primary_monitor->width()/2)+tx,(base->monitorPhyArrange->height()/2)-(primary_monitor->height()/2)+ty);
		else
			monitor->move(base->monitorPhyArrange->width(), base->monitorPhyArrange->height());
	}
}

// int KDisplayConfig::realResolutionSliderValue() {
// 	return base->resolutionSlider->maxValue() - base->resolutionSlider->value();
// }
//
// void KDisplayConfig::setRealResolutionSliderValue(int index) {
// 	base->resolutionSlider->setValue(base->resolutionSlider->maxValue() - index);
// }

TQStringList sortResolutions(TQStringList unsorted) {
	int xres;
	int largest;
	TQStringList sorted;
	TQStringList::Iterator it;
	TQStringList::Iterator largestit;

	while (unsorted.count()) {
		largest = -1;
		for ( it = unsorted.begin(); it != unsorted.end(); ++it ) {
			TQString resolutionstring = *it;
			int separator_pos = resolutionstring.find(" x ");
			TQString x_res_string = resolutionstring.left(separator_pos);
			TQString y_res_string = resolutionstring.right(resolutionstring.length()-separator_pos-3);
			xres = x_res_string.toInt();
			if (xres > largest) {
				largest = xres;
				largestit = it;
			}
		}
		sorted.prepend(*largestit);
		unsorted.remove(largestit);
	}

	return sorted;
}

int KDisplayConfig::realResolutionSliderValue() {
	unsigned int i;
	unsigned int j;
	SingleScreenData *screendata;

	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());
	TQStringList sortedList = screendata->resolutions;
	sortedList = sortResolutions(sortedList);

	j=0;
	for (i=0; i<screendata->resolutions.count(); i++) {
		if ((*sortedList.at(base->resolutionSlider->value())) == (*screendata->resolutions.at(i))) {
			j=i;
		}
	}

	return j;
}

void KDisplayConfig::setRealResolutionSliderValue(int index) {
	unsigned int i;
	unsigned int j;
	SingleScreenData *screendata;

	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());
	TQStringList sortedList = screendata->resolutions;
	sortedList = sortResolutions(sortedList);

	j=0;
	for (i=0; i<screendata->resolutions.count(); i++) {
		if ((*sortedList.at(i)) == (*screendata->resolutions.at(index))) {
			j=i;
		}
	}

	base->resolutionSlider->setValue(j);
}

/**** KDisplayConfig ****/

KDisplayConfig::KDisplayConfig(TQWidget *parent, const char *name, const TQStringList &)
  : TDECModule(KDisplayCFactory::instance(), parent, name), iccTab(0), numberOfProfiles(0), numberOfScreens(0), m_randrsimple(0), activeProfileName(""), m_gammaApplyTimer(0)
{
#ifdef WITH_TDEHWLIB
	TDEHardwareDevices *hwdevices = TDEGlobal::hardwareDevices();
	connect(hwdevices, TQT_SIGNAL(hardwareUpdated(TDEGenericDevice*)), this, TQT_SLOT(deviceChanged(TDEGenericDevice*)));
#endif

	m_randrsimple = new KRandrSimpleAPI();

	TQVBoxLayout *layout = new TQVBoxLayout(this, 0, KDialog::spacingHint());
	if (getuid() != 0) {
		systemconfig = new KSimpleConfig( locateLocal("config", "tdedisplay/", true) + "tdedisplayconfigrc" );
		systemconfig->setFileWriteMode(0600);
	}
	else {
		systemconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/tdedisplay/tdedisplayconfigrc" ));
		systemconfig->setFileWriteMode(0644);
	}

	TDEAboutData *about =
		new TDEAboutData(I18N_NOOP("kcmdisplayconfig"), I18N_NOOP("TDE Display Profile Control Module"),
			0, 0, TDEAboutData::License_GPL,
			I18N_NOOP("(c) 2011 Timothy Pearson"));

	about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
	setAboutData( about );

	m_gammaApplyTimer = new TQTimer();
	connect(m_gammaApplyTimer, SIGNAL(timeout()), this, SLOT(applyGamma()));

	base = new DisplayConfigBase(this);
	profileRulesGrid = new TQGridLayout(base->profileRulesGridWidget, 1, 1, KDialog::marginHint());

	layout->addWidget(base);

	if (getuid() != 0) {
		base->systemEnableSupport->setText(i18n("&Enable local display control for this session"));
	}

// 	setRootOnlyMsg(i18n("<b>The global display configuration is a system wide setting, and requires administrator access</b><br>To alter the system's global display configuration, click on the \"Administrator Mode\" button below.<br>Otherwise, you may change your session-specific display configuration below."));
// 	setUseRootOnlyMsg(true);	// Setting this hides the Apply button!

	base->nonRootWarningLabel->setFrameShape(TQFrame::Box);
	base->nonRootWarningLabel->setFrameShadow(TQFrame::Raised);
	if (getuid() != 0) {
		base->nonRootWarningLabel->setText(i18n("<b>The global display configuration is a system wide setting, and requires administrator access</b><br>To alter the system's global display configuration, click on the \"Administrator Mode\" button below.<br>Otherwise, you may change your session-specific display configuration below."));
	}
	else {
		base->nonRootWarningLabel->hide();
	}

	connect(base->systemEnableSupport, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
	connect(base->systemEnableSupport, TQT_SIGNAL(clicked()), TQT_SLOT(processLockoutControls()));
	connect(base->addProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(addProfile()));
	connect(base->renameProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(renameProfile()));
	connect(base->deleteProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(deleteProfile()));
	connect(base->activateProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(activateProfile()));
	connect(base->reloadProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(reloadProfileFromDisk()));
	connect(base->saveProfileButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(saveProfile()));
	connect(base->systemEnableStartupProfile, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
	connect(base->systemEnableStartupProfile, TQT_SIGNAL(clicked()), this, TQT_SLOT(processLockoutControls()));
	connect(base->startupDisplayProfileList, TQT_SIGNAL(activated(int)), this, TQT_SLOT(changed()));
	connect(base->startupDisplayProfileList, TQT_SIGNAL(activated(int)), this, TQT_SLOT(selectDefaultProfile(int)));
	connect(base->displayProfileList, TQT_SIGNAL(activated(int)), this, TQT_SLOT(selectProfile(int)));

	connect(base->monitorDisplaySelectDD, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
	connect(base->gammamonitorDisplaySelectDD, TQT_SIGNAL(activated(int)), TQT_SLOT(changed()));
	connect(base->gammaTargetSelectDD, TQT_SIGNAL(activated(int)), TQT_SLOT(gammaTargetChanged(int)));
	connect(base->rotationSelectDD, TQT_SIGNAL(activated(int)), TQT_SLOT(rotationInfoChanged()));
	connect(base->refreshRateDD, TQT_SIGNAL(activated(int)), TQT_SLOT(refreshInfoChanged()));
	connect(base->orientationHFlip, TQT_SIGNAL(clicked()), TQT_SLOT(rotationInfoChanged()));
	connect(base->orientationVFlip, TQT_SIGNAL(clicked()), TQT_SLOT(rotationInfoChanged()));
	connect(base->resolutionSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(resolutionSliderChanged(int)));
	connect(base->gammaAllSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(gammaAllSliderChanged(int)));
	connect(base->gammaRedSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(gammaRedSliderChanged(int)));
	connect(base->gammaGreenSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(gammaGreenSliderChanged(int)));
	connect(base->gammaBlueSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(gammaBlueSliderChanged(int)));
	connect(base->monitorDisplaySelectDD, TQT_SIGNAL(activated(int)), TQT_SLOT(selectScreen(int)));
	connect(base->gammamonitorDisplaySelectDD, TQT_SIGNAL(activated(int)), TQT_SLOT(gammaselectScreen(int)));
	connect(base->systemEnableDPMS, TQT_SIGNAL(clicked()), TQT_SLOT(dpmsChanged()));
	connect(base->systemEnableDPMSStandby, TQT_SIGNAL(clicked()), TQT_SLOT(dpmsChanged()));
	connect(base->systemEnableDPMSSuspend, TQT_SIGNAL(clicked()), TQT_SLOT(dpmsChanged()));
	connect(base->systemEnableDPMSPowerDown, TQT_SIGNAL(clicked()), TQT_SLOT(dpmsChanged()));
	connect(base->dpmsStandbyTimeout, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(dpmsChanged()));
	connect(base->dpmsSuspendTimeout, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(dpmsChanged()));
	connect(base->dpmsPowerDownTimeout, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(dpmsChanged()));
	connect(base->monitorPhyArrange, TQT_SIGNAL(workspaceRelayoutNeeded()), this, TQT_SLOT(layoutDragDropDisplay()));

	connect(base->isPrimaryMonitorCB, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
	connect(base->isPrimaryMonitorCB, TQT_SIGNAL(clicked()), TQT_SLOT(ensurePrimaryMonitorIsAvailable()));
	connect(base->isExtendedMonitorCB, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
	connect(base->isExtendedMonitorCB, TQT_SIGNAL(clicked()), TQT_SLOT(updateExtendedMonitorInformation()));

	connect(base->systemEnableSupport, TQT_SIGNAL(toggled(bool)), base->monitorDisplaySelectDD, TQT_SLOT(setEnabled(bool)));

	connect(base->rescanHardware, TQT_SIGNAL(clicked()), TQT_SLOT(rescanHardware()));
	connect(base->loadExistingProfile, TQT_SIGNAL(clicked()), TQT_SLOT(reloadProfile()));
	connect(base->previewConfiguration, TQT_SIGNAL(clicked()), TQT_SLOT(activatePreview()));
	connect(base->identifyMonitors, TQT_SIGNAL(clicked()), TQT_SLOT(identifyMonitors()));

	load();

	iccTab = addTab( "iccconfig", i18n( "Color Profiles" ) );	// [FIXME] No way to save settings here yet

	processLockoutControls();
}

KDisplayConfig::~KDisplayConfig()
{
	delete systemconfig;
	if (m_gammaApplyTimer) {
		delete m_gammaApplyTimer;
		m_gammaApplyTimer = 0;
	}
	if (m_randrsimple) {
		delete m_randrsimple;
		m_randrsimple = 0;
	}
}

void KDisplayConfig::deviceChanged (TDEGenericDevice* device) {
#ifdef WITH_TDEHWLIB
	if (device->type() == TDEGenericDeviceType::Monitor) {
		if (base->rescanHardware->isEnabled()) {
			base->rescanHardware->setEnabled(false);
			rescanHardware();
			base->rescanHardware->setEnabled(true);
		}
	}
#endif
}

void KDisplayConfig::updateExtendedMonitorInformation () {
	SingleScreenData *screendata;

	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());
	screendata->is_extended = base->isExtendedMonitorCB->isChecked();

	refreshDisplayedInformation();
}

void KDisplayConfig::rescanHardware (void) {
	m_randrsimple->destroyScreenInformationObject(m_screenInfoArray[activeProfileName]);
	m_hardwareScreenInfoArray = m_randrsimple->readCurrentDisplayConfiguration();
	m_randrsimple->ensureMonitorDataConsistency(m_hardwareScreenInfoArray);
	m_screenInfoArray[activeProfileName] = m_randrsimple->copyScreenInformationObject(m_hardwareScreenInfoArray);
	numberOfScreens = m_screenInfoArray[activeProfileName].count();
	refreshDisplayedInformation();
}

void KDisplayConfig::reloadProfile (void) {
	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray[activeProfileName]);
	numberOfScreens = m_screenInfoArray[activeProfileName].count();
	refreshDisplayedInformation();
}

void KDisplayConfig::identifyMonitors () {
	unsigned int i;

	TQLabel* idWidget;
	TQPtrList<TQWidget> widgetList;

	Display *randr_display;
	ScreenInfo *randr_screen_info;

	randr_display = tqt_xdisplay();
	randr_screen_info = m_randrsimple->read_screen_info(randr_display);

	for (i = 0; i < m_screenInfoArray[activeProfileName].count(); i++) {
		// Look for ON outputs...
		if (!randr_screen_info->outputs[i]->cur_crtc) {
			continue;
		}
		idWidget = new TQLabel(TQString("Screen\n%1").arg(i+1), (TQWidget*)0, "", TQt::WStyle_Customize | TQt::WStyle_NoBorder | TQt::WStyle_StaysOnTop | TQt::WX11BypassWM | TQt::WDestructiveClose);
		widgetList.append(idWidget);
		idWidget->resize(150, 100);
		idWidget->setAlignment(TQt::AlignCenter);
		TQFont font = idWidget->font();
		font.setBold( true );
		font.setPointSize(24);
		idWidget->setFont( font );
		idWidget->setPaletteForegroundColor(TQt::white);
		idWidget->setPaletteBackgroundColor(TQt::black);
		idWidget->show();
		KDialog::centerOnScreen(idWidget, i);
		TQTimer::singleShot(3000, idWidget, SLOT(close()));
	}

	m_randrsimple->freeScreenInfoStructure(randr_screen_info);
}

void KDisplayConfig::activatePreview() {
	m_randrsimple->applyDisplayConfiguration(m_screenInfoArray[activeProfileName], TRUE);
}

void KDisplayConfig::load()
{
	load( false );
}

void KDisplayConfig::loadProfileFromDiskHelper(bool forceReload) {
	if (forceReload) {
		m_randrsimple->destroyScreenInformationObject(m_screenInfoArray[activeProfileName]);
		m_screenInfoArray.remove(activeProfileName);
	}
	if (!m_screenInfoArray.contains(activeProfileName)) {
		TQPtrList<SingleScreenData> originalInfoArray;
		TQPtrList<SingleScreenData> newInfoArray;
	
		// If a configuration is present, load it in
		// Otherwise, use the current display configuration
		originalInfoArray = m_screenInfoArray[activeProfileName];
		if (getuid() != 0) {
			newInfoArray = m_randrsimple->loadDisplayConfiguration(activeProfileName, locateLocal("config", "/", true));
		}
		else {
			newInfoArray = m_randrsimple->loadDisplayConfiguration(activeProfileName, KDE_CONFDIR);
		}
		if (newInfoArray.count() > 0) {
			m_screenInfoArray[activeProfileName] = newInfoArray;
			m_randrsimple->destroyScreenInformationObject(originalInfoArray);
		}
		else {
			m_screenInfoArray[activeProfileName] = originalInfoArray;
			m_randrsimple->destroyScreenInformationObject(newInfoArray);
		}
	}

	// If there is still no valid configuration, read the active display information from the hardware
	// to initialise the configuration...
	if (m_screenInfoArray[activeProfileName].count() < 1) {
		m_hardwareScreenInfoArray = m_randrsimple->readCurrentDisplayConfiguration();
		m_randrsimple->ensureMonitorDataConsistency(m_hardwareScreenInfoArray);
		m_screenInfoArray[activeProfileName] = m_randrsimple->copyScreenInformationObject(m_hardwareScreenInfoArray);
	}

	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray[activeProfileName]);
	numberOfScreens = m_screenInfoArray[activeProfileName].count();

	reloadProfile();
}

void KDisplayConfig::selectProfile (int slotNumber) {
	TQString selectedProfile = base->displayProfileList->currentText();
	if (selectedProfile == i18n("<default>")) {
		selectedProfile = "";
	}
	activeProfileName = selectedProfile;

	loadProfileFromDiskHelper();
}

void KDisplayConfig::deleteProfile () {
	if (activeProfileName == "") {
		KMessageBox::sorry(this, i18n("You cannot delete the default profile!"), i18n("Invalid operation requested"));
		return;
	}

	int ret = KMessageBox::warningYesNo(this, i18n("<qt><b>You are attempting to delete the display profile '%1'</b><br>If you click Yes, the profile will be permanently removed from disk<p>Do you want to delete this profile?</qt>").arg(activeProfileName), i18n("Delete display profile?"));
	if (ret == KMessageBox::Yes) {
		bool success = false;
		if (getuid() != 0) {
			success = m_randrsimple->deleteDisplayConfiguration(activeProfileName, locateLocal("config", "/", true));
		}
		else {
			success = m_randrsimple->deleteDisplayConfiguration(activeProfileName, KDE_CONFDIR);
		}
		if (success) {
			TQStringList::Iterator it = availableProfileNames.find(activeProfileName);
			if (it != availableProfileNames.end()) {
				availableProfileNames.remove(it);
			}
			profileListChanged();
			selectProfile(base->displayProfileList->currentItem());
		}
		else {
			KMessageBox::error(this, i18n("<qt><b>Unable to delete profile '%1'!</b><p>Please verify that you have permission to access the configuration file</qt>").arg(activeProfileName), i18n("Deletion failed!"));
		}
	}
}

void KDisplayConfig::renameProfile () {
	if (activeProfileName == "") {
		KMessageBox::sorry(this, i18n("You cannot rename the default profile!"), i18n("Invalid operation requested"));
		return;
	}

	// Pop up a text entry box asking for the new name of the profile
	bool _ok = false;
	bool _end = false;
	TQString _new;
	TQString _text = i18n("Please enter the new profile name below:");
	TQString _error;

	while (!_end) {
		_new = KInputDialog::getText( i18n("Display Profile Configuration"),  _error + _text, activeProfileName, &_ok, this);
		if (!_ok ) {
			_end = true;
		}
		else {
			_error = TQString();
			if (!_new.isEmpty()) {
				if (findProfileIndex(_new) != -1) {
					_error = i18n("Error: A profile with that name already exists") + TQString("\n");
				}
				else {
					_end = true;
				}
			}
		}
	}
	if (!_ok) return;


	bool success = false;
	if (getuid() != 0) {
		success = m_randrsimple->renameDisplayConfiguration(activeProfileName, _new, locateLocal("config", "/", true));
	}
	else {
		success = m_randrsimple->renameDisplayConfiguration(activeProfileName, _new, KDE_CONFDIR);
	}

	if (success) {
		TQStringList::Iterator it = availableProfileNames.find(activeProfileName);
		if (it != availableProfileNames.end()) {
			availableProfileNames.remove(it);
		}
		availableProfileNames.append(_new);
		profileListChanged();
		base->displayProfileList->setCurrentItem(_new);
		selectProfile(base->displayProfileList->currentItem());
	}
	else {
		KMessageBox::error(this, i18n("<qt><b>Unable to rename profile '%1'!</b><p>Please verify that you have permission to access the configuration file</qt>").arg(activeProfileName), i18n("Renaming failed!"));
	}
}

void KDisplayConfig::activateProfile() {
	if (getuid() != 0) {
		m_randrsimple->applyDisplayConfiguration(m_screenInfoArray[activeProfileName], TRUE, locateLocal("config", "/", true));
	}
	else {
		m_randrsimple->applyDisplayConfiguration(m_screenInfoArray[activeProfileName], TRUE, KDE_CONFDIR);
	}
	rescanHardware();
}

void KDisplayConfig::reloadProfileFromDisk() {
	loadProfileFromDiskHelper(true);
}

void KDisplayConfig::saveProfile() {
	saveActiveSystemWideProfileToDisk();
}

void KDisplayConfig::addProfile () {
	// Pop up a text entry box asking for the name of the new profile
	bool _ok = false;
	bool _end = false;
	TQString _new;
	TQString _text = i18n("Please enter the new profile name below:");
	TQString _error;

	while (!_end) {
		_new = KInputDialog::getText( i18n("Display Profile Configuration"),  _error + _text, TQString::null, &_ok, this);
		if (!_ok ) {
			_end = true;
		}
		else {
			_error = TQString();
			if (!_new.isEmpty()) {
				if (findProfileIndex(_new) != -1) {
					_error = i18n("Error: A profile with that name already exists") + TQString("\n");
				}
				else {
					_end = true;
				}
			}
		}
	}
	if (!_ok) return;

	m_screenInfoArray[_new] = m_randrsimple->copyScreenInformationObject(m_screenInfoArray[activeProfileName]);

	// Insert the new profile name
	availableProfileNames.append(_new);
	profileListChanged();
	base->displayProfileList->setCurrentItem(_new);
	selectProfile(base->displayProfileList->currentItem());

	updateDisplayedInformation();
	saveActiveSystemWideProfileToDisk();
	emit changed();
}

void KDisplayConfig::updateStartupProfileLabel()
{
	TQString friendlyName = startupProfileName;
	if (friendlyName == "") {
		friendlyName = i18n("<default>");
	}

	base->startupDisplayProfileList->setCurrentItem(friendlyName, false);
}

void KDisplayConfig::selectDefaultProfile(int slotNumber)
{
	TQString selectedProfile = base->startupDisplayProfileList->currentText();
	if (selectedProfile == i18n("<default>")) {
		selectedProfile = "";
	}

	startupProfileName = selectedProfile;
}

void KDisplayConfig::selectScreen (int slotNumber) {
	base->monitorDisplaySelectDD->setCurrentItem(slotNumber);
	base->gammamonitorDisplaySelectDD->setCurrentItem(slotNumber);
	updateDisplayedInformation();
}

void KDisplayConfig::updateArray (void) {
	m_hardwareScreenInfoArray = m_randrsimple->readCurrentDisplayConfiguration();
	m_randrsimple->ensureMonitorDataConsistency(m_hardwareScreenInfoArray);
	m_screenInfoArray[activeProfileName] = m_randrsimple->copyScreenInformationObject(m_hardwareScreenInfoArray);
	numberOfScreens = m_screenInfoArray[activeProfileName].count();
}

void KDisplayConfig::updateDisplayedInformation () {
	// Insert data into the GUI
	unsigned int i;
	SingleScreenData *screendata;

	ensureMonitorDataConsistency();

	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());

	if (!screendata) {
		base->resolutionSlider->setEnabled(false);
		base->refreshRateDD->setEnabled(false);
		base->rotationSelectDD->setEnabled(false);
		base->orientationHFlip->setEnabled(false);
		base->orientationVFlip->setEnabled(false);
		base->isPrimaryMonitorCB->setEnabled(false);
		base->isExtendedMonitorCB->setEnabled(false);
		return;
	}

	if (screendata->screen_connected) {
		base->resolutionSlider->setEnabled(true);
		base->refreshRateDD->setEnabled(true);
		base->rotationSelectDD->setEnabled(true);
		base->orientationHFlip->setEnabled(true);
		base->orientationVFlip->setEnabled(true);
		base->isPrimaryMonitorCB->setEnabled(true);
		base->isExtendedMonitorCB->setEnabled(true);
	}

	// Update the resolutions for the selected screen
	base->resolutionSlider->blockSignals(true);
	base->resolutionSlider->setMaxValue(screendata->resolutions.count()-1);
	setRealResolutionSliderValue(screendata->current_resolution_index);
	resolutionSliderTextUpdate(realResolutionSliderValue());
	base->resolutionSlider->blockSignals(false);

	// Now the refresh rates for the selected screen
	base->refreshRateDD->blockSignals(true);
	base->refreshRateDD->clear();
	for (i=0;i<screendata->refresh_rates.count();i++) {
		base->refreshRateDD->insertItem(screendata->refresh_rates[i], i);
	}
	base->refreshRateDD->setCurrentItem(screendata->current_refresh_rate_index);
	base->refreshRateDD->blockSignals(false);

	// Now the rotations and transformations for the selected screen
	base->rotationSelectDD->blockSignals(true);
	base->orientationHFlip->blockSignals(true);
	base->orientationVFlip->blockSignals(true);
	base->rotationSelectDD->clear();
	if (screendata->supports_transformations) {
		for (i=0;i<screendata->rotations.count();i++) {
			base->rotationSelectDD->insertItem(screendata->rotations[i], i);
		}
		base->rotationSelectDD->setCurrentItem(screendata->current_rotation_index);
		base->orientationHFlip->show();
		base->orientationVFlip->show();
		base->orientationHFlip->setChecked(screendata->has_x_flip);
		base->orientationVFlip->setChecked(screendata->has_y_flip);
	}
	else {
		base->rotationSelectDD->insertItem(screendata->rotations[ROTATION_0_DEGREES_INDEX], ROTATION_0_DEGREES_INDEX);
		base->rotationSelectDD->setCurrentItem(0);
		base->orientationHFlip->hide();
		base->orientationVFlip->hide();
	}
	base->rotationSelectDD->blockSignals(false);
	base->orientationHFlip->blockSignals(false);
	base->orientationVFlip->blockSignals(false);

	base->isPrimaryMonitorCB->blockSignals(true);
	base->isExtendedMonitorCB->blockSignals(true);
	if (screendata->generic_screen_detected) {
		base->isPrimaryMonitorCB->setEnabled(false);
		base->isPrimaryMonitorCB->setChecked(true);
		base->isExtendedMonitorCB->setEnabled(false);
		base->isExtendedMonitorCB->setChecked(true);
	}
	else {
		base->isPrimaryMonitorCB->setEnabled(true);
		base->isPrimaryMonitorCB->setChecked(screendata->is_primary);
		if (screendata->is_primary) {
			base->isExtendedMonitorCB->setEnabled(false);
			base->isExtendedMonitorCB->setChecked(true);
		}
		else {
			base->isExtendedMonitorCB->setEnabled(true);
			base->isExtendedMonitorCB->setChecked(screendata->is_extended);
		}
	}
	base->isPrimaryMonitorCB->blockSignals(false);
	base->isExtendedMonitorCB->blockSignals(false);

	if (!screendata->screen_connected) {
		base->resolutionSlider->setEnabled(false);
		base->refreshRateDD->setEnabled(false);
		base->rotationSelectDD->setEnabled(false);
		base->orientationHFlip->setEnabled(false);
		base->orientationVFlip->setEnabled(false);
		base->isPrimaryMonitorCB->setEnabled(false);
		base->isExtendedMonitorCB->setEnabled(false);
	}
}

void KDisplayConfig::refreshDisplayedInformation () {
	createHotplugRulesGrid();

	// Insert data into the GUI
	int i;
	SingleScreenData *screendata;

	// First, the screens
	int currentScreenIndex = base->monitorDisplaySelectDD->currentItem();
	base->monitorDisplaySelectDD->clear();
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray[activeProfileName].at(i);
		base->monitorDisplaySelectDD->insertItem(screendata->screenFriendlyName, i);
	}
	base->monitorDisplaySelectDD->setCurrentItem(currentScreenIndex);
	base->gammamonitorDisplaySelectDD->clear();
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray[activeProfileName].at(i);
		base->gammamonitorDisplaySelectDD->insertItem(screendata->screenFriendlyName, i);
	}
	base->gammamonitorDisplaySelectDD->setCurrentItem(currentScreenIndex);

	updateDisplayedInformation();

	updateDragDropDisplay();

	screendata = m_screenInfoArray[activeProfileName].at(0);
	if (screendata) {
		base->groupPowerManagement->setEnabled(true);
		base->systemEnableDPMS->setEnabled(screendata->has_dpms);
		base->systemEnableDPMS->setChecked(screendata->enable_dpms);
		base->systemEnableDPMSStandby->setChecked(screendata->dpms_standby_delay!=0);
		base->systemEnableDPMSSuspend->setChecked(screendata->dpms_suspend_delay!=0);
		base->systemEnableDPMSPowerDown->setChecked(screendata->dpms_off_delay!=0);
		base->dpmsStandbyTimeout->setValue(screendata->dpms_standby_delay/60);
		base->dpmsSuspendTimeout->setValue(screendata->dpms_suspend_delay/60);
		base->dpmsPowerDownTimeout->setValue(screendata->dpms_off_delay/60);
	}
	else {
		base->groupPowerManagement->setEnabled(false);
	}
	processDPMSControls();
}

void KDisplayConfig::updateDragDropDisplay() {
	// Insert data into the GUI
	int i;
	int j;
	TQObjectList monitors;
	SingleScreenData *screendata;

	// Clear any screens from the workspace
	monitors = base->monitorPhyArrange->childrenListObject();
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				TQWidget *monitor = TQT_TQWIDGET(monitors.at( i ));
				if ( !monitor->close(TRUE) ) {
					Q_ASSERT("zombie monitor will not go away!");
				}
			}
		}
	}

	ensureMonitorDataConsistency();

	// Add the screens to the workspace
	// Set the scaling small to start with
	base->monitorPhyArrange->resize_factor = 0.0625;	// This always needs to divide by a multiple of 2
	for (j=0;j<2;j++) {
		for (i=0;i<numberOfScreens;i++) {
			screendata = m_screenInfoArray[activeProfileName].at(i);
			if (((j==0) && (screendata->is_primary==true)) || ((j==1) && (screendata->is_primary==false))) {	// This ensures that the primary monitor is always the first one created and placed on the configuration widget
				bool isvisiblyrotated = ((screendata->current_rotation_index == ROTATION_90_DEGREES_INDEX) || (screendata->current_rotation_index == ROTATION_270_DEGREES_INDEX));
				DraggableMonitor *m = new DraggableMonitor( base->monitorPhyArrange, 0, WStyle_Customize | WDestructiveClose | WStyle_NoBorder | WX11BypassWM );
				connect(m, TQT_SIGNAL(workspaceRelayoutNeeded()), this, TQT_SLOT(layoutDragDropDisplay()));
				connect(m, TQT_SIGNAL(monitorSelected(int)), this, TQT_SLOT(selectScreen(int)));
				connect(m, TQT_SIGNAL(monitorDragComplete(int)), this, TQT_SLOT(updateDraggableMonitorInformation(int)));
				m->screen_id = i;
				if (isvisiblyrotated)
					m->setFixedSize(screendata->current_y_pixel_count*base->monitorPhyArrange->resize_factor, screendata->current_x_pixel_count*base->monitorPhyArrange->resize_factor);
				else
					m->setFixedSize(screendata->current_x_pixel_count*base->monitorPhyArrange->resize_factor, screendata->current_y_pixel_count*base->monitorPhyArrange->resize_factor);
				m->setText(TQString("%1").arg(i+1));
				m->show();
				moveMonitor(m, screendata->absolute_x_position, screendata->absolute_y_position);
				updateDraggableMonitorInformation(i);	// Make sure the new monitors don't overlap
			}
		}
	}

	layoutDragDropDisplay();
}

void KDisplayConfig::layoutDragDropDisplay() {
	int i;
	TQObjectList monitors;
	SingleScreenData *screendata;

	// Ensure data is consistent
	ensureMonitorDataConsistency();

	// Arrange the screens
	// First, center the primary monitor
	monitors = base->monitorPhyArrange->childrenListObject();
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
				screendata = m_screenInfoArray[activeProfileName].at(monitor->screen_id);
				moveMonitor(monitor, screendata->absolute_x_position, screendata->absolute_y_position);
			}
		}
	}
}

void KDisplayConfig::ensureMonitorDataConsistency() {
	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray[activeProfileName]);
}

void KDisplayConfig::resolutionSliderTextUpdate(int index) {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());

	base->resolutionLabel->setText(screendata->resolutions[realResolutionSliderValue()] + TQString(" ") + i18n("pixels"));
}

void KDisplayConfig::resolutionSliderChanged(int index) {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());

	screendata->current_resolution_index = realResolutionSliderValue();
	updateDisplayedInformation();
	updateDraggableMonitorInformation(base->monitorDisplaySelectDD->currentItem());

	applyMonitorLayoutRules();

	changed();
}

void KDisplayConfig::rotationInfoChanged() {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());

	screendata->current_rotation_index = base->rotationSelectDD->currentItem();
	screendata->has_x_flip = base->orientationHFlip->isChecked();
	screendata->has_y_flip = base->orientationVFlip->isChecked();
	updateDisplayedInformation();
	updateDraggableMonitorInformation(base->monitorDisplaySelectDD->currentItem());

	applyMonitorLayoutRules();

	changed();
}

void KDisplayConfig::refreshInfoChanged() {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray[activeProfileName].at(base->monitorDisplaySelectDD->currentItem());

	screendata->current_refresh_rate_index = base->refreshRateDD->currentItem();
	updateDisplayedInformation();
	updateDraggableMonitorInformation(base->monitorDisplaySelectDD->currentItem());

	changed();
}

void KDisplayConfig::ensurePrimaryMonitorIsAvailable() {
	// Ensure that only one monitor, and not less than one monitor, is marked as primary
	int i;
	SingleScreenData *screendata;

	// First, the screens
	int currentScreenIndex = base->monitorDisplaySelectDD->currentItem();
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray[activeProfileName].at(i);
		if (i != currentScreenIndex)
			screendata->is_primary = false;
	}
	screendata = m_screenInfoArray[activeProfileName].at(currentScreenIndex);
	screendata->is_primary = true;
	screendata->is_extended = true;
	updateDragDropDisplay();
	refreshDisplayedInformation();
}

int KDisplayConfig::findProfileIndex(TQString profileName) {
	int i;
	for (i=0;i<base->displayProfileList->count();i++) {
		if (base->displayProfileList->text(i) == profileName) {
			return i;
		}
	}
	return -1;
}

void KDisplayConfig::setGammaLabels() {
	SingleScreenData *screendata;

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());

	// Round off the gammas to one decimal place
	screendata->gamma_red = floorf(screendata->gamma_red * 10 + 0.5) / 10;
	screendata->gamma_green = floorf(screendata->gamma_green * 10 + 0.5) / 10;
	screendata->gamma_blue = floorf(screendata->gamma_blue * 10 + 0.5) / 10;

	// Set the labels
	base->gammaAllLabel->setText(TQString("%1").arg(((float)base->gammaAllSlider->value())/10.0, 0, 'f', 1));
	base->gammaRedLabel->setText(TQString("%1").arg(((float)base->gammaRedSlider->value())/10.0, 0, 'f', 1));
	base->gammaGreenLabel->setText(TQString("%1").arg(((float)base->gammaGreenSlider->value())/10.0, 0, 'f', 1));
	base->gammaBlueLabel->setText(TQString("%1").arg(((float)base->gammaBlueSlider->value())/10.0, 0, 'f', 1));
}

void KDisplayConfig::gammaSetAverageAllSlider() {
	float average_gamma;
	SingleScreenData *screendata;

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());
	average_gamma = (screendata->gamma_red+screendata->gamma_green+screendata->gamma_blue)/3.0;
	average_gamma = floorf(average_gamma* 10 + 0.5) / 10;	// Round off the gamma to one decimal place
	base->gammaAllSlider->setValue(average_gamma*10.0);
}

void KDisplayConfig::gammaselectScreen (int slotNumber) {
	SingleScreenData *screendata;

	base->gammaAllSlider->blockSignals(true);
	base->gammaRedSlider->blockSignals(true);
	base->gammaGreenSlider->blockSignals(true);
	base->gammaBlueSlider->blockSignals(true);

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());
	base->gammaRedSlider->setValue(screendata->gamma_red*10.0);
	base->gammaGreenSlider->setValue(screendata->gamma_green*10.0);
	base->gammaBlueSlider->setValue(screendata->gamma_blue*10.0);
	gammaSetAverageAllSlider();
	setGammaLabels();

	base->gammaAllSlider->blockSignals(false);
	base->gammaRedSlider->blockSignals(false);
	base->gammaGreenSlider->blockSignals(false);
	base->gammaBlueSlider->blockSignals(false);
}

void KDisplayConfig::gammaAllSliderChanged(int index) {
	SingleScreenData *screendata;

	base->gammaAllSlider->blockSignals(true);
	base->gammaRedSlider->blockSignals(true);
	base->gammaGreenSlider->blockSignals(true);
	base->gammaBlueSlider->blockSignals(true);

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());

	base->gammaRedSlider->setValue(base->gammaAllSlider->value());
	base->gammaGreenSlider->setValue(base->gammaAllSlider->value());
	base->gammaBlueSlider->setValue(base->gammaAllSlider->value());
	setGammaLabels();

	screendata->gamma_red = ((float)base->gammaAllSlider->value())/10.0;
	screendata->gamma_green = ((float)base->gammaAllSlider->value())/10.0;
	screendata->gamma_blue = ((float)base->gammaAllSlider->value())/10.0;

	m_gammaApplyTimer->start(10, TRUE);

	base->gammaAllSlider->blockSignals(false);
	base->gammaRedSlider->blockSignals(false);
	base->gammaGreenSlider->blockSignals(false);
	base->gammaBlueSlider->blockSignals(false);

	changed();
}

void KDisplayConfig::gammaRedSliderChanged(int index) {
	SingleScreenData *screendata;

	base->gammaAllSlider->blockSignals(true);
	base->gammaRedSlider->blockSignals(true);
	base->gammaGreenSlider->blockSignals(true);
	base->gammaBlueSlider->blockSignals(true);

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());
	screendata->gamma_red = ((float)index)/10.0;
	gammaSetAverageAllSlider();
	setGammaLabels();
	m_gammaApplyTimer->start(10, TRUE);

	base->gammaAllSlider->blockSignals(false);
	base->gammaRedSlider->blockSignals(false);
	base->gammaGreenSlider->blockSignals(false);
	base->gammaBlueSlider->blockSignals(false);

	changed();
}

void KDisplayConfig::gammaGreenSliderChanged(int index) {
	SingleScreenData *screendata;

	base->gammaAllSlider->blockSignals(true);
	base->gammaRedSlider->blockSignals(true);
	base->gammaGreenSlider->blockSignals(true);
	base->gammaBlueSlider->blockSignals(true);

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());
	screendata->gamma_green = ((float)index)/10.0;
	gammaSetAverageAllSlider();
	setGammaLabels();
	m_gammaApplyTimer->start(10, TRUE);

	base->gammaAllSlider->blockSignals(false);
	base->gammaRedSlider->blockSignals(false);
	base->gammaGreenSlider->blockSignals(false);
	base->gammaBlueSlider->blockSignals(false);

	changed();
}

void KDisplayConfig::gammaBlueSliderChanged(int index) {
	SingleScreenData *screendata;

	base->gammaAllSlider->blockSignals(true);
	base->gammaRedSlider->blockSignals(true);
	base->gammaGreenSlider->blockSignals(true);
	base->gammaBlueSlider->blockSignals(true);

	screendata = m_screenInfoArray[activeProfileName].at(base->gammamonitorDisplaySelectDD->currentItem());
	screendata->gamma_blue = ((float)index)/10.0;
	gammaSetAverageAllSlider();
	setGammaLabels();
	m_gammaApplyTimer->start(10, TRUE);

	base->gammaAllSlider->blockSignals(false);
	base->gammaRedSlider->blockSignals(false);
	base->gammaGreenSlider->blockSignals(false);
	base->gammaBlueSlider->blockSignals(false);

	changed();
}

void KDisplayConfig::applyGamma() {
	m_randrsimple->applyDisplayGamma(m_screenInfoArray[activeProfileName]);
}

void KDisplayConfig::gammaTargetChanged (int slotNumber) {
	TQPixmap gammaPixmap( locate("data", TQString("kcontrol/pics/gamma%1.png").arg(base->gammaTargetSelectDD->text(slotNumber))) );
	base->gammaTestImage->setBackgroundPixmap( gammaPixmap );
}

void KDisplayConfig::dpmsChanged() {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray[activeProfileName].at(0);

	processDPMSControls();

	screendata->enable_dpms = base->systemEnableDPMS->isChecked();
	screendata->dpms_standby_delay = (base->systemEnableDPMSStandby->isChecked())?base->dpmsStandbyTimeout->value()*60:0;
	screendata->dpms_suspend_delay = (base->systemEnableDPMSSuspend->isChecked())?base->dpmsSuspendTimeout->value()*60:0;
	screendata->dpms_off_delay = (base->systemEnableDPMSPowerDown->isChecked())?base->dpmsPowerDownTimeout->value()*60:0;

	changed();
}

void KDisplayConfig::createHotplugRulesGrid() {
	const TQObjectList children = base->profileRulesGridWidget->childrenListObject();
	TQObjectList::iterator it = children.begin();
	for (; it != children.end(); ++it) {
		TQWidget *w = dynamic_cast<TQWidget*>(*it);
		if (w) {
			delete w;
		}
	}

	int i = 0;
	int j = 0;
	TQLabel* label;
	SingleScreenData *screendata;
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_hardwareScreenInfoArray.at(i);
		label = new TQLabel(base->profileRulesGridWidget, (TQString("%1").arg(i)).ascii());
		if (screendata) {
			label->setText(screendata->screenUniqueName);
		}
		profileRulesGrid->addWidget(label, 0, i);
		label->show();
	}
	label = new TQLabel(base->profileRulesGridWidget, "<ignore>");
	label->setText(i18n("Activate Profile on Match"));
	profileRulesGrid->addWidget(label, 0, i+1);
	label->show();

	i=0;
	HotPlugRulesList::Iterator it2;
	for (it2=currentHotplugRules.begin(); it2 != currentHotplugRules.end(); ++it2) {
		for (j=0;j<numberOfScreens;j++) {
			int index = (*it2).outputs.findIndex(m_hardwareScreenInfoArray.at(j)->screenUniqueName);

			TQCheckBox* cb = new TQCheckBox(base->profileRulesGridWidget, (TQString("%1:%2").arg(i).arg(j)).ascii());
			connect(cb, TQT_SIGNAL(stateChanged(int)), this, TQT_SLOT(profileRuleCheckBoxStateChanged(int)));
			connect(cb, TQT_SIGNAL(stateChanged(int)), this, TQT_SLOT(changed()));
			cb->setTristate(true);
			if (index < 0) {
				cb->setNoChange();
			}
			else {
				switch ((*it2).states[index]) {
					case HotPlugRule::AnyState:
						cb->setNoChange();
						break;
					case HotPlugRule::Connected:
						cb->setChecked(true);
						break;
					case HotPlugRule::Disconnected:
						cb->setChecked(false);
						break;
				}
			}
			profileRulesGrid->addWidget(cb, i+1, j);
			cb->show();
		}
		KComboBox* combo = new KComboBox(base->profileRulesGridWidget, (TQString("%1").arg(i)).ascii());
		connect(combo, TQT_SIGNAL(activated(int)), this, TQT_SLOT(changed()));
		combo->insertItem(i18n("<default>"));
		for (TQStringList::Iterator it3 = availableProfileNames.begin(); it3 != availableProfileNames.end(); ++it3) {
			combo->insertItem(*it3);
		}
		combo->setCurrentItem((*it2).profileName);
		profileRulesGrid->addWidget(combo, i+1, j+1);
		combo->show();
		TQPushButton* button = new TQPushButton(base->profileRulesGridWidget, (TQString("%1").arg(i)).ascii());
		button->setText(i18n("Delete Rule"));
		connect(button, TQT_SIGNAL(clicked()), this, TQT_SLOT(deleteProfileRule()));
		connect(button, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
		profileRulesGrid->addWidget(button, i+1, j+2);
		button->show();
		i++;
	}

	TQPushButton* button = new TQPushButton(base->profileRulesGridWidget);
	button->setText(i18n("Add New Rule"));
	connect(button, TQT_SIGNAL(clicked()), this, TQT_SLOT(addNewProfileRule()));
	connect(button, TQT_SIGNAL(clicked()), this, TQT_SLOT(changed()));
	profileRulesGrid->addMultiCellWidget(button, i+2, i+2, 0, numberOfScreens+2);
	button->show();
}

void KDisplayConfig::addNewProfileRule() {
	currentHotplugRules.append(HotPlugRule());
	createHotplugRulesGrid();
}

void KDisplayConfig::deleteProfileRule() {
	const TQWidget* w = dynamic_cast<const TQWidget*>(sender());
	if (w) {
		int row = atoi(w->name());
		currentHotplugRules.remove(currentHotplugRules.at(row));
		createHotplugRulesGrid();
	}
}

void KDisplayConfig::profileRuleCheckBoxStateChanged(int state) {
	updateProfileConfigObjectFromGrid();
	emit(changed());
}

void KDisplayConfig::updateProfileConfigObjectFromGrid() {
	const TQObjectList children = base->profileRulesGridWidget->childrenListObject();
	TQObjectList::iterator it = children.begin();
	for (; it != children.end(); ++it) {
		TQWidget *w = dynamic_cast<TQWidget*>(*it);
		TQCheckBox *cb = dynamic_cast<TQCheckBox*>(w);
		TQComboBox *combo = dynamic_cast<TQComboBox*>(w);
		TQLabel* label = dynamic_cast<TQLabel*>(w);

		if (label) {
			if (TQString(w->name()) != TQString("<ignore>")) {
				int col = atoi(w->name());
				HotPlugRulesList::Iterator it2;
				for (it2=currentHotplugRules.begin(); it2 != currentHotplugRules.end(); ++it2) {
					TQStringList &strlist = (*it2).outputs;
					while (strlist.count() < (uint)numberOfScreens) {
						strlist.append("");
					}
					while (strlist.count() > (uint)numberOfScreens) {
						strlist.remove(strlist.at(strlist.count()-1));
					}
					strlist[col] = label->text();
				}
			}
		}
		if (cb) {
			TQStringList rowcol = TQStringList::split(":", cb->name());
			int row = atoi(rowcol[0].ascii());
			int col = atoi(rowcol[1].ascii());
			TQValueList<int> &intlist = (*(currentHotplugRules.at(row))).states;
			while (intlist.count() < (uint)numberOfScreens) {
				intlist.append(HotPlugRule::AnyState);
			}
			while (intlist.count() > (uint)numberOfScreens) {
				intlist.remove(intlist.at(intlist.count()-1));
			}
			int state = cb->state();
			if (state == TQButton::On) {
				intlist[col] = HotPlugRule::Connected;
			}
			else if (state == TQButton::Off) {
				intlist[col] = HotPlugRule::Disconnected;
			}
			else {
				intlist[col] = HotPlugRule::AnyState;
			}
		}
		if (combo) {
			int row = atoi(w->name());
			(*(currentHotplugRules.at(row))).profileName = combo->currentText();
		}
	}
}

void KDisplayConfig::profileListChanged() {
	// Save selected profile settings
	TQString currentDisplayProfileListItem = base->displayProfileList->currentText();
	TQString currentStartupDisplayProfileListItem = base->startupDisplayProfileList->currentText();

	// Clear and reload the combo boxes
	base->displayProfileList->clear();
	base->startupDisplayProfileList->clear();
	base->displayProfileList->insertItem(i18n("<default>"));
	base->startupDisplayProfileList->insertItem(i18n("<default>"));
	for (TQStringList::Iterator it = availableProfileNames.begin(); it != availableProfileNames.end(); ++it) {
		base->displayProfileList->insertItem(*it);
		base->startupDisplayProfileList->insertItem(*it);
	}

	// Restore selected profile settings if possible
	if (base->displayProfileList->contains(currentDisplayProfileListItem)) {
		base->displayProfileList->setCurrentItem(currentDisplayProfileListItem);
	}
	else {
		base->displayProfileList->setCurrentItem(0);
	}
	if (base->startupDisplayProfileList->contains(currentStartupDisplayProfileListItem)) {
		base->startupDisplayProfileList->setCurrentItem(currentStartupDisplayProfileListItem);
	}
	else {
		base->startupDisplayProfileList->setCurrentItem(0);
	}

	createHotplugRulesGrid();
}

void KDisplayConfig::processDPMSControls() {
	if (base->systemEnableDPMS->isChecked()) {
		base->systemEnableDPMSStandby->setEnabled(true);
		base->systemEnableDPMSSuspend->setEnabled(true);
		base->systemEnableDPMSPowerDown->setEnabled(true);
		base->dpmsStandbyTimeout->setEnabled(base->systemEnableDPMSStandby->isChecked());
		base->dpmsSuspendTimeout->setEnabled(base->systemEnableDPMSSuspend->isChecked());
		base->dpmsPowerDownTimeout->setEnabled(base->systemEnableDPMSPowerDown->isChecked());
	}
	else {
		base->systemEnableDPMSStandby->setEnabled(false);
		base->systemEnableDPMSSuspend->setEnabled(false);
		base->systemEnableDPMSPowerDown->setEnabled(false);
		base->dpmsStandbyTimeout->setEnabled(false);
		base->dpmsSuspendTimeout->setEnabled(false);
		base->dpmsPowerDownTimeout->setEnabled(false);
	}

	if (base->systemEnableDPMSStandby->isChecked()) base->dpmsSuspendTimeout->setMinValue(base->dpmsStandbyTimeout->value());
	else base->dpmsSuspendTimeout->setMinValue(1);
	if (base->systemEnableDPMSSuspend->isChecked()) base->dpmsPowerDownTimeout->setMinValue(base->dpmsSuspendTimeout->value());
	else if (base->systemEnableDPMSStandby->isChecked()) base->dpmsPowerDownTimeout->setMinValue(base->dpmsStandbyTimeout->value());
	else base->dpmsPowerDownTimeout->setMinValue(1);
}

void KDisplayConfig::processLockoutControls() {
	if (!systemconfig->checkConfigFilesWritable( true )) {
		base->globalTab->setEnabled(false);
		base->resolutionTab->setEnabled(false);
		base->gammaTab->setEnabled(false);
		base->powerTab->setEnabled(false);
		base->displayProfileList->setEnabled(false);
		base->addProfileButton->setEnabled(false);
		base->renameProfileButton->setEnabled(false);
		base->deleteProfileButton->setEnabled(false);
		base->reloadProfileButton->setEnabled(false);
		base->saveProfileButton->setEnabled(false);
		base->activateProfileButton->setEnabled(false);
		base->startupDisplayProfileList->setEnabled(false);
		base->systemEnableStartupProfile->setEnabled(false);
		base->groupProfileRules->setEnabled(false);
	}
	else {
		base->globalTab->setEnabled(true);
		if (base->systemEnableSupport->isChecked()) {
			base->resolutionTab->setEnabled(true);
			base->gammaTab->setEnabled(true);
			base->powerTab->setEnabled(true);
			base->displayProfileList->setEnabled(true);
			base->addProfileButton->setEnabled(true);
			base->renameProfileButton->setEnabled(true);
			base->deleteProfileButton->setEnabled(true);
			base->reloadProfileButton->setEnabled(true);
			base->saveProfileButton->setEnabled(true);
			base->activateProfileButton->setEnabled(true);
			base->systemEnableStartupProfile->setEnabled(true);
			base->groupProfileRules->setEnabled(true);
			if (base->systemEnableStartupProfile->isChecked()) {
				base->startupDisplayProfileList->setEnabled(true);
			}
			else {
				base->startupDisplayProfileList->setEnabled(false);
			}
		}
		else {
			base->resolutionTab->setEnabled(false);
			base->gammaTab->setEnabled(false);
			base->powerTab->setEnabled(false);
			base->displayProfileList->setEnabled(false);
			base->addProfileButton->setEnabled(false);
			base->renameProfileButton->setEnabled(false);
			base->deleteProfileButton->setEnabled(false);
			base->reloadProfileButton->setEnabled(false);
			base->saveProfileButton->setEnabled(false);
			base->activateProfileButton->setEnabled(false);
			base->startupDisplayProfileList->setEnabled(false);
			base->systemEnableStartupProfile->setEnabled(false);
			base->groupProfileRules->setEnabled(false);
		}
	}

	base->loadExistingProfile->setEnabled(false);	// Disable this until it works properly!
	base->loadExistingProfile->hide();		// Same as above
}

TDECModule* KDisplayConfig::addTab( const TQString name, const TQString label )
{
	// [FIXME] This is incomplete...Apply may not work...
	TQWidget *page = new TQWidget( base->mainTabContainerWidget, name.latin1() );
	TQVBoxLayout *top = new TQVBoxLayout( page, KDialog::marginHint() );

	TDECModule *kcm = TDECModuleLoader::loadModule( name, page );

	if ( kcm )
	{
		top->addWidget( kcm );
		base->mainTabContainerWidget->addTab( page, label );

		connect( kcm, TQT_SIGNAL( changed(bool) ), this, TQT_SLOT( changed() ) );
		//m_modules.insert(kcm, false);
		return kcm;
	}
	else {
		delete page;
		return NULL;
	}
}

void KDisplayConfig::load(bool useDefaults )
{
	if (getuid() != 0) {
		availableProfileNames = m_randrsimple->getDisplayConfigurationProfiles(locateLocal("config", "/", true));
	}
	else {
		availableProfileNames = m_randrsimple->getDisplayConfigurationProfiles(KDE_CONFDIR);
	}
	profileListChanged();

	// Update the toggle buttons with the current configuration
	updateArray();

	if (getuid() != 0) {
		base->systemEnableSupport->setChecked(m_randrsimple->getDisplayConfigurationEnabled(locateLocal("config", "/", true)));
		base->systemEnableStartupProfile->setChecked(m_randrsimple->getDisplayConfigurationStartupAutoApplyEnabled(locateLocal("config", "/", true)));
		startupProfileName = m_randrsimple->getDisplayConfigurationStartupAutoApplyName(locateLocal("config", "/", true));
	}
	else {
		base->systemEnableStartupProfile->setChecked(m_randrsimple->getDisplayConfigurationStartupAutoApplyEnabled(KDE_CONFDIR));
		base->systemEnableSupport->setChecked(m_randrsimple->getDisplayConfigurationEnabled(KDE_CONFDIR));
		startupProfileName = m_randrsimple->getDisplayConfigurationStartupAutoApplyName(KDE_CONFDIR);
	}
	updateStartupProfileLabel();

	refreshDisplayedInformation();

	gammaselectScreen(base->gammamonitorDisplaySelectDD->currentItem());
	base->gammaTargetSelectDD->clear();
	base->gammaTargetSelectDD->insertItem("1.4", 0);
	base->gammaTargetSelectDD->insertItem("1.6", 1);
	base->gammaTargetSelectDD->insertItem("1.8", 2);
	base->gammaTargetSelectDD->insertItem("2.0", 3);
	base->gammaTargetSelectDD->insertItem("2.2", 4);
	base->gammaTargetSelectDD->insertItem("2.4", 5);
	base->gammaTargetSelectDD->setCurrentItem(4);
	gammaTargetChanged(4);

	if (getuid() != 0) {
		currentHotplugRules = m_randrsimple->getHotplugRules(locateLocal("config", "/", true));
	}
	else {
		currentHotplugRules = m_randrsimple->getHotplugRules(KDE_CONFDIR);
	}
	createHotplugRulesGrid();

	emit changed(useDefaults);
}

void KDisplayConfig::saveActiveSystemWideProfileToDisk()
{
	if (getuid() != 0) {
		m_randrsimple->saveDisplayConfiguration(base->systemEnableSupport->isChecked(), base->systemEnableStartupProfile->isChecked(), activeProfileName, startupProfileName, locateLocal("config", "/", true), m_screenInfoArray[activeProfileName]);
	}
	else {
		m_randrsimple->saveDisplayConfiguration(base->systemEnableSupport->isChecked(), base->systemEnableStartupProfile->isChecked(), activeProfileName, startupProfileName, KDE_CONFDIR, m_screenInfoArray[activeProfileName]);
	}
}

void KDisplayConfig::save()
{
	if (m_randrsimple->applyDisplayConfiguration(m_screenInfoArray[activeProfileName], TRUE)) {
		saveActiveSystemWideProfileToDisk();

		updateProfileConfigObjectFromGrid();
		if (getuid() != 0) {
			m_randrsimple->saveHotplugRules(currentHotplugRules, locateLocal("config", "/", true));
		}
		else {
			m_randrsimple->saveHotplugRules(currentHotplugRules, KDE_CONFDIR);
		}

		// Write system configuration
		systemconfig->setGroup(NULL);
		systemconfig->writeEntry("EnableDisplayControl", base->systemEnableSupport->isChecked());
		systemconfig->writeEntry("EnableAutoStartProfile", base->systemEnableStartupProfile->isChecked());
		systemconfig->writeEntry("StartupProfileName", startupProfileName);

		systemconfig->sync();
		
		if (iccTab) {
			iccTab->save();
		}

		emit changed(false);
	}
	else {
		// Signal that settings were NOT applied
		TQTimer *t = new TQTimer( this );
		connect(t, SIGNAL(timeout()), SLOT(changed()) );
		t->start( 100, FALSE );
	}
}

void KDisplayConfig::defaults()
{
	load( true );
}

TQString KDisplayConfig::handbookSection() const
{
 	int index = base->mainTabContainerWidget->currentPageIndex();
 	if (index == 0)
		return "displayconfig-global";
	else if (index == 1)
		return "displayconfig-resolution";
	else if (index == 2)
		return "displayconfig-brightness";
	else if (index == 3)
		return "displayconfig-power";
	else if (index == 4)
		return "displayconfig-colors";
 	else
 		return TQString::null;
}

TQString KDisplayConfig::quickHelp() const
{
	return i18n("<h1>Monitor & Display Configuration</h1> This module allows you to configure monitors attached to your"
	" computer via TDE.");
}

#include "displayconfig.moc"
