/**
 * (c) Martin R. Jones 1996
 * (c) David Faure 1998, 2000
 * (c) John Firebaugh 2003
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

#ifndef desktopbehavior_h
#define desktopbehavior_h

#include "desktopbehavior.h"
#include "tqlistview.h"
#include <tdeconfig.h>
#include <tdecmodule.h>

class DesktopBehavior : public DesktopBehaviorBase
{
        Q_OBJECT
public:
        DesktopBehavior(TDEConfig *config, TQWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void load( bool useDefaults );
        virtual void save();
        virtual void defaults();
        virtual TQString quickHelp() const;
        virtual TQString handbookSection() const;
        friend class DesktopBehaviorPreviewItem;
        friend class DesktopBehaviorMediaItem;

signals:
        void changed();

private slots:
	void enableChanged();
	void enableGridChanged();
	void comboBoxChanged();
	void editButtonPressed();
	void mediaListViewChanged(TQListViewItem * item);
	void spacingChanged(int value);

private:
        TDEConfig *g_pConfig;

	void fillMediaListView();
	void saveMediaListView();
	void setMediaListViewEnabled(bool enabled);

	void toggleSpacingOpts();

        // Combo for the menus
        void fillMenuCombo( TQComboBox * combo );

        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU, BOOKMARKSMENU=12 } menuChoice;
        bool m_bHasMedia;
};

class DesktopBehaviorModule : public TDECModule
{
        Q_OBJECT

public:
        DesktopBehaviorModule(TDEConfig *config, TQWidget *parent = 0L, const char *name = 0L );
        virtual void load() { m_behavior->load(); emit TDECModule::changed( false ); }
        virtual void save() { m_behavior->save(); emit TDECModule::changed( false ); }
        virtual void defaults() { m_behavior->defaults(); emit TDECModule::changed( true ); }
        virtual TQString handbookSection() const { return m_behavior->handbookSection(); };

private slots:
        void changed();

private:
        DesktopBehavior* m_behavior;
};

#endif
