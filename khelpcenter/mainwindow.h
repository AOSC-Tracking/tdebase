#ifndef __mainwindow_h__
#define __mainwindow_h__

#include <tqptrlist.h>

#include <tdemainwindow.h>
#include <tdeio/job.h>
#include <kurl.h>
#include <tdeparts/browserextension.h>
#include <dcopclient.h>
#include <dcopobject.h>

#include "navigator.h"
#include "glossary.h"

class TDEHTMLPart;
class TQSplitter;

class LogDialog;

namespace KHC {

class View;

class MainWindow : public TDEMainWindow, public DCOPObject
{
    TQ_OBJECT
    K_DCOP
  public:
    MainWindow();
    ~MainWindow();

  k_dcop:
    void openUrl( const TQString &url );
    void openUrl( const TQString &url, const TQCString& startup_id );
    void showHome();
    void lastSearch();

  public slots:
    void print();
    void statusBarMessage(const TQString &m);
    void slotShowHome();
    void slotLastSearch();
    void showSearchStderr();
    /**
      Show document corresponding to given URL in viewer part.
    */
    void viewUrl( const KURL &url,
                  const KParts::URLArgs &args = KParts::URLArgs() );
    /**
      Show document corresponding to given URL in viewer part.
    */
    void viewUrl( const TQString & );

    /**
      Open document corresponding to given URL, i.e. show it in the viewer part
      and select the corresponding entry in the navigator widget.
    */
    void openUrl( const KURL &url );

  protected:
    void setupActions();

    virtual void saveProperties( TDEConfig *config );
    virtual void readProperties( TDEConfig *config );

    void readConfig();
    void writeConfig();

  protected slots:
    void enableLastSearchAction();
    void enableCopyTextAction();

  private:
    void stop();

  private slots:
    void slotGlossSelected(const GlossaryEntry &entry);
    void slotStarted(TDEIO::Job *job);
    void slotInfoMessage(TDEIO::Job *, const TQString &);
    void goInternalUrl( const KURL & );
    /**
      This function is called when the user clicks on a link in the viewer part.
    */
    void slotOpenURLRequest( const KURL &url,
                             const KParts::URLArgs &args);
    void documentCompleted();
    void slotIncFontSizes();
    void slotDecFontSizes();
    void slotConfigureFonts();
    void slotCopySelectedText();

private:
    void updateZoomActions();

    TQSplitter *mSplitter;
    View *mDoc;
    Navigator *mNavigator;

    TDEAction *mLastSearchAction;
    TDEAction *mCopyText;
    LogDialog *mLogDialog;
};

}

#endif
