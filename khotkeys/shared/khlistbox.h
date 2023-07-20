/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2002 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _KHLISTBOX_H_
#define _KHLISTBOX_H_

#include <tqtimer.h>

#include <tdelistbox.h>

namespace KHotKeys
{

class KHListBox
    : public TQListBox
    {
    TQ_OBJECT
    TQ_PROPERTY( bool forceSelect READ forceSelect WRITE setForceSelect )
    public:
        KHListBox( TQWidget* parent_P, const char* name_P = NULL );
        virtual void clear();
        virtual void insertItem( TQListBoxItem* item_P );
        bool forceSelect() const;
        void setForceSelect( bool force_P );
    signals:
        void current_changed( TQListBoxItem* item_P );
    private slots:
        void slot_selection_changed( TQListBoxItem* item_P );
        void slot_selection_changed();
        void slot_current_changed( TQListBoxItem* item_P );
        void slot_insert_select();
    private:
        TQListBoxItem* saved_current_item;
        bool in_clear;
        bool force_select;
        TQTimer insert_select_timer;
    };

//***************************************************************************
// Inline
//***************************************************************************

inline
void KHListBox::setForceSelect( bool force_P )
    {
    force_select = force_P;
    }
    
inline
bool KHListBox::forceSelect() const
    {
    return force_select;
    }

} // namespace KHotKeys

#endif
