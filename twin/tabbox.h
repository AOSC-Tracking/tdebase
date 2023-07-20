/*****************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 1999, 2000 Matthias Ettrich <ettrich@kde.org>
Copyright (C) 2003 Lubos Lunak <l.lunak@kde.org>

You can Freely distribute this program under the GNU General Public
License. See the file "COPYING" for the exact licensing terms.
******************************************************************/

#ifndef KWIN_TABBOX_H
#define KWIN_TABBOX_H

#include <tqframe.h>
#include <tqtimer.h>
#include <tqvaluelist.h>
#include "utils.h"

class TQLabel;

namespace KWinInternal
{

class Workspace;
class Client;

class TabBox : public TQFrame
    {
    TQ_OBJECT
    public:
        TabBox( Workspace *ws, const char *name=0 );
        ~TabBox();

        Client* currentClient();
        void setCurrentClient( Client* c );
        int currentDesktop();
        
    // DesktopMode and WindowsMode are based on the order in which the desktop
    //  or window were viewed.
    // DesktopListMode lists them in the order created.
        enum Mode { DesktopMode, DesktopListMode, WindowsMode };
        void setMode( Mode mode );
        void setAppsOnly( bool a );
        Mode mode() const;
        bool isAppsOnly() const;

        void reset();
        void nextPrev( bool next = TRUE);

        void delayedShow();
        void hide();

        void handleMouseEvent( XEvent* );

        Workspace* workspace() const;

        void reconfigure();

    protected:
        void showEvent( TQShowEvent* );
        void hideEvent( TQHideEvent* );
        void drawContents( TQPainter * );

    private:
        void createClientList(ClientList &list, int desktop /*-1 = all*/, Client *start, bool chain);
        void updateOutline();

    private:
        Client* current_client;
        Mode m;
        Workspace* wspace;
        ClientList clients;
        int desk;
        int lineHeight;
        bool showMiniIcon;
        bool appsOnly;
        TQTimer delayedShowTimer;
        TQString no_tasks;
        bool options_traverse_all;
        Window outline_left, outline_right, outline_top, outline_bottom;
    };


/*!
  Returns the tab box' workspace
 */
inline Workspace* TabBox::workspace() const
    {
    return wspace;
    }

/*!
  Returns the current mode, either DesktopListMode or WindowsMode

  \sa setMode()
 */
inline TabBox::Mode TabBox::mode() const
    {
    return m;
    }

/*! 
  Returns the appsOnly variable

  \sa setAppsOnly()
  */
inline bool TabBox::isAppsOnly() const
    {
    return appsOnly;
    }


} // namespace

#endif
