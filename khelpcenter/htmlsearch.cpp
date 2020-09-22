#include <kdebug.h>
#include <tdeconfig.h>

#include "docentry.h"

#include "htmlsearch.h"
#include "htmlsearch.moc"

using namespace KHC;

HTMLSearch::HTMLSearch()
{
  mConfig = new TDEConfig("khelpcenterrc", true);
  mConfig->setGroup( "htdig" );
}

HTMLSearch::~HTMLSearch()
{
    delete mConfig;
}

void HTMLSearch::setupDocEntry( KHC::DocEntry *entry )
{
//  kdDebug() << "HTMLSearch::setupDocEntry(): " << entry->name() << endl;

  if ( entry->searchMethod().lower() != "htdig" ) return;

  if ( entry->search().isEmpty() )
    entry->setSearch( defaultSearch( entry ) );
  if ( entry->indexer().isEmpty() )
    entry->setIndexer( defaultIndexer( entry ) );
  if ( entry->indexTestFile().isEmpty() )
    entry->setIndexTestFile( defaultIndexTestFile( entry ) );

//  entry->dump();
}

TQString HTMLSearch::defaultSearch( KHC::DocEntry *entry )
{
  TQString htsearch = "cgi:";
  htsearch += mConfig->readPathEntry( "htsearch" );
  htsearch += "?words=%k&method=and&format=-desc&config=";
  htsearch += entry->identifier();

  return htsearch;
}

TQString HTMLSearch::defaultIndexer( KHC::DocEntry * )
{
  TQString indexer = mConfig->readPathEntry( "indexer" );
  indexer += " --indexdir=%i %f";

  return indexer;
}

TQString HTMLSearch::defaultIndexTestFile( KHC::DocEntry *entry )
{
  return entry->identifier() + ".exists";
}
