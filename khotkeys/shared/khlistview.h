/****************************************************************************

 KHotKeys
 
 Copyright (C) 1999-2002 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _KHLISTVIEW_H_
#define _KHLISTVIEW_H_

#include <tqtimer.h>

#include <tdelistview.h>
#include <kdemacros.h>

namespace KHotKeys
{

class TDE_EXPORT KHListView
    : public TDEListView
    {
    TQ_OBJECT
    TQ_PROPERTY( bool forceSelect READ forceSelect WRITE setForceSelect )
    public:
        KHListView( TQWidget* parent_P, const char* name_P = NULL );
        virtual void clear();
        virtual void insertItem( TQListViewItem* item_P );
        virtual void clearSelection();
        bool forceSelect() const;
        void setForceSelect( bool force_P );
    signals:
        void current_changed( TQListViewItem* item_P );
    protected:
        virtual void contentsDropEvent (TQDropEvent*);
    private slots:
        void slot_selection_changed( TQListViewItem* item_P );
        void slot_selection_changed();
        void slot_current_changed( TQListViewItem* item_P );
        void slot_insert_select();
    private:
        TQListViewItem* saved_current_item;
        bool in_clear;
        bool ignore;
        bool force_select;
        TQTimer insert_select_timer;
    };

//***************************************************************************
// Inline
//***************************************************************************

inline
void KHListView::setForceSelect( bool force_P )
    {
    force_select = force_P;
    }
    
inline
bool KHListView::forceSelect() const
    {
    return force_select;
    }
    
} // namespace KHotKeys

#endif
