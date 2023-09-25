#ifndef __HTMLSEARCH_H__
#define __HTMLSEARCH_H__


#include <tqstring.h>
#include <tqstringlist.h>


class TQWidget;
class TQProgressDialog;


class TDEProcess;


class ProgressDialog;


class HTMLSearch : public TQObject
{
  TQ_OBJECT

public:

  HTMLSearch();

  bool generateIndex(TQString lang, TQWidget *parent=0);

  TQString search(TQString lang, TQString words, TQString method="and", int matches=10,
		 TQString format="builtin-long", TQString sort="score");


protected slots:

  void htdigStdout(TDEProcess *proc, char *buffer, int buflen);
  void htdigExited(TDEProcess *proc);
  void htmergeExited(TDEProcess *proc);
  void htsearchStdout(TDEProcess *proc, char *buffer, int buflen);
  void htsearchExited(TDEProcess *proc);


protected:

  TQString dataPath(const TQString& lang);

  bool saveFilesList(const TQString& lang);
  void scanDir(const TQString& dir);

  bool createConfig(const TQString& lang);


private:

  TQStringList   _files;
  TDEProcess      *_proc;
  int           _filesToDig, _filesDigged, _filesScanned;
  volatile bool _htdigRunning, _htmergeRunning, _htsearchRunning;
  TQString       _searchResult;
  ProgressDialog *progress;

};


#endif
