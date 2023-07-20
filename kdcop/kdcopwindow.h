/*
 * Copyright (C) 2000 by Matthias Kalle Dalheimer <kalle@kde.org>
 *
 * Licensed under the Artistic License.
 */

#ifndef __KDCOPWINDOW_H__
#define __KDCOPWINDOW_H__

class DCOPClient;
class TQListBoxItem;
class TQListViewItem;
class TDEAction;
class TDESelectAction;
class TQWidgetStack;
class TQLabel;
class KDCOPListView;

#include <tdemainwindow.h>
#include "kdcoplistview.h"
#include "kdcopview.h"

class KDCOPWindow : public TDEMainWindow
{
  TQ_OBJECT

  public:

    KDCOPWindow( TQWidget* parent = 0, const char* name = 0 );

  protected slots:

    void slotCurrentChanged( TQListViewItem* item );
    void slotItemExecuted();
    void slotCallFunction( TQListViewItem* item );
    void slotApplicationRegistered(const TQCString &);
    void slotApplicationUnregistered(const TQCString &);
    void slotFillApplications();
    void slotCopy();
    void slotMode();
    void slotReload();
    void slotResultListContextMenu(TQListBoxItem *item, const TQPoint &point);

  private:
    void fillObjects( DCOPBrowserItem*, const char* app );
    void fillFunctions( DCOPBrowserItem*, const char* app, const char* obj );

    bool getParameters
      (
       const TQString  & unNormalisedSignature,
       TQString        & normalisedSignature,
       TQStringList    & types,
       TQStringList    & names
      );

    bool demarshal(TQCString & replyType, TQDataStream & reply, TQListBox *theList);

    DCOPClient    * dcopClient;
    TDEAction       * exeaction;
    TDESelectAction * langmode;
    kdcopview	  * mainView;
    TQVBoxLayout  * mainLayout;
};



#endif
