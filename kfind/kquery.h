#ifndef KQUERY_H
#define KQUERY_H

#include <time.h>

#include <tqobject.h>
#include <tqregexp.h>
#include <tqptrqueue.h>
#include <tqdir.h>
#include <tqstringlist.h>

#include <tdeio/job.h>
#include <kurl.h>
#include <kprocess.h>

class KFileItem;

class KQuery : public TQObject
{
  TQ_OBJECT

 public:
  KQuery(TQObject *parent = 0, const char * name = 0);
  ~KQuery();

  void setSizeRange( int mode, TDEIO::filesize_t value1, TDEIO::filesize_t value2 );
  void setTimeRange( time_t from, time_t to );
  void setRegExp( const TQString &regexp, bool caseSensitive );
  void setRecursive( bool recursive );
  void setPath(const KURL & url );
  void setFileType( int filetype );
  void setMimeType( const TQStringList & mimetype );
  void setContext( const TQString & context, bool casesensitive, 
    bool search_binary, bool useRegexp );
  void setUsername( TQString username );
  void setGroupname( TQString groupname );
	void setMetaInfo(const TQString &metainfo, const TQString &metainfokey);
  void setUseFileIndex(bool);

  void start();
  void kill();
  const KURL& url()              {return m_url;};

 private:
  /* Check if file meets the find's requirements*/
  inline void processQuery(KFileItem*);

 public slots: 
  /* List of files found using slocate */
  void slotListEntries(TQStringList);
 protected slots:
  /* List of files found using TDEIO */
  void slotListEntries(TDEIO::Job *, const TDEIO::UDSEntryList &);
  void slotResult(TDEIO::Job *);
  void slotCanceled(TDEIO::Job *);
  void slotreceivedSdtout(TDEProcess*,char*,int);
  void slotreceivedSdterr(TDEProcess*,char*,int);
  void slotendProcessLocate(TDEProcess*);

 signals:
  void addFile(const KFileItem *filename, const TQString& matchingLine);
  void result(int);

 private:
  void checkEntries();

  int m_filetype;
  int m_sizemode;
  TDEIO::filesize_t m_sizeboundary1;
  TDEIO::filesize_t m_sizeboundary2;
  KURL m_url;
  time_t m_timeFrom;
  time_t m_timeTo;
  TQRegExp m_regexp;// regexp for file content
  bool m_recursive;
  TQStringList m_mimetype;
  TQString m_context;
  TQString m_username;
  TQString m_groupname;
  TQString m_metainfo;
  TQString m_metainfokey;
  bool m_casesensitive;
  bool m_search_binary;
  bool m_regexpForContent;
  bool m_useLocate;
  char* bufferLocate;
  int bufferLocateLength;
  TQStringList locateList;
  TDEProcess *processLocate;
  TQPtrList<TQRegExp> m_regexps;// regexps for file name
//  TQValueList<bool> m_regexpsContainsGlobs;  // what should this be good for ? Alex
  TDEIO::ListJob *job;
  bool m_insideCheckEntries;
  TQPtrQueue<KFileItem> m_fileItems;
  TQRegExp* metaKeyRx;
  int m_result;
  TQStringList ignore_mimetypes;
  TQStringList ooo_mimetypes;     // OpenOffice.org mimetypes
  TQStringList koffice_mimetypes;
};

#endif
