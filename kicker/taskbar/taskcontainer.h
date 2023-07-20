/*****************************************************************

Copyright (c) 2001 Matthias Elter <elter@kde.org>
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

#ifndef __taskcontainer_h__
#define __taskcontainer_h__

#include <tqpixmap.h>
#include <tqtimer.h>
#include <tqtoolbutton.h>

#include "kickertip.h"
#include "taskmanager.h"

class TaskBar;
class TaskBarSettings;

typedef TQValueList<TQPixmap*> PixmapList;

class TaskContainer : public TQToolButton, public KickerTip::Client
{
    TQ_OBJECT

public:
    typedef TQValueList<TaskContainer*> List;
    typedef TQValueList<TaskContainer*>::iterator Iterator;

    TaskContainer(Task::Ptr, TaskBar*, TaskBarSettings* settingsObject, TaskBarSettings* globalSettingsObject, TQWidget *parent = 0, const char *name = 0);
    TaskContainer(Startup::Ptr, PixmapList&, TaskBar*, TaskBarSettings* settingsObject, TaskBarSettings* globalSettingsObject, TQWidget *parent = 0, const char *name = 0);
    virtual ~TaskContainer();

    void setArrowType( TQt::ArrowType at );

    void init();

    void add(Task::Ptr);
    void remove(Task::Ptr);
    void remove(Startup::Ptr);

    bool contains(Task::Ptr);
    bool contains(Startup::Ptr);
    bool contains(WId);

    bool isEmpty();
    bool onCurrentDesktop();
    bool isIconified();
    bool isOnScreen();
    bool isHidden();

    TQString id();
    int desktop();
    TQString name();

    virtual TQSizePolicy sizePolicy () const;

    void publishIconGeometry( TQPoint );
    void desktopChanged( int );
    void windowChanged(Task::Ptr);
    void settingsChanged();
    bool eventFilter( TQObject *o, TQEvent *e );

    int taskCount() const { return tasks.count(); }
    int filteredTaskCount() const { return m_filteredTasks.count(); }

    bool activateNextTask( bool forward, bool& forcenext );

    void updateKickerTip(KickerTip::Data&);

    void finish();

    void setBackground();

    Task::List taskList() const { return tasks; }

public slots:
    void updateNow();

signals:
    void showMe(TaskContainer*);

protected:
    void paintEvent(TQPaintEvent*);
    void drawButton(TQPainter*);
    void resizeEvent(TQResizeEvent*);
    void mousePressEvent(TQMouseEvent*);
    void mouseReleaseEvent(TQMouseEvent*);
    void mouseMoveEvent(TQMouseEvent*);
    void dragEnterEvent(TQDragEnterEvent*);
    void dragLeaveEvent(TQDragLeaveEvent*);
    void dropEvent(TQDropEvent*);
    void enterEvent(TQEvent*);
    void leaveEvent(TQEvent*);
    bool startDrag(const TQPoint& pos);
    void stopTimers();

    void performAction(int);
    void popupMenu(int);

    void updateFilteredTaskList();
    void updateIconSize();
    void recalculateIconRect();

protected slots:
    void animationTimerFired();
    void attentionTimerFired();
    void dragSwitch();
    void iconChanged();
    void setLastActivated();
    void taskChanged(bool geometryChangeOnly);
    void showMe();

    void slotTaskMoveBeginning();
    void slotTaskMoveLeft();
    void slotTaskMoveRight();
    void slotTaskMoveEnd();

private:
    void checkAttention(const Task::Ptr changed_task = NULL);
    TQPopupMenu* makeTaskMoveMenu();
    TQString                    sid;
    TQTimer                     animationTimer;
    TQTimer                     dragSwitchTimer;
    TQTimer                     attentionTimer;
    TQTimer                     m_paintEventCompressionTimer;
    int                         currentFrame;
    PixmapList                  frames;
    int                         attentionState;
    TQRect                      iconRect;
    int                         iconSize;
    TQPixmap                    animBg;
    Task::List                  tasks;
    Task::List                  m_filteredTasks;
    Task::Ptr                   lastActivated;
    TQPopupMenu*                m_menu;
    Startup::Ptr                m_startup;
    ArrowType                   arrowType;
    TaskBar*                    taskBar;
    bool                        discardNextMouseEvent;
    bool                        aboutToActivate;
    bool                        m_mouseOver;
    bool                        m_paintEventCompression;
    enum                        { ATTENTION_BLINK_TIMEOUT = 4 };
    TQPoint                     m_dragStartPos;
    TaskBarSettings*            m_settingsObject;
    TaskBarSettings*            m_globalSettingsObject;
};

#endif
