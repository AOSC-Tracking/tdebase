/****************************************************************************

 KHotKeys
 
 Copyright (C) 2005        Olivier Goffart <ogoffart @ kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#include "voice_settings_tab.h"

#include <tdelocale.h>
#include <tqcombobox.h>
#include <knuminput.h>
#include <tqcheckbox.h>
#include <kkeybutton.h>
#include <kkeydialog.h>

#include "kcmkhotkeys.h"
#include "windowdef_list_widget.h"

namespace KHotKeys
{

Voice_settings_tab::Voice_settings_tab( TQWidget* parent_P, const char* name_P )
    : Voice_settings_tab_ui( parent_P, name_P )
    {
		connect( keyButton , TQ_SIGNAL(capturedShortcut (const TDEShortcut &)) , this, TQ_SLOT(slotCapturedKey( const TDEShortcut& )));
    }

void Voice_settings_tab::read_data()
    {
		keyButton->setShortcut( module->voice_shortcut() );
    }

void Voice_settings_tab::write_data() const
    {
		module->set_voice_shortcut( keyButton->shortcut() );
    }

void Voice_settings_tab::clear_data()
    {
    // "global" tab, not action specific, do nothing
    }
	
void Voice_settings_tab::slotCapturedKey( const TDEShortcut& cut)
   {
	   /*for(uint seq=0; seq<TDEShortcut::MAX_SEQUENCES; seq++)
	   {
		   KKeySequance key=cut.seq(seq);
		   if(key.isNull())
			   continue;
		   if(key.count() > 1)
			   return;
	   }*/
	   
	   if(KKeyChooser::checkGlobalShortcutsConflict(cut,true,this))
		   return;
	   if(KKeyChooser::checkStandardShortcutsConflict(cut,true,this))
		   return;
	   
	   keyButton->setShortcut(cut);
	   module->changed();
   }
} // namespace KHotKeys

#include "voice_settings_tab.moc"
