/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#ifndef KWIN_CLIENT_H
#define KWIN_CLIENT_H

#include <tqframe.h>
#include <tqvbox.h>
#include <tqpixmap.h>
#include <netwm.h>
#include <kdebug.h>
#include <assert.h>
#include <kshortcut.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <fixx11h.h>

#include "utils.h"
#include "options.h"
#include "workspace.h"
#include "kdecoration.h"
#include "rules.h"

class TQTimer;
class KProcess;
class KStartupInfoData;

namespace KWinInternal
{

class Workspace;
class Client;
class WinInfo;
class SessionInfo;
class Bridge;

class Client : public TQObject, public KDecorationDefines
    {
    Q_OBJECT
    public:
        Client( Workspace *ws );
        Window window() const;
        Window frameId() const;
        Window wrapperId() const;
        Window decorationId() const;

        Workspace* workspace() const;
        const Client* transientFor() const;
        Client* transientFor();
        bool isTransient() const;
        bool isModalSystemNotification() const;
        bool groupTransient() const;
        bool wasOriginallyGroupTransient() const;
        ClientList mainClients() const; // call once before loop , is not indirect
        bool hasTransient( const Client* c, bool indirect ) const;
        const ClientList& transients() const; // is not indirect
        void checkTransient( Window w );
        Client* findModal();
        const Group* group() const;
        Group* group();
        void checkGroup( Group* gr = NULL, bool force = false );
        void changeClientLeaderGroup( Group* gr );
    // prefer isXXX() instead
        NET::WindowType windowType( bool direct = false, int supported_types = SUPPORTED_WINDOW_TYPES_MASK ) const;
        const WindowRules* rules() const;
        void removeRule( Rules* r );
        void setupWindowRules( bool ignore_temporary );
        void applyWindowRules();
        void updateWindowRules();

        TQRect geometry() const;
        TQSize size() const;
        TQSize minSize() const;
        TQSize maxSize() const;
        TQPoint pos() const;
        TQRect rect() const;
        int x() const;
        int y() const;
        int width() const;
        int height() const;
        TQPoint clientPos() const; // inside of geometry()
        TQSize clientSize() const;

        bool windowEvent( XEvent* e );
        virtual bool eventFilter( TQObject* o, TQEvent* e );

        bool manage( Window w, bool isMapped );

        void releaseWindow( bool on_shutdown = false );

        enum Sizemode // how to resize the window in order to obey constains (mainly aspect ratios)
            {
            SizemodeAny,
            SizemodeFixedW, // try not to affect width
            SizemodeFixedH, // try not to affect height
            SizemodeMax // try not to make it larger in either direction
            };
        TQSize adjustedSize( const TQSize&, Sizemode mode = SizemodeAny ) const;
        TQSize adjustedSize() const;

        TQPixmap icon() const;
        TQPixmap miniIcon() const;

        bool isActive() const;
        void setActive( bool, bool updateOpacity = true );

        int desktop() const;
        void setDesktop( int );
        bool isOnDesktop( int d ) const;
        bool isOnCurrentDesktop() const;
        bool isOnAllDesktops() const;
        void setOnAllDesktops( bool set );
        
        bool isOnScreen( int screen ) const; // true if it's at least partially there
        int screen() const; // the screen where the center is

    // !isMinimized() && not hidden, i.e. normally visible on some virtual desktop
        bool isShown( bool shaded_is_shown ) const;

        bool isShade() const; // true only for ShadeNormal
        ShadeMode shadeMode() const; // prefer isShade()
        void setShade( ShadeMode mode );
        bool isShadeable() const;

        bool isMinimized() const;
        bool isMaximizable() const;
        TQRect geometryRestore() const;
        MaximizeMode maximizeModeRestore() const;
        MaximizeMode maximizeMode() const;
        bool isMinimizable() const;
        void setMaximize( bool vertically, bool horizontally );

        void setFullScreen( bool set, bool user );
        bool isFullScreen() const;
        bool isFullScreenable( bool fullscreen_hack = false ) const;
        bool userCanSetFullScreen() const;
        TQRect geometryFSRestore() const { return geom_fs_restore; } // only for session saving
        int fullScreenMode() const { return fullscreen_mode; } // only for session saving

        bool isUserNoBorder() const;
        void setUserNoBorder( bool set );
        bool userCanSetNoBorder() const;
        bool noBorder() const;

        bool skipTaskbar( bool from_outside = false ) const;
        void setSkipTaskbar( bool set, bool from_outside );

        bool skipPager() const;
        void setSkipPager( bool );

        bool keepAbove() const;
        void setKeepAbove( bool );
        bool keepBelow() const;
        void setKeepBelow( bool );
        Layer layer() const;
        Layer belongsToLayer() const;
        void invalidateLayer();

        void setModal( bool modal );
        bool isModal() const;

    // auxiliary functions, depend on the windowType
        bool wantsTabFocus() const;
        bool wantsInput() const;
        bool hasNETSupport() const;
        bool isMovable() const;
        bool isDesktop() const;
        bool isDock() const;
        bool isToolbar() const;
        bool isTopMenu() const;
        bool isMenu() const;
        bool isNormalWindow() const; // normal as in 'NET::Normal or NET::Unknown non-transient'
        bool isDialog() const;
        bool isSplash() const;
        bool isUtility() const;
    // returns true for "special" windows and false for windows which are "normal"
    // (normal=window which has a border, can be moved by the user, can be closed, etc.)
    // true for Desktop, Dock, Splash, Override and TopMenu (and Toolbar??? - for now)
    // false for Normal, Dialog, Utility and Menu (and Toolbar??? - not yet) TODO
        bool isSpecialWindow() const;

        bool isResizable() const;
        bool isCloseable() const; // may be closed by the user (may have a close button)

        void takeActivity( int flags, bool handled, allowed_t ); // takes ActivityFlags as arg (in utils.h)
        void takeFocus( allowed_t );
        void demandAttention( bool set = true );

        void setMask( const TQRegion& r, int mode = X::Unsorted );
        TQRegion mask() const;

        void updateDecoration( bool check_workspace_pos, bool force = false );
        void checkBorderSizes();

    // drop shadow
        bool isShadowed() const;
        void setShadowed(bool shadowed);
        Window shadowId() const;
        // Aieee, a friend function! Unpleasant, yes, but it's needed by
        // raiseClient() to redraw a window's shadow when it is active prior to
        // being raised.
        friend void Workspace::raiseClient(Client *);
        // Wouldn't you know it, friend functions breed. This one's needed to
        // enable a DCOP function that causes all shadows obscuring a changed
        // window to be redrawn.
        friend void Workspace::updateOverlappingShadows(WId);

    // shape extensions
        bool shape() const;
        void updateShape();

        void setGeometry( int x, int y, int w, int h, ForceGeometry_t force = NormalGeometrySet );
        void setGeometry( const TQRect& r, ForceGeometry_t force = NormalGeometrySet );
        void move( int x, int y, ForceGeometry_t force = NormalGeometrySet );
        void move( const TQPoint & p, ForceGeometry_t force = NormalGeometrySet );
        // plainResize() simply resizes
        void plainResize( int w, int h, ForceGeometry_t force = NormalGeometrySet );
        void plainResize( const TQSize& s, ForceGeometry_t force = NormalGeometrySet );
        // resizeWithChecks() resizes according to gravity, and checks workarea position
        void resizeWithChecks( int w, int h, ForceGeometry_t force = NormalGeometrySet );
        void resizeWithChecks( const TQSize& s, ForceGeometry_t force = NormalGeometrySet );
        void keepInArea( TQRect area, bool partial = false );

        void growHorizontal();
        void shrinkHorizontal();
        void growVertical();
        void shrinkVertical();

        bool providesContextHelp() const;
        KShortcut shortcut() const;
        void setShortcut( const TQString& cut );

        bool performMouseCommand( Options::MouseCommand, TQPoint globalPos, bool handled = false );

        TQCString windowRole() const;
        TQCString sessionId();
        TQCString resourceName() const;
        TQCString resourceClass() const;
        TQCString wmCommand();
        TQCString wmClientMachine( bool use_localhost ) const;
        Window   wmClientLeader() const;
        pid_t pid() const;

        TQRect adjustedClientArea( const TQRect& desktop, const TQRect& area ) const;

        Colormap colormap() const;

    // updates visibility depending on being shaded, virtual desktop, etc.
        void updateVisibility();
    // hides a client - basically like minimize, but without effects, it's simply hidden
        void hideClient( bool hide );

        TQString caption( bool full = true ) const;
        void updateCaption();

        void keyPressEvent( uint key_code ); // FRAME ??
        void updateMouseGrab();
        Window moveResizeGrabWindow() const;

        const TQPoint calculateGravitation( bool invert, int gravity = 0 ) const; // FRAME public?

        void NETMoveResize( int x_root, int y_root, NET::Direction direction );
        void NETMoveResizeWindow( int flags, int x, int y, int width, int height );
        void restackWindow( Window above, int detail, NET::RequestSource source, Time timestamp, bool send_event = false );
        
        void gotPing( Time timestamp );

        static TQCString staticWindowRole(WId);
        static TQCString staticSessionId(WId);
        static TQCString staticWmCommand(WId);
        static TQCString staticWmClientMachine(WId);
        static Window   staticWmClientLeader(WId);

        void checkWorkspacePosition();
        void updateUserTime( Time time = CurrentTime );
        Time userTime() const;
        bool hasUserTimeSupport() const;
        bool ignoreFocusStealing() const;

    // does 'delete c;'
        static void deleteClient( Client* c, allowed_t );

        static bool resourceMatch( const Client* c1, const Client* c2 );
        static bool belongToSameApplication( const Client* c1, const Client* c2, bool active_hack = false );
        static void readIcons( Window win, TQPixmap* icon, TQPixmap* miniicon );

        void minimize( bool avoid_animation = false );
        void unminimize( bool avoid_animation = false );
        void closeWindow();
        void killWindow();
        void maximize( MaximizeMode );
        void toggleShade();
        void showContextHelp();
        void cancelShadeHover();
        void cancelAutoRaise();
        void destroyClient();
        void checkActiveModal();
        void setOpacity(bool translucent, uint opacity = 0);
        void setShadowSize(uint shadowSize);
        void updateOpacity();
        void updateShadowSize();
        bool hasCustomOpacity(){return custom_opacity;}
        void setCustomOpacityFlag(bool custom = true);
        bool getWindowOpacity();
        int opacityPercentage();
        void checkAndSetInitialRuledOpacity();
        uint ruleOpacityInactive();
        uint ruleOpacityActive();
        unsigned int opacity();
        bool isBMP();
        void setBMP(bool b);
        bool touches(const Client* c);
        void setShapable(bool b);
        bool hasStrut() const;

    private slots:
        void autoRaise();
        void shadeHover();
        void shortcutActivated();
	void updateOpacityCache();


    private:
        friend class Bridge; // FRAME
        virtual void processMousePressEvent( TQMouseEvent* e );

    private: // TODO cleanup the order of things in the .h file
    // use Workspace::createClient()
        virtual ~Client(); // use destroyClient() or releaseWindow()

        Position mousePosition( const TQPoint& ) const;
        void setCursor( Position m );
        void setCursor( const TQCursor& c );

        void  animateMinimizeOrUnminimize( bool minimize );
        TQPixmap animationPixmap( int w );
    // transparent stuff
        void drawbound( const TQRect& geom );
        void clearbound();
        void doDrawbound( const TQRect& geom, bool clear );

    // handlers for X11 events
        bool mapRequestEvent( XMapRequestEvent* e );
        void unmapNotifyEvent( XUnmapEvent*e );
        void destroyNotifyEvent( XDestroyWindowEvent*e );
        void configureRequestEvent( XConfigureRequestEvent* e );
        void propertyNotifyEvent( XPropertyEvent* e );
        void clientMessageEvent( XClientMessageEvent* e );
        void enterNotifyEvent( XCrossingEvent* e );
        void leaveNotifyEvent( XCrossingEvent* e );
        void focusInEvent( XFocusInEvent* e );
        void focusOutEvent( XFocusOutEvent* e );

        bool buttonPressEvent( Window w, int button, int state, int x, int y, int x_root, int y_root );
        bool buttonReleaseEvent( Window w, int button, int state, int x, int y, int x_root, int y_root );
        bool motionNotifyEvent( Window w, int state, int x, int y, int x_root, int y_root );

    // drop shadows
	void drawIntersectingShadows();
	void drawOverlappingShadows(bool waitForMe);
	TQRegion getExposedRegion(TQRegion occludedRegion, int x, int y,
	    int w, int h, int thickness, int xOffset, int yOffset);
	void imposeCachedShadow(TQPixmap &pixmap, TQRegion exposed);
	void imposeRegionShadow(TQPixmap &pixmap, TQRegion occluded,
	    TQRegion exposed, int thickness, double maxOpacity = 0.75);

        void processDecorationButtonPress( int button, int state, int x, int y, int x_root, int y_root );

    private slots:
        void pingTimeout();
        void processKillerExited();
        void demandAttentionKNotify();
	void drawShadow();
	void drawShadowAfter(Client *after);
	void drawDelayedShadow();
	void removeShadow();

    signals:
	void shadowDrawn();


    private:
    // ICCCM 4.1.3.1, 4.1.4 , NETWM 2.5.1
        void setMappingState( int s );
        int mappingState() const;
        bool isIconicState() const;
        bool isNormalState() const;
        bool isManaged() const; // returns false if this client is not yet managed
        void updateAllowedActions( bool force = false );
        TQSize sizeForClientSize( const TQSize&, Sizemode mode = SizemodeAny, bool noframe = false ) const;
        void changeMaximize( bool horizontal, bool vertical, bool adjust );
        void checkMaximizeGeometry();
        int checkFullScreenHack( const TQRect& geom ) const; // 0 - none, 1 - one xinerama screen, 2 - full area
        void updateFullScreenHack( const TQRect& geom );
        void getWmNormalHints();
        void getMotifHints();
        void getIcons();
        void getWmClientLeader();
        void getWmClientMachine();
        void fetchName();
        void fetchIconicName();
        TQString readName() const;
        void setCaption( const TQString& s, bool force = false );
        bool hasTransientInternal( const Client* c, bool indirect, ConstClientList& set ) const;
        void finishWindowRules();
        void setShortcutInternal( const KShortcut& cut );

        void updateWorkareaDiffs();
        void checkDirection( int new_diff, int old_diff, TQRect& rect, const TQRect& area );
        static int computeWorkareaDiff( int left, int right, int a_left, int a_right );
        void configureRequest( int value_mask, int rx, int ry, int rw, int rh, int gravity, bool from_tool );
        NETExtendedStrut strut() const;
        int checkShadeGeometry( int w, int h );
        void postponeGeometryUpdates( bool postpone );

        bool startMoveResize();
        void finishMoveResize( bool cancel );
        void leaveMoveResize();
        void checkUnrestrictedMoveResize();
        void handleMoveResize( int x, int y, int x_root, int y_root );
        void positionGeometryTip();
        void grabButton( int mod );
        void ungrabButton( int mod );
        void resetMaximize();
        void resizeDecoration( const TQSize& s );
        void setDecoHashProperty(uint topHeight, uint rightWidth, uint bottomHeight, uint leftWidth);
        void unsetDecoHashProperty();

        void pingWindow();
        void killProcess( bool ask, Time timestamp = CurrentTime );
        void updateUrgency();
        static void sendClientMessage( Window w, Atom a, Atom protocol,
            long data1 = 0, long data2 = 0, long data3 = 0 );

        void embedClient( Window w, const XWindowAttributes &attr );    
        void detectNoBorder();
        void detectShapable();
        void destroyDecoration();
        void updateFrameExtents();

        void rawShow(); // just shows it
        void rawHide(); // just hides it

        Time readUserTimeMapTimestamp( const KStartupInfoId* asn_id, const KStartupInfoData* asn_data,
            bool session ) const;
        Time readUserCreationTime() const;
        static bool sameAppWindowRoleMatch( const Client* c1, const Client* c2, bool active_hack );
        void startupIdChanged();

        Window client;
        Window wrapper;
        Window frame;
        KDecoration* decoration;
        Workspace* wspace;
        Bridge* bridge;
        int desk;
        bool buttonDown;
        bool moveResizeMode;
        bool move_faked_activity;
        Window move_resize_grab_window;
        bool unrestrictedMoveResize;
        bool isMove() const 
            {
            return moveResizeMode && mode == PositionCenter;
            }
        bool isResize() const 
            {
            return moveResizeMode && mode != PositionCenter;
            }

        Position mode;
        TQPoint moveOffset;
        TQPoint invertedMoveOffset;
        TQRect moveResizeGeom;
        TQRect initialMoveResizeGeom;
        XSizeHints  xSizeHint;
        void sendSyntheticConfigureNotify();
        int mapping_state;
        void readTransient();
        Window verifyTransientFor( Window transient_for, bool set );
        void addTransient( Client* cl );
        void removeTransient( Client* cl );
        void removeFromMainClients();
        void cleanGrouping();
        void checkGroupTransients();
        void setTransient( Window new_transient_for_id );
        Client* transient_for;
        Window transient_for_id;
        Window original_transient_for_id;
        ClientList transients_list; // SELI make this ordered in stacking order?
        ShadeMode shade_mode;
        uint active :1;
        uint deleting : 1; // true when doing cleanup and destroying the client
        uint keep_above : 1; // NET::KeepAbove (was stays_on_top)
        uint is_shape :1;
        uint skip_taskbar :1;
        uint original_skip_taskbar :1; // unaffected by KWin
        uint Pdeletewindow :1; // does the window understand the DeleteWindow protocol?
        uint Ptakefocus :1;// does the window understand the TakeFocus protocol?
        uint Ptakeactivity : 1; // does it support _NET_WM_TAKE_ACTIVITY
        uint Pcontexthelp : 1; // does the window understand the ContextHelp protocol?
        uint Pping : 1; // does it support _NET_WM_PING?
        uint input :1; // does the window want input in its wm_hints
        uint skip_pager : 1;
        uint motif_noborder : 1;
        uint motif_may_resize : 1;
        uint motif_may_move :1;
        uint motif_may_close : 1;
        uint keep_below : 1; // NET::KeepBelow
        uint minimized : 1;
        uint hidden : 1; // forcibly hidden by calling hide()
        uint modal : 1; // NET::Modal
        uint noborder : 1;
        uint user_noborder : 1;
        uint urgency : 1; // XWMHints, UrgencyHint
        uint ignore_focus_stealing : 1; // don't apply focus stealing prevention to this client
        uint demands_attention : 1;
        WindowRules client_rules;
        void getWMHints();
        void readIcons();
        void getWindowProtocols();
        TQPixmap icon_pix;
        TQPixmap miniicon_pix;
        TQCursor cursor;
    // FullScreenHack - non-NETWM fullscreen (noborder,size of desktop)
    // DON'T reorder - saved to config files !!!
        enum FullScreenMode { FullScreenNone, FullScreenNormal, FullScreenHack };
        FullScreenMode fullscreen_mode;
        MaximizeMode max_mode;
        TQRect geom_restore;
        TQRect geom_fs_restore;
        MaximizeMode maxmode_restore;
        int workarea_diff_x, workarea_diff_y;
        WinInfo* info;
        TQTimer* autoRaiseTimer;
        TQTimer* shadeHoverTimer;
        Colormap cmap;
        TQCString resource_name;
        TQCString resource_class;
        TQCString client_machine;
        TQString cap_normal, cap_iconic, cap_suffix;
        WId wmClientLeaderWin;
        TQCString window_role;
        Group* in_group;
        Window window_group;
        Layer in_layer;
        TQTimer* ping_timer;
        KProcess* process_killer;
        Time ping_timestamp;
        Time user_time;
        unsigned long allowed_actions;
        TQRect frame_geometry;
        TQSize client_size;
        int postpone_geometry_updates; // >0 - new geometry is remembered, but not actually set
        bool pending_geometry_update;
        bool shade_geometry_change;
        int border_left, border_right, border_top, border_bottom;

        Client* shadowAfterClient;
        TQWidget* shadowWidget;
        TQMemArray<double> activeOpacityCache;
        TQMemArray<double> inactiveOpacityCache;
        TQMemArray<double>* opacityCache;
        TQRegion shapeBoundingRegion;
        TQTimer* shadowDelayTimer;
        bool shadowMe;

        TQRegion _mask;
        static bool check_active_modal; // see Client::checkActiveModal()
        KShortcut _shortcut;
        friend struct FetchNameInternalPredicate;
        friend struct CheckIgnoreFocusStealingProcedure;
        friend struct ResetupRulesProcedure;
        friend class GeometryUpdatesPostponer;
        void show() { assert( false ); } // SELI remove after Client is no longer TQWidget
        void hide() { assert( false ); }
        uint opacity_;
        uint savedOpacity_;
        bool custom_opacity;
        uint rule_opacity_active; //translucency rules
        uint rule_opacity_inactive; //dto.
        //int shadeOriginalHeight;
        bool isBMP_;
        TQTimer* demandAttentionKNotifyTimer;

        friend bool performTransiencyCheck();
    };

// helper for Client::postponeGeometryUpdates() being called in pairs (true/false)
class GeometryUpdatesPostponer
    {
    public:
        GeometryUpdatesPostponer( Client* c )
            : cl( c ) { cl->postponeGeometryUpdates( true ); }
        ~GeometryUpdatesPostponer()
            { cl->postponeGeometryUpdates( false ); }
    private:
        Client* cl;
    };


// NET WM Protocol handler class
class WinInfo : public NETWinInfo
    {
    private:
        typedef KWinInternal::Client Client; // because of NET::Client
    public:
        WinInfo( Client* c, Display * display, Window window,
                Window rwin, const unsigned long pr[], int pr_size );
        virtual void changeDesktop(int desktop);
        virtual void changeState( unsigned long state, unsigned long mask );
    private:
        Client * m_client;
    };

inline Window Client::window() const
    {
    return client;
    }

inline Window Client::frameId() const
    {
    return frame;
    }

inline Window Client::wrapperId() const
    {
    return wrapper;
    }

inline Window Client::decorationId() const
    {
    return decoration != NULL ? decoration->widget()->winId() : None;
    }

inline Workspace* Client::workspace() const
    {
    return wspace;
    }

inline const Client* Client::transientFor() const
    {
    return transient_for;
    }

inline Client* Client::transientFor()
    {
    return transient_for;
    }

inline bool Client::groupTransient() const
    {
    return transient_for_id == workspace()->rootWin();
    }

// needed because verifyTransientFor() may set transient_for_id to root window,
// if the original value has a problem (window doesn't exist, etc.)
inline bool Client::wasOriginallyGroupTransient() const
    {
    return original_transient_for_id == workspace()->rootWin();
    }

inline bool Client::isTransient() const
    {
    return transient_for_id != None;
    }

inline const ClientList& Client::transients() const
    {
    return transients_list;
    }

inline const Group* Client::group() const
    {
    return in_group;
    }

inline Group* Client::group()
    {
    return in_group;
    }

inline int Client::mappingState() const
    {
    return mapping_state;
    }

inline TQCString Client::resourceName() const
    {
    return resource_name; // it is always lowercase
    }

inline TQCString Client::resourceClass() const
    {
    return resource_class; // it is always lowercase
    }

inline
bool Client::isMinimized() const
    {
    return minimized;
    }

inline bool Client::isActive() const
    {
    return active;
    }

/*!
  Returns the virtual desktop within the workspace() the client window
  is located in, 0 if it isn't located on any special desktop (not mapped yet),
  or NET::OnAllDesktops. Do not use desktop() directly, use
  isOnDesktop() instead.
 */
inline int Client::desktop() const
    {
    return desk;
    }

inline bool Client::isOnAllDesktops() const
    {
    return desk == NET::OnAllDesktops;
    }
/*!
  Returns whether the client is on the virtual desktop \a d.
  This is always TRUE for onAllDesktops clients.
 */
inline bool Client::isOnDesktop( int d ) const
    {
    return desk == d || /*desk == 0 ||*/ isOnAllDesktops();
    }

inline
bool Client::isShown( bool shaded_is_shown ) const
    {
    return !isMinimized() && ( !isShade() || shaded_is_shown ) && !hidden;
    }

inline
bool Client::isShade() const
    {
    return shade_mode == ShadeNormal;
    }

inline
ShadeMode Client::shadeMode() const
    {
    return shade_mode;
    }

inline TQPixmap Client::icon() const
    {
    return icon_pix;
    }

inline TQPixmap Client::miniIcon() const
    {
    return miniicon_pix;
    }

inline TQRect Client::geometryRestore() const
    {
    return geom_restore;
    }

inline Client::MaximizeMode Client::maximizeModeRestore() const
    {
    return maxmode_restore;
    }

inline Client::MaximizeMode Client::maximizeMode() const
    {
    return max_mode;
    }

inline bool Client::skipTaskbar( bool from_outside ) const
    {
    return from_outside ? original_skip_taskbar : skip_taskbar;
    }

inline bool Client::skipPager() const
    {
    return skip_pager;
    }

inline bool Client::keepBelow() const
    {
    return keep_below;
    }

inline bool Client::shape() const
    {
    return is_shape;
    }


inline bool Client::isFullScreen() const
    {
    return fullscreen_mode != FullScreenNone;
    }

inline bool Client::isModal() const
    {
    return modal;
    }

inline bool Client::hasNETSupport() const
    {
    return info->hasNETSupport();
    }

inline Colormap Client::colormap() const
    {
    return cmap;
    }

inline pid_t Client::pid() const
    {
    return info->pid();
    }

inline void Client::invalidateLayer()
    {
    in_layer = UnknownLayer;
    }

inline bool Client::isIconicState() const
    {
    return mapping_state == IconicState;
    }

inline bool Client::isNormalState() const
    {
    return mapping_state == NormalState;
    }

inline bool Client::isManaged() const
    {
    return mapping_state != WithdrawnState;
    }

inline TQCString Client::windowRole() const
    {
    return window_role;
    }

inline TQRect Client::geometry() const
    {
    return frame_geometry;
    }

inline TQSize Client::size() const
    {
    return frame_geometry.size();
    }

inline TQPoint Client::pos() const
    {
    return frame_geometry.topLeft();
    }

inline int Client::x() const
    {
    return frame_geometry.x();
    }

inline int Client::y() const
    {
    return frame_geometry.y();
    }

inline int Client::width() const
    {
    return frame_geometry.width();
    }

inline int Client::height() const
    {
    return frame_geometry.height();
    }

inline TQRect Client::rect() const
    {
    return TQRect( 0, 0, width(), height());
    }

inline TQPoint Client::clientPos() const
    {
    return TQPoint( border_left, border_top );
    }

inline TQSize Client::clientSize() const
    {
    return client_size;
    }

inline void Client::setGeometry( const TQRect& r, ForceGeometry_t force )
    {
    setGeometry( r.x(), r.y(), r.width(), r.height(), force );
    }

inline void Client::move( const TQPoint & p, ForceGeometry_t force )
    {
    move( p.x(), p.y(), force );
    }

inline void Client::plainResize( const TQSize& s, ForceGeometry_t force )
    {
    plainResize( s.width(), s.height(), force );
    }

inline bool Client::isShadowed() const
    {
    return shadowMe;
    }

inline Window Client::shadowId() const
    {
    return shadowWidget != NULL ? shadowWidget->winId() : None;
    }

inline void Client::resizeWithChecks( const TQSize& s, ForceGeometry_t force )
    {
    resizeWithChecks( s.width(), s.height(), force );
    }

inline bool Client::hasUserTimeSupport() const
    {
    return info->userTime() != -1U;
    }
    
inline bool Client::ignoreFocusStealing() const
    {
    return ignore_focus_stealing;
    }

inline const WindowRules* Client::rules() const
    {
    return &client_rules;
    }

KWIN_PROCEDURE( CheckIgnoreFocusStealingProcedure, cl->ignore_focus_stealing = options->checkIgnoreFocusStealing( cl ));

inline Window Client::moveResizeGrabWindow() const
    {
    return move_resize_grab_window;
    }

inline KShortcut Client::shortcut() const
    {
    return _shortcut;
    }

inline bool Client::isBMP()
    {
    return isBMP_;
    }

inline void Client::setBMP(bool b)
    {
    isBMP_ = b;
    }

inline void Client::removeRule( Rules* rule )
    {
    client_rules.remove( rule );
    }

#ifdef NDEBUG
inline
kndbgstream& operator<<( kndbgstream& stream, const Client* ) { return stream; }
inline
kndbgstream& operator<<( kndbgstream& stream, const ClientList& ) { return stream; }
inline
kndbgstream& operator<<( kndbgstream& stream, const ConstClientList& ) { return stream; }
#else
kdbgstream& operator<<( kdbgstream& stream, const Client* );
kdbgstream& operator<<( kdbgstream& stream, const ClientList& );
kdbgstream& operator<<( kdbgstream& stream, const ConstClientList& );
#endif

KWIN_COMPARE_PREDICATE( WindowMatchPredicate, Window, cl->window() == value );
KWIN_COMPARE_PREDICATE( FrameIdMatchPredicate, Window, cl->frameId() == value );
KWIN_COMPARE_PREDICATE( WrapperIdMatchPredicate, Window, cl->wrapperId() == value );

} // namespace

#endif
