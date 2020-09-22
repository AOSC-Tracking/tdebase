#include "stdlib.h"

#include <tqstylesheet.h>
#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <tdelocale.h>
#include <tdemessagebox.h>

#include "docmetainfo.h"
#include "formatter.h"
#include "view.h"
#include "searchhandler.h"
#include "prefs.h"

#include "searchengine.h"

namespace KHC
{

SearchTraverser::SearchTraverser( SearchEngine *engine, int level ) :
  mMaxLevel( 999 ), mEngine( engine), mLevel( level )
{
#if 0
  kdDebug() << "SearchTraverser(): " << mLevel
    << "  0x" << TQString::number( int( this ), 16 ) << endl;
#endif
}

SearchTraverser::~SearchTraverser()
{
#if 0
    kdDebug() << "~SearchTraverser(): " << mLevel
      << "  0x" << TQString::number( int( this ), 16 ) << endl;
#endif

  TQString section;
  if ( parentEntry() ) {
    section = parentEntry()->name();
  } else {
    section = ("Unknown Section");
  }

  if ( !mResult.isEmpty() ) {
    mEngine->view()->writeSearchResult(
      mEngine->formatter()->sectionHeader( section ) );
    mEngine->view()->writeSearchResult( mResult );
  }
}

void SearchTraverser::process( DocEntry * )
{
  kdDebug() << "SearchTraverser::process()" << endl;
}

void SearchTraverser::startProcess( DocEntry *entry )
{
//  kdDebug() << "SearchTraverser::startProcess(): " << entry->name() << "  "
//    << "SEARCH: '" << entry->search() << "'" << endl;

  if ( !mEngine->canSearch( entry ) || !entry->searchEnabled() ) {
    mNotifyee->endProcess( entry, this );
    return;
  }

//  kdDebug() << "SearchTraverser::startProcess(): " << entry->identifier()
//    << endl;

  SearchHandler *handler = mEngine->handler( entry->documentType() );

  if ( !handler ) {
    TQString txt;
    if ( entry->documentType().isEmpty() ) {
      txt = i18n("Error: No document type specified.");
    } else {
      txt = i18n("Error: No search handler for document type '%1'.")
        .arg( entry->documentType() );
    }
    showSearchError( handler, entry, txt );
    return;
  }

  connectHandler( handler );

  handler->search( entry, mEngine->words(), mEngine->maxResults(),
    mEngine->operation() );

//  kdDebug() << "SearchTraverser::startProcess() done: " << entry->name() << endl;
}

void SearchTraverser::connectHandler( SearchHandler *handler )
{
  TQMap<SearchHandler *,int>::Iterator it;
  it = mConnectCount.find( handler );
  int count = 0;
  if ( it != mConnectCount.end() ) count = *it;
  if ( count == 0 ) {
    connect( handler, TQT_SIGNAL( searchError( SearchHandler *, DocEntry *, const TQString & ) ),
      TQT_SLOT( showSearchError( SearchHandler *, DocEntry *, const TQString & ) ) );
    connect( handler, TQT_SIGNAL( searchFinished( SearchHandler *, DocEntry *, const TQString & ) ),
      TQT_SLOT( showSearchResult( SearchHandler *, DocEntry *, const TQString & ) ) );
  }
  mConnectCount[ handler ] = ++count;
}

void SearchTraverser::disconnectHandler( SearchHandler *handler )
{
  TQMap<SearchHandler *,int>::Iterator it;
  it = mConnectCount.find( handler );
  if ( it == mConnectCount.end() ) {
    kdError() << "SearchTraverser::disconnectHandler() handler not connected."
      << endl;
  } else {
    int count = *it;
    --count;
    if ( count == 0 ) {
      disconnect( handler, TQT_SIGNAL( searchError( SearchHandler *, DocEntry *, const TQString & ) ),
        this, TQT_SLOT( showSearchError( SearchHandler *, DocEntry *, const TQString & ) ) );
      disconnect( handler, TQT_SIGNAL( searchFinished( SearchHandler *, DocEntry *, const TQString & ) ),
        this, TQT_SLOT( showSearchResult( SearchHandler *, DocEntry *, const TQString & ) ) );
    }
    mConnectCount[ handler ] = count;
  }
}

DocEntryTraverser *SearchTraverser::createChild( DocEntry *parentEntry )
{
//  kdDebug() << "SearchTraverser::createChild() level " << mLevel << endl;

  if ( mLevel >= mMaxLevel ) {
    ++mLevel;
    return this;
  } else {
    DocEntryTraverser *t = new SearchTraverser( mEngine, mLevel + 1 );
    t->setParentEntry( parentEntry );
    return t;
  }
}

DocEntryTraverser *SearchTraverser::parentTraverser()
{
//  kdDebug() << "SearchTraverser::parentTraverser(): level: " << mLevel << endl;

  if ( mLevel > mMaxLevel ) {
    return this;
  } else {
    return mParent;
  }
}

void SearchTraverser::deleteTraverser()
{
//  kdDebug() << "SearchTraverser::deleteTraverser()" << endl;

  if ( mLevel > mMaxLevel ) {
    --mLevel;
  } else {
    delete this;
  }
}

void SearchTraverser::showSearchError( SearchHandler *handler, DocEntry *entry, const TQString &error )
{
//  kdDebug() << "SearchTraverser::showSearchError(): " << entry->name()
//    << endl;

  mResult += mEngine->formatter()->docTitle( entry->name() );
  mResult += mEngine->formatter()->paragraph( error );

  mEngine->logError( entry, error );

  disconnectHandler( handler );

  mNotifyee->endProcess( entry, this );
}

void SearchTraverser::showSearchResult( SearchHandler *handler, DocEntry *entry, const TQString &result )
{
//  kdDebug() << "SearchTraverser::showSearchResult(): " << entry->name()
//    << endl;

  mResult += mEngine->formatter()->docTitle( entry->name() );
  mResult += mEngine->formatter()->processResult( result );

  disconnectHandler( handler );

  mNotifyee->endProcess( entry, this );
}

void SearchTraverser::finishTraversal()
{
//  kdDebug() << "SearchTraverser::finishTraversal()" << endl;

  mEngine->view()->writeSearchResult( mEngine->formatter()->footer() );
  mEngine->view()->endSearchResult();

  mEngine->finishSearch();
}


SearchEngine::SearchEngine( View *destination )
  : TQObject(),
    mProc( 0 ), mSearchRunning( false ), mView( destination ),
    mRootTraverser( 0 )
{
  mLang = TDEGlobal::locale()->language().left( 2 );
}

SearchEngine::~SearchEngine()
{
  delete mRootTraverser;
}

bool SearchEngine::initSearchHandlers()
{
  TQStringList resources = TDEGlobal::dirs()->findAllResources(
    "appdata", "searchhandlers/*.desktop" );
  TQStringList::ConstIterator it;
  for( it = resources.begin(); it != resources.end(); ++it ) {
    TQString filename = *it;
    kdDebug() << "SearchEngine::initSearchHandlers(): " << filename << endl;
    SearchHandler *handler = SearchHandler::initFromFile( filename );
    if ( !handler || !handler->checkPaths() ) {
      TQString txt = i18n("Unable to initialize SearchHandler from file '%1'.")
        .arg( filename );
      kdWarning() << txt << endl;
//      KMessageBox::sorry( mView->widget(), txt );
    } else {
      TQStringList documentTypes = handler->documentTypes();
      TQStringList::ConstIterator it;
      for( it = documentTypes.begin(); it != documentTypes.end(); ++it ) {
        mHandlers.insert( *it, handler );
      }
    }
  }

  if ( mHandlers.isEmpty() ) {
    TQString txt = i18n("No valid search handler found.");
    kdWarning() << txt << endl;
//    KMessageBox::sorry( mView->widget(), txt );
    return false;
  }

  return true;
}

void SearchEngine::searchStdout(TDEProcess *, char *buffer, int len)
{
  if ( !buffer || len == 0 )
    return;

  TQString bufferStr;
  char *p;
  p = (char*) malloc( sizeof(char) * (len+1) );
  p = strncpy( p, buffer, len );
  p[len] = '\0';

  mSearchResult += bufferStr.fromUtf8(p);

  free(p);
}

void SearchEngine::searchStderr(TDEProcess *, char *buffer, int len)
{
  if ( !buffer || len == 0 )
    return;

  mStderr.append( TQString::fromUtf8( buffer, len ) );
}

void SearchEngine::searchExited(TDEProcess *)
{
  kdDebug() << "Search terminated" << endl;
  mSearchRunning = false;
}

bool SearchEngine::search( TQString words, TQString method, int matches,
                           TQString scope )
{
  if ( mSearchRunning ) return false;

  // These should be removed
  mWords = words;
  mMethod = method;
  mMatches = matches;
  mScope = scope;

  // Saner variables to store search parameters:
  mWordList = TQStringList::split( " ", words );
  mMaxResults = matches;
  if ( method == "or" ) mOperation = Or;
  else mOperation = And;

  TDEConfig *cfg = TDEGlobal::config();
  cfg->setGroup( "Search" );
  TQString commonSearchProgram = cfg->readPathEntry( "CommonProgram" );
  bool useCommon = cfg->readBoolEntry( "UseCommonProgram", false );

  if ( commonSearchProgram.isEmpty() || !useCommon ) {
    if ( !mView ) {
      return false;
    }

    TQString txt = i18n("Search Results for '%1':").arg( TQStyleSheet::escape(words) );

    mStderr = "<b>" + txt + "</b>\n";

    mView->beginSearchResult();
    mView->writeSearchResult( formatter()->header( i18n("Search Results") ) );
    mView->writeSearchResult( formatter()->title( txt ) );

    if ( mRootTraverser ) {
      kdDebug() << "SearchEngine::search(): mRootTraverser not null." << endl;
      return false;
    }
    mRootTraverser = new SearchTraverser( this, 0 );
    DocMetaInfo::self()->startTraverseEntries( mRootTraverser );

    return true;
  } else {
    TQString lang = TDEGlobal::locale()->language().left(2);

    if ( lang.lower() == "c" || lang.lower() == "posix" )
	  lang = "en";

    // if the string contains '&' replace with a '+' and set search method to and
    if (mWords.find("&") != -1) {
      mWords.replace("&", " ");
      method = "and";
    }

    // replace whitespace with a '+'
    mWords = mWords.stripWhiteSpace();
    mWords = mWords.simplifyWhiteSpace();
    mWords.replace(TQRegExp("\\s"), "+");

    commonSearchProgram = substituteSearchQuery( commonSearchProgram );

    kdDebug() << "Common Search: " << commonSearchProgram << endl;

    mProc = new TDEProcess();

    TQStringList cmd = TQStringList::split( " ", commonSearchProgram );
    TQStringList::ConstIterator it;
    for( it = cmd.begin(); it != cmd.end(); ++it ) {
      TQString arg = *it;
      if ( arg.left( 1 ) == "\"" && arg.right( 1 ) =="\"" ) {
        arg = arg.mid( 1, arg.length() - 2 );
      }
      *mProc << arg.utf8();
    }

    connect( mProc, TQT_SIGNAL( receivedStdout( TDEProcess *, char *, int ) ),
             TQT_SLOT( searchStdout( TDEProcess *, char *, int ) ) );
    connect( mProc, TQT_SIGNAL( receivedStderr( TDEProcess *, char *, int ) ),
             TQT_SLOT( searchStderr( TDEProcess *, char *, int ) ) );
    connect( mProc, TQT_SIGNAL( processExited( TDEProcess * ) ),
             TQT_SLOT( searchExited( TDEProcess * ) ) );

    mSearchRunning = true;
    mSearchResult = "";
    mStderr = "<b>" + commonSearchProgram + "</b>\n\n";

    mProc->start(TDEProcess::NotifyOnExit, TDEProcess::All);

    while (mSearchRunning && mProc->isRunning())
      kapp->processEvents();

    if ( !mProc->normalExit() || mProc->exitStatus() != 0 ) {
      kdError() << "Unable to run search program '" << commonSearchProgram
                << "'" << endl;
      delete mProc;

      return false;
    }

    delete mProc;

    // modify the search result
    mSearchResult = mSearchResult.replace("http://localhost/", "file:/");
    mSearchResult = mSearchResult.mid( mSearchResult.find( '<' ) );

    mView->beginSearchResult();
    mView->writeSearchResult( mSearchResult );
    mView->endSearchResult();

    emit searchFinished();
  }

  return true;
}

TQString SearchEngine::substituteSearchQuery( const TQString &query )
{
  TQString result = query;
  result.replace( "%k", mWords );
  result.replace( "%n", TQString::number( mMatches ) );
  result.replace( "%m", mMethod );
  result.replace( "%l", mLang );
  result.replace( "%s", mScope );

  return result;
}

TQString SearchEngine::substituteSearchQuery( const TQString &query,
  const TQString &identifier, const TQStringList &words, int maxResults,
  Operation operation, const TQString &lang )
{
  TQString result = query;
  result.replace( "%i", identifier );
  result.replace( "%w", words.join( "+" ) );
  result.replace( "%m", TQString::number( maxResults ) );
  TQString o;
  if ( operation == Or ) o = "or";
  else o = "and";
  result.replace( "%o", o );
  result.replace( "%d", Prefs::indexDirectory() );
  result.replace( "%l", lang );

  return result;
}

Formatter *SearchEngine::formatter() const
{
  return mView->formatter();
}

View *SearchEngine::view() const
{
  return mView;
}

void SearchEngine::finishSearch()
{
  delete mRootTraverser;
  mRootTraverser = 0;

  emit searchFinished();
}

TQString SearchEngine::errorLog() const
{
  return mStderr;
}

void SearchEngine::logError( DocEntry *entry, const TQString &msg )
{
  mStderr += entry->identifier() + ": " + msg;
}

bool SearchEngine::isRunning() const
{
  return mSearchRunning;
}

SearchHandler *SearchEngine::handler( const TQString &documentType ) const
{
  TQMap<TQString,SearchHandler *>::ConstIterator it;
  it = mHandlers.find( documentType );

  if ( it == mHandlers.end() ) return 0;
  else return *it;
}

TQStringList SearchEngine::words() const
{
  return mWordList;
}

int SearchEngine::maxResults() const
{
  return mMaxResults;
}

SearchEngine::Operation SearchEngine::operation() const
{
  return mOperation;
}

bool SearchEngine::canSearch( DocEntry *entry )
{
  return entry->docExists() && !entry->documentType().isEmpty() &&
    handler( entry->documentType() );
}

bool SearchEngine::needsIndex( DocEntry *entry )
{
  if ( !canSearch( entry ) ) return false;

  SearchHandler *h = handler( entry->documentType() );
  if ( h->indexCommand( entry->identifier() ).isEmpty() ) return false;

  return true;
}

}

#include "searchengine.moc"
