/*
 *  Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>
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

#ifndef __kcmlaunch_h__
#define __kcmlaunch_h__

#include <tdecmodule.h>

class TQCheckBox;
class TQComboBox;
class TQGroupBox;

class KIntNumInput;

class LaunchConfig : public TDECModule
{
  TQ_OBJECT

  public:

    LaunchConfig(TQWidget * parent = 0, const char * name = 0, const TQStringList &list = TQStringList() );

    virtual ~LaunchConfig();

    void load();
    void load(bool useDefaults);
    void save();
    void defaults();

  protected slots:

    void checkChanged();
    void slotBusyCursor(int);
    void slotTaskbarButton(bool);

  protected:

    enum FeedbackStyle
    {
      BusyCursor            = 1 << 0,
      TaskbarButton         = 1 << 1,

//      Default = BusyCursor | TaskbarButton
      Default = 0
    };


  private:

    TQLabel    * lbl_cursorTimeout;
    TQLabel    * lbl_taskbarTimeout;
    TQComboBox * cb_busyCursor;
    TQCheckBox * cb_taskbarButton;
    KIntNumInput * sb_cursorTimeout;
    KIntNumInput * sb_taskbarTimeout;

};

#endif
