/*
 *  Copyright (c) 2005      Stefan Nikolaus <stefan.nikolaus@kdemail.net>
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

#ifndef __USER_INTER_CONFIG_H__
#define __USER_INTER_CONFIG_H__

#include <tdecmodule.h>

class userInterOpts;

class userInterConfig : public TDECModule
{
    TQ_OBJECT

public:
    userInterConfig(TDEConfig *config, TQString group,
                    TQWidget *parent = 0, const char *name = 0);

    void load();
    void save();
    void defaults();

public slots:
    void notChanged();

private:
    userInterOpts *m_widget;
};

#endif // __USER_INTER_CONFIG_H__
