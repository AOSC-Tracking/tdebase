/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _INPUT_H_
#define _INPUT_H_

#include <tqobject.h>
#include <tqwindowdefs.h>
#include <tqmap.h>
#include <tqwidget.h>
#include <tqvaluelist.h>
#include <tdeshortcut.h>

#include <X11/X.h>
#include <fixx11h.h>

class TDEGlobalAccel;

namespace KHotKeys
{

class Kbd_receiver
    {
    public:
        virtual bool handle_key( const TDEShortcut& shortcut_P ) = 0;
    };

class Kbd
    : public TQObject
    {
    TQ_OBJECT
    public:
	Kbd( bool grabbing_enabled_P, TQObject* parent_P );
        virtual ~Kbd();
	void insert_item( const TDEShortcut& shortcut_P, Kbd_receiver* receiver_P );
        void remove_item( const TDEShortcut& shortcut_P, Kbd_receiver* receiver_P );
        void activate_receiver( Kbd_receiver* receiver_P );
        void deactivate_receiver( Kbd_receiver* receiver_P );
        static bool send_macro_key( const KKey& key, Window window_P = InputFocus );
    protected:
        bool x11EventFilter( const XEvent* );                                                              
        void grab_shortcut( const TDEShortcut& shortcut_P );
        void ungrab_shortcut( const TDEShortcut& shortcut_P );
    private slots:
        void key_slot( TQString key_P );
        void update_connections();
    private:
        struct Receiver_data
            {
            Receiver_data();
            TQValueList< TDEShortcut > shortcuts;
            bool active;
            };
        TQMap< Kbd_receiver*, Receiver_data > receivers;
        TQMap< TDEShortcut, int > grabs;
        TDEGlobalAccel* kga;
    };

class Mouse
    {
    public:
        static bool send_mouse_button( int button_P, bool release_P );
    };


//***************************************************************************
// Inline
//***************************************************************************

// Kbd::Receiver_data

inline
Kbd::Receiver_data::Receiver_data()
    : active( false )
    {
    }

} // namespace KHotKeys

#endif
