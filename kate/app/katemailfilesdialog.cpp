/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

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
*/

#include "katemailfilesdialog.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katedocmanager.h"

#include <klistview.h>
#include <klocale.h>
#include <kurl.h>

#include <tqevent.h>
#include <tqlabel.h>
#include <tqstringlist.h>
#include <tqvbox.h>

/* a private check list item, that can store a Kate::Document*.  */
class KateDocCheckItem : public TQCheckListItem {
  public:
    KateDocCheckItem( TQListView *parent, const TQString& text, Kate::Document *d )
      : TQCheckListItem( parent, text, TQCheckListItem::CheckBox ), mdoc(d) {};
    Kate::Document *doc() { return mdoc; };
  private:
    Kate::Document *mdoc;
};

///////////////////////////////////////////////////////////////////////////
// KateMailDialog implementation
///////////////////////////////////////////////////////////////////////////
KateMailDialog::KateMailDialog( TQWidget *parent, KateMainWindow  *mainwin )
  : KDialogBase( parent, "kate mail dialog", true, i18n("Email Files"),
                Ok|Cancel|User1, Ok, false,
                KGuiItem( i18n("&Show All Documents >>") ) ),
    mainWindow( mainwin )
{
  setButtonGuiItem( KDialogBase::Ok, KGuiItem( i18n("&Mail..."), "mail_send") );
  mw = makeVBoxMainWidget();
  mw->installEventFilter( this );

  lInfo = new TQLabel( i18n(
        "<p>Press <strong>Mail...</strong> to email the current document."
        "<p>To select more documents to send, press <strong>Show All Documents&nbsp;&gt;&gt;</strong>."), mw );
  // TODO avoid untill needed - later
  list = new KListView( mw );
  list->addColumn( i18n("Name") );
  list->addColumn( i18n("URL") );
  Kate::Document *currentDoc = mainWindow->viewManager()->activeView()->getDoc();
  uint n = KateDocManager::self()->documents();
  uint i = 0;
  TQCheckListItem *item;
  while ( i < n ) {
    Kate::Document *doc = KateDocManager::self()->document( i );
    if ( doc ) {
      item = new KateDocCheckItem( list, doc->docName(), doc );
      item->setText( 1, doc->url().prettyURL() );
      if ( doc == currentDoc ) {
        item->setOn( true );
        item->setSelected( true );
      }
    }
    i++;
  }
  list->hide();
  connect( this, TQT_SIGNAL(user1Clicked()), this, TQT_SLOT(slotShowButton()) );
  mw->setMinimumSize( lInfo->sizeHint() );
}

TQPtrList<Kate::Document> KateMailDialog::selectedDocs()
{
  TQPtrList<Kate::Document> l;
  TQListViewItem *item = list->firstChild();
  while ( item ) {
    if ( ((KateDocCheckItem*)item)->isOn() )
      l.append( ((KateDocCheckItem*)item)->doc() );
    item = item->nextSibling();
  }
  return l;
}

void KateMailDialog::slotShowButton()
{
  if ( list->isVisible() ) {
    setButtonText( User1, i18n("&Show All Documents >>") );
    list->hide();
  }
  else {
    list->show();
    setButtonText( User1, i18n("&Hide Document List <<") );
    lInfo->setText( i18n("Press <strong>Mail...</strong> to send selected documents") );

  }
  mw->setMinimumSize( TQSize( lInfo->sizeHint().width(), mw->sizeHint().height()) );
  setMinimumSize( calculateSize( mw->minimumSize().width(), mw->sizeHint().height() ) );
  resize( width(), minimumHeight() );
}
#include "katemailfilesdialog.moc"
