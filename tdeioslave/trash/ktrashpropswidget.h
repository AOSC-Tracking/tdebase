/*
   This file is part of the TDE project

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

#ifndef KTRASHPROPSWIDGET_H
#define KTRASHPROPSWIDGET_H

#include "kpropertiesdialog.h"

class KDoubleSpinBox;
class TQCheckBox;
class TQComboBox;
class TQLabel;
class TQRadioButton;
class TQSpinBox;
class TrashImpl;

class KTrashPropsWidget : public TQWidget
{
  TQ_OBJECT

  public:
    KTrashPropsWidget(TQWidget *parent=0, const char *name=0);
   ~KTrashPropsWidget();

	  void load();
	  void save();
	  void setDefaultValues();

	signals:
		void changed(bool state);

  protected slots:
		void setDirty();
    void percentSizeChanged(double);
		void fixedSizeChanged(double);
		void fixedSizeUnitActivated (int);
		void rbPercentSizeToggled(bool);
		void rbFixedSizeToggled(bool);
    void trashChanged(int);
    void useTypeChanged();

  private:
    void readConfig();
    void writeConfig();
    void setupGui();

    TQCheckBox *mUseTimeLimit;
    TQSpinBox  *mDays;
    TQCheckBox *mUseSizeLimit;
    int         mSizeLimitType;
    TQRadioButton *mRbPercentSize, *mRbFixedSize;
    KDoubleSpinBox *mPercentSize;
    KDoubleSpinBox *mFixedSize;
    TQComboBox *mFixedSizeUnit;
    TQLabel    *mSizeLabel, *mLimitLabel;
    TQComboBox *mLimitReachedAction;

    TrashImpl *mTrashImpl;
    TQString mCurrentTrash;
    bool inhibitChangedSignal;

    typedef struct {
      bool useTimeLimit;
      int days;
      bool useSizeLimit;
      int sizeLimitType;
      double percent;
      double fixedSize;
      int fixedSizeUnit;
      int actionType;
    } ConfigEntry;

    typedef TQMap<TQString, ConfigEntry> ConfigMap;
    ConfigMap mConfigMap;
};

#endif
