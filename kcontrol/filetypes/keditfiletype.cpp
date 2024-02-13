/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#include "filetypedetails.h"
#include "typeslistitem.h"
#include "keditfiletype.h"

#include <tqfile.h>

#include <dcopclient.h>
#include <tdeapplication.h>
#include <tdeaboutdata.h>
#include <kdebug.h>
#include <tdecmdlineargs.h>
#include <tdesycoca.h>
#include <kstandarddirs.h>

#ifdef TQ_WS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

FileTypeDialog::FileTypeDialog( KMimeType::Ptr mime )
  : KDialogBase( 0L, 0, false, TQString::null, /* Help | */ Cancel | Apply | Ok,
                 Ok, false )
{
  init( mime, false );
}

FileTypeDialog::FileTypeDialog( KMimeType::Ptr mime, bool newItem )
  : KDialogBase( 0L, 0, false, TQString::null, /* Help | */ Cancel | Apply | Ok,
                 Ok, false )
{
  init( mime, newItem );
}

void FileTypeDialog::init( KMimeType::Ptr mime, bool newItem )
{
  m_details = new FileTypeDetails( this );
  TQListView * dummyListView = new TQListView( m_details );
  dummyListView->hide();
  m_item = new TypesListItem( dummyListView, mime, newItem );
  m_details->setTypeItem( m_item );

  // This code is very similar to kcdialog.cpp
  setMainWidget( m_details );
  connect(m_details, TQ_SIGNAL(changed(bool)), this, TQ_SLOT(clientChanged(bool)));
  // TODO setHelp()
  enableButton(Apply, false);

  connect( KSycoca::self(), TQ_SIGNAL( databaseChanged() ), TQ_SLOT( slotDatabaseChanged() ) );
}

void FileTypeDialog::save()
{
  if (m_item->isDirty()) {
    m_item->sync();
    KService::rebuildKSycoca(this);
  }
}

void FileTypeDialog::slotApply()
{
  save();
}

void FileTypeDialog::slotOk()
{
  save();
  accept();
}

void FileTypeDialog::clientChanged(bool state)
{
  // enable/disable buttons
  enableButton(User1, state);
  enableButton(Apply, state);
}

void FileTypeDialog::slotDatabaseChanged()
{
  if ( KSycoca::self()->isChanged( "mime" ) )
  {
      m_item->refresh();
  }
}

#include "keditfiletype.moc"

static TDECmdLineOptions options[] =
{
  { "parent <winid>", I18N_NOOP("Makes the dialog transient for the window specified by winid"), 0 },
  { "+mimetype",   I18N_NOOP("File type to edit (e.g. text/html)"), 0 },
  TDECmdLineLastOption
};

int main(int argc, char ** argv)
{
  TDELocale::setMainCatalogue("filetypes");
  TDEAboutData aboutData( "keditfiletype", I18N_NOOP("KEditFileType"), "1.0",
                        I18N_NOOP("TDE file type editor - simplified version for editing a single file type"),
                        TDEAboutData::License_GPL,
                        I18N_NOOP("(c) 2000, KDE developers") );
  aboutData.addAuthor("Preston Brown",0, "pbrown@kde.org");
  aboutData.addAuthor("David Faure",0, "faure@kde.org");

  TDECmdLineArgs::init( argc, argv, &aboutData );
  TDECmdLineArgs::addCmdLineOptions( options ); // Add our own options.
  TDEApplication app;
  TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();

  if (args->count() == 0)
    TDECmdLineArgs::usage();

  TQString arg = args->arg(0);

  bool createType = arg.startsWith( "*" );

  KMimeType::Ptr mime;
  
  if ( createType ) {
    TQString mimeString = "application/x-kdeuser%1";
    TQString loc;
    int inc = 0;
    do {
      ++inc;
      loc = locateLocal( "mime", mimeString.arg( inc ) + ".desktop" );
    }
    while ( TQFile::exists( loc ) );

    TQStringList patterns;
    if ( arg.length() > 2 )
	patterns << arg.lower() << arg.upper();
    TQString comment;
    if ( arg.startsWith( "*." ) && arg.length() >= 3 ) {
	TQString type = arg.mid( 3 ).prepend( arg[2].upper() );
        comment = i18n( "%1 File" ).arg( type );
    }
    mime = new KMimeType( loc, mimeString.arg( inc ), TQString::null, comment, patterns );
  }
  else { 
    mime = KMimeType::mimeType( arg );
  if (!mime)
      kdFatal() << "Mimetype " << arg << " not found" << endl;
  }

  FileTypeDialog dlg( mime, createType );
#if defined TQ_WS_X11
  if( args->isSet( "parent" )) {
    bool ok;
    long id = args->getOption("parent").toLong(&ok);
    if (ok)
      XSetTransientForHint( tqt_xdisplay(), dlg.winId(), id );
  }
#endif
  args->clear();
  if ( !createType )
  dlg.setCaption( i18n("Edit File Type %1").arg(mime->name()) );
  else {
    dlg.setCaption( i18n("Create New File Type %1").arg(mime->name()) );
    dlg.enableButton( KDialogBase::Apply, true );
  }
  app.setMainWidget( &dlg );
  dlg.show(); // non-modal

  return app.exec();
}

