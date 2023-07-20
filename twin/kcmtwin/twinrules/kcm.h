/*
 * Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __KCM_H__
#define __KCM_H__

#include <tdecmodule.h>
#include <tdeconfig.h>

class TDEConfig;
class TDEAboutData;

namespace KWinInternal
{

class KCMRulesList;

class KCMRules
    : public TDECModule
    {
    TQ_OBJECT
    public:
        KCMRules( TQWidget *parent, const char *name );
        virtual void load();
        virtual void save();
        virtual void defaults();
        virtual TQString quickHelp() const;
    protected slots:
        void moduleChanged( bool state );
    private:
        KCMRulesList* widget;
        TDEConfig config;
    };

} // namespace

#endif
