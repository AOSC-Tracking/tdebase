#ifndef SCOPEITEM_H
#define SCOPEITEM_H

#include <tqlistview.h>

#include "docmetainfo.h"

namespace KHC {

class ScopeItem : public TQCheckListItem
{
  public:
    ScopeItem( TQListView *parent, DocEntry *entry )
      : TQCheckListItem( parent, entry->name(), TQCheckListItem::CheckBox ),
        mEntry( entry ), mObserver( 0 ) {}

    ScopeItem( TQListViewItem *parent, DocEntry *entry )
      : TQCheckListItem( parent, entry->name(), TQCheckListItem::CheckBox ),
        mEntry( entry ), mObserver( 0 ) {}

    DocEntry *entry()const { return mEntry; }

    int rtti() const { return rttiId(); }

    static int rttiId() { return 734678; }

    class Observer
    {
      public:
        virtual void scopeItemChanged( ScopeItem * ) = 0;
    };

    void setObserver( Observer *o ) { mObserver = o; }

  protected:
    void stateChange ( bool )
    {
      if ( mObserver ) mObserver->scopeItemChanged( this );
    }

  private:
    DocEntry *mEntry;

    Observer *mObserver;
};

}

#endif
