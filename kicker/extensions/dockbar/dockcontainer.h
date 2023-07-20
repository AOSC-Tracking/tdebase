/*****************************************************************

Copyright (c) 2000 Matthias Elter <elter@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef __dockcontainer_h__
#define __dockcontainer_h__

#include <tqframe.h>
#include <tqvaluevector.h>

class DockContainer : public TQFrame
{
    TQ_OBJECT

public:
    typedef TQValueVector<DockContainer*> Vector;

    DockContainer( TQString command, TQWidget *parent, 
                   TQString resname,
                   TQString resclass,
                   bool undocked_style=false);

    void embed(WId);
    void unembed();
    void kill();

    WId embeddedWinId() const;
    TQString command() const;
    TQString resClass() const;
    TQString resName() const;
    void askNewCommand(bool bad_command=true);
    void popupMenu(TQPoint p);
    static int& sz();
    static int& border();
signals:
    void embeddedWindowDestroyed(DockContainer*);
    void settingsChanged(DockContainer*);

protected:
    bool x11Event( XEvent * );

private:
    WId _embeddedWinId;
    TQString _command;
    TQString _resName, _resClass;
};


inline WId DockContainer::embeddedWinId() const
{
    return _embeddedWinId;
}

inline TQString DockContainer::command() const
{
    return _command;
}

inline TQString DockContainer::resClass() const
{
    return _resClass;
}

inline TQString DockContainer::resName() const
{
    return _resName;
}


#endif

