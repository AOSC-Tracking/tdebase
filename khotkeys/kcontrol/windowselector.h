/****************************************************************************

 KHotKeys
 
 Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

 Distributed under the terms of the GNU General Public License version 2.
 
****************************************************************************/

#ifndef _WINDOWSELECTOR_H_
#define _WINDOWSELECTOR_H_

#include <tqwidget.h>

namespace KHotKeys
{

class WindowSelector
    : public TQWidget
    {
    TQ_OBJECT
    public:
        WindowSelector( TQObject* receiver, const char* slot );
        void select();
    protected:
        virtual bool x11Event( XEvent* e );
    signals:
        void selected_signal( WId w );
    private:
        WId findRealWindow( WId w, int depth = 0 );
    };
    
//***************************************************************************
// Inline
//***************************************************************************

} // namespace KHotKeys

#endif
