/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#define _KCMKHOTKEYS_CPP_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kcmkhotkeys.h"

#include <unistd.h>
#include <stdlib.h>

#include <tqlayout.h>
#include <tqsplitter.h>

#include <tdecmodule.h>
#include <tdeaboutdata.h>
#include <tdelocale.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <tdeapplication.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <tdemessagebox.h>
#include <tdeglobal.h>
#include <ksimpleconfig.h>
#include <tdefiledialog.h>
#include <dcopref.h>
#include <klibloader.h>

#include <input.h>
#include <triggers.h>
#include <action_data.h>

#include "tab_widget.h"
#include "actions_listview_widget.h"
#include "main_buttons_widget.h"
#include "voicerecorder.h"

extern "C"
{
    TDE_EXPORT TDECModule* create_khotkeys( TQWidget* parent_P, const char* name_P )
    {
//    sleep( 20 ); // CHECKME DEBUG
    TDEGlobal::locale()->insertCatalogue("khotkeys");
    KHotKeys::Module* ret = new KHotKeys::Module( parent_P, name_P );
    ret->load(); // CHECKME
    return ret;
    }
}

namespace KHotKeys
{

Module::Module( TQWidget* parent_P, const char* )
    : TDECModule( parent_P, "khotkeys" ), _actions_root( NULL ), _current_action_data( NULL ),
        listview_is_changed( false ), deleting_action( false )
    {
    setButtons( Help | Cancel | Apply | Ok );
    module = this;
    init_global_data( false, this ); // don't grab keys
    init_arts();
    TQVBoxLayout* vbox = new TQVBoxLayout( this ); 
    vbox->setSpacing( 6 );
    vbox->setMargin( 11 );
    TQSplitter* splt = new TQSplitter( this );
    actions_listview_widget = new Actions_listview_widget( splt );
    tab_widget = new Tab_widget( splt );
    vbox->addWidget( splt );
    buttons_widget = new Main_buttons_widget( this );
    vbox->addWidget( buttons_widget );
    connect( actions_listview_widget, TQ_SIGNAL( current_action_changed()),
        TQ_SLOT( listview_current_action_changed()));
    connect( buttons_widget, TQ_SIGNAL( new_action_pressed()),  TQ_SLOT( new_action()));
    connect( buttons_widget, TQ_SIGNAL( new_action_group_pressed()),  TQ_SLOT( new_action_group()));
    connect( buttons_widget, TQ_SIGNAL( delete_action_pressed()),  TQ_SLOT( delete_action()));
    connect( buttons_widget, TQ_SIGNAL( global_settings_pressed()), TQ_SLOT( global_settings()));
//    listview_current_action_changed(); // init
						
		TDEAboutData* about = new TDEAboutData("kcmkhotkeys", I18N_NOOP("KHotKeys"), KHOTKEYS_VERSION,
      0,
      TDEAboutData::License_GPL,
      I18N_NOOP("(c) 1999-2005 Lubos Lunak"), 0, 0);
    about->addAuthor("Lubos Lunak", I18N_NOOP("Maintainer"), "l.lunak@kde.org");
    setAboutData( about );
							
		}
    
Module::~Module()
    {
    _current_action_data = NULL;
    tab_widget->load_current_action(); // clear tab_widget
    delete _actions_root;
    module = NULL;
    }
    
void Module::load()
    {
    actions_listview_widget->clear();
    delete _actions_root;
    settings.actions = NULL;
    _current_action_data = NULL;
    settings.read_settings( true );
    _actions_root = settings.actions;
    kdDebug( 1217 ) << "actions_root:" << _actions_root << endl;
    actions_listview_widget->build_up();
    tab_widget->load_current_action();
    emit TDECModule::changed( false ); // HACK otherwise the module would be changed from the very beginning
    }

void Module::save()
    {
    tab_widget->save_current_action_changes();
    settings.actions = _actions_root;
    settings.write_settings();
    if(kapp->dcopClient()->isApplicationRegistered( "khotkeys" ))
        {
        // khotkeys running as a standalone application
        TQByteArray data;
        kdDebug( 1217 ) << "telling khotkeys standalone application to reread configuration" << endl;
        kapp->dcopClient()->send( "khotkeys", "khotkeys", "reread_configuration()", data );
        }
    else
        {
        TQCString replyType;
        TQByteArray replyData;
        if (kapp->dcopClient()->call("kded", "kded", "loadedModules()",
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
                    kapp->dcopClient()->send( "kded", "khotkeys", "reread_configuration()", data );
                    }
                }
            }
        }
    emit TDECModule::changed( false );
    }


TQString Module::quickHelp() const
    {
    return ""; // TODO CHECKME
    }

void Module::action_name_changed( const TQString& name_P )
    {
    current_action_data()->set_name( name_P );
    actions_listview_widget->action_name_changed( name_P );
    }
    
void Module::listview_current_action_changed()
    {
    // CHECKME tohle je trosku hack, aby se pri save zmenenych hodnot ve stare vybrane polozce
    // zmenila data v te stare polozce a ne nove aktivni
    listview_is_changed = true;
    set_new_current_action( !deleting_action );
    listview_is_changed = false;
    }

void Module::set_new_current_action( bool save_old_P )
    {
    if( save_old_P )
        tab_widget->save_current_action_changes();
    _current_action_data = actions_listview_widget->current_action_data();
    kdDebug( 1217 ) << "set_new_current_action : " << _current_action_data << endl;
    tab_widget->load_current_action();
    buttons_widget->enable_delete( current_action_data() != NULL );
    }

// CHECKME volano jen z Tab_widget pro nastaveni zmenenych dat ( novy Action_data_base )
void Module::set_current_action_data( Action_data_base* data_P )
    {
    delete _current_action_data;
    _current_action_data = data_P;
    actions_listview_widget->set_action_data( data_P, listview_is_changed );
//    tab_widget->load_current_action(); CHECKME asi neni treba
    }
    
#if 0

}
#include <iostream>
#include <iomanip>
namespace KHotKeys {

void check_tree( Action_data_group* b, int lev_P = 0 )
    {
    using namespace std;
    cerr << setw( lev_P ) << "" << b << ":Group:" << b->name().latin1() << ":" << b->parent() << endl;
    for( Action_data_group::Iterator it = b->first_child();
         it;
         ++it )
        if( Action_data_group* g = dynamic_cast< Action_data_group* >( *it ))
            check_tree( g, lev_P + 1 );
        else
            cerr << setw( lev_P + 1 ) << "" << (*it) << ":Action:" << (*it)->name().latin1() << ":" << (*it)->parent() << endl;
    }

#endif
    
void Module::new_action()
    {
    tab_widget->save_current_action_changes();
//    check_tree( actions_root());
    Action_data_group* parent = current_action_data() != NULL
        ? dynamic_cast< Action_data_group* >( current_action_data()) : NULL;
    if( parent == NULL )
        {
        if( current_action_data() != NULL )
            parent = current_action_data()->parent();
        else
            parent = module->actions_root();
        }
    Action_data_base* item = new Generic_action_data( parent, i18n( "New Action" ), "",
        new Trigger_list( "" ), new Condition_list( "", NULL ), new Action_list( "" ), true );
    actions_listview_widget->new_action( item );
//    check_tree( actions_root());
    set_new_current_action( false );
    }

// CHECKME spojit tyhle dve do jedne    
void Module::new_action_group()
    {
    tab_widget->save_current_action_changes();
//    check_tree( actions_root());
    Action_data_group* parent = current_action_data() != NULL
        ? dynamic_cast< Action_data_group* >( current_action_data()) : NULL;
    if( parent == NULL )
        {
        if( current_action_data() != NULL )
            parent = current_action_data()->parent();
        else
            parent = module->actions_root();
        }
    Action_data_base* item = new Action_data_group( parent, i18n( "New Action Group" ), "",
        new Condition_list( "", NULL ), Action_data_group::SYSTEM_NONE, true );
    actions_listview_widget->new_action( item );
//    check_tree( actions_root());
    set_new_current_action( false );
    }

void Module::delete_action()
    {
    delete _current_action_data;
    _current_action_data = NULL;
    deleting_action = true; // CHECKME zase tak trosku hack, jinak by se snazilo provest save
    actions_listview_widget->delete_action(); // prave mazane polozky
    deleting_action = false;
    set_new_current_action( false );
    }

void Module::global_settings()
    {
    actions_listview_widget->set_current_action( NULL );
    set_new_current_action( true );
    }

void Module::set_gestures_exclude( Windowdef_list* windows )
    {
    delete settings.gestures_exclude;
    settings.gestures_exclude = windows;
    }

void Module::import()
    {
    TQString file = KFileDialog::getOpenFileName( TQString::null, "*.khotkeys", topLevelWidget(),
        i18n( "Select File with Actions to Be Imported" ));
    if( file.isEmpty())
        return;
    KSimpleConfig cfg( file, true );
    if( !settings.import( cfg, true ))
        {
        KMessageBox::error( topLevelWidget(),
            i18n( "Import of the specified file failed. Most probably the file is not a valid "
                "file with actions." ));
        return;
        }
    actions_listview_widget->clear();
    actions_listview_widget->build_up();
    tab_widget->load_current_action();
    emit TDECModule::changed( true );
    }
    
void Module::changed()
    {
    emit TDECModule::changed( true );
    }

void Module::init_arts()
    {
#ifdef HAVE_ARTS
    if( haveArts())
        {
        KLibrary* arts = KLibLoader::self()->library( "khotkeys_arts" );
        if( arts == NULL )
            kdDebug( 1217 ) << "Couldn't load khotkeys_arts:" << KLibLoader::self()->lastErrorMessage() << endl;
        if( arts != NULL && VoiceRecorder::init( arts ))
            ; // ok
        else
            disableArts();
        }
#endif
    }

Module* module; // CHECKME

} // namespace KHotKeys

#include "kcmkhotkeys.moc"
