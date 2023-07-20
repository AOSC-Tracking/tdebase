/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2002 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _GESTURE_TRIGGERS_TAB_H_
#define _GESTURE_TRIGGERS_TAB_H_

#include <gesture_triggers_tab_ui.h>

#include <tqstring.h>

#include <kdialogbase.h>

namespace KHotKeys
{

class Trigger_list;
class Action_data;

class Gesture_triggers_tab
    : public Gesture_triggers_tab_ui
    {
    TQ_OBJECT
    public:
        Gesture_triggers_tab( TQWidget* parent_P = NULL, const char* name_P = NULL );
        void set_data( const Trigger_list* triggers_P );
        Trigger_list* get_data( Action_data* data_P ) const;
    protected:
        virtual void edit_gesture_pressed1();
        virtual void edit_gesture_pressed2();
        virtual void edit_gesture_pressed3();
    public slots:
        void clear_data();
    private:
        TQString gesture1, gesture2, gesture3;
    };

class GestureRecordPage;

class Gesture_edit_dialog
    : public KDialogBase
    {
    TQ_OBJECT
    public:
        Gesture_edit_dialog( const TQString& gesture_P );
        TQString edit_gesture();
    private:
        // CHECKME accept() ?
        TQString _gesture;
        GestureRecordPage *_page;
    };        
            

//***************************************************************************
// Inline
//***************************************************************************

} // namespace KHotKeys

#endif
