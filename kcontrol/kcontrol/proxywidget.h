/*

  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/



#ifndef __PROXYWIDGET_H__
#define __PROXYWIDGET_H__


class TQWidget;
class TQPushButton;
class TQFrame;

class TDECModule;
class TDEAboutData;

#include "dockcontainer.h"
#include <tqscrollview.h>

class ProxyView;

class ProxyWidget : public TQWidget
{
  TQ_OBJECT

public:

  ProxyWidget(TDECModule *client, TQString title, const char *name=0, bool run_as_root = false);
  ~ProxyWidget();

  TQString quickHelp() const;
  TQString handbookDocPath() const;
  TQString handbookSection() const;
  const TDEAboutData *aboutData() const;

public slots:

  void handbookClicked();
  void helpClicked();
  void defaultClicked();
  void applyClicked();
  void resetClicked();
  void rootClicked();

  void clientChanged(bool state);


signals:

  void closed();
  void handbookRequest();
  void helpRequest();
  void changed(bool state);
  void runAsRoot();
  void quickHelpChanged();

private:

  TQPushButton *_handbook, *_default, *_apply, *_reset, *_root;
  TQFrame      *_sep;
  TDECModule    *_client;
    ProxyView *view;

};


#endif
