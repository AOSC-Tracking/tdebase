/*****************************************************************

Copyright (c) 2001 Matthias Elter <elter@kde.org>
Copyright (c) 2002 John Firebaugh <jfirebaugh@kde.org>
Copyright (c) 2005 Aaron Seigo <aseigo@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.#

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*****************************************************************
*/

#include <assert.h>

#ifdef Q_OS_SOLARIS
#include <procfs.h>
#endif /* SunOS */

#include <tqbitmap.h>
#include <tqcolor.h>
#include <tqcursor.h>
#include <tqimage.h>
#include <tqpainter.h>
#include <tqpixmap.h>
#include <tqstyle.h>
#include <tqstylesheet.h>
#include <tqtooltip.h>
#include <tqfile.h>

#include <tdeapplication.h>
#include <kdebug.h>
#include <tdeglobalsettings.h>
#include <tdelocale.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <kimageeffect.h>

#ifdef TQ_WS_X11
#include <X11/Xlib.h>
#include <netwm.h>
#include <fixx11h.h>
#endif

#include "global.h"
#include "kickerSettings.h"
#include "paneldrag.h"
#include "taskbar.h"
#include "taskbarsettings.h"
#include "tasklmbmenu.h"
#include "taskrmbmenu.h"

#include "taskcontainer.h"
#include "taskcontainer.moc"

#define READ_MERGED_TASKBAR_SETTING(x) ((m_settingsObject->useGlobalSettings())?m_globalSettingsObject->x():m_settingsObject->x())
#define READ_MERGED_TASKBAR_ACTION(x) ((m_settingsObject->useGlobalSettings())?m_globalSettingsObject->action(x):m_settingsObject->action(x))

static Bool netwm_atoms_created = False;
static Atom net_wm_pid = 0;

static const int netAtomCount = 1;
static void create_atoms(Display *d) {
    static const char * const names[netAtomCount] =
    {
        "_NET_WM_PID"
    };

    Atom atoms[netAtomCount], *atomsp[netAtomCount] =
    {
        &net_wm_pid
    };

    assert( !netwm_atoms_created );

    int i = netAtomCount;
    while (i--)
        atoms[i] = 0;

    XInternAtoms(d, (char **) names, netAtomCount, False, atoms);

    i = netAtomCount;
    while (i--)
        *atomsp[i] = atoms[i];

    netwm_atoms_created = True;
}

bool is_process_resumable(pid_t pid) {
#ifdef Q_OS_SOLARIS
    TQFile procStatFile(TQString("/proc/%1/lwp/1/lwpsinfo").arg(pid));
    if (procStatFile.open(IO_ReadOnly)) {
        TQByteArray statRaw = procStatFile.readAll();
        lwpsinfo_t *inf = (lwpsinfo_t *)statRaw.data();

        procStatFile.close();
        if( inf->pr_sname == 'T' ) {
            return true;
        }
    }
#else /* default */
    TQFile procStatFile(TQString("/proc/%1/stat").arg(pid));
    if (procStatFile.open(IO_ReadOnly)) {
        TQByteArray statRaw = procStatFile.readAll();
        procStatFile.close();
        TQString statString(statRaw);
        TQStringList statFields = TQStringList::split(" ", statString, true);
        TQString tcomm = statFields[1];
        TQString state = statFields[2];
        if( state == "T" ) {
            return true;
        }
    }
#endif /* read process status */
    return false;
}

TaskContainer::TaskContainer(Task::Ptr task, TaskBar* bar, TaskBarSettings* settingsObject, TaskBarSettings* globalSettingsObject, TQWidget *parent, const char *name)
    : TQToolButton(parent, name),
      animationTimer(0, "TaskContainer::animationTimer"),
      dragSwitchTimer(0, "TaskContainer::dragSwitchTimer"),
      attentionTimer(0, "TaskContainer::attentionTimer"),
      m_paintEventCompressionTimer(0, "TaskContainer::paintEventCompressionTimer"),
      currentFrame(0),
      attentionState(-1),
      lastActivated(0),
      m_menu(0),
      m_startup(0),
      arrowType(TQt::UpArrow),
      taskBar(bar),
      discardNextMouseEvent(false),
      aboutToActivate(false),
      m_mouseOver(false),
      m_paintEventCompression(false),
      m_settingsObject(settingsObject),
      m_globalSettingsObject(globalSettingsObject)
{
    init();
    setAcceptDrops(true); // Always enabled to activate task during drag&drop.

    add(task);

    // we abuse this timer once to get shown
    // no point in having another timer just for this, and
    // a single shot won't do because we need to stop the timer
    // in case our task is deleted out from under us
    dragSwitchTimer.start(0, true);
}

TaskContainer::TaskContainer(Startup::Ptr startup, PixmapList& startupFrames, TaskBar* bar, TaskBarSettings* settingsObject, TaskBarSettings* globalSettingsObject, TQWidget *parent, const char *name)
    : TQToolButton(parent, name),
      animationTimer(0, "TaskContainer::animationTimer"),
      dragSwitchTimer(0, "TaskContainer::dragSwitchTimer"),
      attentionTimer(0, "TaskContainer::attentionTimer"),
      m_paintEventCompressionTimer(0, "TaskContainer::paintEventCompressionTimer"),
      currentFrame(0),
      frames(startupFrames),
      attentionState(-1),
      lastActivated(0),
      m_menu(0),
      m_startup(startup),
      arrowType(TQt::LeftArrow),
      taskBar(bar),
      discardNextMouseEvent(false),
      aboutToActivate(false),
      m_mouseOver(false),
      m_paintEventCompression(false),
      m_settingsObject(settingsObject),
      m_globalSettingsObject(globalSettingsObject)
{
    init();
    setEnabled(false);

    sid = m_startup->bin();

    connect(m_startup, TQ_SIGNAL(changed()), TQ_SLOT(update()));

    dragSwitchTimer.start(333, true);
}

void TaskContainer::init()
{
    if (m_settingsObject)
    {
        m_settingsObject->readConfig();
    }
    if (m_globalSettingsObject)
    {
        m_globalSettingsObject->readConfig();
    }

    if (!netwm_atoms_created) create_atoms(TQPaintDevice::x11AppDisplay());

    int iconSize = READ_MERGED_TASKBAR_SETTING(iconSize);

    setWFlags(TQt::WNoAutoErase);
    setBackgroundMode(NoBackground);
    animBg = TQPixmap(iconSize, iconSize);

    installEventFilter(KickerTip::the());

    connect(&animationTimer, TQ_SIGNAL(timeout()), TQ_SLOT(animationTimerFired()));
    connect(&dragSwitchTimer, TQ_SIGNAL(timeout()), TQ_SLOT(showMe()));
    connect(&attentionTimer, TQ_SIGNAL(timeout()), TQ_SLOT(attentionTimerFired()));
    connect(&m_paintEventCompressionTimer, TQ_SIGNAL(timeout()), TQ_SLOT(updateNow()));
}

TaskContainer::~TaskContainer()
{
    if (m_menu)
    {
        delete m_menu;
        m_menu = 0;
    }

    stopTimers();
}

void TaskContainer::showMe()
{
    if(!frames.isEmpty() && taskBar->showIcons())
        animationTimer.start(100);

    emit showMe(this);
    disconnect(&dragSwitchTimer, TQ_SIGNAL(timeout()), this, TQ_SLOT(showMe()));
    connect(&dragSwitchTimer, TQ_SIGNAL(timeout()), TQ_SLOT(dragSwitch()));
}

void TaskContainer::stopTimers()
{
    animationTimer.stop();
    dragSwitchTimer.stop();
    attentionTimer.stop();
}

void TaskContainer::taskChanged(bool geometryOnlyChange)
{
    if (geometryOnlyChange)
    {
        // we really don't care about those changes, which we may be getting
        // thanks to the pager, for instance, turning it on in taskmanager.
        // // let's ignore them so we don't end up with tons of processing going on
        return;
    }

    const TQObject* source = sender();
    Task::Ptr task = 0;
    Task::List::const_iterator itEnd = tasks.constEnd();
    for (Task::List::const_iterator it = tasks.constBegin(); it != itEnd; ++it)
    {
        if (*it == source)
        {
            task = *it;
            break;
        }
    }

    if (task)
    {
        checkAttention(task);
    }

    KickerTip::Client::updateKickerTip();
    update();
}

void TaskContainer::iconChanged()
{
    const TQObject* source = sender();
    Task::Ptr task = 0;
    Task::List::const_iterator itEnd = tasks.constEnd();
    for (Task::List::const_iterator it = tasks.constBegin(); it != itEnd; ++it)
    {
        if (*it == source)
        {
            task = *it;
            break;
        }
    }

    if (task && !m_filteredTasks.empty() && task != m_filteredTasks.first())
    {
        if (m_menu)
        {
            m_menu->update();
        }
        return;
    }

    KickerTip::Client::updateKickerTip();
    TQToolButton::update();
}

void TaskContainer::setLastActivated()
{
    Task::List::const_iterator itEnd = m_filteredTasks.constEnd();
    for (Task::List::const_iterator it = m_filteredTasks.constBegin(); it != itEnd; ++it)
    {
        Task::Ptr t = *it;
        if ( t->isActive() )
        {
            lastActivated = t;
            return;
        }
    }
    lastActivated = 0L;
}


void TaskContainer::animationTimerFired()
{
    if (!frames.isEmpty() && taskBar->showIcons() && frames.at(currentFrame) != frames.end())
    {
      TQPixmap *pm = *frames.at(currentFrame);

      // draw pixmap
      if ( pm && !pm->isNull() )
      {
        // we only have to redraw the background for frames 0, 8 and 9
        if ( currentFrame == 0 || currentFrame > 7 )
        {
          // double buffered painting
          TQPixmap composite( animBg );
          bitBlt( &composite, 0, 0, pm );
          bitBlt( this, iconRect.x(), iconRect.y(), &composite );
        }
        else
          bitBlt( this, iconRect.x(), iconRect.y(), pm );
      }

      // increment frame counter
      if ( currentFrame >= 9)
        currentFrame = 0;
      else
        currentFrame++;
    }
}

void TaskContainer::checkAttention(const Task::Ptr t)
{
    bool attention = t ? t->demandsAttention() : false;
    if (attention && attentionState == -1) // was activated
    {
        attentionState = 0;
        attentionTimer.start(500);
    }
    else if(!attention && attentionState >= 0)
    { // need to check all
        Task::List::iterator itEnd = tasks.end();
        for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
        {
            if ((*it)->demandsAttention())
            {
                attention = true;
                break;
            }
        }

        if (!attention)
        {
            attentionTimer.stop();
            attentionState = -1;
        }
    }
}

void TaskContainer::attentionTimerFired()
{
    assert( attentionState != -1 );
    if (attentionState < READ_MERGED_TASKBAR_SETTING(attentionBlinkIterations)*2)
    {
        ++attentionState;
    }
    else if (READ_MERGED_TASKBAR_SETTING(attentionBlinkIterations) < 1000)
    {
        attentionTimer.stop();
    }
    else
    {
        // we have a "forever" blinker (attentionBlinkIterations > 999) and have reached
        // the upper limit. so we need to decrement the attentionState to make it blink
        --attentionState;
    }
    update();
}

TQSizePolicy TaskContainer::sizePolicy() const
{
    return TQSizePolicy( TQSizePolicy::Expanding, TQSizePolicy::Expanding );
}

void TaskContainer::resizeEvent( TQResizeEvent * )
{
    recalculateIconRect();
}

void TaskContainer::recalculateIconRect()
{
    iconSize = READ_MERGED_TASKBAR_SETTING(iconSize);

    if(taskBar->showText())
    {
        TQRect br( style().subRect( TQStyle::SR_PushButtonContents, this ) );
        iconRect = TQStyle::visualRect( TQRect(br.x() + 2, (height() - iconSize) / 2, iconSize, iconSize), this );
    }
    else
    {
        iconRect = TQStyle::visualRect( TQRect((width() - iconSize) / 2, (height() - iconSize) / 2, iconSize, iconSize), this );
    }
}

void TaskContainer::add(Task::Ptr task)
{
    if (!task)
    {
        return;
    }

    tasks.append(task);

    if (sid.isEmpty())
    {
        sid = task->classClass();
    }

    updateFilteredTaskList();
    checkAttention(task);

    KickerTip::Client::updateKickerTip();
    update();

    connect(task, TQ_SIGNAL(changed(bool)), TQ_SLOT(taskChanged(bool)));
    connect(task, TQ_SIGNAL(iconChanged()), TQ_SLOT(iconChanged()));
    connect(task, TQ_SIGNAL(activated()), TQ_SLOT(setLastActivated()));
}

void TaskContainer::remove(Task::Ptr task)
{
    if (!task)
    {
        return;
    }

    task->publishIconGeometry(TQRect());
    for (Task::List::Iterator it = tasks.begin(); it != tasks.end(); ++it)
    {
        if ((*it) == task)
        {
            tasks.erase(it);
            break;
        }
    }

    updateFilteredTaskList();

    if (isEmpty())
    {
        stopTimers();
        return;
    }

    checkAttention();
    KickerTip::Client::updateKickerTip();
    update();
}

void TaskContainer::remove(Startup::Ptr startup)
{
    if (!startup || startup != m_startup)
    {
        return;
    }

    m_startup = 0;
    animationTimer.stop();
    frames.clear();

    if (!tasks.isEmpty())
    {
        setEnabled(true);
    }
}

bool TaskContainer::contains(Task::Ptr task)
{
    if (!task)
    {
        return false;
    }

    for (Task::List::Iterator it = tasks.begin(); it != tasks.end(); ++it)
    {
        if ((*it) == task)
        {
            return true;
        }
    }

    return false;
}

bool TaskContainer::contains(Startup::Ptr startup)
{
    return startup && (m_startup == startup);
}

bool TaskContainer::contains(WId win)
{
    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        if ((*it)->window() == win)
        {
            return true;
        }
    }

    return false;
}

bool TaskContainer::isEmpty()
{
    return (tasks.isEmpty() && !m_startup);
}

bool TaskContainer::isHidden()
{
    return ((m_filteredTasks.count() < 1) && !m_startup);
}

TQString TaskContainer::id()
{
    return sid;
}

void TaskContainer::updateNow()
{
    m_paintEventCompression = true;
    update();
}

void TaskContainer::setBackground()
{
    updateNow();
}

void TaskContainer::paintEvent( TQPaintEvent* )
{
    if (!m_paintEventCompression)
    {
        if (!m_paintEventCompressionTimer.isActive())
        {
            m_paintEventCompressionTimer.start(30, true);
        }
        return;
    }

    m_paintEventCompression = false;
    TQPixmap* pm = new TQPixmap(size());

    const TQPixmap* background = taskBar->backgroundPixmap();

    if (background)
    {
        TQPoint pt = mapTo(taskBar, TQPoint(0, 0)) + taskBar->backgroundOffset();
        TQPainter p(pm);
        p.drawTiledPixmap(0, 0, width(), height(), *background, pt.x(), pt.y());
        p.end();
    }
    else
    {
        pm->fill(taskBar->paletteBackgroundColor());
    }

    TQPainter p;
    p.begin(pm ,this);
    drawButton(&p);
    p.end();

    TQPixmap iconPixmapToSet = *pm;
    if (TQPaintDevice::x11AppDepth() == 32) iconPixmapToSet.convertFromImage(KImageEffect::convertToPremultipliedAlpha( iconPixmapToSet.convertToImage() ));

    bitBlt(this, 0, 0, &iconPixmapToSet);
    delete pm;
}

void TaskContainer::drawButton(TQPainter *p)
{
    if (isEmpty())
    {
        return;
    }

    // get a pointer to the pixmap we're drawing on
    TQPixmap *pm((TQPixmap*)p->device());
    TQPixmap pixmap; // icon
    Task::Ptr task = 0;
    bool iconified = !READ_MERGED_TASKBAR_SETTING(showOnlyIconified);
    bool halo = READ_MERGED_TASKBAR_SETTING(haloText);
    bool alwaysDrawButtons = READ_MERGED_TASKBAR_SETTING(drawButtons);
    bool drawButton = alwaysDrawButtons ||
                      (m_mouseOver && !halo && isEnabled() &&
                       READ_MERGED_TASKBAR_SETTING(showButtonOnHover));
    TQFont font(TDEGlobalSettings::taskbarFont());
    recalculateIconRect();

    // draw sunken if we contain the active task
    bool active = false;
    bool demandsAttention = false;
    Task::List::iterator itEnd = m_filteredTasks.end();
    for (Task::List::iterator it = m_filteredTasks.begin(); it != itEnd; ++it)
    {
        task = *it;
        if (iconified && !task->isIconified())
        {
            iconified = false;
        }

        if (task->isActive())
        {
            active = true;
        }

        if (task->demandsAttention())
        {
            demandsAttention = attentionState == READ_MERGED_TASKBAR_SETTING(attentionBlinkIterations) ||
                               attentionState % 2 == 0;
        }
    }

    font.setBold(active);

    TQColorGroup colors = palette().active();

    if (READ_MERGED_TASKBAR_SETTING(useCustomColors))
    {
        colors.setColor( TQColorGroup::Button, READ_MERGED_TASKBAR_SETTING(taskBackgroundColor));
        colors.setColor( TQColorGroup::Background, READ_MERGED_TASKBAR_SETTING(taskBackgroundColor) );
        colors.setColor( TQColorGroup::ButtonText, READ_MERGED_TASKBAR_SETTING(inactiveTaskTextColor) );
        colors.setColor( TQColorGroup::Text, READ_MERGED_TASKBAR_SETTING(inactiveTaskTextColor) );
    }

    if (demandsAttention)
    {
        if (!drawButton)
        {
            halo = true;

            TQRect r = rect();
            TQColor line = colors.highlight();
            r.addCoords(2, 2, -2, -2);
            p->fillRect(r, line);
            for (int i = 0; i < 2; ++i)
            {
                line = KickerLib::blendColors(line, colors.background());
                p->setPen(TQPen(line, 1, TQt::SolidLine, TQt::RoundCap, TQt::RoundJoin));
                r.addCoords(-1, -1, 1, 1);
                p->drawRect(r);
            }
        }

        // blink until blink timeout, then display differently without blinking
        colors.setColor( TQColorGroup::Button,     colors.highlight() );
        colors.setColor( TQColorGroup::Background, colors.highlight() );
        colors.setColor( TQColorGroup::ButtonText, colors.highlightedText() );
        colors.setColor( TQColorGroup::Text,       colors.highlightedText() );
    }

    if (active || aboutToActivate)
    {
        colors.setColor(TQColorGroup::Button, colors.button().dark(110));
    }

    // get the task icon
    if (task)
    {
        pixmap = task->icon(iconSize, iconSize, true);
    }

    bool sunken = isDown() || (alwaysDrawButtons && (active || aboutToActivate));
    bool reverse = TQApplication::reverseLayout();
    TQRect br(style().subRect(TQStyle::SR_PushButtonContents, this));
    TQPoint shift = TQPoint(style().pixelMetric(TQStyle::PM_ButtonShiftHorizontal),
                          style().pixelMetric(TQStyle::PM_ButtonShiftVertical));

    // draw button background
    if (drawButton)
    {
        if (READ_MERGED_TASKBAR_SETTING(drawButtons) && KickerSettings::showDeepButtons()) {
            style().drawPrimitive(TQStyle::PE_ButtonBevel, p,
                              TQRect(1, 1, width()-3, height()-2),
                              colors, sunken ? TQStyle::Style_On : TQStyle::Style_Raised);
        }
        else {
            style().drawPrimitive(TQStyle::PE_ButtonTool, p,
                              TQRect(1, 1, width()-2, height()-2),
                              colors, sunken ? TQStyle::Style_Down : TQStyle::Style_Raised);
        }
    }

    // shift button label on sunken buttons
    if (sunken)
    {
        p->translate(shift.x(), shift.y());
    }

    TQString text = name();  // find text
    int textPos = ( taskBar->showIcons() && (!pixmap.isNull() || m_startup)) ? 2 + iconSize + 2 : 0;

    // show icons
    if (taskBar->showIcons())
    {
      if (pixmap.isNull() && m_startup)
          pixmap = tdeApp->iconLoader()->loadIcon(m_startup->icon(), TDEIcon::Panel, iconSize);

      if ( !pixmap.isNull() )
      {
          // make sure it is no larger than icon size
          if ( pixmap.width() > iconSize || pixmap.height() > iconSize )
          {
              TQImage tmp = pixmap.convertToImage();
              pixmap.convertFromImage( tmp.smoothScale( iconSize, iconSize ) );
          }

          // fade out the icon when minimized
          if (iconified)
              TDEIconEffect::semiTransparent( pixmap );

          // draw icon
          TQRect pmr(0, 0, pixmap.width(), pixmap.height());
          pmr.moveCenter(iconRect.center());
          p->drawPixmap(pmr, pixmap);
      }

      // modified overlay icon
      static TQString modStr = "[" + i18n( "modified" ) + "]";
      int modStrPos = text.find( modStr );
      if (modStrPos >= 0)
      {
        TQRect r;
        TQPixmap modPixmap = SmallIcon("modified");
        if (iconified)
        {
            TDEIconEffect::semiTransparent(modPixmap);
        }

        if (taskBar->showText()) // has text
        {
            // +1 because we include a space after the closing brace.
            text.remove(modStrPos, modStr.length() + 1);

            // draw modified overlay
            if (!modPixmap.isNull())
            {
                r = TQStyle::visualRect(TQRect(br.x() + textPos,(height() - iconSize) / 2, iconSize, iconSize), this);
                textPos += iconSize + 2;
            }
        }
        else if (taskBar->showIcons()) // has only icon
        {
            r = TQRect(0, 0, iconSize / 2, iconSize / 2);
            r.moveBottomRight(iconRect.bottomRight());
        }

         p->drawPixmap(r, modPixmap);
      }
    }

    // draw text
    if (taskBar->showText())
    {
      if (!text.isEmpty())
      {
          TQRect tr = TQStyle::visualRect(TQRect(br.x() + textPos + 1, 0,
                                          width() - textPos, height()), this);
          int textFlags = AlignVCenter | SingleLine;
          textFlags |= reverse ? AlignRight : AlignLeft;
          TQPen textPen;

          // get the color for the text label
          if (iconified)
          {
              textPen = TQPen(KickerLib::blendColors(colors.button(), colors.buttonText()));
          }
          else if (!active)
          {
              textPen = TQPen(colors.buttonText());
          }
          else // hack for the dotNET style and others
          {
              if (READ_MERGED_TASKBAR_SETTING(useCustomColors))
              {
                  textPen = TQPen(READ_MERGED_TASKBAR_SETTING(activeTaskTextColor));
              }
              else
              {
                  textPen = TQPen(colors.buttonText()); // textPen = p->pen();
              }
          }
          bool deepButtons = READ_MERGED_TASKBAR_SETTING(drawButtons) && KickerSettings::showDeepButtons();
          int availableWidth = width() - (br.x() * 2) - textPos - 2 - (deepButtons ? 2 : 0);
          if (m_filteredTasks.count() > 1)
          {
              availableWidth -= 8;
          }

          if (TQFontMetrics(font).width(text) > availableWidth)
          {
              // draw text into overlay pixmap
              TQPixmap tpm(*pm);
              TQPainter tp(&tpm);

              if (sunken)
              {
                  tp.translate(shift.x(), shift.y());
              }

              tp.setFont(font);
              tp.setPen(textPen);

              if (halo)
              {
                  taskBar->textShadowEngine()->drawText(tp, tr, textFlags, text, size());
              }
              else
              {
                  tp.drawText(tr, textFlags, text);
              }

              // blend text into background image
              TQImage img = pm->convertToImage();
              TQImage timg = tpm.convertToImage();
              KImageEffect::blend(img, timg, *taskBar->blendGradient(size()), KImageEffect::Red);

              // End painting before assigning the pixmap
              TQPaintDevice* opd = p->device();
              p->end();
              pm->convertFromImage(img);
              p->begin(opd ,this);
          }
          else
          {
              p->setFont(font);
              p->setPen(textPen);

              if (halo)
              {
                  taskBar->textShadowEngine()->drawText(*p, tr, textFlags, text, size());
              }
              else
              {
                  p->drawText(tr, textFlags, text);
              }
          }
      }
    }

    if (!frames.isEmpty() && m_startup && frames.at(currentFrame) != frames.end())
    {
        TQPixmap *anim = *frames.at(currentFrame);

        if (anim && !anim->isNull())
        {
            // save the background for the other frames
            bitBlt(&animBg, TQPoint(0,0), pm, iconRect);
            // draw the animation frame
            bitBlt(pm, iconRect.x(), iconRect.y(), anim);
        }
    }

    if (sunken)
    {
        // Change the painter back so the arrow, etc gets drawn in the right location
        p->translate(-shift.x(), -shift.y());
    }

    // draw popup arrow
    if ((m_filteredTasks.count() > 1) && (!(READ_MERGED_TASKBAR_SETTING(drawButtons) && KickerSettings::showDeepButtons())))
    {
        TQStyle::PrimitiveElement e = TQStyle::PE_ArrowLeft;

        switch (arrowType)
        {
            case TQt::LeftArrow:  e = TQStyle::PE_ArrowLeft;  break;
            case TQt::RightArrow: e = TQStyle::PE_ArrowRight; break;
            case TQt::UpArrow:    e = TQStyle::PE_ArrowUp;    break;
            case TQt::DownArrow:  e = TQStyle::PE_ArrowDown;  break;
        }

        int flags = TQStyle::Style_Enabled;
        TQRect ar = TQStyle::visualRect(TQRect(br.x() + br.width() - 8 - 2,
                                            br.y(), 8, br.height()), this);
        if (sunken)
        {
            flags |= TQStyle::Style_Down;
        }

        style().drawPrimitive(e, p, ar, colors, flags);
    }

    // draw mouse over frame in transparent mode
    if (m_mouseOver && halo)
        KickerLib::drawBlendedRect(p, TQRect(0, 0, width(), height()), colorGroup().foreground());

    if (aboutToActivate)
    {
        aboutToActivate = false;
    }
}

TQString TaskContainer::name()
{
    // default to container id
    TQString text;

    // single task -> use mainwindow caption
    if (m_filteredTasks.count() == 1)
    {
        text = m_filteredTasks.first()->visibleName();
    }
    else if (m_filteredTasks.count() > 1)
    {
        // multiple tasks -> use the common part of all captions
        // if it is more descriptive than the class name
        const TQString match = m_filteredTasks.first()->visibleName();
        unsigned int maxLength = match.length();
        unsigned int i = 0;
        bool stop = false;

        // what we do is find the right-most letter than the names do NOT have
        // in common, and then use everything UP TO that as the name in the button
        while (i < maxLength)
        {
            TQChar check = match.at(i).lower();
            Task::List::iterator itEnd = m_filteredTasks.end();
            for (Task::List::iterator it = m_filteredTasks.begin(); it != itEnd; ++it)
            {
                // we're doing a lot of Utf8 -> TQString conversions here
                // by repeatedly calling visibleIconicName() =/
                if (check != (*it)->visibleName().at(i).lower())
                {
                    if (i > 0)
                    {
                        --i;
                    }
                    stop = true;
                    break;
                }
            }

            if (stop)
            {
                break;
            }

            ++i;
        }

        // strip trailing crap
        while (i > 0 && !match.at(i).isLetterOrNumber())
        {
            --i;
        }

        // more descriptive than id()?
        if (i > 0 && (i + 1) >= id().length())
        {
            text = match.left(i + 1);
        }
    }
    else if (m_startup && !m_startup->text().isEmpty())
    {
        // fall back to startup name
        text = m_startup->text();
    }

    if (text.isEmpty())
    {
        text = id();

        // Upper case first letter: seems to be the right thing to do for most cases
        text[0] = text[0].upper();
    }

    if (m_filteredTasks.count() > 1)
    {
        // this is faster than (" [%1]").arg() or +
        // and it's as fast as using append, but cleaner looking
        text += " [";
        text += TQString::number(m_filteredTasks.count());
        text += "]";
    }

    return text;
}

void TaskContainer::mousePressEvent( TQMouseEvent* e )
{
    if (discardNextMouseEvent)
    {
        discardNextMouseEvent = false;
        return;
    }

    if (e->button() == TQt::LeftButton)
    {
        m_dragStartPos = e->pos();
    }
    else
    {
        m_dragStartPos = TQPoint();
    }

    int buttonAction = 0;

    // On left button, only do actions that invoke a menu.
    // Other actions will be handled in mouseReleaseEvent
    switch (e->button())
    {
        case TQt::LeftButton:
            buttonAction = READ_MERGED_TASKBAR_ACTION(m_settingsObject->LeftButton);
            break;
        case TQt::MidButton:
            buttonAction = READ_MERGED_TASKBAR_ACTION(m_settingsObject->MiddleButton);
            break;
        case TQt::RightButton:
        default:
            buttonAction = READ_MERGED_TASKBAR_ACTION(m_settingsObject->RightButton);
            break;
    }

    if ((buttonAction == m_settingsObject->ShowTaskList &&
          m_filteredTasks.count() > 1) ||
        buttonAction == m_settingsObject->ShowOperationsMenu)
    {
        performAction(buttonAction);
    }
}

void TaskContainer::mouseReleaseEvent(TQMouseEvent *e)
{
    m_dragStartPos = TQPoint();

    if (!READ_MERGED_TASKBAR_SETTING(drawButtons))
    {
        setDown(false);
    }

    // This is to avoid the flicker caused by redrawing the
    // button as unpressed just before it's activated.
    if (!rect().contains(e->pos()))
    {
        TQToolButton::mouseReleaseEvent(e);
        return;
    }

    int buttonAction = 0;

    switch (e->button())
    {
        case TQt::LeftButton:
            buttonAction = READ_MERGED_TASKBAR_ACTION(m_settingsObject->LeftButton);
            break;
        case TQt::MidButton:
            buttonAction = READ_MERGED_TASKBAR_ACTION(m_settingsObject->MiddleButton);
            break;
        case TQt::RightButton:
        default:
            buttonAction = READ_MERGED_TASKBAR_ACTION(m_settingsObject->RightButton);
            break;
    }

    if ((buttonAction == m_settingsObject->ShowTaskList &&
         m_filteredTasks.count() > 1) ||
        buttonAction == m_settingsObject->ShowOperationsMenu)
    {
        return;
    }

    if (buttonAction == TaskBarSettings::ActivateRaiseOrMinimize ||
        buttonAction == TaskBarSettings::Activate)
    {
        aboutToActivate = true;
    }

    performAction( buttonAction );
    TQTimer::singleShot(0, this, TQ_SLOT(update()));
}

void TaskContainer::performAction(int action)
{
    if (m_filteredTasks.isEmpty())
    {
        return;
    }

    switch( action ) {
    case TaskBarSettings::ShowTaskList:
    // If there is only one task, the correct behavior is
    // to activate, raise, or iconify it, not show the task menu.
    if( m_filteredTasks.count() > 1 ) {
                popupMenu( m_settingsObject->ShowTaskList );
    } else {
                performAction( TaskBarSettings::ActivateRaiseOrMinimize );
    }
    break;
    case TaskBarSettings::ShowOperationsMenu:
        popupMenu( m_settingsObject->ShowOperationsMenu );
    break;
    case TaskBarSettings::ActivateRaiseOrMinimize:
    if (m_filteredTasks.isEmpty())
    {
        break;
    }
    if (m_filteredTasks.count() == 1)
    {
        m_filteredTasks.first()->activateRaiseOrIconify();
    }
    else
    {
        // multiple tasks -> cycle list
        bool hasLastActivated = false;
        Task::List::iterator itEnd = m_filteredTasks.end();
        for (Task::List::iterator it = m_filteredTasks.begin(); it != itEnd; ++it)
        {
            if ((*it) == lastActivated)
            {
                hasLastActivated = true;
            }

            if ((*it)->isActive())
            {
                // activate next
                ++it;
                if (it == itEnd)
                {
                    it = m_filteredTasks.begin();
                }
                (*it)->activateRaiseOrIconify();
                return;
            }
        }

        if (hasLastActivated)
        {
            lastActivated->activateRaiseOrIconify();
        }
        else
        {
            m_filteredTasks[0]->activateRaiseOrIconify();
        }
    }
    break;
    case TaskBarSettings::Activate:
        m_filteredTasks.first()->activate();
    break;
    case TaskBarSettings::Raise:
        m_filteredTasks.first()->raise();
    break;
    case TaskBarSettings::Lower:
        m_filteredTasks.first()->lower();
    break;
    case TaskBarSettings::Minimize:
        m_filteredTasks.first()->toggleIconified();
    break;
    case TaskBarSettings::Close:
        m_filteredTasks.first()->close();
    break;
    case TaskBarSettings::ToCurrentDesktop:
        m_filteredTasks.first()->toCurrentDesktop();
    break;
    default:
        kdWarning(1210) << "Unknown taskbar action!" << endl;
        break;
    }
}

// forcenext == true means the last entry in the previous
// taskcontainer was active -> activate first
bool TaskContainer::activateNextTask(bool forward, bool& forcenext)
{
    if (forcenext)
    {
        if (m_filteredTasks.isEmpty())
        {
            return false;
        }

        if (forward)
        {
            m_filteredTasks.first()->activate();
        }
        else
        {
            m_filteredTasks.last()->activate();
        }

        forcenext = false;
        return true;
    }

    Task::List::iterator itEnd = m_filteredTasks.end();
    for (Task::List::iterator it = m_filteredTasks.begin();
         it != itEnd;
         ++it)
    {
        if ((*it)->isActive())
        {
            if (forward)
            {
                ++it;
                if (it == itEnd)
                {
                    forcenext = true;
                    return false;
                }

                (*it)->activate();
                return true;
            }
            else if (it == m_filteredTasks.begin())
            {
                forcenext = true;
                return false;
            }

            --it;
            (*it)->activate();
            return true;
        }
    }

    return false;
}

void TaskContainer::popupMenu(int action)
{
    if (action == m_settingsObject->ShowTaskList )
    {
        m_menu = new TaskLMBMenu(m_filteredTasks);
    }
    else if (action == m_settingsObject->ShowOperationsMenu)
    {
        if (!tdeApp->authorizeTDEAction("twin_rmb"))
        {
            return;
        }

        m_menu = new TaskRMBMenu(m_filteredTasks, taskBar->showAllWindows(), (READ_MERGED_TASKBAR_SETTING(allowDragAndDropReArrange))?makeTaskMoveMenu():NULL);
    }
    else
    {
        return;
    }

    // calc popup menu position
    TQPoint pos(mapToGlobal(TQPoint(0, 0)));

    switch( arrowType )
    {
        case RightArrow:
            pos.setX(pos.x() + width());
            break;
        case LeftArrow:
            pos.setX(pos.x() - m_menu->sizeHint().width());
            break;
        case DownArrow:
            if ( TQApplication::reverseLayout() )
                pos.setX( pos.x() + width() - m_menu->sizeHint().width() );
            pos.setY( pos.y() + height() );
            break;
        case UpArrow:
            if ( TQApplication::reverseLayout() )
                pos.setX( pos.x() + width() - m_menu->sizeHint().width() );
            pos.setY(pos.y() - m_menu->sizeHint().height());
            break;
        default:
            break;
    }
    m_menu->installEventFilter( this );
    m_menu->exec( pos );

    if (m_menu) {
        delete m_menu;
    }
    m_menu = 0;
}

TQPopupMenu* TaskContainer::makeTaskMoveMenu()
{
    TaskMoveDestination::TaskMoveDestination capabilities = taskBar->taskMoveCapabilities(this);

    int id;
    TQPopupMenu* menu = new TQPopupMenu();

    menu->setCheckable(false);

    id = menu->insertItem(SmallIconSet("go-first"),
                          i18n("Move to Beginning"),
                          this, TQ_SLOT(slotTaskMoveBeginning()));
    menu->setItemEnabled(id, (capabilities & TaskMoveDestination::Left));

    id = menu->insertItem(SmallIconSet("back"),
                          i18n("Move Left"),
                          this, TQ_SLOT(slotTaskMoveLeft()));
    menu->setItemEnabled(id, (capabilities & TaskMoveDestination::Left));

    id = menu->insertItem(SmallIconSet("forward"),
                          i18n("Move Right"),
                          this, TQ_SLOT(slotTaskMoveRight()));
    menu->setItemEnabled(id, (capabilities & TaskMoveDestination::Right));

    id = menu->insertItem(SmallIconSet("go-last"),
                          i18n("Move to End"),
                          this, TQ_SLOT(slotTaskMoveEnd()));
    menu->setItemEnabled(id, (capabilities & TaskMoveDestination::Right));

    return menu;
}

void TaskContainer::slotTaskMoveBeginning()
{
    taskBar->taskMoveHandler(TaskMoveDestination::Beginning, taskList());
}

void TaskContainer::slotTaskMoveLeft()
{
    taskBar->taskMoveHandler(TaskMoveDestination::Left, taskList());
}

void TaskContainer::slotTaskMoveRight()
{
    taskBar->taskMoveHandler(TaskMoveDestination::Right, taskList());
}

void TaskContainer::slotTaskMoveEnd()
{
    taskBar->taskMoveHandler(TaskMoveDestination::End, taskList());
}

void TaskContainer::mouseMoveEvent( TQMouseEvent* e )
{
    kdDebug() << "regular move" << endl;
    if (!m_dragStartPos.isNull())
    {
        startDrag(e->pos());
    }

    TQToolButton::mouseMoveEvent(e);
}

bool TaskContainer::startDrag(const TQPoint& pos)
{
    int delay = TDEGlobalSettings::dndEventDelay();

    if ((m_dragStartPos - pos).manhattanLength() > delay)
    {
        if (!m_filteredTasks.first()->isActive())
        {
            setDown(false);
        }

        TaskDrag* drag = new TaskDrag(m_filteredTasks, this);

        if (!m_filteredTasks.isEmpty())
        {
            kdDebug() << m_filteredTasks.first()->name() << endl;
            drag->setPixmap(m_filteredTasks.first()->pixmap());
        }

        drag->dragMove();
        return true;
    }

    return false;
}

// This is the code that gives us the proper behavior
// when a popup menu is displayed and we are clicked:
// close the menu, and don't reopen it immediately.
// It's copied from TQToolButton. Unfortunately Qt is lame
// as usual and makes interesting stuff private or
// non-virtual, so we have to copy code.
bool TaskContainer::eventFilter(TQObject *o, TQEvent *e)
{
    switch ( e->type() )
    {
        case TQEvent::MouseButtonPress:
        case TQEvent::MouseButtonDblClick:
        {
            TQMouseEvent *me = (TQMouseEvent*)e;
            TQPoint p = me->globalPos();
            if ( TQApplication::widgetAt( p, true ) == this )
            {
                if (me->type() == TQEvent::MouseButtonPress &&
                    me->button() == TQt::LeftButton)
                {
                    m_dragStartPos = mapFromGlobal(p);
                }

                discardNextMouseEvent = true;
            }
            break;
        }
        case TQEvent::MouseButtonRelease:
        {
            m_dragStartPos = TQPoint();
            break;
        }
        case TQEvent::MouseMove:
        {
            if (!m_dragStartPos.isNull())
            {
                TQMouseEvent* me = static_cast<TQMouseEvent*>(e);
                TQPoint p(me->globalPos());

                if (me->state() & TQt::LeftButton &&
                    TQApplication::widgetAt(p, true) == this)
                {
                    kdDebug() << "event move" << endl;
                    if (startDrag(mapFromGlobal(p)))
                    {
                        TQPopupMenu* menu = dynamic_cast<TQPopupMenu*>(o);

                        if (menu)
                        {
                            menu->hide();
                        }
                    }
                }
            }
            break;
        }

        default:
        break;
    }

    return TQToolButton::eventFilter( o, e );
}

void TaskContainer::setArrowType( TQt::ArrowType at )
{
    if (arrowType == at)
    {
        return;
    }

    arrowType = at;
    update();
}

void TaskContainer::publishIconGeometry( TQPoint global )
{
    TQPoint p = global + geometry().topLeft();

    Task::List::const_iterator itEnd = tasks.constEnd();
    for (Task::List::const_iterator it = tasks.constBegin(); it != itEnd; ++it)
    {
        Task::Ptr t = *it;
        t->publishIconGeometry(TQRect(p.x(), p.y(), width(), height()));
    }
}

void TaskContainer::dragEnterEvent( TQDragEnterEvent* e )
{
    // ignore applet drags
    if (PanelDrag::canDecode(e))
    {
        return;
    }

    if (e->source() && (e->source()->parent() == this->parent()) && TaskDrag::canDecode(e) && READ_MERGED_TASKBAR_SETTING(allowDragAndDropReArrange) && (!READ_MERGED_TASKBAR_SETTING(sortByApp)))
    {
        e->accept();
    }

    // if a dragitem is held for over a taskbutton for two seconds,
    // activate corresponding window
    if (m_filteredTasks.isEmpty())
    {
        return;
    }

    if (!m_filteredTasks.first()->isActive() ||
        m_filteredTasks.count() > 1)
    {
        dragSwitchTimer.start(1000, true);
    }

    TQToolButton::dragEnterEvent( e );
}

void TaskContainer::dropEvent( TQDropEvent* e )
{
    // Ignore all drops except tasks
    if (!TaskDrag::canDecode(e)) {
        return;
    }

    if ((e->source()->parent() == this->parent()) && TaskDrag::canDecode(e) && READ_MERGED_TASKBAR_SETTING(allowDragAndDropReArrange) && (!READ_MERGED_TASKBAR_SETTING(sortByApp)))
    {
        if (taskBar->taskMoveHandler(TaskMoveDestination::Position, TaskDrag::decode(e), TQWidget::mapTo(taskBar, e->pos()))) {
            e->accept();
        }
    }

    dragSwitchTimer.stop();

    TQToolButton::dropEvent( e );
}

void TaskContainer::dragLeaveEvent( TQDragLeaveEvent* e )
{
    dragSwitchTimer.stop();

    TQToolButton::dragLeaveEvent( e );
}

void TaskContainer::enterEvent(TQEvent* e)
{
    TQToolTip::remove(this);
    m_mouseOver = true;
    updateNow();

    if (tasks.isEmpty())
    {
        TQToolButton::enterEvent(e);
        return;
    }

    if (!KickerSettings::showMouseOverEffects())
    {
        TQString tooltip = "<qt>" + TQStyleSheet::escape(name()) + "</qt>";
        TQToolTip::add(this, tooltip);
        return;
    }
}

void TaskContainer::leaveEvent(TQEvent*)
{
    m_mouseOver = false;
    updateNow();
}

void TaskContainer::dragSwitch()
{
    if (m_filteredTasks.isEmpty())
    {
        return;
    }

    if (m_filteredTasks.count() == 1)
    {
        m_filteredTasks.first()->activate();
    }
    else
    {
        popupMenu(m_settingsObject->ShowTaskList);
    }
}

int TaskContainer::desktop()
{
    if ( tasks.isEmpty() )
        return TaskManager::the()->currentDesktop();

    if ( tasks.count() > 1 )
        return TaskManager::the()->numberOfDesktops();

    return tasks.first()->desktop();
}

bool TaskContainer::onCurrentDesktop()
{
    if (m_startup)
    {
        return true;
    }

    Task::List::const_iterator itEnd = tasks.constEnd();
    for (Task::List::const_iterator it = tasks.constBegin(); it != itEnd; ++it)
    {
        Task::Ptr t = *it;
        if (t->isOnCurrentDesktop())
        {
            return true;
        }
    }

    return false;
}

bool TaskContainer::isOnScreen()
{
    if (isEmpty())
    {
        return false;
    }

    int screen = taskBar->showScreen();
    if ((tasks.isEmpty() && m_startup) || screen == -1)
    {
        return true;
    }

    Task::List::iterator itEnd = tasks.end();
    for (Task::List::iterator it = tasks.begin(); it != itEnd; ++it)
    {
        if ((*it)->isOnScreen( screen ))
        {
            return true;
        }
    }

    return false;
}

bool TaskContainer::isIconified()
{
    if (isEmpty())
    {
        return false;
    }

    if (tasks.isEmpty() && m_startup)
    {
        return true;
    }

    Task::List::const_iterator itEnd = tasks.constEnd();
    for (Task::List::const_iterator it = tasks.constBegin(); it != itEnd; ++it)
    {
        if ((*it)->isIconified())
        {
            return true;
        }
    }

    return false;
}

void TaskContainer::updateFilteredTaskList()
{
    m_filteredTasks.clear();

    Task::List::const_iterator itEnd = tasks.constEnd();
    for (Task::List::const_iterator it = tasks.constBegin(); it != itEnd; ++it)
    {
        Task::Ptr t = *it;
        if ((taskBar->showAllWindows() || t->isOnCurrentDesktop()) &&
            (!READ_MERGED_TASKBAR_SETTING(showOnlyIconified) || t->isIconified()))
        {
            pid_t pid = 0;
#ifdef TQ_WS_X11
            Atom type_ret;
            int format_ret;
            unsigned long nitems_ret = 0, unused = 0;
            unsigned char *data_ret = 0;
            if (XGetWindowProperty(TQPaintDevice::x11AppDisplay(), t->window(), net_wm_pid, 0l, 1l,
                                   False, XA_CARDINAL, &type_ret, &format_ret,
                                   &nitems_ret, &unused, &data_ret) == Success) {
                if (type_ret == XA_CARDINAL && format_ret == 32 && nitems_ret == 1) {
                    pid = *((long *) data_ret);
                }
                if ( data_ret )
                    XFree(data_ret);
            }
#endif
            if (pid < 0) {
                m_filteredTasks.append(t);
            }
            else if (READ_MERGED_TASKBAR_SETTING(showTaskStates) != m_settingsObject->ShowAll) {
                if (is_process_resumable(pid)) {
                    if (READ_MERGED_TASKBAR_SETTING(showTaskStates) == m_settingsObject->ShowAll) {
                        m_filteredTasks.append(t);
                    }
                    else if (READ_MERGED_TASKBAR_SETTING(showTaskStates) == m_settingsObject->ShowStopped) {
                        m_filteredTasks.append(t);
                    }
                    else if (READ_MERGED_TASKBAR_SETTING(showTaskStates) == m_settingsObject->ShowRunning) {
                        t->publishIconGeometry( TQRect());
                    }
                    else {
                        m_filteredTasks.append(t);
                    }
                }
                else {
                    if (READ_MERGED_TASKBAR_SETTING(showTaskStates) == m_settingsObject->ShowAll) {
                        m_filteredTasks.append(t);
                    }
                    else if (READ_MERGED_TASKBAR_SETTING(showTaskStates) == m_settingsObject->ShowStopped) {
                        t->publishIconGeometry( TQRect());
                    }
                    else if (READ_MERGED_TASKBAR_SETTING(showTaskStates) == m_settingsObject->ShowRunning) {
                        m_filteredTasks.append(t);
                    }
                    else {
                        m_filteredTasks.append(t);
                    }
                }
            }
            else {
                m_filteredTasks.append(t);
            }
        }
        else
        {
            t->publishIconGeometry( TQRect());
        }
    }

    // sort container list by desktop
    if (taskBar->sortByDesktop() && m_filteredTasks.count() > 1)
    {
        TQValueVector<TQPair<int, Task::Ptr> > sorted;
        sorted.resize(m_filteredTasks.count());
        int i = 0;

        Task::List::const_iterator itEnd = m_filteredTasks.constEnd();
        for (Task::List::const_iterator it = m_filteredTasks.constBegin(); it != itEnd; ++it)
        {
            Task::Ptr t = *it;
            sorted[i] = (qMakePair(t->desktop(), t));
            ++i;
        }

        qHeapSort(sorted);

        m_filteredTasks.clear();
        for (TQValueVector<TQPair<int, Task::Ptr> >::iterator it = sorted.begin();
             it != sorted.end();
             ++it)
        {
            m_filteredTasks.append((*it).second);
        }
    }
}

void TaskContainer::desktopChanged(int)
{
    updateFilteredTaskList();
    KickerTip::Client::updateKickerTip();
    update();
}

void TaskContainer::windowChanged(Task::Ptr)
{
    updateFilteredTaskList();
    KickerTip::Client::updateKickerTip();
    update();
}

void TaskContainer::settingsChanged()
{
    updateFilteredTaskList();
    KickerTip::Client::updateKickerTip();
    update();
}

void TaskContainer::updateKickerTip(KickerTip::Data& data)
{
    int iconSize = READ_MERGED_TASKBAR_SETTING(iconSize);

    if (m_startup)
    {
        data.message = m_startup->text();
        data.duration = 4000;
        data.subtext = i18n("Loading application ...");
        data.icon = TDEGlobal::iconLoader()->loadIcon(m_startup->icon(),
                                                      TDEIcon::Small,
                                                      iconSize,
                                                      TDEIcon::DefaultState,
                                                      0, true);
        return;
    }

    TQPixmap pixmap;
    TQString name;
    TQString details;

    if (m_filteredTasks.count() > 0)
    {
        if (READ_MERGED_TASKBAR_SETTING(showThumbnails) &&
            m_filteredTasks.count() == 1)
        {
            Task::Ptr t = m_filteredTasks.first();

            pixmap = t->thumbnail(READ_MERGED_TASKBAR_SETTING(thumbnailMaxDimension));
        }

        if (pixmap.isNull() && tasks.count())
        {
            // try to load icon via net_wm
            pixmap = KWin::icon(tasks.last()->window(), iconSize, iconSize, true);
        }

        // Collect all desktops the tasks are on. Sort naturally.
        TQMap<int, TQString> desktopMap;
        bool demandsAttention = false;
        bool modified = false;
        bool allDesktops = false;
        Task::List::const_iterator itEnd = m_filteredTasks.constEnd();
        for (Task::List::const_iterator it = m_filteredTasks.constBegin(); it != itEnd; ++it)
        {
            Task::Ptr t = *it;
            if (t->demandsAttention())
            {
                demandsAttention = true;
            }

            if (t->isModified())
            {
                modified = true;
            }

            if (t->isOnAllDesktops())
            {
                allDesktops = true;
                desktopMap.clear();
            }
            else if (!allDesktops)
            {
                desktopMap.insert(t->desktop(),
                                TaskManager::the()->desktopName(t->desktop()));
            }
        }

        if (READ_MERGED_TASKBAR_SETTING(showAllWindows) && KWin::numberOfDesktops() > 1)
        {
            if (desktopMap.isEmpty())
            {
                details.append(i18n("On all desktops"));
            }
            else
            {
                TQStringList desktopNames = desktopMap.values();
                details.append(i18n("On %1").arg(TQStyleSheet::escape(desktopNames.join(", "))) + "<br>");
            }
        }

        if (demandsAttention)
        {
            details.append(i18n("Requesting attention") + "<br>");
        }

        name = this->name();
        if (modified)
        {
            details.append(i18n("Has unsaved changes"));

            static TQString modStr = "[" + i18n( "modified" ) + "]";
            int modStrPos = name.find(modStr);

            if (modStrPos >= 0)
            {
                // +1 because we include a space after the closing brace.
                name.remove(modStrPos, modStr.length() + 1);
            }
        }
    }

    data.message = TQStyleSheet::escape(name);
    data.subtext = details;
    data.icon = pixmap;
    data.direction = KickerLib::arrowToDirection(arrowType);
}

void TaskContainer::finish()
{
    // Disconnect all signal/slot connections to avoid triggering a popupMenu() call,
    // whose event loop is the root of all (or at least much) evil.
    // Unfortunately, we can't just do "disconnect()", because that gets us a bunch
    // of dangling TQGuardedPtr objects (most notably in QTipManager.) (kling)

    animationTimer.disconnect();
    dragSwitchTimer.disconnect();
    attentionTimer.disconnect();

    if (m_startup)
        m_startup->disconnect(this);

    for (Task::List::Iterator it = tasks.begin(); it != tasks.end(); ++it)
    {
        (*it)->disconnect(this);
    }

    if (m_menu)
        m_menu->close();
}
