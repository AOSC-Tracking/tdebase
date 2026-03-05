/*****************************************************************

Copyright (c) 2000-2001 Matthias Elter <elter@kde.org>
Copyright (c) 2001 Richard Moore <rich@kde.org>

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

#ifndef __taskmanager_h__
#define __taskmanager_h__

#include <sys/types.h>

#include <tqobject.h>
#include <tqpixmap.h>
#include <tqpoint.h>
#include <tqptrlist.h>
#include <tqpixmap.h>
#include <tqdragobject.h>
#include <tqrect.h>
#include <tqvaluelist.h>
#include <tqvaluevector.h>

#include <ksharedptr.h>
#include <tdestartupinfo.h>
#include <twin.h>

#include <config.h>

#if defined(HAVE_XCOMPOSITE) && \
    defined(HAVE_XRENDER) && \
    defined(HAVE_XFIXES)
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#if XCOMPOSITE_VERSION >= 00200 && \
    XFIXES_VERSION >= 20000 && \
    (RENDER_MAJOR > 0 || RENDER_MINOR >= 6)
#define THUMBNAILING_POSSIBLE
#endif
#endif

class TWinModule;
class TaskManager;

typedef TQValueList<WId> WindowList;

/**
 * A dynamic interface to a task (main window).
 *
 * @see TaskManager
 * @see TWinModule
 */
class TDE_EXPORT Task: public TQObject, public TDEShared
{
    TQ_OBJECT
    TQ_PROPERTY( TQString visibleIconicName READ visibleIconicName )
    TQ_PROPERTY( TQString iconicName READ iconicName )
    TQ_PROPERTY( TQString visibleIconicNameWithState READ visibleIconicNameWithState )
    TQ_PROPERTY( TQString visibleName READ visibleName )
    TQ_PROPERTY( TQString name READ name )
    TQ_PROPERTY( TQString visibleNameWithState READ visibleNameWithState )
    TQ_PROPERTY( TQPixmap pixmap READ pixmap )
    TQ_PROPERTY( bool maximized READ isMaximized )
    TQ_PROPERTY( bool minimized READ isMinimized )
    // KDE4 deprecated
    TQ_PROPERTY( bool iconified READ isIconified )
    TQ_PROPERTY( bool shaded READ isShaded WRITE setShaded )
    TQ_PROPERTY( bool active READ isActive )
    TQ_PROPERTY( bool onCurrentDesktop READ isOnCurrentDesktop )
    TQ_PROPERTY( bool onAllDesktops READ isOnAllDesktops )
    TQ_PROPERTY( bool alwaysOnTop READ isAlwaysOnTop WRITE setAlwaysOnTop )
    TQ_PROPERTY( bool modified READ isModified )
    TQ_PROPERTY( bool demandsAttention READ demandsAttention )
    TQ_PROPERTY( int desktop READ desktop )
    TQ_PROPERTY( double thumbnailSize READ thumbnailSize WRITE setThumbnailSize )
    TQ_PROPERTY( bool hasThumbnail READ hasThumbnail )
    TQ_PROPERTY( TQPixmap thumbnail READ thumbnail )

public:
    typedef TDESharedPtr<Task> Ptr;
    typedef TQValueVector<Task::Ptr> List;
    typedef TQMap<WId, Task::Ptr> Dict;

    Task(WId win, TQObject *parent, const char *name = 0);
    virtual ~Task();

    WId window() const { return _win; }
    KWin::WindowInfo info() const { return _info; }

#if 0 // this would use (_NET_)WM_ICON_NAME, which is shorter, but can be different from window name
    TQString visibleIconicName() const { return _info.visibleIconName(); }
    TQString visibleIconicNameWithState() const { return _info.visibleIconNameWithState(); }
    TQString iconicName() const { return _info.iconName(); }
#else
    TQString visibleIconicName() const { return _info.visibleName(); }
    TQString visibleIconicNameWithState() const { return _info.visibleNameWithState(); }
    TQString iconicName() const { return _info.name(); }
#endif
    TQString visibleName() const { return _info.visibleName(); }
    TQString visibleNameWithState() const { return _info.visibleNameWithState(); }
    TQString name() const { return _info.name(); }
    TQString className();
    TQString classClass();

    /**
     * A list of the window ids of all transient windows (dialogs) associated
     * with this task.
     */
    WindowList transients() const { return _transients; }

    /**
     * Returns a 16x16 (TDEIcon::Small) icon for the task. This method will
     * only fall back to a static icon if there is no icon of any size in
     * the WM hints.
     */
    TQPixmap pixmap() const { return _pixmap; }

    /**
     * Returns the best icon for any of the TDEIcon::StdSizes. If there is no
     * icon of the specified size specified in the WM hints, it will try to
     * get one using TDEIconLoader.
     *
     * <pre>
     *   bool gotStaticIcon;
     *   TQPixmap icon = myTask->icon( TDEIcon::SizeMedium, gotStaticIcon );
     * </pre>
     *
     * @param size Any of the constants in TDEIcon::StdSizes.
     * @param isStaticIcon Set to true if TDEIconLoader was used, false otherwise.
     * @see TDEIcon
     */
    TQPixmap bestIcon( int size, bool &isStaticIcon );

    /**
     * Tries to find an icon for the task with the specified size. If there
     * is no icon that matches then it will either resize the closest available
     * icon or return a null pixmap depending on the value of allowResize.
     *
     * Note that the last icon is cached, so a sequence of calls with the same
     * parameters will only query the NET properties if the icon has changed or
     * none was found.
     */
    TQPixmap icon( int width, int height, bool allowResize = false );

    /**
     * Returns true iff the windows with the specified ids should be grouped
     * together in the task list.
     */
    static bool idMatch(const TQString &, const TQString &);

    // state

    /**
     * Returns true if the task's window is maximized.
     */
    bool isMaximized() const;

    /**
     * Returns true if the task's window is minimized.
     */
    bool isMinimized() const;

    /**
     * @deprecated
     * Returns true if the task's window is minimized(iconified).
     */
    bool isIconified() const;

    /**
     * Returns true if the task's window is shaded.
     */
    bool isShaded() const;

    /**
     * Returns true if the task's window is the active window.
     */
    bool isActive() const;

    /**
     * Returns true if the task's window is the topmost non-iconified,
     * non-always-on-top window.
     */
    bool isOnTop() const;

    /**
     * Returns true if the task's window is on the current virtual desktop.
     */
    bool isOnCurrentDesktop() const;

    /**
     * Returns true if the task's window is on all virtual desktops.
     */
    bool isOnAllDesktops() const;

    /**
     * Returns true if the task's window will remain at the top of the
     * stacking order.
     */
    bool isAlwaysOnTop() const;

    /**
     * Returns true if the task's window will remain at the bottom of the
     * stacking order.
     */
    bool isKeptBelowOthers() const;

    /**
     * Returns true if the task's window is in full screen mode
     */
    bool isFullScreen() const;

    /**
     * Returns true if the document the task is editing has been modified.
     * This is currently handled heuristically by looking for the string
     * '[i18n_modified]' in the window title where i18n_modified is the
     * word 'modified' in the current language.
     */
    bool isModified() const ;

    /**
     * Returns the desktop on which this task's window resides.
     */
    int desktop() const { return _info.desktop(); }

    /**
     * Returns true if the task is not active but demands user's attention.
     */
    bool demandsAttention() const;


    /**
    * Returns true if the window is on the specified screen of a multihead configuration
    */
    bool isOnScreen( int screen ) const;

    /**
     * Returns true if the task should be shown in taskbar-like apps
     */
    bool showInTaskbar() const { return _info.state() ^ NET::SkipTaskbar; }

    /**
     * Returns true if the task should be shown in pager-like apps
     */
    bool showInPager() const { return _info.state() ^ NET::SkipPager; }

    /**
     * Returns the geometry for this window
     */
    TQRect geometry() const { return _info.geometry(); }

    /**
     * Returns the geometry for the from of this window
     */
    TQRect frameGeometry() const { return _info.frameGeometry(); }

    // internal

    //* @internal
    void refresh(unsigned int dirty);
    //* @internal
    void refreshIcon();
    //* @internal
    void addTransient( WId w, const NETWinInfo& info );
    //* @internal
    void removeTransient( WId w );
    //* @internal
    bool hasTransient(WId w) const { return _transients.find(w) != _transients.end(); }
    //* @internal
    void updateDemandsAttentionState( WId w );
    //* @internal
    void setActive(bool a);

    // For thumbnails

    /**
     * Returns the current thumbnail size.
     */
    double thumbnailSize() const { return _thumbSize; }

    /**
     * Sets the size for the window thumbnail. For example a size of
     * 0.2 indicates the thumbnail will be 20% of the original window
     * size.
     */
    void setThumbnailSize( double size ) { _thumbSize = size; }

    /**
     * Returns true if this task has a thumbnail. Note that this method
     * can only ever return true after a call to updateThumbnail().
     */
    bool hasThumbnail() const { return !_thumb.isNull(); }

    /**
     * Returns the thumbnail for this task (or a null image if there is
     * none).
     */
    const TQPixmap &thumbnail() const { return _thumb; }

    TQPixmap thumbnail(int maxDimension);

    void updateWindowPixmap();

public slots:
    // actions

    /**
     * Maximise the main window of this task.
     */
    void setMaximized(bool);
    void toggleMaximized();

    /**
     * Restore the main window of the task (if it was iconified).
     */
    void restore();

    /**
     * Move the window of this task.
     */
    void move();

    /**
     * Resize the window of this task.
     */
    void resize();

    /**
     * Iconify the task.
     */
    void setIconified(bool);
    void toggleIconified();

    /**
     * Close the task's window.
     */
    void close();

    /**
     * Raise the task's window.
     */
    void raise();

    /**
     * Lower the task's window.
     */
    void lower();

   /**
     * Activate the task's window.
     */
    void activate();

    /**
     * Perform the action that is most appropriate for this task. If it
     * is not active, activate it. Else if it is not the top window, raise
     * it. Otherwise, iconify it.
     */
    void activateRaiseOrIconify();

    /**
     * If true, the task's window will remain at the top of the stacking order.
     */
    void setAlwaysOnTop(bool);
    void toggleAlwaysOnTop();

    /**
     * If true, the task's window will remain at the bottom of the stacking order.
     */
    void setKeptBelowOthers(bool);
    void toggleKeptBelowOthers();

    /**
     * If true, the task's window will enter full screen mode.
     */
    void setFullScreen(bool);
    void toggleFullScreen();

    /**
     * If true then the task's window will be shaded. Most window managers
     * represent this state by displaying on the window's title bar.
     */
    void setShaded(bool);
    void toggleShaded();

    /**
     * Moves the task's window to the specified virtual desktop.
     */
    void toDesktop(int);

    /**
     * Moves the task's window to the current virtual desktop.
     */
    void toCurrentDesktop();

    /**
     * This method informs the window manager of the location at which this
     * task will be displayed when iconised. It is used, for example by the
     * KWin inconify animation.
     */
    void publishIconGeometry(TQRect);

    /**
     * Tells the task to generate a new thumbnail. When the thumbnail is
     * ready the thumbnailChanged() signal will be emitted.
     */
    void updateThumbnail();

    /**
     * Tile the task's window to the specified position. The position is one of the
     * valid value for ActiveBorder enum
     */
    void tileTo(int);

signals:
    /**
     * Indicates that this task has changed in some way.
     */
    void changed(bool geometryChangeOnly);

    /**
     * Indicates that the icon for this task has changed.
     */
    void iconChanged();

    /**
     * Indicates that this task is now the active task.
     */
    void activated();

    /**
     * Indicates that this task is no longer the active task.
     */
    void deactivated();

    /**
     * Indicates that the thumbnail for this task has changed.
     */
    void thumbnailChanged();

protected slots:
    //* @internal
    void generateThumbnail();

protected:
    void findWindowFrameId();

private:
    bool                _active;
    WId                 _win;
    WId                 m_frameId;
    TQPixmap             _pixmap;
    KWin::WindowInfo    _info;
    WindowList          _transients;
    WindowList          _transients_demanding_attention;

    int                 _lastWidth;
    int                 _lastHeight;
    bool                _lastResize;
    TQPixmap             _lastIcon;

    double _thumbSize;
    TQPixmap _thumb;
    TQPixmap _grab;
    TQRect m_iconGeometry;
#ifdef THUMBNAILING_POSSIBLE
    Pixmap              m_windowPixmap;
#endif // THUMBNAILING_POSSIBLE
};


/**
 * Provids a drag object for tasks across desktops.
 */
class TDE_EXPORT TaskDrag : public TQStoredDrag
{
public:
    /**
     * Constructs a task drag object for a task list.
     */
    TaskDrag(const Task::List& tasks, TQWidget* source = 0,
             const char* name = 0);
    ~TaskDrag();

    /**
     * Returns true if the mime source can be decoded to a TaskDrag.
     */
    static bool canDecode( const TQMimeSource* e );

    /**
     * Decodes the tasks from the mime source and returns them if successful.
     * Otherwise an empty task list is returned.
     */
    static Task::List decode( const TQMimeSource* e );
};


/**
 * Represents a task which is in the process of starting.
 *
 * @see TaskManager
 */
class TDE_EXPORT Startup: public TQObject, public TDEShared
{
    TQ_OBJECT
    TQ_PROPERTY( TQString text READ text )
    TQ_PROPERTY( TQString bin READ bin )
    TQ_PROPERTY( TQString icon READ icon )

public:
    typedef TDESharedPtr<Startup> Ptr;
    typedef TQValueVector<Startup::Ptr> List;

    Startup( const TDEStartupInfoId& id, const TDEStartupInfoData& data, TQObject * parent,
        const char *name = 0);
    virtual ~Startup();

    /**
     * The name of the starting task (if known).
     */
    TQString text() const { return _data.findName(); }

    /**
     * The name of the executable of the starting task.
     */
    TQString bin() const { return _data.bin(); }

    /**
     * The name of the icon to be used for the starting task.
     */
    TQString icon() const { return _data.findIcon(); }
    void update( const TDEStartupInfoData& data );
    const TDEStartupInfoId& id() const { return _id; }

signals:
    /**
     * Indicates that this startup has changed in some way.
     */
    void changed();

private:
    TDEStartupInfoId _id;
    TDEStartupInfoData _data;
    class StartupPrivate *d;
};


/**
 * A generic API for task managers. This class provides an easy way to
 * build NET compliant task managers. It provides support for startup
 * notification, virtual desktops and the full range of WM properties.
 *
 * @see Task
 * @see Startup
 * @see TWinModule
 */
class TDE_EXPORT TaskManager : public TQObject
{
    TQ_OBJECT
    TQ_PROPERTY( int currentDesktop READ currentDesktop )
    TQ_PROPERTY( int numberOfDesktops READ numberOfDesktops )

public:
    static TaskManager* the();
    ~TaskManager();

    /**
     * Returns the task for a given WId, or 0 if there is no such task.
     */
    Task::Ptr findTask(WId w);

    /**
     * Returns the task for a given location, or 0 if there is no such task.
     */
    Task::Ptr findTask(int desktop, const TQPoint& p);

    /**
     * Returns a list of all current tasks.
     */
    Task::Dict tasks() const { return m_tasksByWId; }

    /**
     * Returns a list of all current startups.
     */
    Startup::List startups() const { return _startups; }

    /**
     * Returns the name of the nth desktop.
     */
    TQString desktopName(int n) const;

    /**
     * Returns the number of virtual desktops.
     */
    int numberOfDesktops() const;

    /**
     * Returns the number of the current desktop.
     */
    int currentDesktop() const;

    /**
     * Returns true if the specified task is on top.
     */
    bool isOnTop(const Task*);

    /**
     * Tells the task manager whether or not we care about geometry
     * updates. This generates a lot of activity so should only be used
     * when necessary.
     */
    void trackGeometry() { m_trackGeometry = true; }
    void trackGeometry(bool track) { m_trackGeometry = track; }

    /**
    * Returns whether the Window with WId wid is on the screen screen
    */
    static bool isOnScreen( int screen, const WId wid );

    TWinModule* winModule() const { return m_winModule; }

    void setXCompositeEnabled(bool state);
    static bool xCompositeEnabled() { return m_xCompositeEnabled != 0; }

signals:
    /**
     * Emitted when a new task has started.
     */
    void taskAdded(Task::Ptr);

    /**
     * Emitted when a task has terminated.
     */
    void taskRemoved(Task::Ptr);

    /**
     * Emitted when a new task is expected.
     */
    void startupAdded(Startup::Ptr);

    /**
     * Emitted when a startup item should be removed. This could be because
     * the task has started, because it is known to have died, or simply
     * as a result of a timeout.
     */
    void startupRemoved(Startup::Ptr);

    /**
     * Emitted when the current desktop changes.
     */
    void desktopChanged(int desktop);

    /**
     * Emitted when a window changes desktop.
     */
    void windowChanged(Task::Ptr);
    void windowChangedGeometry(Task::Ptr);

protected slots:
    //* @internal
    void windowAdded(WId);
    //* @internal
    void windowRemoved(WId);
    //* @internal
    void windowChanged(WId, unsigned int);

    //* @internal
    void activeWindowChanged(WId);
    //* @internal
    void currentDesktopChanged(int);
    //* @internal
    void killStartup( const TDEStartupInfoId& );
    //* @internal
    void killStartup(Startup::Ptr);

    //* @internal
    void gotNewStartup( const TDEStartupInfoId&, const TDEStartupInfoData& );
    //* @internal
    void gotStartupChange( const TDEStartupInfoId&, const TDEStartupInfoData& );

protected:
    void configure_startup();
    void updateWindowPixmap(WId);

private:
    TaskManager();

    Task::Ptr               _active;
    Task::Dict m_tasksByWId;
    WindowList _skiptaskbar_windows;
    Startup::List _startups;
    TDEStartupInfo* _startup_info;
    TWinModule* m_winModule;
    bool m_trackGeometry;

    static TaskManager* m_self;
    static uint m_xCompositeEnabled;

    class TaskManagerPrivate *d;
};

#endif
