/*  This file is part of the KDE libraries
    Copyright (C) 2000 Malte Starostik <malte@kde.org>
                  2000 Carsten Pfeiffer <pfeiffer@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <stdlib.h>
#include <unistd.h>
#ifdef __FreeBSD__
    #include <machine/param.h>
#endif
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <tqfile.h>
#include <tqbitmap.h>
#include <tqpixmap.h>
#include <tqpainter.h>
#include <tqimage.h>
#include <tqbuffer.h>

#include <kdatastream.h> // Do not remove, needed for correct bool serialization
#include <kurl.h>
#include <tdeapplication.h>
#include <tdeglobal.h>
#include <kiconloader.h>
#include <kimageeffect.h>
#include <kmimetype.h>
#include <klibloader.h>
#include <kdebug.h>
#include <kservice.h>
#include <kservicetype.h>
#include <kuserprofile.h>
#include <tdefilemetainfo.h>
#include <tdelocale.h>

#include <config.h> // For HAVE_NICE
#include "thumbnail.h"
#include <tdeio/thumbcreator.h>

// Use correctly TDEInstance instead of TDEApplication (but then no TQPixmap)
#undef USE_KINSTANCE
// Fix thumbnail: protocol
#define THUMBNAIL_HACK (1)

#ifdef THUMBNAIL_HACK
# include <tqfileinfo.h>
# include <ktrader.h>
#endif

// Recognized metadata entries:
// mimeType     - the mime type of the file, used for the overlay icon if any
// width        - maximum width for the thumbnail
// height       - maximum height for the thumbnail
// iconSize     - the size of the overlay icon to use if any
// iconAlpha    - the transparency value used for icon overlays
// plugin       - the name of the plugin library to be used for thumbnail creation.
//                Provided by the application to save an addition TDETrader
//                query here.
// shmid        - the shared memory segment id to write the image's data to.
//                The segment is assumed to provide enough space for a 32-bit
//                image sized width x height pixels.
//                If this is given, the data returned by the slave will be:
//                    int width
//                    int height
//                    int depth
//                Otherwise, the data returned is the image in PNG format.
 
using namespace TDEIO;

extern "C"
{
    TDE_EXPORT int kdemain(int argc, char **argv);
}


int kdemain(int argc, char **argv)
{
#ifdef HAVE_NICE
    nice( 5 );
#endif

#ifdef USE_KINSTANCE
    TDEInstance instance("tdeio_thumbnail");
#else
    // creating TDEApplication in a slave is not a very good idea,
    // as dispatchLoop() doesn't allow it to process its messages,
    // so it for example wouldn't reply to ksmserver - on the other
    // hand, this slave uses QPixmaps for some reason, and they
    // need QApplication
    // and HTML previews need even TDEApplication :(
    // session management is therefore forcibly disabled on creation
    // to prevent session management hangs and stalls
    TDEApplication::disableAutoDcopRegistration();

    TDEApplication app(argc, argv, "tdeio_thumbnail", false, true, false);
#endif

    if (argc != 4)
    {
        kdError(7115) << "Usage: tdeio_thumbnail protocol domain-socket1 domain-socket2" << endl;
        exit(-1);
    }

    ThumbnailProtocol slave(argv[2], argv[3]);
    slave.dispatchLoop();

    return 0;
}

ThumbnailProtocol::ThumbnailProtocol(const TQCString &pool, const TQCString &app)
    : SlaveBase("thumbnail", pool, app)
{
    m_creators.setAutoDelete(true);
    m_iconDict.setAutoDelete(true);
    m_iconSize = 0;
}

ThumbnailProtocol::~ThumbnailProtocol()
{
}

void ThumbnailProtocol::get(const KURL &url)
{
    m_mimeType = metaData("mimeType");
    kdDebug(7115) << "Wanting MIME Type:" << m_mimeType << endl;
#ifdef THUMBNAIL_HACK
    // ### HACK
    bool direct=false;
    if (m_mimeType.isEmpty())
    {
        kdDebug(7115) << "PATH: " << url.path() << endl;
        TQFileInfo info(url.path());
        if (info.isDir())
        {
            // We cannot process a directory
            error(TDEIO::ERR_IS_DIRECTORY,url.path());
            return;
        }
        else if (!info.exists())
        {
            // The file does not exist
            error(TDEIO::ERR_DOES_NOT_EXIST,url.path());
            return;
        }
        else if (!info.isReadable())
        {
            // The file is not readable!
            error(TDEIO::ERR_COULD_NOT_READ,url.path());
            return;
        }
        m_mimeType = KMimeType::findByURL(url)->name();
        kdDebug(7115) << "Guessing MIME Type:" << m_mimeType << endl;
        direct=true; // thumbnail: was probably called from Konqueror
    }
#endif

    if (m_mimeType.isEmpty())
    {
        error(TDEIO::ERR_INTERNAL, i18n("No MIME Type specified."));
        return;
    }

    m_width = metaData("width").toInt();
    m_height = metaData("height").toInt();
    int iconSize = metaData("iconSize").toInt();

    if (m_width < 0 || m_height < 0)
    {
        error(TDEIO::ERR_INTERNAL, i18n("No or invalid size specified."));
        return;
    }
#ifdef THUMBNAIL_HACK
    else if (!m_width || !m_height)
    {
        kdDebug(7115) << "Guessing height, width, icon size!" << endl;
        m_width=128;
        m_height=128;
        iconSize=128;
    }
#endif

    if (!iconSize)
        iconSize = TDEGlobal::iconLoader()->currentSize(TDEIcon::Desktop);
    if (iconSize != m_iconSize)
        m_iconDict.clear();
    m_iconSize = iconSize;

    m_iconAlpha = metaData("iconAlpha").toInt();
    if (m_iconAlpha)
        m_iconAlpha = (m_iconAlpha << 24) | 0xffffff;

    TQImage img;

    TDEConfigGroup group( TDEGlobal::config(), "PreviewSettings" );

    
    // ### KFMI
    bool kfmiThumb = false;
    if (group.readBoolEntry( "UseFileThumbnails", true )) {
        KService::Ptr service =
            KServiceTypeProfile::preferredService( m_mimeType, "KFilePlugin");

        if ( service && service->isValid() && /*url.isLocalFile() && */
            service->property("SupportsThumbnail").toBool())
        {
            KFileMetaInfo info(url.path(), m_mimeType, KFileMetaInfo::Thumbnail);
            if (info.isValid())
            {
                KFileMetaInfoItem item = info.item(KFileMimeTypeInfo::Thumbnail);
                if (item.isValid() && item.value().type() == TQVariant::Image)
                {
                    img = item.value().toImage();
                    kdDebug(7115) << "using KFMI for the thumbnail\n";
                    kfmiThumb = true;
                }
            }
        }
    }
    ThumbCreator::Flags flags = ThumbCreator::None;

    if (!kfmiThumb)
    {
        kdDebug(7115) << "using thumb creator for the thumbnail\n";
        TQString plugin = metaData("plugin");
#ifdef THUMBNAIL_HACK
        if (plugin.isEmpty())
        {
            TDETrader::OfferList plugins = TDETrader::self()->query("ThumbCreator");
            TQMap<TQString, KService::Ptr> mimeMap;
    
            for (TDETrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it)
            {
                TQStringList mimeTypes = (*it)->property("MimeTypes").toStringList();
                for (TQStringList::ConstIterator mt = mimeTypes.begin(); mt != mimeTypes.end(); ++mt)
                {
                    if  ((*mt)==m_mimeType)
                    {
                        plugin=(*it)->library();
                        break;
                    }
                }
                if (!plugin.isEmpty())
                    break;
            }
        }
        kdDebug(7115) << "Guess plugin: " << plugin << endl;
#endif
        if (plugin.isEmpty())
        {
            error(TDEIO::ERR_INTERNAL, i18n("No plugin specified."));
            return;
        }
        
        ThumbCreator *creator = m_creators[plugin];
        if (!creator)
        {
            // Don't use KLibFactory here, this is not a TQObject and
            // neither is ThumbCreator
            KLibrary *library = KLibLoader::self()->library(TQFile::encodeName(plugin));
            if (library)
            {
                newCreator create = (newCreator)library->symbol("new_creator");
                if (create)
                    creator = create();
            }
            if (!creator)
            {
                error(TDEIO::ERR_INTERNAL, i18n("Cannot load ThumbCreator %1").arg(plugin));
                return;
            }
            m_creators.insert(plugin, creator);
        }

        if (!creator->create(url.path(), m_width, m_height, img))
        {
            error(TDEIO::ERR_INTERNAL, i18n("Cannot create thumbnail for %1").arg(url.path()));
            return;
        }
        flags = creator->flags();
    }

    if (img.width() > m_width || img.height() > m_height)
    {
        double imgRatio = (double)img.height() / (double)img.width();
        if (imgRatio > (double)m_height / (double)m_width)
            img = img.smoothScale( int(TQMAX((double)m_height / imgRatio, 1)), m_height);
        else
            img = img.smoothScale(m_width, int(TQMAX((double)m_width * imgRatio, 1)));
    }

// ### FIXME
#ifndef USE_KINSTANCE
    if (flags & ThumbCreator::DrawFrame)
    {
        TQPixmap pix;
        pix.convertFromImage(img);
        int x2 = pix.width() - 1;
        int y2 = pix.height() - 1;
        // paint a black rectangle around the "page"
        TQPainter p;
        p.begin( &pix );
        p.setPen( TQColor( 48, 48, 48 ));
        p.drawLine( x2, 0, x2, y2 );
        p.drawLine( 0, y2, x2, y2 );
        p.setPen( TQColor( 215, 215, 215 ));
        p.drawLine( 0, 0, x2, 0 );
        p.drawLine( 0, 0, 0, y2 );
        p.end();

        const TQBitmap *mask = pix.mask();
        if ( mask ) // need to update it so we can see the frame
        {
            TQBitmap bitmap( *mask );
            TQPainter painter;
            painter.begin( &bitmap );
            painter.drawLine( x2, 0, x2, y2 );
            painter.drawLine( 0, y2, x2, y2 );
            painter.drawLine( 0, 0, x2, 0 );
            painter.drawLine( 0, 0, 0, y2 );
            painter.end();

            pix.setMask( bitmap );
        }

        img = pix.convertToImage();
    }
#endif

    if ((flags & ThumbCreator::BlendIcon) && TDEGlobal::iconLoader()->alphaBlending(TDEIcon::Desktop))
    {
        // blending the mimetype icon in
        TQImage icon = getIcon();

        int x = img.width() - icon.width() - 4;
        x = TQMAX( x, 0 );
        int y = img.height() - icon.height() - 6;
        y = TQMAX( y, 0 );
        KImageEffect::blendOnLower( x, y, icon, img );
    }

    if (img.isNull())
    {
        error(TDEIO::ERR_INTERNAL, i18n("Failed to create a thumbnail."));
        return;
    }

    const TQString shmid = metaData("shmid");
    if (shmid.isEmpty())
    {
#ifdef THUMBNAIL_HACK
        if (direct)
        {
            // If thumbnail was called directly from Konqueror, then the image needs to be raw
            //kdDebug(7115) << "RAW IMAGE TO STREAM" << endl;
            TQBuffer buf;
            if (!buf.open(IO_WriteOnly))
            {
                error(TDEIO::ERR_INTERNAL, i18n("Could not write image."));
                return;
            }
            img.save(&buf,"PNG");
            buf.close();
            data(buf.buffer());
        }
        else
#endif
        {
            TQByteArray imgData;
            TQDataStream stream( imgData, IO_WriteOnly );
            //kdDebug(7115) << "IMAGE TO STREAM" << endl;
            stream << img;
            data(imgData);
        }
    }
    else
    {
        TQByteArray imgData;
        TQDataStream stream( imgData, IO_WriteOnly );
        //kdDebug(7115) << "IMAGE TO SHMID" << endl;
        void *shmaddr = shmat(shmid.toInt(), 0, 0);
        if (shmaddr == (void *)-1)
        {
            error(TDEIO::ERR_INTERNAL, i18n("Failed to attach to shared memory segment %1").arg(shmid));
            return;
        }
        if (img.width() * img.height() > m_width * m_height)
        {
            error(TDEIO::ERR_INTERNAL, i18n("Image is too big for the shared memory segment"));
            shmdt((char*)shmaddr);
            return;
        }
        if( img.depth() != 32 )           // TDEIO::PreviewJob and this code below completely
            img = img.convertDepth( 32 ); // ignores colortable :-/, so make sure there is none
        stream << img.width() << img.height() << img.depth()
               << img.hasAlphaBuffer();
        memcpy(shmaddr, img.bits(), img.numBytes());
        shmdt((char*)shmaddr);
        data(imgData);
    }
    finished();
}

const TQImage& ThumbnailProtocol::getIcon()
{
    TQImage* icon = m_iconDict.find(m_mimeType);
    if ( !icon ) // generate it!
    {
        icon = new TQImage( KMimeType::mimeType(m_mimeType)->pixmap( TDEIcon::Desktop, m_iconSize ).convertToImage() );
        icon->setAlphaBuffer( true );

        int w = icon->width();
        int h = icon->height();
        for ( int y = 0; y < h; y++ )
        {
            TQRgb *line = (TQRgb *) icon->scanLine( y );
            for ( int x = 0; x < w; x++ )
                line[x] &= m_iconAlpha; // transparency
        }

        m_iconDict.insert( m_mimeType, icon );
    }

    return *icon;
}

