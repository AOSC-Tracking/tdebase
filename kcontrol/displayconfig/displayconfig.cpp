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

#include <tqcheckbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqlineedit.h>
#include <tqpushbutton.h>
#include <tqtabwidget.h>
#include <tqtimer.h>

#include <dcopclient.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kinputdialog.h>
#include <kurlrequester.h>
#include <kcmoduleloader.h>
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

using namespace std;

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
	int fallback_mode;
	long test_distance;
	long test_center_x;
	long test_center_y;
	for ( int i = 0; i < rectangles.size(); i++ ) {
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
	for ( int i = 0; i < rectangles.size(); i++ ) {
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
	int j;
	DraggableMonitor *primary_monitor;
	DraggableMonitor *moved_monitor;
	SingleScreenData *screendata;
	TQObjectList monitors;

	// Find the moved draggable monitor object
	monitors = base->monitorPhyArrange->childrenListObject();
	if ( monitors.count() ) {
		for ( i = 0; i < int(monitors.count()); ++i ) {
			if (::tqqt_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )))) {
				DraggableMonitor *monitor = static_cast<DraggableMonitor*>(TQT_TQWIDGET(monitors.at( i )));
				if (monitor->screen_id == monitor_id) {
					moved_monitor = monitor;
					screendata = m_screenInfoArray.at(moved_monitor->screen_id);
				}
			}
		}
	}

	TQString rotationDesired = *screendata->rotations.at(screendata->current_rotation_index);
	bool isvisiblyrotated = ((rotationDesired == "Rotate 90 degrees") || (rotationDesired == "Rotate 270 degrees"));

	if (screendata->is_extended) {
		moved_monitor->show();
	}
	else {
		moved_monitor->hide();
	}

	// Handle resizing
	if (isvisiblyrotated)
		moved_monitor->setFixedSize(screendata->current_y_pixel_count*base->monitorPhyArrange->resize_factor, screendata->current_x_pixel_count*base->monitorPhyArrange->resize_factor);
	else
		moved_monitor->setFixedSize(screendata->current_x_pixel_count*base->monitorPhyArrange->resize_factor, screendata->current_y_pixel_count*base->monitorPhyArrange->resize_factor);

	// Find the primary monitor
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray.at(i);
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

			screendata = m_screenInfoArray.at(monitor_id);
			screendata->absolute_x_position = offset_x;
			screendata->absolute_y_position = offset_y;
		}
		else {
			// Reset the position of the primary monitor
			moveMonitor(primary_monitor, 0, 0);
		}
	}
	else {
		printf("[WARNING] Display layout broken...\n\r"); fflush(stdout);
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
	int j;
	bool monitor_was_moved;
	SingleScreenData *screendata;
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
	int j;
	bool primary_found;
	DraggableMonitor *primary_monitor;
	SingleScreenData *screendata;

	// Find the primary monitor
	primary_found = false;
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray.at(i);
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
	int i;
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
	int i;
	int j;
	SingleScreenData *screendata;

	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());
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
	int i;
	int j;
	SingleScreenData *screendata;

	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());
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
  : KCModule(KDisplayCFactory::instance(), parent, name), iccTab(0), m_randrsimple(0), m_gammaApplyTimer(0)
{

	m_randrsimple = new KRandrSimpleAPI();

	TQVBoxLayout *layout = new TQVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());
	systemconfig = new KSimpleConfig( TQString::fromLatin1( KDE_CONFDIR "/kdisplay/kdisplayconfigrc" ));
	systemconfig->setFileWriteMode(0644);

	KAboutData *about =
		new KAboutData(I18N_NOOP("kcmdisplayconfig"), I18N_NOOP("TDE Display Profile Control Module"),
			0, 0, KAboutData::License_GPL,
			I18N_NOOP("(c) 2011 Timothy Pearson"));

	about->addAuthor("Timothy Pearson", 0, "kb9vqf@pearsoncomputing.net");
	setAboutData( about );

	m_gammaApplyTimer = new TQTimer();
	connect(m_gammaApplyTimer, SIGNAL(timeout()), this, SLOT(applyGamma()));

	base = new DisplayConfigBase(this);
	layout->add(base);

	setRootOnlyMsg(i18n("<b>The global display configuration is a system wide setting, and requires administrator access</b><br>To alter the system's global display configuration, click on the \"Administrator Mode\" button below."));
	setUseRootOnlyMsg(true);

	connect(base->systemEnableSupport, TQT_SIGNAL(clicked()), TQT_SLOT(changed()));
	connect(base->systemEnableSupport, TQT_SIGNAL(clicked()), TQT_SLOT(processLockoutControls()));
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

void KDisplayConfig::updateExtendedMonitorInformation () {
	SingleScreenData *screendata;

	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());
	screendata->is_extended = base->isExtendedMonitorCB->isChecked();

	refreshDisplayedInformation();
}

void KDisplayConfig::rescanHardware (void) {
	m_randrsimple->destroyScreenInformationObject(m_screenInfoArray);
	m_screenInfoArray = m_randrsimple->readCurrentDisplayConfiguration();
	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray);
	numberOfScreens = m_screenInfoArray.count();
	refreshDisplayedInformation();
}

void KDisplayConfig::reloadProfile (void) {
	// FIXME
	m_randrsimple->destroyScreenInformationObject(m_screenInfoArray);
	m_screenInfoArray = m_randrsimple->loadSystemwideDisplayConfiguration("", KDE_CONFDIR);
	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray);
	numberOfScreens = m_screenInfoArray.count();
	refreshDisplayedInformation();
}

void KDisplayConfig::identifyMonitors () {
	int i;

	TQLabel* idWidget;
	TQPtrList<TQWidget> widgetList;

	Display *randr_display;
	ScreenInfo *randr_screen_info;
	XRROutputInfo *output_info;

	randr_display = qt_xdisplay();
	randr_screen_info = m_randrsimple->read_screen_info(randr_display);

	for (i = 0; i < m_screenInfoArray.count(); i++) {
		output_info = randr_screen_info->outputs[i]->info;
		// Look for ON outputs...
		if (!randr_screen_info->outputs[i]->cur_crtc) {
			continue;
		}
		SingleScreenData *screendata = m_screenInfoArray.at(i);
		idWidget = new TQLabel(TQString("Screen\n%1").arg(i+1), (TQWidget*)0, "", Qt::WStyle_Customize | Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop | Qt::WX11BypassWM | Qt::WDestructiveClose);
		widgetList.append(idWidget);
		idWidget->resize(150, 100);
		idWidget->setAlignment(Qt::AlignCenter);
		TQFont font = idWidget->font();
		font.setBold( true );
		font.setPointSize(24);
		idWidget->setFont( font );
		idWidget->setPaletteForegroundColor(Qt::white);
		idWidget->setPaletteBackgroundColor(Qt::black);
		idWidget->show();
		KDialog::centerOnScreen(idWidget, i);
		TQTimer::singleShot(3000, idWidget, SLOT(close()));
	}

	m_randrsimple->freeScreenInfoStructure(randr_screen_info);
}

void KDisplayConfig::deleteProfile () {

}

void KDisplayConfig::renameProfile () {

}

void KDisplayConfig::addProfile () {

}

void KDisplayConfig::activatePreview() {
	m_randrsimple->applySystemwideDisplayConfiguration(m_screenInfoArray, TRUE);
}

void KDisplayConfig::load()
{
	load( false );
}

void KDisplayConfig::selectProfile (int slotNumber) {

}

void KDisplayConfig::selectScreen (int slotNumber) {
	base->monitorDisplaySelectDD->setCurrentItem(slotNumber);
	base->gammamonitorDisplaySelectDD->setCurrentItem(slotNumber);
	updateDisplayedInformation();
}

void KDisplayConfig::updateArray (void) {
	m_screenInfoArray = m_randrsimple->readCurrentDisplayConfiguration();
	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray);
	numberOfScreens = m_screenInfoArray.count();
}

void KDisplayConfig::updateDisplayedInformation () {
	// Insert data into the GUI
	int i;
	SingleScreenData *screendata;

	ensureMonitorDataConsistency();

	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());

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
		base->rotationSelectDD->insertItem("Normal", 0);
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
	// Insert data into the GUI
	int i;
	SingleScreenData *screendata;

	// First, the screens
	int currentScreenIndex = base->monitorDisplaySelectDD->currentItem();
	base->monitorDisplaySelectDD->clear();
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray.at(i);
		base->monitorDisplaySelectDD->insertItem(screendata->screenFriendlyName, i);
	}
	base->monitorDisplaySelectDD->setCurrentItem(currentScreenIndex);
	base->gammamonitorDisplaySelectDD->clear();
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray.at(i);
		base->gammamonitorDisplaySelectDD->insertItem(screendata->screenFriendlyName, i);
	}
	base->gammamonitorDisplaySelectDD->setCurrentItem(currentScreenIndex);

	updateDisplayedInformation();

	updateDragDropDisplay();

	screendata = m_screenInfoArray.at(0);
	base->systemEnableDPMS->setEnabled(screendata->has_dpms);
	base->systemEnableDPMS->setChecked(screendata->enable_dpms);
	base->systemEnableDPMSStandby->setChecked(screendata->dpms_standby_delay!=0);
	base->systemEnableDPMSSuspend->setChecked(screendata->dpms_suspend_delay!=0);
	base->systemEnableDPMSPowerDown->setChecked(screendata->dpms_off_delay!=0);
	base->dpmsStandbyTimeout->setValue(screendata->dpms_standby_delay/60);
	base->dpmsSuspendTimeout->setValue(screendata->dpms_suspend_delay/60);
	base->dpmsPowerDownTimeout->setValue(screendata->dpms_off_delay/60);
	processDPMSControls();
}

void KDisplayConfig::updateDragDropDisplay() {
	// Insert data into the GUI
	int i;
	int j;
	int largest_x_pixels;
	int largest_y_pixels;
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

	int currentScreenIndex = base->monitorDisplaySelectDD->currentItem();

	ensureMonitorDataConsistency();

	// Add the screens to the workspace
	// Set the scaling small to start with
	base->monitorPhyArrange->resize_factor = 0.0625;	// This always needs to divide by a multiple of 2
	for (j=0;j<2;j++) {
		for (i=0;i<numberOfScreens;i++) {
			screendata = m_screenInfoArray.at(i);
			if (((j==0) && (screendata->is_primary==true)) || ((j==1) && (screendata->is_primary==false))) {	// This ensures that the primary monitor is always the first one created and placed on the configuration widget
				TQString rotationDesired = *screendata->rotations.at(screendata->current_rotation_index);
				bool isvisiblyrotated = ((rotationDesired == "Rotate 90 degrees") || (rotationDesired == "Rotate 270 degrees"));
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
	int largest_x_pixels;
	int largest_y_pixels;
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
				screendata = m_screenInfoArray.at(monitor->screen_id);
				moveMonitor(monitor, screendata->absolute_x_position, screendata->absolute_y_position);
			}
		}
	}
}

void KDisplayConfig::ensureMonitorDataConsistency() {
	m_randrsimple->ensureMonitorDataConsistency(m_screenInfoArray);
}

void KDisplayConfig::resolutionSliderTextUpdate(int index) {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());

	base->resolutionLabel->setText(screendata->resolutions[realResolutionSliderValue()] + TQString(" ") + i18n("pixels"));
}

void KDisplayConfig::resolutionSliderChanged(int index) {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());

	screendata->current_resolution_index = realResolutionSliderValue();
	updateDisplayedInformation();
	updateDraggableMonitorInformation(base->monitorDisplaySelectDD->currentItem());

	changed();
}

void KDisplayConfig::rotationInfoChanged() {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());

	screendata->current_rotation_index = base->rotationSelectDD->currentItem();
	screendata->has_x_flip = base->orientationHFlip->isChecked();
	screendata->has_y_flip = base->orientationVFlip->isChecked();
	updateDisplayedInformation();
	updateDraggableMonitorInformation(base->monitorDisplaySelectDD->currentItem());

	changed();
}

void KDisplayConfig::refreshInfoChanged() {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray.at(base->monitorDisplaySelectDD->currentItem());

	screendata->current_refresh_rate_index = base->refreshRateDD->currentItem();
	updateDisplayedInformation();
	updateDraggableMonitorInformation(base->monitorDisplaySelectDD->currentItem());

	changed();
}

TQString KDisplayConfig::extractFileName(TQString displayName, TQString profileName) {

}

void KDisplayConfig::ensurePrimaryMonitorIsAvailable() {
	// Ensure that only one monitor, and not less than one monitor, is marked as primary
	int i;
	SingleScreenData *screendata;

	// First, the screens
	int currentScreenIndex = base->monitorDisplaySelectDD->currentItem();
	for (i=0;i<numberOfScreens;i++) {
		screendata = m_screenInfoArray.at(i);
		if (i != currentScreenIndex)
			screendata->is_primary = false;
	}
	screendata = m_screenInfoArray.at(currentScreenIndex);
	screendata->is_primary = true;
	screendata->is_extended = true;
	updateDragDropDisplay();
	refreshDisplayedInformation();
}

int KDisplayConfig::findProfileIndex(TQString profileName) {

}

int KDisplayConfig::findScreenIndex(TQString screenName) {

}

void KDisplayConfig::setGammaLabels() {
	SingleScreenData *screendata;

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());

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

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());
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

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());
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

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());

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

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());
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

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());
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

	screendata = m_screenInfoArray.at(base->gammamonitorDisplaySelectDD->currentItem());
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
	m_randrsimple->applySystemwideDisplayGamma(m_screenInfoArray);
}

void KDisplayConfig::gammaTargetChanged (int slotNumber) {
	TQPixmap gammaPixmap( locate("data", TQString("kcontrol/pics/gamma%1.png").arg(base->gammaTargetSelectDD->text(slotNumber))) );
	base->gammaTestImage->setBackgroundPixmap( gammaPixmap );
}

void KDisplayConfig::dpmsChanged() {
	SingleScreenData *screendata;
	screendata = m_screenInfoArray.at(0);

	processDPMSControls();

	screendata->enable_dpms = base->systemEnableDPMS->isChecked();
	screendata->dpms_standby_delay = (base->systemEnableDPMSStandby->isChecked())?base->dpmsStandbyTimeout->value()*60:0;
	screendata->dpms_suspend_delay = (base->systemEnableDPMSSuspend->isChecked())?base->dpmsSuspendTimeout->value()*60:0;
	screendata->dpms_off_delay = (base->systemEnableDPMSPowerDown->isChecked())?base->dpmsPowerDownTimeout->value()*60:0;

	changed();
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
	if (getuid() != 0 || !systemconfig->checkConfigFilesWritable( true )) {
		base->globalTab->setEnabled(false);
		base->resolutionTab->setEnabled(false);
		base->gammaTab->setEnabled(false);
		base->powerTab->setEnabled(false);
	}
	else {
		base->globalTab->setEnabled(true);
		if (base->systemEnableSupport->isChecked()) {
			base->resolutionTab->setEnabled(true);
			base->gammaTab->setEnabled(true);
			base->powerTab->setEnabled(true);
		}
		else {
			base->resolutionTab->setEnabled(false);
			base->gammaTab->setEnabled(false);
			base->powerTab->setEnabled(false);
		}
	}

	base->loadExistingProfile->setEnabled(false);	// Disable this until it works properly!
	base->loadExistingProfile->hide();		// Same as above
}

KCModule* KDisplayConfig::addTab( const TQString name, const TQString label )
{
	// [FIXME] This is incomplete...Apply may not work...
	TQWidget *page = new TQWidget( base->mainTabContainerWidget, name.latin1() );
	TQVBoxLayout *top = new TQVBoxLayout( page, KDialog::marginHint() );

	KCModule *kcm = KCModuleLoader::loadModule( name, page );

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
	// Update the toggle buttons with the current configuration
	int i;
	int j;

	updateArray();

	systemconfig->setGroup(NULL);
	base->systemEnableSupport->setChecked(systemconfig->readBoolEntry("EnableDisplayControl", false));

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

	emit changed(useDefaults);
}

void KDisplayConfig::save()
{
	if (m_randrsimple->applySystemwideDisplayConfiguration(m_screenInfoArray, TRUE)) {
		m_randrsimple->saveSystemwideDisplayConfiguration(base->systemEnableSupport->isChecked(), "", KDE_CONFDIR, m_screenInfoArray);

		// Write system configuration
		systemconfig->setGroup(NULL);
		systemconfig->writeEntry("EnableDisplayControl", base->systemEnableSupport->isChecked());

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

TQString KDisplayConfig::quickHelp() const
{
	return i18n("<h1>Monitor & Display Configuration</h1> This module allows you to configure monitors attached to your"
	" computer via TDE.");
}

#include "displayconfig.moc"
