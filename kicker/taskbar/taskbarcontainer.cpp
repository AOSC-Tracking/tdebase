/*****************************************************************

Copyright (c) 2001 John Firebaugh <jfirebaugh@kde.org>

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

#include <tqlayout.h>
#include <tqtimer.h>
#include <tqfile.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <twindowlistmenu.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "simplebutton.h"

#include "taskbar.h"
#include "taskbarsettings.h"

#include "taskbarcontainer.h"
#include "taskbarcontainer.moc"

#define GLOBAL_TASKBAR_CONFIG_FILE_NAME "ktaskbarrc"

#define READ_MERGED_TASBKAR_SETTING(x) ((settingsObject->useGlobalSettings())?globalSettingsObject->x():settingsObject->x())

TaskBarContainer::TaskBarContainer( bool enableFrame, TQString configFileOverride, TQWidget *parent, const char *name )
    : TQFrame(parent, name),
      configFile(configFileOverride),
      direction( KPanelApplet::Up ),
      showWindowListButton( true ),
      windowListButton(0),
      windowListMenu(0),
      settingsObject(NULL),
      globalSettingsObject(NULL)
{
    if (configFile == "")
    {
        configFile = GLOBAL_TASKBAR_CONFIG_FILE_NAME;
    }
    TQFile configFileObject(locateLocal("config", configFile));
    if (!configFileObject.exists())
    {
        TDEConfig globalConfig(GLOBAL_TASKBAR_CONFIG_FILE_NAME, TRUE, TRUE);
        TDEConfig localConfig(configFile);
        globalConfig.copyTo(configFile, &localConfig);
        localConfig.writeEntry("UseGlobalSettings", TRUE);
        localConfig.sync();
    }
    settingsObject = new TaskBarSettings(TDESharedConfig::openConfig(configFile));
    globalSettingsObject = new TaskBarSettings(TDESharedConfig::openConfig(GLOBAL_TASKBAR_CONFIG_FILE_NAME));

    setAcceptDrops(true); // Always enabled to activate task during drag&drop.

    setBackgroundOrigin( AncestorOrigin );

    uint margin;
    if ( enableFrame )
    {
        setFrameStyle( Sunken | StyledPanel );
        margin = frameWidth();
    }
    else
    {
        setFrameStyle( NoFrame );
        margin = 0;
    }

    layout = new TQBoxLayout( this, TQApplication::reverseLayout() ?
                                   TQBoxLayout::RightToLeft :
                                   TQBoxLayout::LeftToRight );
    layout->setMargin( margin );

    // scrollable taskbar
    taskBar = new TaskBar(settingsObject, globalSettingsObject, this);
    layout->addWidget( taskBar );

    connect( taskBar, TQ_SIGNAL( containerCountChanged() ), TQ_SIGNAL( containerCountChanged() ) );

    setBackground();

    // read settings and setup layout
    configure();

    connectDCOPSignal("", "", "kdeTaskBarConfigChanged()",
                      "configChanged()", false);
}

TaskBarContainer::~TaskBarContainer()
{
    if (windowListMenu) delete windowListMenu;
    if (settingsObject) delete settingsObject;
    if (globalSettingsObject) delete globalSettingsObject;
}

void TaskBarContainer::configure()
{
    setFont(READ_MERGED_TASBKAR_SETTING(taskbarFont));
    showWindowListButton = READ_MERGED_TASBKAR_SETTING(showWindowListBtn);

    if (!showWindowListButton)
    {
        delete windowListButton;
        windowListButton = 0;
        delete windowListMenu;
        windowListMenu = 0;
    }
    else if (windowListButton == 0)
    {
        // window list button
        windowListButton = new SimpleButton(this);
        windowListMenu= new KWindowListMenu;
        connect(windowListButton, TQ_SIGNAL(pressed()),
                TQ_SLOT(showWindowListMenu()));
        connect(windowListMenu, TQ_SIGNAL(aboutToHide()),
                TQ_SLOT(windowListMenuAboutToHide()));

        // geometry
        TQString icon;
        switch (direction)
        {
            case KPanelApplet::Up:
                icon = "1uparrow";
                windowListButton->setMaximumHeight(BUTTON_MAX_WIDTH);
                break;
            case KPanelApplet::Down:
                icon = "1downarrow";
                windowListButton->setMaximumHeight(BUTTON_MAX_WIDTH);
                break;
            case KPanelApplet::Left:
                icon = "1leftarrow";
                windowListButton->setMaximumWidth(BUTTON_MAX_WIDTH);
                break;
            case KPanelApplet::Right:
                icon = "1rightarrow";
                windowListButton->setMaximumWidth(BUTTON_MAX_WIDTH);
                break;
        }

        windowListButton->setPixmap(tdeApp->iconLoader()->loadIcon(icon,
                                                                 TDEIcon::Panel,
                                                                 16));
        windowListButton->setMinimumSize(windowListButton->sizeHint());
        layout->insertWidget(0, windowListButton);
        windowListButton->show();
    }
}

void TaskBarContainer::configChanged()
{
    // we have a separate method here to connect to the DCOP signal
    // instead of connecting direclty to taskbar so that Taskbar
    // doesn't have to also connect to the DCOP signal (less places
    // to change/fix it if/when it changes) without calling
    // configure() twice on taskbar on start up
    settingsObject->readConfig();
    globalSettingsObject->readConfig();

    configure();
    taskBar->configure();
}

void TaskBarContainer::preferences()
{
    TQByteArray data;

    if (!tdeApp->dcopClient()->isAttached())
    {
        tdeApp->dcopClient()->attach();
    }

    if (configFile == GLOBAL_TASKBAR_CONFIG_FILE_NAME)
    {
        tdeApp->dcopClient()->send("kicker", "kicker", "showTaskBarConfig()", data);
    }
    else
    {
        TQDataStream args( data, IO_WriteOnly );
        args << configFile;
        tdeApp->dcopClient()->send("kicker", "kicker", "showTaskBarConfig(TQString)", data);
    }
}

void TaskBarContainer::orientationChange(Orientation o)
{
    if (o == TQt::Horizontal)
     {
        if (windowListButton)
        {
            windowListButton->setFixedWidth(WINDOWLISTBUTTON_SIZE);
            windowListButton->setMaximumHeight(BUTTON_MAX_WIDTH);
        }
        layout->setDirection(TQApplication::reverseLayout() ?
                                TQBoxLayout::RightToLeft :
                                TQBoxLayout::LeftToRight);
    }
    else
    {
        if (windowListButton)
        {
            windowListButton->setMaximumWidth(BUTTON_MAX_WIDTH);
            windowListButton->setFixedHeight(WINDOWLISTBUTTON_SIZE);
        }
        layout->setDirection(TQBoxLayout::TopToBottom);
    }

    taskBar->setOrientation(o);
    if (windowListButton)
    {
        windowListButton->setOrientation(o);
    }
    layout->activate();
}

void TaskBarContainer::popupDirectionChange(KPanelApplet::Direction d)
{
    direction = d;
    ArrowType at = UpArrow;

    TQString icon;
    switch (d)
    {
        case KPanelApplet::Up:
            icon = "1uparrow";
            at = UpArrow;
            break;
        case KPanelApplet::Down:
            icon = "1downarrow";
            at = DownArrow;
            break;
        case KPanelApplet::Left:
            icon = "1leftarrow";
            at = LeftArrow;
            break;
        case KPanelApplet::Right:
            icon = "1rightarrow";
            at = RightArrow;
            break;
    }

    taskBar->setArrowType(at);

    if (windowListButton)
    {
        windowListButton->setPixmap(tdeApp->iconLoader()->loadIcon(icon,
                                                                 TDEIcon::Panel,
                                                                 16));
        windowListButton->setMinimumSize(windowListButton->sizeHint());
    }
}

void TaskBarContainer::showWindowListMenu()
{
    if (!windowListMenu)
        return;

    windowListMenu->init();

    // calc popup menu position
    TQPoint pos( mapToGlobal( TQPoint(0,0) ) );

    switch( direction ) {
        case KPanelApplet::Right:
            pos.setX( pos.x() + width() );
            break;
        case KPanelApplet::Left:
            pos.setX( pos.x() - windowListMenu->sizeHint().width() );
            break;
        case KPanelApplet::Down:
            pos.setY( pos.y() + height() );
            break;
        case KPanelApplet::Up:
            pos.setY( pos.y() - windowListMenu->sizeHint().height() );
        default:
            break;
    }

    disconnect( windowListButton, TQ_SIGNAL( pressed() ), this, TQ_SLOT( showWindowListMenu() ) );
    windowListMenu->exec( pos );
    TQTimer::singleShot(100, this, TQ_SLOT(reconnectWindowListButton()));
}

void TaskBarContainer::windowListMenuAboutToHide()
{
    // this ensures that when clicked AGAIN, the window list button doesn't cause the
    // window list menu to show again. usability, you see. hoorah.
    windowListButton->setDown( false );
}

void TaskBarContainer::reconnectWindowListButton()
{
    connect( windowListButton, TQ_SIGNAL( pressed() ), TQ_SLOT( showWindowListMenu() ) );
}

TQSize TaskBarContainer::sizeHint( KPanelExtension::Position p, TQSize maxSize) const
{
    TQSize size = taskBar->sizeHint( p, maxSize );
    if ( (p == KPanelExtension::Left || p == KPanelExtension::Right) && showWindowListButton ) {
        return TQSize( size.width(), size.height() + WINDOWLISTBUTTON_SIZE );
    }
    return size;
}

void TaskBarContainer::setBackground()
{
    taskBar->setBackground();
}

void TaskBarContainer::dragEnterEvent( TQDragEnterEvent* e )
{
    // ignore all drags other than tasks
    if (!TaskDrag::canDecode(e))
    {
        return;
    }

    if ((e->source()->parent() == taskBar->viewport()) && TaskDrag::canDecode(e) && READ_MERGED_TASBKAR_SETTING(allowDragAndDropReArrange) && (!READ_MERGED_TASBKAR_SETTING(sortByApp)))
    {
        e->accept();
    }
}

void TaskBarContainer::dragLeaveEvent( TQDragLeaveEvent* e )
{
    TQFrame::dragLeaveEvent( e );
}

void TaskBarContainer::dropEvent( TQDropEvent* e )
{
    // ignore all drags other than tasks
    if (!TaskDrag::canDecode(e))
    {
        return;
    }

    if ((e->source()->parent() == taskBar->viewport()) && TaskDrag::canDecode(e) && READ_MERGED_TASBKAR_SETTING(allowDragAndDropReArrange) && (!READ_MERGED_TASBKAR_SETTING(sortByApp)))
    {
        if (taskBar->taskMoveHandler(TaskMoveDestination::Position, TaskDrag::decode(e), taskBar->mapFrom(this, e->pos()))) {
            e->accept();
        }
    }
}
