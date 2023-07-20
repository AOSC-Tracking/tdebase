/*
  This file is part of KHelpcenter.

  Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
#ifndef KCMHELPCENTER_H
#define KCMHELPCENTER_H

#include <kdialog.h>
#include <kdialogbase.h>

#include <dcopobject.h>

#include "scopeitem.h"

class TQPushButton;
class TQProgressBar;
class TQTextEdit;
class TQLabel;

class TDEProcess;
class TDEConfig;
class TDEAboutData;
class KTempFile;
class KURLRequester;
class TDEListView;

namespace KHC {
class HtmlSearchConfig;
class DocEntry;
class SearchEngine;
}

class IndexDirDialog : public KDialogBase
{
    TQ_OBJECT
  public:
    IndexDirDialog( TQWidget *parent );

  protected slots:
    void slotOk();
    void slotUrlChanged( const TQString &_url);    
  private:
    KURLRequester *mIndexUrlRequester;
};

class IndexProgressDialog : public KDialog
{
    TQ_OBJECT
  public:
    IndexProgressDialog( TQWidget *parent );
    ~IndexProgressDialog();

    void setTotalSteps( int );
    void advanceProgress();
    void setLabelText( const TQString & );
    void setMinimumLabelWidth( int width );
    void setFinished( bool );

    void appendLog( const TQString &text );

  signals:
    void closed();
    void cancelled();

  protected:
    void hideDetails();

  protected slots:
    void slotEnd();
    void toggleDetails();

  private:
    TQLabel *mLabel;
    TQProgressBar *mProgressBar;
    TQLabel *mLogLabel;
    TQTextEdit *mLogView;
    TQPushButton *mDetailsButton;
    TQPushButton *mEndButton;

    bool mFinished;
};

class KCMHelpCenterIface : virtual public DCOPObject
{
    K_DCOP
  k_dcop:
    virtual void slotIndexProgress() = 0;
    virtual void slotIndexError( const TQString & ) = 0;
};

class KCMHelpCenter : public KDialogBase, virtual public KCMHelpCenterIface
{
    TQ_OBJECT
  public:
    KCMHelpCenter( KHC::SearchEngine *, TQWidget *parent = 0,
      const char *name = 0 );
    ~KCMHelpCenter();
    
    void load();
    bool save();
    void defaults();

  public slots:

  signals:
    void searchIndexUpdated();

  protected slots:
    bool buildIndex();
    void cancelBuildIndex();
    void slotIndexFinished( TDEProcess * );
    void slotIndexProgress();
    void slotIndexError( const TQString & );
    void slotReceivedStdout(TDEProcess *proc, char *buffer, int buflen);
    void slotReceivedStderr(TDEProcess *proc, char *buffer, int buflen);
    void slotProgressClosed();

    void slotOk();

    void showIndexDirDialog();

    void checkSelection();

  protected:
    void setupMainWidget( TQWidget *parent );
    void updateStatus();
    void startIndexProcess();

    void deleteProcess();
    void deleteCmdFile();

    void advanceProgress();

    /**
     * Find a user-writeable location for the indices, if the current location
     * is not.
     */
    void findWriteableIndexDir();
  private:
    KHC::SearchEngine *mEngine;
  
    TDEListView *mListView;
    TQLabel *mIndexDirLabel;
    TQPushButton *mBuildButton;
    IndexProgressDialog *mProgressDialog;
    
    TQValueList<KHC::DocEntry *> mIndexQueue;
    TQValueList<KHC::DocEntry *>::ConstIterator mCurrentEntry;
    
    TDEConfig *mConfig;
    
    TDEAboutData *mAboutData;

    KHC::HtmlSearchConfig *mHtmlSearchTab;
    TQWidget *mScopeTab;

    KTempFile *mCmdFile;

    TDEProcess *mProcess;

    bool mIsClosing;

    TQString mStdOut;
    TQString mStdErr;

    bool mRunAsRoot;
};

#endif
