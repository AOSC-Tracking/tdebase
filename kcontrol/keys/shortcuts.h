/*
 * shortcuts.h
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#ifndef __SHORTCUTS_MODULE_H
#define __SHORTCUTS_MODULE_H

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqpushbutton.h>
#include <tqradiobutton.h>
#include <tqtabwidget.h>
#include "tdeaccelaction.h"
#include <kcombobox.h>
#include <kkeydialog.h>

class ShortcutsModule : public TQWidget
{
	TQ_OBJECT
 public:
	ShortcutsModule( TQWidget *parent = 0, const char *name = 0 );
	~ShortcutsModule();

	void load();
	void save();
	void defaults();
	TQString quickHelp() const;

 protected:
	void initGUI();
	void createActionsGeneral();
	void createActionsSequence();
	void readSchemeNames();
	void saveScheme();
	void resizeEvent(TQResizeEvent *e);

 signals:
	void changed( bool );

 protected slots:
	void slotSchemeCur();
	void slotKeyChange();
	void slotSelectScheme( int = 0 );
	void slotSaveSchemeAs();
	void slotRemoveScheme();
	void slotUseRmWinKeysClicked();

 private:
	TQTabWidget* m_pTab;
	TQRadioButton *m_prbPre, *m_prbNew;
	KComboBox* m_pcbSchemes;
	TQPushButton* m_pbtnSave, * m_pbtnRemove;
	int m_nSysSchemes;
	TQStringList m_rgsSchemeFiles;
	TDEAccelActions m_actionsGeneral, m_actionsSequence;//, m_actionsApplication;
	TDEShortcutList* m_pListGeneral, * m_pListSequence, * m_pListApplication;
	KKeyChooser* m_pkcGeneral, * m_pkcSequence, * m_pkcApplication;
	TQCheckBox* m_useRmWinKeys;
	bool m_bUseRmWinKeys;
};

#endif // __SHORTCUTS_MODULE_H
