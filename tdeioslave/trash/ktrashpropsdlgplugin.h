/*
   This file is part of the TDE project

   Copyright (C) 2008 Tobias Koenig <tokoe@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KTRASHPROPSDLGPLUGIN_H
#define KTRASHPROPSDLGPLUGIN_H

#include <kpropertiesdialog.h>

class KDoubleSpinBox;
class TQCheckBox;
class TQComboBox;
class TQFrame;
class TQLabel;
class TQSpinBox;
class TrashImpl;

class KTrashPropsDlgPlugin : public KPropsDlgPlugin
{
  Q_OBJECT

  public:
    KTrashPropsDlgPlugin( KPropertiesDialog *dialog, const char*, const TQStringList& );
    ~KTrashPropsDlgPlugin();

    virtual void applyChanges();

  private slots:
    void percentChanged( double );
    void trashChanged( int );
    void useTypeChanged();

  private:
    void readConfig();
    void writeConfig();
    void setupGui( TQFrame *frame );

    TQCheckBox *mUseTimeLimit;
    TQSpinBox *mDays;
    TQCheckBox *mUseSizeLimit;
    TQWidget *mSizeWidget;
    KDoubleSpinBox *mPercent;
    TQLabel *mSizeLabel;
    TQComboBox *mLimitReachedAction;

    TrashImpl *mTrashImpl;
    TQString mCurrentTrash;

    typedef struct {
      bool useTimeLimit;
      int days;
      bool useSizeLimit;
      double percent;
      int actionType;
    } ConfigEntry;

    typedef TQMap<TQString, ConfigEntry> ConfigMap;
    ConfigMap mConfigMap;
};

#endif
