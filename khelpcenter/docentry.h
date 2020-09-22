#ifndef DOCENTRY_H
#define DOCENTRY_H

#include <tqstring.h>
#include <tqvaluelist.h>

namespace KHC {

class DocEntry
{
  public:
    typedef TQValueList<DocEntry *> List;

    DocEntry();
    
    DocEntry( const TQString &name, const TQString &url = TQString::null,
              const TQString &icon = TQString::null );
    
    void setName( const TQString & );
    TQString name() const;
    
    void setSearch( const TQString & );
    TQString search() const;
    
    void setIcon( const TQString & );
    TQString icon() const;
    
    void setUrl( const TQString & );
    TQString url() const;

    void setInfo( const TQString & );
    TQString info() const;

    void setLang( const TQString & );
    TQString lang() const;
    
    void setIdentifier( const TQString & );
    TQString identifier() const;

    void setIndexer( const TQString & );
    TQString indexer() const;

    void setIndexTestFile( const TQString & );
    TQString indexTestFile() const;

    void setWeight( int );
    int weight() const;

    void setSearchMethod( const TQString & );
    TQString searchMethod() const;

    void enableSearch( bool enabled );
    bool searchEnabled() const;

    void setSearchEnabledDefault( bool enabled );
    bool searchEnabledDefault() const;

    void setDocumentType( const TQString & );
    TQString documentType() const;

    void setDirectory( bool );
    bool isDirectory() const;

    bool readFromFile( const TQString &fileName );

    bool indexExists( const TQString &indexDir );

    bool docExists() const;

    void addChild( DocEntry * );
    bool hasChildren();
    DocEntry *firstChild();
    List children();
  
    void setParent( DocEntry * );
    DocEntry *parent();
  
    void setNextSibling( DocEntry * );
    DocEntry *nextSibling();

    TQString khelpcenterSpecial() const;

    bool isSearchable();
    
    void dump() const;

  protected:
    void init();

  private:
    TQString mName;
    TQString mSearch;
    TQString mIcon;
    TQString mUrl;
    TQString mInfo;
    TQString mLang;
    mutable TQString mIdentifier;
    TQString mIndexer;
    TQString mIndexTestFile;
    int mWeight;
    TQString mSearchMethod;
    bool mSearchEnabled;
    bool mSearchEnabledDefault;
    TQString mDocumentType;
    bool mDirectory;

    TQString mKhelpcenterSpecial;

    List mChildren;
    DocEntry *mParent;
    DocEntry *mNextSibling;
};

}

#endif
