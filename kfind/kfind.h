/***********************************************************************
 *
 *  Kfind.h
 *
 ***********************************************************************/

#ifndef KFIND_H
#define KFIND_H

#include <tqwidget.h>
#include <tdefileitem.h>
#include <kdirlister.h>

class TQString;
class KPushButton;

class KQuery;
class KURL;
class KfindTabWidget;

class Kfind: public TQWidget
{
    TQ_OBJECT

public:
    Kfind(TQWidget * parent = 0, const char * name = 0);
    ~Kfind();

    void setURL( const KURL &url );

    void setQuery(KQuery * q) { query = q; }
    void searchFinished();

    void saveState( TQDataStream *stream );
    void restoreState( TQDataStream *stream );

public slots:
    void startSearch();
    void stopSearch();
    //void newSearch();
    void saveResults();

signals:
    void haveResults(bool);
    void resultSelected(bool);

    void started();
    void destroyMe();

private:
    void setFocus();
    KfindTabWidget *tabWidget;
    KPushButton *mSearch;
    KPushButton *mStop;
    KPushButton *mSave;
    KQuery *query;

public:
    KDirLister *dirlister;
};

#endif


