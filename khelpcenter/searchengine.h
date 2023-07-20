#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <tqobject.h>
#include <tqptrlist.h>
#include <tqstring.h>

#include <kpixmap.h>
#include <tdeio/job.h>

#include "docentrytraverser.h"

class TQWidget;
class TDEProcess;
class TDEConfig;
class TDEHTMLPart;

namespace KHC {

class Formatter;
class SearchEngine;
class View;
class SearchHandler;

class SearchTraverser : public TQObject, public DocEntryTraverser
{
    TQ_OBJECT
  public:
    SearchTraverser( SearchEngine *engine, int level );
    ~SearchTraverser();

    void process( DocEntry * );
    
    void startProcess( DocEntry * );

    DocEntryTraverser *createChild( DocEntry * );

    DocEntryTraverser *parentTraverser();

    void deleteTraverser();

    void finishTraversal();

  protected:
    void connectHandler( SearchHandler *handler );
    void disconnectHandler( SearchHandler *handler );

  protected slots:
    void showSearchResult( SearchHandler *, DocEntry *, const TQString &result );
    void showSearchError( SearchHandler *, DocEntry *, const TQString &error );

  private:
    const int mMaxLevel;
  
    SearchEngine *mEngine;
    int mLevel;

    DocEntry *mEntry;
    TQString mJobData;
    
    TQString mResult;
    
    TQMap<SearchHandler *, int> mConnectCount;
};


class SearchEngine : public TQObject
{
    TQ_OBJECT
  public:
    enum Operation { And, Or };

    SearchEngine( View * );
    ~SearchEngine();

    bool initSearchHandlers();

    bool search( TQString words, TQString method = "and", int matches = 5,
                 TQString scope = "" );

    Formatter *formatter() const; 
    View *view() const;

    TQString substituteSearchQuery( const TQString &query );

    static TQString substituteSearchQuery( const TQString &query,
      const TQString &identifier, const TQStringList &words, int maxResults,
      Operation operation, const TQString &lang );

    void finishSearch();

    /**
      Append error message to error log.
    */
    void logError( DocEntry *entry, const TQString &msg );

    /**
      Return error log.
    */
    TQString errorLog() const;

    bool isRunning() const;

    SearchHandler *handler( const TQString &documentType ) const;

    TQStringList words() const;
    int maxResults() const;
    Operation operation() const;

    bool canSearch( DocEntry * );
    bool needsIndex( DocEntry * );

  signals:
    void searchFinished();

  protected slots:
    void searchStdout(TDEProcess *proc, char *buffer, int buflen);
    void searchStderr(TDEProcess *proc, char *buffer, int buflen);
    void searchExited(TDEProcess *proc);

  protected:
    void processSearchQueue();
    
  private:
    TDEProcess *mProc;
    bool mSearchRunning;
    TQString mSearchResult;

    TQString mStderr;

    View *mView;
    
    TQString mWords;
    int mMatches;
    TQString mMethod;
    TQString mLang;
    TQString mScope;

    TQStringList mWordList;
    int mMaxResults;
    Operation mOperation;
    
    DocEntryTraverser *mRootTraverser;

    TQMap<TQString, SearchHandler *> mHandlers;
};

}

#endif
