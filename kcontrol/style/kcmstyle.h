/*
 * KCMStyle
 * Copyright (C) 2002 Karol Szwed <gallium@kde.org>
 * Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>
 *
 * Portions Copyright (C) TrollTech AS.
 *
 * Based on kcmdisplay
 * Copyright (C) 1997-2002 kcmdisplay Authors.
 * (see Help -> About Style Settings)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __KCMSTYLE_H
#define __KCMSTYLE_H

#include <tqstring.h>
#include <tqtimer.h>

#include <tdecmodule.h>
#include <knuminput.h>

#include "stylepreview.h"
#include "menupreview.h"

class KComboBox;
class TQCheckBox;
class TQComboBox;
class TQFrame;
class TQGroupBox;
class TQLabel;
class TQListBox;
class TQListViewItem;
class TQSettings;
class TQSlider;
class TQSpacerItem;
class TQStyle;
class TQTabWidget;
class TQVBoxLayout;
class StyleConfigDialog;
class WidgetPreview;

struct StyleEntry {
	TQString name;
	TQString desc;
	TQString configPage;
	bool hidden;
};

class KCMStyle : public TDECModule
{
	Q_OBJECT

public:
	KCMStyle( TQWidget* parent = 0, const char* name = 0 );
	~KCMStyle();

	virtual void load();
	virtual void load(bool useDefaults);
	virtual void save();
	virtual void defaults();

	virtual TQString handbookSection() const;

protected:
	bool findStyle( const TQString& str, int& combobox_item );
	void switchStyle(const TQString& styleName, bool force = false);
	void setStyleRecursive(TQWidget* w, TQStyle* s);

	void loadStyle( TDEConfig& config );
	void loadEffects( TDEConfig& config );
	void loadMisc( TDEConfig& config );
	void addWhatsThis();

protected slots:
	void styleSpecificConfig();
	void updateConfigButton();
	
	void setEffectsDirty();
	void setToolbarsDirty();
	void setStyleDirty();

	void styleChanged();
	void menuEffectChanged( bool enabled );
	void menuEffectChanged();
	void menuEffectTypeChanged();

private:
	TQString currentStyle();

	bool m_bEffectsDirty, m_bStyleDirty, m_bToolbarsDirty;
	TQDict<StyleEntry> styleEntries;
	TQMap <TQString,TQString> nameToStyleKey;

	TQVBoxLayout* mainLayout;
	TQTabWidget* tabWidget;
	TQWidget *page1, *page2, *page3;
	TQVBoxLayout* page1Layout;
	TQVBoxLayout* page2Layout;
	TQVBoxLayout* page3Layout;

	// Page1 widgets
	TQGroupBox* gbWidgetStyle;
	TQVBoxLayout* gbWidgetStyleLayout;
	TQHBoxLayout* hbLayout;
	KComboBox* cbStyle;
	TQPushButton* pbConfigStyle;
	TQLabel* lblStyleDesc;
	StylePreview* stylePreview;
	TQStyle* appliedStyle;
	TQPalette palette;
	TQCheckBox* cbIconsOnButtons;
	TQCheckBox* cbScrollablePopupMenus;
	TQCheckBox* cbAutoHideAccelerators;
	TQCheckBox* cbMenuAltKeyNavigation;
	TQCheckBox* cbEnableTooltips;
	KIntNumInput *m_popupMenuDelay;

	// Page2 widgets
	TQCheckBox* cbEnableEffects;

	TQFrame* containerFrame;
	TQGridLayout* containerLayout;
	TQComboBox* comboTooltipEffect;
	TQComboBox* comboRubberbandEffect;
	TQComboBox* comboComboEffect;
	TQComboBox* comboMenuEffect;
	TQComboBox* comboMenuHandle;

	TQLabel* lblTooltipEffect;
	TQLabel* lblRubberbandEffect;
	TQLabel* lblComboEffect;
	TQLabel* lblMenuEffect;
	TQLabel* lblMenuHandle;
	TQSpacerItem* comboSpacer;

	TQFrame* menuContainer;
	TQGridLayout* menuContainerLayout;
	MenuPreview* menuPreview;
	TQVBox* sliderBox;
	TQSlider* slOpacity;
	TQComboBox* comboMenuEffectType;
	TQLabel* lblMenuEffectType;
	TQLabel* lblMenuOpacity;
	TQCheckBox* cbMenuShadow;
	TQCheckBox* cbTearOffHandles;

	// Page3 widgets
	TQGroupBox* gbVisualAppearance;

	TQCheckBox* cbHoverButtons;
	TQCheckBox* cbTransparentToolbars;
	TQComboBox* comboToolbarIcons;

};

#endif // __KCMSTYLE_H
