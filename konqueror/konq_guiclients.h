/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_guiclients_h__
#define __konq_guiclients_h__

#include <kxmlguiclient.h>
#include <tqobject.h>
#include <tqdict.h>
#include <ktrader.h>

class TDEAction;
class TDEActionCollection;
class KonqMainWindow;
class KonqView;

/**
 * This XML-GUI-Client is passed to KonqPopupMenu to add extra actions into it,
 * using the XMLGUI merging. It offers embedding actions and tabbed-browsing actions.
 * Its XML looks like this:
 * @code

 <kpartgui name="konqueror" >
 <Menu name="popupmenu" >
  <menu group="preview" name="preview submenu" >
   <text>Preview In</text>
   <action group="preview" name="0" />
   <action group="preview" name="1" />
  </menu>
  <action group="tabhandling" name="sameview" />
  <action group="tabhandling" name="newview" />
  <action group="tabhandling" name="openintab" />
  <separator group="tabhandling" />
 </Menu>
 </kpartgui>

 * @endcode
 */
class PopupMenuGUIClient : public KXMLGUIClient
{
public:
  PopupMenuGUIClient( KonqMainWindow *mainWindow, const TDETrader::OfferList &embeddingServices,
                      bool isIntoTrash, bool doTabHandling );
  virtual ~PopupMenuGUIClient();

  virtual TDEAction *action( const TQDomElement &element ) const;

private:
  void addEmbeddingService( TQDomElement &menu, int idx, const TQString &name, const KService::Ptr &service );

  KonqMainWindow *m_mainWindow;

  TQDomDocument m_doc;
};

class ToggleViewGUIClient : public TQObject
{
  TQ_OBJECT
public:
  ToggleViewGUIClient( KonqMainWindow *mainWindow );
  virtual ~ToggleViewGUIClient();

  bool empty() const { return m_empty; }

  TQPtrList<TDEAction> actions() const;
  TDEAction *action( const TQString &name ) { return m_actions[ name ]; }

  void saveConfig( bool add, const TQString &serviceName );

private slots:
  void slotToggleView( bool toggle );
  void slotViewAdded( KonqView *view );
  void slotViewRemoved( KonqView *view );
private:
  KonqMainWindow *m_mainWindow;
  TQDict<TDEAction> m_actions;
  bool m_empty;
  TQMap<TQString,bool> m_mapOrientation;
};

#endif
