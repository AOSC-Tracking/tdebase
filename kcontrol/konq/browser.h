/* This file is part of the KDE project
   Copyright (C) 2002 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#ifndef __KBROWSER_OPTIONS_H__
#define __KBROWSER_OPTIONS_H__

#include <tdecmodule.h>

class TDEConfig;
class TQTabWidget;

//-----------------------------------------------------------------------------

class KBrowserOptions : public TDECModule
{
  TQ_OBJECT
public:
  KBrowserOptions(TDEConfig *config, TQString group, TQWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();
  virtual TQString quickHelp() const;
  virtual TQString handbookDocPath() const;
  virtual TQString handbookSection() const;

private:
   
  TDECModule *appearance;
  TDECModule *behavior;
  TDECModule *previews;
  TDECModule *kuick;
  TQTabWidget *m_tab;
};

#endif
