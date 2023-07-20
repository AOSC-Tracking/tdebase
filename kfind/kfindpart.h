/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#ifndef kfindpart__h
#define kfindpart__h

#include <tdeparts/browserextension.h>
#include <tdeparts/part.h>
#include <tdefileitem.h>
#include <kdebug.h>
#include <tqptrlist.h>
#include <konq_dirpart.h>

class KQuery;
class TDEAboutData;
//added
class KonqPropsView;
class TDEAction;
class TDEToggleAction;
class TDEActionMenu;
class TQIconViewItem;
class IconViewBrowserExtension;
//end added

class KFindPart : public KonqDirPart//KParts::ReadOnlyPart
{
  friend class KFindPartBrowserExtension;
    TQ_OBJECT
    TQ_PROPERTY( bool showsResult READ showsResult )
public:
    KFindPart( TQWidget * parentWidget, const char *widgetName, 
	       TQObject *parent, const char *name, const TQStringList & /*args*/ );
    virtual ~KFindPart();

    static TDEAboutData *createAboutData();

    virtual bool doOpenURL( const KURL &url );
    virtual bool doCloseURL() { return true; }
    virtual bool openFile() { return false; }

    bool showsResult() const { return m_bShowsResult; }
    
    virtual void saveState( TQDataStream &stream );
    virtual void restoreState( TQDataStream &stream );

  // "Cut" icons : disable those whose URL is in lst, enable the rest //added for konqdirpart
  virtual void disableIcons( const KURL::List & ){};
  virtual const KFileItem * currentItem(){return 0;};

signals:
    // Konqueror connects directly to those signals
    void started(); // started a search
    void clear(); // delete all items
    void newItems(const KFileItemList&); // found this/these item(s)
    void finished(); // finished searching
    void canceled(); // the user canceled the search
    void findClosed(); // close us
    void deleteItem( KFileItem *item);

protected slots:
    void slotStarted();
    void slotDestroyMe();
    void addFile(const KFileItem *item, const TQString& matchingLine);
    /* An item has been removed, so update konqueror's view */
    void removeFile(KFileItem *item);
    void slotResult(int errorCode);
    void newFiles(const KFileItemList&);
  // slots connected to the directory lister  //added for konqdirpart
//  virtual void slotStarted();
  virtual void slotCanceled(){};
  virtual void slotCompleted(){};
  virtual void slotNewItems( const KFileItemList& ){};
  virtual void slotDeleteItem( KFileItem * ){};
  virtual void slotRefreshItems( const KFileItemList& ){};
  virtual void slotClear(){};
  virtual void slotRedirection( const KURL & ){};

private:
    Kfind * m_kfindWidget;
    KQuery *query;
    bool m_bShowsResult; // whether the dirpart shows the results of a search or not
    /**
     * The internal storage of file items
     */
    TQPtrList<KFileItem> m_lstFileItems;
};

#endif
