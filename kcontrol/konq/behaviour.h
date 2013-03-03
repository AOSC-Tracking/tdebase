/**
 *  Copyright (c) 2001 David Faure <david@mandrakesoft.com>
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

#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__

#include <kcmodule.h>

class TQCheckBox;
class TQLabel;
class TQRadioButton;
class TQSpinBox;
class TQVButtonGroup;

class KConfig;
class KURLRequester;

//-----------------------------------------------------------------------------


class KBehaviourOptions : public KCModule
{
  Q_OBJECT
public:
  KBehaviourOptions(KConfig *config, TQString group, TQWidget *parent=0, const char *name=0);
    ~KBehaviourOptions();
  virtual void load();
  virtual void load( bool useDefaults );
  virtual void save();
  virtual void defaults();

protected slots:

  void updateWinPixmap(bool);
  void slotShowTips(bool);
private:

  KConfig *g_pConfig;
  TQString groupname;

  TQCheckBox *cbNewWin;
  TQCheckBox *cbListProgress;
  TQCheckBox *cbShowArchivesAsFolders;

  TQLabel *winPixmap;

  KURLRequester *homeURL;

  TQVButtonGroup *bgOneProcess;
  //TQLabel *fileTips;
  //TQSpinBox  *sbToolTip;
  TQCheckBox *cbShowTips;
  TQCheckBox *cbShowPreviewsInTips;
  TQCheckBox *cbRenameDirectlyIcon;

  TQCheckBox *cbMoveToTrash;
  TQCheckBox *cbDelete;
  TQCheckBox *cbShowDeleteCommand;
};

#endif		// __BEHAVIOUR_H__
