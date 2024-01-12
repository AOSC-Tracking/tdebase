/*
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    ---
    Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/

#include "katemwmodonhddialog.h"
#include "katemwmodonhddialog.moc"

#include "katedocmanager.h"

#include <kate/document.h>

#include <kiconloader.h>
#include <tdelistview.h>
#include <tdelocale.h>
#include <tdemessagebox.h>
#include <kprocio.h>
#include <krun.h>
#include <tdetempfile.h>
#include <kpushbutton.h>

#include <tqlabel.h>
#include <tqlistview.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqwhatsthis.h>
#include <tqvbox.h>

class KateDocItem : public TQCheckListItem
{
  public:
    KateDocItem( Kate::Document *doc, const TQString &status, TDEListView *lv )
  : TQCheckListItem( lv, doc->url().prettyURL(), CheckBox ),
        document( doc )
    {
      setText( 1, status );
      if ( ! doc->isModified() )
        setOn( On );
    }
    ~KateDocItem() {};

    Kate::Document *document;
};


KateMwModOnHdDialog::KateMwModOnHdDialog( DocVector docs, TQWidget *parent, const char *name )
  : KDialogBase( parent, name, true, i18n("Documents Modified on Disk"),
                 User1|User2|User3, User3, false,
                 KGuiItem (i18n("&Ignore"), "window-close"),
                 KGuiItem (i18n("&Overwrite"), "document-save"),
                 KGuiItem (i18n("&Reload"), "reload") )
{
  setButtonWhatsThis( User1, i18n(
      "Removes the modified flag from the selected documents and closes the "
      "dialog if there are no more unhandled documents.") );
  setButtonWhatsThis( User2, i18n(
      "Overwrite selected documents, discarding the disk changes and closes the "
      "dialog if there are no more unhandled documents.") );
  setButtonWhatsThis( User3, i18n(
      "Reloads the selected documents from disk and closes the dialog if there "
      "are no more unhandled documents.") );

  TQVBox *w = makeVBoxMainWidget();
  w->setSpacing( KDialog::spacingHint() );

  TQHBox *lo1 = new TQHBox( w );

  // dialog text
  TQLabel *icon = new TQLabel( lo1 );
  icon->setPixmap( DesktopIcon("messagebox_warning") );

  TQLabel *t = new TQLabel( i18n(
      "<qt>The documents listed below has changed on disk.<p>Select one "
      "or more at the time and press an action button until the list is empty.</qt>"), lo1 );
  lo1->setStretchFactor( t, 1000 );

  // document list
  lvDocuments = new TDEListView( w );
  lvDocuments->addColumn( i18n("Filename") );
  lvDocuments->addColumn( i18n("Status on Disk") );
  lvDocuments->setSelectionMode( TQListView::Single );

  TQStringList l;
  l << "" << i18n("Modified") << i18n("Created") << i18n("Deleted");
  for ( uint i=0; i < docs.size(); i++ )
    new KateDocItem( docs[i], l[ (uint)KateDocManager::self()->documentInfo( docs[i] )->modifiedOnDiscReason ], lvDocuments );

  connect( lvDocuments, TQ_SIGNAL(selectionChanged()), this, TQ_SLOT(slotSelectionChanged()) );

  // diff button
  TQHBox *lo2 = new TQHBox ( w );
  TQWidget *d = new TQWidget (lo2);
  lo2->setStretchFactor (d, 2);
  btnDiff = new KPushButton( KGuiItem (i18n("&View Difference"), "edit"), lo2 );

  TQWhatsThis::add( btnDiff, i18n(
      "Calculates the difference between the the editor contents and the disk "
      "file for the selected document, and shows the difference with the "
      "default application. Requires diff(1).") );
  connect( btnDiff, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotDiff()) );

  slotSelectionChanged();
  m_tmpfile = 0;
}

KateMwModOnHdDialog::~KateMwModOnHdDialog()
{
}

void KateMwModOnHdDialog::slotUser1()
{
  handleSelected( Ignore );
}

void KateMwModOnHdDialog::slotUser2()
{
  handleSelected( Overwrite );
}

void KateMwModOnHdDialog::slotUser3()
{
  handleSelected( Reload );
}

void KateMwModOnHdDialog::handleSelected( int action )
{
  // collect all items we can remove
  TQValueList<TQListViewItem *> itemsToDelete;
  for ( TQListViewItemIterator it ( lvDocuments );  it.current(); ++it )
  {
    KateDocItem *item = static_cast<KateDocItem *>(it.current());
    
    if ( item->isOn() )
    {
      int reason = (int)KateDocManager::self()->documentInfo( item->document )->modifiedOnDiscReason;
      bool succes = true;
      
      Kate::DocumentExt *dext = documentExt( item->document );
      if ( ! dext ) continue;

      dext->setModifiedOnDisk( 0 );
      switch ( action )
      {
        case Overwrite:
          succes = item->document->save();
          if ( ! succes )
          {
            KMessageBox::sorry( this,
                                i18n("Could not save the document \n'%1'").
                                    arg( item->document->url().prettyURL() ) );
          }
          break;

        case Reload:
          item->document->reloadFile();
          break;

        default:
          break;
      }

      if ( succes )
        itemsToDelete.append (item);
      else
        dext->setModifiedOnDisk( reason );
    }
  }

  // remove the marked items
  for (unsigned int i=0; i < itemsToDelete.count(); ++i)
    delete itemsToDelete[i];

  // any documents left unhandled?
  if ( ! lvDocuments->childCount() )
    done( Ok );
}

void KateMwModOnHdDialog::slotSelectionChanged()
{
  // set the diff button enabled
  btnDiff->setEnabled( lvDocuments->currentItem() &&
      KateDocManager::self()->documentInfo( ((KateDocItem*)lvDocuments->currentItem())->document )->modifiedOnDiscReason != 3 );
}

// ### the code below is slightly modified from tdelibs/kate/part/katedialogs,
// class KateModOnHdPrompt.
void KateMwModOnHdDialog::slotDiff()
{
  if ( m_tmpfile ) // we are allready somewhere in this process.
    return;

  if ( ! lvDocuments->currentItem() )
    return;

  Kate::Document *doc = ((KateDocItem*)lvDocuments->currentItem())->document;

  // don't try o diff a deleted file
  if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDiscReason == 3 )
    return;

  // Start a TDEProcess that creates a diff
  KProcIO *p = new KProcIO();
  p->setComm( TDEProcess::All );
  *p << "diff" << "-u" << "-" <<  doc->url().path();
  connect( p, TQ_SIGNAL(processExited(TDEProcess*)), this, TQ_SLOT(slotPDone(TDEProcess*)) );
  connect( p, TQ_SIGNAL(readReady(KProcIO*)), this, TQ_SLOT(slotPRead(KProcIO*)) );

  setCursor( WaitCursor );

  p->start( TDEProcess::NotifyOnExit, true );

  uint lastln =  doc->numLines();
  for ( uint l = 0; l <  lastln; l++ )
    p->writeStdin(  doc->textLine( l ), l < lastln );

  p->closeWhenDone();
}

void KateMwModOnHdDialog::slotPRead( KProcIO *p)
{
  // create a file for the diff if we haven't one allready
  if ( ! m_tmpfile )
    m_tmpfile = new KTempFile();
  // put all the data we have in it
  TQString stmp;
  bool dataRead = false;
  while ( p->readln( stmp, false ) > -1 ) {
    *m_tmpfile->textStream() << stmp << endl;
    dataRead = true;
  }

  if (dataRead)
    p->ackRead();
}

void KateMwModOnHdDialog::slotPDone( TDEProcess *p )
{
  setCursor( ArrowCursor );
  if( ! m_tmpfile )
  {
    // dominik: there were only whitespace changes, so that the diff returned by
    // diff(1) has 0 bytes. So slotPRead() is never called, as there is
    // no data, so that m_tmpfile was never created and thus is NULL.
    // NOTE: would be nice, if we could produce a fake-diff, so that kompare
    //       tells us "The files are identical". Right now, we get an ugly
    //       "Could not parse diff output".
    m_tmpfile = new KTempFile();
  }
  m_tmpfile->close();

  if ( ! p->normalExit() /*|| p->exitStatus()*/ )
  {
    KMessageBox::sorry( this,
                        i18n("The diff command failed. Please make sure that "
                            "diff(1) is installed and in your PATH."),
                        i18n("Error Creating Diff") );
    delete m_tmpfile;
    m_tmpfile = 0;
    return;
  }

  KRun::runURL( m_tmpfile->name(), "text/x-diff", true );
  delete m_tmpfile;
  m_tmpfile = 0;
}
