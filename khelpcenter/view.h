#ifndef __view_h__
#define __view_h__

#include <tdehtml_part.h>

#include "glossary.h"
#include "navigator.h"

class TDEActionCollection;

namespace DOM {
  class Node;
}

namespace KHC {

class Formatter;

class View : public TDEHTMLPart
{
    TQ_OBJECT
  public:
    View( TQWidget *parentWidget, const char *widgetName,
          TQObject *parent, const char *name, TDEHTMLPart::GUIProfile prof,
          TDEActionCollection *col );

    ~View();

    virtual bool openURL( const KURL &url );

    virtual void saveState( TQDataStream &stream );
    virtual void restoreState( TQDataStream &stream );

    enum State { Docu, About, Search };

    int state() const { return mState; }
    TQString title() const { return mTitle; }

    static TQString langLookup( const TQString &fname );

    void beginSearchResult();
    void writeSearchResult( const TQString & );
    void endSearchResult();

    void beginInternal( const KURL & );
    KURL internalUrl() const;

    int zoomStepping() const { return m_zoomStepping; }

    Formatter *formatter() const { return mFormatter; }

    void copySelectedText();

  public slots:
    void lastSearch();
    void slotIncFontSizes();
    void slotDecFontSizes();
    void slotReload( const KURL &url = KURL() );
    void slotCopyLink();
    bool nextPage(bool checkOnly = false);
    bool prevPage(bool checkOnly = false);

  signals:
    void searchResultCacheAvailable();

  protected:
    bool eventFilter( TQObject *o, TQEvent *e );

  private slots:
    void setTitle( const TQString &title );
    void showMenu( const TQString& url, const TQPoint& pos);

  private:
    void showAboutPage();
    KURL urlFromLinkNode( const DOM::Node &n ) const;
 
    int mState;
    TQString mTitle;

    TQString mSearchResult;
    KURL mInternalUrl;

    int m_zoomStepping;

    Formatter *mFormatter;
    TDEActionCollection *mActionCollection;
    TQString mCopyURL;
};

}

#endif
