/*
 *  userInterOpts_impl.h
 *
 *  Copyright (c) 2002 Aaron J. Seigo <aseigo@olympusproject.org>
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

#ifndef __USERINTERFACE_OPTIONS_IMPL_H
#define __USERINTERFACE_OPTIONS_IMPL_H

#include "userInterOpts.h"
      
class userInterOpts : public userInterOptsBase
{
  TQ_OBJECT

  public:
    userInterOpts(TDEConfig *config, TQString groupName,
                  TQWidget* parent =0, const char* name =0);

    void load();
    void load(bool useDefaults);
    void save();
    void defaults();
        
  signals:
    void changed();

  protected:
    TDEConfig *m_pConfig;
    TQString   m_groupName;

  private slots:
    void slotChanged();
};

#endif
