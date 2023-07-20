/***************************************************************************
 *   Copyright (C) 2003 by Martin Koller                                   *
 *   m.koller@surfeu.at                                                    *
 *   This file is part of the Trinity Control Center Module for Joysticks  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#ifndef _JOYWIDGET_H_
#define _JOYWIDGET_H_

#include <tqwidget.h>

class JoyDevice;

class PosWidget;
class TQLabel;
class TQTable;
class TQTimer;
class TQComboBox;
class TQPushButton;
class TQCheckBox;
class TQHBox;

// the widget which displays all buttons, values, etc.
class JoyWidget : public TQWidget
{
  TQ_OBJECT
  
  public:
    JoyWidget(TQWidget *parent = 0, const char *name = 0);

    ~JoyWidget();

    // initialize list of possible devices and open the first available
    void init();

  public slots:
    // reset calibration values to their value when this KCM was started
    void resetCalibration();

  private slots:
    void checkDevice();
    void deviceChanged(const TQString &dev);
    void traceChanged(bool);
    void calibrateDevice();

  private:
    void showDeviceProps(JoyDevice *joy);  // fill widgets with given device parameters
    void restoreCurrDev(); // restores the content of the combobox to reflect the current open device

  private:
    TQHBox  *messageBox;
    TQLabel *message;  // in case of no device, show here a message rather than in a dialog
    TQComboBox *device;
    PosWidget *xyPos;
    TQTable *buttonTbl;
    TQTable *axesTbl;
    TQCheckBox *trace;
    TQPushButton *calibrate;

    TQTimer *idle;

    JoyDevice *joydev;
};

#endif
