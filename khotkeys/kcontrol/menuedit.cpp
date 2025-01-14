/****************************************************************************

 KHotKeys

 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.

****************************************************************************/

// BEWARE ! unbelievably messy code

#define _MENUEDIT_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "menuedit.h"

#include <tdeglobal.h>
#include <tdelocale.h>
#include <tdeaccel.h>
#include <tdeapplication.h>
#include <dcopclient.h>
#include <tqlabel.h>
#include <tqlineedit.h>
#include <tqlayout.h>
#include <kkeydialog.h>

#include <settings.h>
#include <action_data.h>

namespace KHotKeys
{

static TQObject* owner = NULL;

void khotkeys_init()
    {
    // I hope this works
    TDEGlobal::locale()->insertCatalogue("khotkeys");
    // CHECKME hack
    assert( owner == NULL );
    owner = new TQObject;
    init_global_data( false, owner );
    }

void khotkeys_cleanup()
    {
    delete owner;
    owner = NULL;
    }

Menuentry_shortcut_action_data* khotkeys_get_menu_entry_internal2(
    const Action_data_group* data_P, const TQString& entry_P )
    {
    if( !data_P->enabled( false ))
        return NULL;
    for( Action_data_group::Iterator it = data_P->first_child();
         it;
         ++it )
        {
        if( !(*it)->enabled( true ))
            continue;
        if( Menuentry_shortcut_action_data* entry
            = dynamic_cast< Menuentry_shortcut_action_data* >( *it ))
            {
               KService::Ptr service = entry->action() ? entry->action()->service() : KService::Ptr(0);
               if ( service && (service->storageId() == entry_P) )
                    return entry;
            }
        if( Action_data_group* group = dynamic_cast< Action_data_group* >( *it ))
            {
            Menuentry_shortcut_action_data* data
                = khotkeys_get_menu_entry_internal2( group, entry_P );
            if( data != NULL )
                return data;
            }
        }
    return NULL;
    }

Action_data_group* khotkeys_get_menu_root( Action_data_group* data_P )
    {
    for( Action_data_group::Iterator it = data_P->first_child();
         it;
         ++it )
        if( Action_data_group* group = dynamic_cast< Action_data_group* >( *it ))
            {
            if( group->system_group() == Action_data_group::SYSTEM_MENUENTRIES )
                return group;
            }
    return new Action_data_group( data_P, i18n( MENU_EDITOR_ENTRIES_GROUP_NAME ),
        i18n( "These entries were created using Menu Editor." ), new Condition_list( "", NULL ), // CHECKME tenhle condition list
        Action_data_group::SYSTEM_MENUENTRIES, true );
    }

Menuentry_shortcut_action_data* khotkeys_get_menu_entry_internal( Action_data_group* data_P,
    const TQString& entry_P )
    {
    return khotkeys_get_menu_entry_internal2( khotkeys_get_menu_root( data_P ), entry_P );
    }

TQString khotkeys_get_menu_shortcut( Menuentry_shortcut_action_data* data_P )
    {
    if( data_P->trigger() != NULL )
        return data_P->trigger()->shortcut().toString();
    return "";
    }

void khotkeys_get_all_shortcuts_internal(const Action_data_group* data_P, TQStringList &result)
    {
    if( !data_P->enabled( false ))
        return;
    for( Action_data_group::Iterator it = data_P->first_child();
         it;
         ++it )
        {
        if( !(*it)->enabled( true ))
            continue;
        if( Menuentry_shortcut_action_data* entry
            = dynamic_cast< Menuentry_shortcut_action_data* >( *it ))
            {
               if (entry->trigger() && !entry->trigger()->shortcut().isNull())
                    result.append(entry->trigger()->shortcut().toString());
            }
        if( Action_data_group* group = dynamic_cast< Action_data_group* >( *it ))
            {
               khotkeys_get_all_shortcuts_internal( group, result );
            }
        }
    }


TQStringList khotkeys_get_all_shortcuts( )
    {
    TQStringList result;
    Settings settings;
    settings.read_settings( true );

    khotkeys_get_all_shortcuts_internal(settings.actions, result);

    return result;
    }


KService::Ptr khotkeys_find_menu_entry_internal(const Action_data_group* data_P, const TQString &shortcut_P)
    {
    if( !data_P->enabled( false ))
        return 0;
    for( Action_data_group::Iterator it = data_P->first_child();
         it;
         ++it )
        {
        if( !(*it)->enabled( true ))
            continue;
        if( Menuentry_shortcut_action_data* entry
            = dynamic_cast< Menuentry_shortcut_action_data* >( *it ))
            {
               if (entry->trigger() &&
                   entry->trigger()->shortcut().toString() == shortcut_P)
               {
                  if (entry->action())
                     return entry->action()->service();
                  return 0;
               }
            }
        if( Action_data_group* group = dynamic_cast< Action_data_group* >( *it ))
            {
               KService::Ptr result = khotkeys_find_menu_entry_internal( group, shortcut_P );
               if (result)
                  return result;
            }
        }
        return 0;
    }


KService::Ptr khotkeys_find_menu_entry( const TQString& shortcut_P )
    {
    Settings settings;
    settings.read_settings( true );

    return khotkeys_find_menu_entry_internal(settings.actions, shortcut_P);
    }


void khotkeys_send_reread_config()
    {
    TQByteArray data;
    if( !tdeApp->dcopClient()->isAttached())
        tdeApp->dcopClient()->attach();
    if(tdeApp->dcopClient()->isApplicationRegistered( "khotkeys" ))
        {
        // khotkeys running as a standalone application
        TQByteArray data;
        kdDebug( 1217 ) << "telling khotkeys standalone application to reread configuration" << endl;
        tdeApp->dcopClient()->send( "khotkeys", "khotkeys", "reread_configuration()", data );
        }
    else
        {
        TQCString replyType;
        TQByteArray replyData;
        if (tdeApp->dcopClient()->call("kded", "kded", "loadedModules()",
              TQByteArray(), replyType, replyData))
            {
            if (replyType == "QCStringList")
                {
                TQDataStream reply(replyData, IO_ReadOnly);
                QCStringList modules;
                reply >> modules;
                if (modules.contains("khotkeys"))
                    {
                    // khotkeys running as a kded service
                    TQByteArray data;
                    kdDebug( 1217 ) << "telling khotkeys kded daemon to reread configuration" << endl;
                    tdeApp->dcopClient()->send( "kded", "khotkeys", "reread_configuration()", data );
                    }
                }
            }
        }
    }

TQString khotkeys_get_menu_entry_shortcut( const TQString& entry_P )
    {
    Settings settings;
    settings.read_settings( true );
    Menuentry_shortcut_action_data* entry
        = khotkeys_get_menu_entry_internal( settings.actions, entry_P );
    if( entry == NULL )
        {
        delete settings.actions;
        return "";
        }
    TQString shortcut = khotkeys_get_menu_shortcut( entry );
    delete settings.actions;
    return shortcut;
    }

bool khotkeys_menu_entry_moved( const TQString& new_P, const TQString& old_P )
    {
    Settings settings;
    settings.read_settings( true );
    Menuentry_shortcut_action_data* entry
        = khotkeys_get_menu_entry_internal( settings.actions, old_P );
    if( entry == NULL )
        {
        delete settings.actions;
        return false;
        }
    Action_data_group* parent = entry->parent();
    TQString new_name = new_P;
    if( entry->name().startsWith( i18n( "TDE Menu - " )))
        new_name = i18n( "TDE Menu - " ) + new_P;
    Menuentry_shortcut_action_data* new_entry = new Menuentry_shortcut_action_data( parent,
        new_name, entry->comment(), entry->enabled( true ));
    new_entry->set_trigger( entry->trigger()->copy( new_entry ));
    new_entry->set_action( new Menuentry_action( new_entry, new_P ));
    delete entry;
    settings.write_settings();
    delete settings.actions;
    khotkeys_send_reread_config();
    return true;
    }

void khotkeys_menu_entry_deleted( const TQString& entry_P )
    {
    Settings settings;
    settings.read_settings( true );
    Menuentry_shortcut_action_data* entry
        = khotkeys_get_menu_entry_internal( settings.actions, entry_P );
    if( entry == NULL )
        {
        delete settings.actions;
        return;
        }
    delete entry;
    settings.write_settings();
    delete settings.actions;
    khotkeys_send_reread_config();
    }

TQString khotkeys_change_menu_entry_shortcut( const TQString& entry_P,
    const TQString& shortcut_P )
    {
    Settings settings;
    settings.read_settings( true );
    Menuentry_shortcut_action_data* entry
        = khotkeys_get_menu_entry_internal( settings.actions, entry_P );
    bool new_entry = ( entry == NULL );
    if( new_entry )
        {
        entry = new Menuentry_shortcut_action_data( NULL, i18n( "TDE Menu - " ) + entry_P,
            "" );
        entry->set_action( new Menuentry_action( entry, entry_P ));
        }
    else
        {
        // erase the trigger, i.e. replace with a copy with no trigger and no parent yet
        Menuentry_shortcut_action_data* entry_tmp = new Menuentry_shortcut_action_data( NULL,
            entry->name(), entry->comment(), entry->enabled( false ));
        entry_tmp->set_action( new Menuentry_action( entry_tmp, entry_P ));
        delete entry;
        entry = entry_tmp;
        }
    TQString shortcut = "";
    // make sure the shortcut is valid
    shortcut = (TDEShortcut( shortcut_P )).toStringInternal();
    if( !shortcut.isEmpty())
        entry->set_trigger( new Shortcut_trigger( entry, TDEShortcut( shortcut )));
    if( shortcut.isEmpty())
        {
        delete entry;
        if( !new_entry ) // remove from config file
            {
            settings.write_settings();
            khotkeys_send_reread_config();
            }
        delete settings.actions;
        return "";
        }
    entry->reparent( khotkeys_get_menu_root( settings.actions ));
    settings.write_settings();
    khotkeys_send_reread_config();
    return shortcut;
    }

// CHECKME nejaka forma kontroly, ze tahle kl. kombinace neni pouzita pro jiny menuentry ?

} // namespace KHotKeys

//
// exported functions
//

void khotkeys_init()
    {
    KHotKeys::khotkeys_init();
    }

void khotkeys_cleanup()
    {
    KHotKeys::khotkeys_cleanup();
    }

TQString khotkeys_get_menu_entry_shortcut( const TQString& entry_P )
    {
    return KHotKeys::khotkeys_get_menu_entry_shortcut( entry_P );
    }

bool khotkeys_menu_entry_moved( const TQString& new_P, const TQString& old_P )
    {
    return KHotKeys::khotkeys_menu_entry_moved( new_P, old_P );
    }

void khotkeys_menu_entry_deleted( const TQString& entry_P )
    {
    KHotKeys::khotkeys_menu_entry_deleted( entry_P );
    }

TQString khotkeys_change_menu_entry_shortcut( const TQString& entry_P,
    const TQString& shortcut_P )
    {
    return KHotKeys::khotkeys_change_menu_entry_shortcut( entry_P, shortcut_P );
    }

TQStringList khotkeys_get_all_shortcuts( )
    {
    return KHotKeys::khotkeys_get_all_shortcuts();
    }

KService::Ptr khotkeys_find_menu_entry( const TQString& shortcut_P )
    {
    return KHotKeys::khotkeys_find_menu_entry( shortcut_P );
    }
