#ifndef KHC_HTMLSEARCH_H
#define KHC_HTMLSEARCH_H

#include <tqobject.h>
#include <tqstring.h>

class TDEConfig;

namespace KHC {

class DocEntry;

class HTMLSearch : public TQObject
{
    TQ_OBJECT
  public:
    HTMLSearch();
    ~HTMLSearch();

    void setupDocEntry( KHC::DocEntry * );

    TQString defaultSearch( KHC::DocEntry * );
    TQString defaultIndexer( KHC::DocEntry * );
    TQString defaultIndexTestFile( KHC::DocEntry * );

  private:
    TDEConfig *mConfig;
};

}

#endif
