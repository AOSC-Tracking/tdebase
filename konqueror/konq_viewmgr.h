/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __konq_viewmgr_h__
#define __konq_viewmgr_h__

#include "konq_factory.h"

#include <tqnamespace.h>
#include <tqobject.h>
#include <tqmap.h>
#include <tqguardedptr.h>

#include <ktrader.h>

#include <tdeparts/partmanager.h>
#include "konq_openurlrequest.h"

class TQString;
class TQStringList;
class TQTimer;
class TDEConfig;
class KonqMainWindow;
class KonqFrameBase;
class KonqFrameContainer;
class KonqFrameContainerBase;
class KonqFrameTabs;
class KonqView;
class BrowserView;
class TDEActionMenu;

namespace KParts
{
  class ReadOnlyPart;
}

class KonqViewManager : public KParts::PartManager
{
  TQ_OBJECT
public:
  KonqViewManager( KonqMainWindow *mainWindow );
  ~KonqViewManager();

  KonqView* Initialize( const TQString &serviceType, const TQString &serviceName );

  /**
   * Splits the view, depending on orientation, either horizontally or
   * vertically. The first of the resulting views will contain the initial
   * view, the other will be a new one, constructed from the given
   * Service Type.
   * If no Service Type was provided it takes the one from the current view.
   * Returns the newly created view or 0L if the view couldn't be created.
   *
   * @param newOneFirst if true, move the new view as the first one (left or top)
   */
  KonqView* splitView( TQt::Orientation orientation,
                       const TQString & serviceType = TQString::null,
                       const TQString & serviceName = TQString::null,
                       bool newOneFirst = false, bool forceAutoEmbed = false );

  /**
   * Does basically the same as splitView() but inserts the new view at the top
   * of the view tree.
   * Returns the newly created view or 0L if the view couldn't be created.
   *
   * @param newOneFirst if true, move the new view as the first one (left or top)
   */
  KonqView* splitWindow( TQt::Orientation orientation,
                         const TQString & serviceType = TQString::null,
                         const TQString & serviceName = TQString::null,
                         bool newOneFirst = false);

  /**
   * Converts a Container or View docContainer into a Tabs
   */
  void convertDocContainer();


  /**
   * Adds a tab to m_pMainContainer
   */
  KonqView* addTab(const TQString &serviceType = TQString::null,
                   const TQString &serviceName = TQString::null,
                   bool passiveMode = false, bool openAfterCurrentPage = false );



  /**
   * Duplicates the specified tab, or else the current one if none is specified
   */
  void duplicateTab( KonqFrameBase* tab = 0L, bool openAfterCurrentPage = false );

  /**
   * creates a new tab from a history entry  
   * used for MMB on back/forward
   */
  KonqView* addTabFromHistory( int steps, bool openAfterCurrentPage );

  /**
   * Break the current tab off into a new window,
   * if none is specified, the current one is used
   */
  void breakOffTab( KonqFrameBase* tab = 0L );

  /**
   * Guess!:-)
   * Also takes care of setting another view as active if @p view was the active view
   */
  void removeView( KonqView *view );

  /**
   * Removes specified tab, if none is specified it remvoe the current tab
   * Also takes care of setting another view as active if the active view was in this tab
   */
  void removeTab( KonqFrameBase* tab = 0L );

  /**
   * Removes all, but the specified tab. If no tab is specified every tab, but the current will be removed
   * Also takes care of setting the specified tab as active if the active view was not in this tab
   */
  void removeOtherTabs( KonqFrameBase* tab = 0L );

  /**
   * Locates and activates the next tab
   *
   */
  void activateNextTab();

  /**
   * Locates and activates the previous tab
   *
   */
  void activatePrevTab();

  /**
   * Activate given tab
   *
   */
  void activateTab(int position);

    void moveTabBackward();
    void moveTabForward();

    void reloadAllTabs();

  /**
   * Brings the tab specified by @p view to the front of the stack
   *
   */
  void showTab( KonqView *view );

  /**
   * Updates favicon pixmaps used in tabs
   *
   */
  void updatePixmaps();

  /**
   * Saves the current view layout to a config file.
   * Remove config file before saving, especially if saveURLs is false.
   * @param cfg the config file
   * @param saveURLs whether to save the URLs in the profile
   * @param saveWindowSize whether to save the size of the window in the profile
   */
  void saveViewProfile( TDEConfig & cfg, bool saveURLs, bool saveWindowSize );

  /**
   * Saves the current view layout to a config file.
   * Remove config file before saving, especially if saveURLs is false.
   * @param fileName the name of the config file
   * @param profileName the name of the profile
   * @param saveURLs whether to save the URLs in the profile
   * @param saveWindowSize whether to save the size of the window in the profile
   */
  void saveViewProfile( const TQString & fileName, const TQString & profileName,
                        bool saveURLs, bool saveWindowSize );

  /**
   * Loads a view layout from a config file. Removes all views before loading.
   * @param cfg the config file
   * @param filename if set, remember the file name of the profile (for save settings)
   * It has to be under the profiles dir. Otherwise, set to TQString::null
   * @param forcedURL if set, the URL to open, whatever the profile says
   * @param req attributes related to @p forcedURL
   * @param resetWindow if the profile doesn't have attributes like size or toolbar
   * settings, they will be reset to the defaults
   */
  void loadViewProfile( TDEConfig &cfg, const TQString & filename,
                        const KURL & forcedURL = KURL(),
                        const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                        bool resetWindow = false, bool openURL = true );

  /**
   * Loads a view layout from a config file. Removes all views before loading.
   * @param path the full path to the config file
   * @param filename if set, remember the file name of the profile (for save settings)
   * It has to be under the profiles dir. Otherwise, set to TQString::null
   * @param forcedURL if set, the URL to open, whatever the profile says
   * @param req attributes related to @p forcedURL
   * @param resetWindow if the profile doesn't have attributes like size or toolbar
   * settings, they will be reset to the defaults
   */
  void loadViewProfile( const TQString & path, const TQString & filename,
                        const KURL & forcedURL = KURL(),
                        const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                        bool resetWindow = false, bool openURL = true );
  /**
   * Return the filename of the last profile that was loaded
   * by the view manager. For "save settings".
   */
  TQString currentProfile() const { return m_currentProfile; }
  /**
   * Return the name (i18n'ed) of the last profile that was loaded
   * by the view manager. For "save settings".
   */
  TQString currentProfileText() const { return m_currentProfileText; }

  /**
   * Whether we are currently loading a profile
   */
  bool isLoadingProfile() const { return m_bLoadingProfile; }

  void clear();

  KonqView *chooseNextView( KonqView *view );

  /**
   * Called whenever
   * - the total number of views changed
   * - the number of views in passive mode changed
   * The implementation takes care of showing or hiding the statusbar indicators
   */
  void viewCountChanged();

  void setProfiles( TDEActionMenu *profiles );

  void profileListDirty( bool broadcast = true );

  KonqFrameBase *docContainer() const { return m_pDocContainer; }
  void setDocContainer( KonqFrameBase* docContainer ) { m_pDocContainer = docContainer; }

  KonqMainWindow *mainWindow() const { return m_pMainWindow; }

  /**
   * Reimplemented from PartManager
   */
  virtual void removePart( KParts::Part * part );

  /**
   * Reimplemented from PartManager
   */
  virtual void setActivePart( KParts::Part *part, TQWidget *widget = 0L );

  void setActivePart( KParts::Part *part, bool immediate );

  void showProfileDlg( const TQString & preselectProfile );

  /**
   *   The widget is the one which you are referring to.
   */
  static TQSize readConfigSize( TDEConfig &cfg, TQWidget *widget = NULL);

#ifndef NDEBUG
  void printFullHierarchy( KonqFrameContainerBase * container );
#endif

  void setLoading( KonqView *view, bool loading );
  
  void showHTML(bool b);

  TQString profileHomeURL() const { return m_profileHomeURL; }

  //Update options
  void reparseConfiguration();

protected slots:
  void emitActivePartChanged();

  void slotProfileDlg();

  void slotProfileActivated( int id );

  void slotProfileListAboutToShow();

  void slotPassiveModePartDeleted();

  void slotActivePartChanged ( KParts::Part *newPart );

protected:

  /**
   * Load the config entries for a view.
   * @param cfg the config file
   * ...
   * @param defaultURL the URL to use if the profile doesn't contain urls
   * @param openURL whether to open urls at all (from the profile or using @p defaultURL).
   *  (this is set to false when we have a forcedURL to open)
   */
  void loadItem( TDEConfig &cfg, KonqFrameContainerBase *parent,
                 const TQString &name, const KURL & defaultURL, bool openURL, bool openAfterCurrentPage = false );

  // Disabled - we do it ourselves
  virtual void setActiveInstance( TDEInstance * ) {}

private:

  /**
   * Creates a new View based on the given ServiceType. If serviceType is empty
   * it clones the current view.
   * Returns the newly created view.
   */
  KonqViewFactory createView( const TQString &serviceType,
                              const TQString &serviceName,
                              KService::Ptr &service,
                              TDETrader::OfferList &partServiceOffers,
                              TDETrader::OfferList &appServiceOffers,
			      bool forceAutoEmbed = false );

  /**
   * Mainly creates the backend structure(KonqView) for a view and
   * connects it
   */
  KonqView *setupView( KonqFrameContainerBase *parentContainer,
                       KonqViewFactory &viewFactory,
                       const KService::Ptr &service,
                       const TDETrader::OfferList &partServiceOffers,
                       const TDETrader::OfferList &appServiceOffers,
                       const TQString &serviceType,
                       bool passiveMode, bool openAfterCurrentPage = false);

#ifndef NDEBUG
  //just for debugging
  void printSizeInfo( KonqFrameBase* frame,
                      KonqFrameContainerBase* parent,
                      const char* msg );
#endif

  KonqMainWindow *m_pMainWindow;

  KonqFrameBase *m_pDocContainer;

  TQGuardedPtr<TDEActionMenu> m_pamProfiles;
  bool m_bProfileListDirty;
  bool m_bLoadingProfile;
  TQString m_currentProfile;
  TQString m_currentProfileText;
  TQString m_profileHomeURL;

  TQMap<TQString, TQString> m_mapProfileNames;

  TQTimer *m_activePartChangedTimer;
};

#endif
