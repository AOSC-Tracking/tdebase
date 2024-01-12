/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Idea by Gael Duval
// Implementation by David Faure

#include <config.h>

#include <tdeapplication.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <tdelocale.h>
#include <tdecmdlineargs.h>
#include <tdeaboutdata.h>
#include <kurifilter.h>
#include <tdeio/job.h>

#include <tqscrollview.h>
#include "kwebdesktop.h"
#include <kmimetype.h>
#include <tdeparts/componentfactory.h>
#include "kwebdesktopsettings.h"

#include "kwebdesktop.moc"

static TDECmdLineOptions options[] =
{
  { "+width", I18N_NOOP("Width of the image to create"), 0 },
  { "+height", I18N_NOOP("Height of the image to create"), 0 },
  { "+file", I18N_NOOP("File sname where to dump the output in png format"), 0 },
  { "+[URL]", I18N_NOOP("URL to open (if not specified, it is read from kwebdesktoprc)"), 0 },
  TDECmdLineLastOption
};

KWebDesktopRun::KWebDesktopRun( KWebDesktop* webDesktop, const KURL & url )
    : m_webDesktop(webDesktop), m_url(url)
{
    kdDebug() << "KWebDesktopRun::KWebDesktopRun starting get" << endl;
    TDEIO::Job * job = TDEIO::get(m_url, false, false);
    connect( job, TQ_SIGNAL( result( TDEIO::Job *)),
             this, TQ_SLOT( slotFinished(TDEIO::Job *)));
    connect( job, TQ_SIGNAL( mimetype( TDEIO::Job *, const TQString &)),
             this, TQ_SLOT( slotMimetype(TDEIO::Job *, const TQString &)));
}

void KWebDesktopRun::slotMimetype( TDEIO::Job *job, const TQString &_type )
{
    TDEIO::SimpleJob *sjob = static_cast<TDEIO::SimpleJob *>(job);
    // Update our URL in case of a redirection
    m_url = sjob->url();
    TQString type = _type; // necessary copy if we plan to use it
    sjob->putOnHold();
    kdDebug() << "slotMimetype : " << type << endl;

    KParts::ReadOnlyPart* part = m_webDesktop->createPart( type );
    // Now open the URL in the part
    if ( part )
        part->openURL( m_url );
}

void KWebDesktopRun::slotFinished( TDEIO::Job * job )
{
    // The whole point of all this is to abort silently on error
    if (job->error())
    {
        kdDebug() << job->errorString() << endl;
        kapp->exit(1);
    }
}


int main( int argc, char **argv )
{
    TDEAboutData data( "kwebdesktop", I18N_NOOP("TDE Web Desktop"),
                     VERSION,
                     I18N_NOOP("Displays an HTML page as the background of the desktop"),
                     TDEAboutData::License_GPL,
                     "(c) 2000, David Faure <faure@kde.org>" );
    data.addAuthor( "David Faure", I18N_NOOP("developer and maintainer"), "faure@kde.org" );

    TDECmdLineArgs::init( argc, argv, &data );

    TDECmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    TDEApplication app;

    TDECmdLineArgs *args = TDECmdLineArgs::parsedArgs();
    if ( args->count() != 3 && args->count() != 4 )
    {
       args->usage();
       return 1;
    }
    const int width = TQCString(args->arg(0)).toInt();
    const int height = TQCString(args->arg(1)).toInt();
    TQCString imageFile = args->arg(2);
    TQString url;
    if (args->count() == 4)
        url = TQString::fromLocal8Bit(args->arg(3));

    KWebDesktop *webDesktop = new KWebDesktop( 0, imageFile, width, height );

    if (url.isEmpty())
      url = KWebDesktopSettings::uRL();
    // Apply uri filter
    KURIFilterData uridata = url;
    KURIFilter::self()->filterURI( uridata );
    KURL u = uridata.uri();

    // Now start getting, to ensure mimetype and possible connection
    KWebDesktopRun * run = new KWebDesktopRun( webDesktop, u );

    int ret = app.exec();

    TDEIO::SimpleJob::removeOnHold(); // Kill any slave that was put on hold.
    delete webDesktop;
    delete run;
    //tdehtml::Cache::clear();

    return ret;
}

void KWebDesktop::slotCompleted()
{
    kdDebug() << "KWebDesktop::slotCompleted" << endl;
    // Dump image to m_imageFile
    TQPixmap snapshot = TQPixmap::grabWidget( m_part->widget() );
    snapshot.save( m_imageFile, "PNG" );
    // And terminate the app.
    kapp->quit();
}

KParts::ReadOnlyPart* KWebDesktop::createPart( const TQString& mimeType )
{
    delete m_part;
    m_part = 0;

    KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
    if ( !mime || mime == KMimeType::defaultMimeTypePtr() )
        return 0;
    if ( mime->is( "text/html" ) || mime->is( "text/xml" ) || mime->is( "application/xhtml+xml" ) )
    {
        TDEHTMLPart* htmlPart = new TDEHTMLPart;
        htmlPart->widget()->resize(m_width,m_height);

        htmlPart->setMetaRefreshEnabled(false);
        htmlPart->setJScriptEnabled(false);
        htmlPart->setJavaEnabled(false);
        htmlPart->setPluginsEnabled(false);

        ((TQScrollView *)htmlPart->widget())->setFrameStyle( TQFrame::NoFrame );
        ((TQScrollView *)htmlPart->widget())->setHScrollBarMode( TQScrollView::AlwaysOff );
        ((TQScrollView *)htmlPart->widget())->setVScrollBarMode( TQScrollView::AlwaysOff );

        connect( htmlPart, TQ_SIGNAL( completed() ), this, TQ_SLOT( slotCompleted() ) );
        m_part = htmlPart;
    } else {
        // Try to find an appropriate viewer component
        m_part = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>
                 ( mimeType, TQString(), 0, 0, this, 0 );
        if ( !m_part )
            kdWarning() << "No handler found for " << mimeType << endl;
        else {
            kdDebug() << "Loaded " << m_part->className() << endl;
            connect( m_part, TQ_SIGNAL( completed() ),
                     this, TQ_SLOT( slotCompleted() ) );
        }
    }
    if ( m_part ) {
        connect( m_part, TQ_SIGNAL( canceled(const TQString &) ),
                 this, TQ_SLOT( slotCompleted() ) );
    }
    return m_part;
}

KWebDesktop::~KWebDesktop()
{
    delete m_part;
}
