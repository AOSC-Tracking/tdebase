/*****************************************************************

Copyright (c) 2002 John Firebaugh <jfirebaugh@kde.org>

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

#include <tqtimer.h>
#include <tqcursor.h>
#include <kdebug.h>

#include "unhidetrigger.h"
#include "unhidetrigger.moc"

UnhideTrigger* UnhideTrigger::the()
{
	static UnhideTrigger UnhideTrigger;
	return &UnhideTrigger;
}

UnhideTrigger::UnhideTrigger()
	: _lastTrigger( None )
	, _lastXineramaScreen( -1 )
	, enabledCount( 0 )
{
	_timer = new TQTimer( this, "UnhideTrigger" );
	connect( _timer, TQ_SIGNAL(timeout()), TQ_SLOT(pollMouse()) );
}

void UnhideTrigger::setEnabled( bool enable )
{
	if( enable ) {
		enabledCount++;
	} else {
		enabledCount--;
	}

	if ( enabledCount > 0 && !_timer->isActive() ) {
		_timer->start( 100 );
	} else if( enabledCount <= 0 ) {
		_timer->stop();
	}
}

bool UnhideTrigger::isEnabled() const
{
	return _timer->isActive();
}

void UnhideTrigger::pollMouse()
{
    TQPoint pos = TQCursor::pos();
    for(int s = 0; s < TQApplication::desktop()->numScreens(); s++)
    {
        TQRect r = TQApplication::desktop()->screenGeometry(s);
        if (pos.x() == r.left())
        {
            if (pos.y() == r.top())
            {
                emitTrigger(TopLeft, s);
            }
            else if (pos.y() == r.bottom())
            {
                emitTrigger(BottomLeft, s);
            }
            else
            {
                emitTrigger(Left, s);
            }
        }
        else if (pos.x() == r.right())
        {
            if (pos.y() == r.top())
            {
                emitTrigger(TopRight, s);
            }
            else if (pos.y() == r.bottom())
            {
                emitTrigger(BottomRight, s);
            }
            else
            {
                emitTrigger(Right, s);
            }
        }
        else if (pos.y() == r.top())
        {
            emitTrigger(Top, s);
        }
        else if (pos.y() == r.bottom())
        {
            emitTrigger(Bottom, s);
        }
        else if (_lastTrigger != None)
        {
            emitTrigger(None, -1);
        }
    }
}

void UnhideTrigger::resetTriggerThrottle()
{
	_lastTrigger = None;
	_lastXineramaScreen = -1;
}

void UnhideTrigger::emitTrigger( Trigger t, int XineramaScreen )
{
	if( _lastTrigger == t  && _lastXineramaScreen == XineramaScreen)
		return;

	resetTriggerThrottle();
	emit triggerUnhide( t, XineramaScreen );
}

void UnhideTrigger::triggerAccepted( Trigger t, int XineramaScreen )
{
	_lastTrigger = t;
	_lastXineramaScreen = XineramaScreen;
}

