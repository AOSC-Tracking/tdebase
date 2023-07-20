/***********************************************************************
 *
 *  KfindDlg.h
 *
 ***********************************************************************/

#ifndef KFINDDLG_H
#define KFINDDLG_H

#include <kdialogbase.h>
#include <kdirlister.h>
#include <kdirwatch.h>

class TQString;

class KQuery;
class KURL;
class KFileItem;
class KfindTabWidget;
class KfindWindow;
class KStatusBar;

class KfindDlg: public KDialogBase
{
TQ_OBJECT

public:
  KfindDlg(const KURL & url, TQWidget * parent = 0, const char * name = 0);
  ~KfindDlg();
  void copySelection();

  void setStatusMsg(const TQString &);
  void setProgressMsg(const TQString &);

private:
  void closeEvent(TQCloseEvent *);
  /*Return a TQStringList of all subdirs of d*/
  TQStringList getAllSubdirs(TQDir d);

public slots:
  void startSearch();
  void stopSearch();
  void newSearch();
  void addFile(const KFileItem* item, const TQString& matchingLine);
  void setFocus();
  void slotResult(int);
//  void slotSearchDone();
  void  about ();
  void slotDeleteItem(const TQString&);
  void slotNewItems( const TQString&  );

signals:
  void haveResults(bool);
  void resultSelected(bool);

private:
  KfindTabWidget *tabWidget;
  KfindWindow * win;

  bool isResultReported;
  KQuery *query;
  KStatusBar *mStatusBar;
  KDirLister *dirlister;
  KDirWatch *dirwatch;
};

#endif


