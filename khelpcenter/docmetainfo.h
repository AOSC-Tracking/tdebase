#ifndef DOCMETAINFO_H
#define DOCMETAINFO_H

#include <tqstring.h>
#include <tqstringlist.h>
#include <tqvaluelist.h>
#include <tqdir.h>
#include <tqmap.h>

#include "docentry.h"

namespace KHC {

class HTMLSearch;
class DocEntryTraverser;

/*!
  This class provides some meta information about help documents.
*/
class DocMetaInfo
{
  public:
    /*!
      Return instance of DocMetaInfo. There can only be one instance at a time.
    */
    static DocMetaInfo *self();

    ~DocMetaInfo();

    void scanMetaInfo( bool force = false );

    DocEntry *addDocEntry( const TQString &fileName );

    void addDocEntry( DocEntry * );

    DocEntry::List docEntries();
    
    DocEntry::List searchEntries();

    void traverseEntries( DocEntryTraverser * );

    void startTraverseEntries( DocEntryTraverser *traverser );
    void startTraverseEntry( DocEntry *entry, DocEntryTraverser *traverser );
    void endProcess( DocEntry *entry, DocEntryTraverser *traverser );
    void endTraverseEntries( DocEntryTraverser * );

    static TQString languageName( const TQString &langcode );

  protected:
    DocEntry *scanMetaInfoDir( const TQString &filename, DocEntry *parent );
    DocEntry *addDirEntry( const TQDir &dir, DocEntry *parent );
    void traverseEntry( DocEntry *, DocEntryTraverser * );

  private:
    /*!
      DocMetaInfo is a singleton. Private constructor prevents direct
      instantisation.
    */
    DocMetaInfo();
  
    DocEntry::List mDocEntries;
    DocEntry::List mSearchEntries;

    DocEntry mRootEntry;

    TQStringList mLanguages;

    TQMap<TQString,TQString> mLanguageNames;

    HTMLSearch *mHtmlSearch;

    static bool mLoaded;

    static DocMetaInfo *mSelf;
};

}

#endif
