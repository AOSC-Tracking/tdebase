/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _ACTIONS_H_
#define _ACTIONS_H_


#include <tqstring.h>
#include <tqptrlist.h>
#include <tqtimer.h>

#include <kservice.h>

class TDEConfig;

#include "khotkeysglobal.h"

namespace KHotKeys
{

class Action_data;
class Windowdef_list;

// this one is a base for all "real" resulting actions, e.g. running a command,
// Action_data instances usually contain at least one Action
class TDE_EXPORT Action
    {
    public:
        Action( Action_data* data_P );
        Action( TDEConfig& cfg_P, Action_data* data_P );
        virtual ~Action();
        virtual void execute() = 0;
        virtual TQString description() const = 0;
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual Action* copy( Action_data* data_P ) const = 0;
        static Action* create_cfg_read( TDEConfig& cfg_P, Action_data* data_P );
    protected:
        Action_data* const data;
    KHOTKEYS_DISABLE_COPY( Action );
    };

class TDE_EXPORT Action_list
    : public TQPtrList< Action >
    {
    public:
        Action_list( const TQString& comment_P );
        Action_list( TDEConfig& cfg_P, Action_data* data_P );
        void cfg_write( TDEConfig& cfg_P ) const;
        typedef TQPtrListIterator< Action > Iterator;
        const TQString& comment() const;
    private:
        TQString _comment;
    KHOTKEYS_DISABLE_COPY( Action_list );
    };

class TDE_EXPORT Command_url_action
    : public Action
    {
    typedef Action base;
    public:
        Command_url_action( Action_data* data_P, const TQString& command_url_P );
        Command_url_action( TDEConfig& cfg_P, Action_data* data_P );
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual void execute();
        virtual TQString description() const;
        const TQString& command_url() const;
        virtual Action* copy( Action_data* data_P ) const;
    protected:
        TQTimer timeout;
    private:
        TQString _command_url;
    };
    
class TDE_EXPORT Menuentry_action
    : public Command_url_action
    {
    typedef Command_url_action base;
    public:
        Menuentry_action( Action_data* data_P, const TQString& menuentry_P );
        Menuentry_action( TDEConfig& cfg_P, Action_data* data_P );
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual void execute();
        virtual TQString description() const;
        virtual Action* copy( Action_data* data_P ) const;
        KService::Ptr service() const;
    private:
        KService::Ptr _service;
    };
    
class TDE_EXPORT Dcop_action
    : public Action
    {
    typedef Action base;
    public:
        Dcop_action( Action_data* data_P, const TQString& app_P, const TQString& obj_P,
            const TQString& call_P, const TQString& args_P );
        Dcop_action( TDEConfig& cfg_P, Action_data* data_P );
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual void execute();
        const TQString& remote_application() const;
        const TQString& remote_object() const;
        const TQString& called_function() const;
        const TQString& arguments() const;
        virtual TQString description() const;
        virtual Action* copy( Action_data* data_P ) const;
    private:
        TQString app;
        TQString obj;
        TQString call;
        TQString args;
    };
        
class TDE_EXPORT Keyboard_input_action
    : public Action
    {
    typedef Action base;
    public:
        Keyboard_input_action( Action_data* data_P, const TQString& input_P, 
            const Windowdef_list* dest_window_P, bool active_window_P );
        Keyboard_input_action( TDEConfig& cfg_P, Action_data* data_P );
        virtual ~Keyboard_input_action();
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual void execute();
        const TQString& input() const;
        // send to specific window: dest_window != NULL
        // send to active window: dest_window == NULL && activeWindow() == true
        // send to action window: dest_window == NULL && activeWindow() == false
        const Windowdef_list* dest_window() const;
        bool activeWindow() const;
        virtual TQString description() const;
        virtual Action* copy( Action_data* data_P ) const;
    private:
        TQString _input;
        const Windowdef_list* _dest_window;
        bool _active_window;
    };

class TDE_EXPORT Activate_window_action
    : public Action
    {
    typedef Action base;
    public:
        Activate_window_action( Action_data* data_P, const Windowdef_list* window_P );
        Activate_window_action( TDEConfig& cfg_P, Action_data* data_P );
        virtual ~Activate_window_action();
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual void execute();
        const Windowdef_list* window() const;
        virtual TQString description() const;
        virtual Action* copy( Action_data* data_P ) const;
    private:
        const Windowdef_list* _window;
    };
        
class TDE_EXPORT Waiting_action
    : public Action
    {
    typedef Action base;
    public:
        Waiting_action( Action_data* data_P, const int waiting_time );
        Waiting_action( TDEConfig& cfg_P, Action_data* data_P );
        virtual void cfg_write( TDEConfig& cfg_P ) const;
        virtual void execute();
        virtual TQString description() const;
        virtual Action* copy( Action_data* data_P ) const;

        int _waiting_time;
    };

//***************************************************************************
// Inline
//***************************************************************************

// Action
    
inline
Action::Action( Action_data* data_P )
    : data( data_P )
    {
    }
    
inline
Action::Action( TDEConfig&, Action_data* data_P )
    : data( data_P )
    {
    }
    
inline
Action::~Action()
    {
    }

// Action_list
        
inline
Action_list::Action_list( const TQString& comment_P )
    : TQPtrList< Action >(), _comment( comment_P )
    {
    setAutoDelete( true );
    }

inline
const TQString& Action_list::comment() const
    {
    return _comment;
    }

// Command_url_action
    
inline
Command_url_action::Command_url_action( Action_data* data_P, const TQString& command_url_P )
    : Action( data_P ), _command_url( command_url_P )
    {
    }

inline
const TQString& Command_url_action::command_url() const
    {
    return _command_url;
    }
    
// Menuentry_action

inline
Menuentry_action::Menuentry_action( Action_data* data_P, const TQString& menuentry_P )
    : Command_url_action( data_P, menuentry_P )
    {
    }
    
inline
Menuentry_action::Menuentry_action( TDEConfig& cfg_P, Action_data* data_P )
    : Command_url_action( cfg_P, data_P )
    {
    }

// DCOP_action

inline
Dcop_action::Dcop_action( Action_data* data_P, const TQString& app_P, const TQString& obj_P,
    const TQString& call_P, const TQString& args_P )
    : Action( data_P ), app( app_P ), obj( obj_P ), call( call_P ), args( args_P )
    {
    }

inline
const TQString& Dcop_action::remote_application() const
    {
    return app;
    }
    
inline
const TQString& Dcop_action::remote_object() const
    {
    return obj;
    }
    
inline
const TQString& Dcop_action::called_function() const
    {
    return call;
    }
    
inline
const TQString& Dcop_action::arguments() const
    {
    return args;
    }

// Keyboard_input_action

inline
Keyboard_input_action::Keyboard_input_action( Action_data* data_P, const TQString& input_P,
    const Windowdef_list* dest_window_P, bool active_window_P )
    : Action( data_P ), _input( input_P ), _dest_window( dest_window_P ), _active_window( active_window_P )
    {
    }
    
inline
const TQString& Keyboard_input_action::input() const
    {
    return _input;
    }
    
inline
const Windowdef_list* Keyboard_input_action::dest_window() const
    {
    return _dest_window;
    }

inline
bool Keyboard_input_action::activeWindow() const
    {
    return _active_window;
    }

// Activate_window_action

inline
Activate_window_action::Activate_window_action( Action_data* data_P,
    const Windowdef_list* window_P )
    : Action( data_P ), _window( window_P )
    {
    }
    
inline
const Windowdef_list* Activate_window_action::window() const
    {
    return _window;
    }

// Waiting_action

inline
Waiting_action::Waiting_action( Action_data* data_P, const int waiting_time)
    : Action( data_P ), _waiting_time(waiting_time)
    {
    }

} // namespace KHotKeys

#endif
