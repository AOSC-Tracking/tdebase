/* This file is part of the KDE project
   Copyright (C) 2001 Malte Starostik <malte@kde.org>

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

#include <string.h>
#include <time.h>

#include <tqbuffer.h>
#include <tqfile.h>
#include <tqcache.h>
#include <tqimage.h>
#include <tqtimer.h>

#include <kdatastream.h> // DO NOT REMOVE, otherwise bool marshalling breaks
#include <kicontheme.h>
#include <kimageio.h>
#include <ksimpleconfig.h>
#include <tdestandarddirs.h>
#include <tdeio/job.h>

#include "favicons.moc"

struct FaviconsModulePrivate
{
    virtual ~FaviconsModulePrivate() { delete config; }

    struct DownloadInfo
    {
        TQString hostOrURL;
        bool isHost;
        TQByteArray iconData;
    };
    TQMap<TDEIO::Job *, DownloadInfo> downloads;
    TQStringList failedDownloads;
    KSimpleConfig *config;
    TQPtrList<TDEIO::Job> killJobs;
    TDEIO::MetaData metaData;
    TQString faviconsDir;
    TQCache<TQString> faviconsCache;
};

FaviconsModule::FaviconsModule(const TQCString &obj)
    : KDEDModule(obj)
{
    // create our favicons folder so that TDEIconLoader knows about it
    d = new FaviconsModulePrivate;
    d->faviconsDir = TDEGlobal::dirs()->saveLocation( "cache", "favicons/" );
    d->faviconsDir.truncate(d->faviconsDir.length()-9); // Strip off "favicons/"
    d->metaData.insert("ssl_no_client_cert", "TRUE");
    d->metaData.insert("ssl_militant", "TRUE");
    d->metaData.insert("UseCache", "false");
    d->metaData.insert("cookies", "none");
    d->metaData.insert("no-auth", "true");
    d->config = new KSimpleConfig(locateLocal("data", "konqueror/faviconrc"));
    d->killJobs.setAutoDelete(true);
    d->faviconsCache.setAutoDelete(true);
}

FaviconsModule::~FaviconsModule()
{
    delete d;
}

TQString removeSlash(TQString result) 
{
    for (unsigned int i = result.length() - 1; i > 0; --i)
        if (result[i] != '/')
        {
            result.truncate(i + 1);
            break;
        }

    return result;
}


TQString FaviconsModule::iconForURL(const KURL &url)
{
    if (url.host().isEmpty())
        return TQString::null;

    TQString icon;
    TQString simplifiedURL = simplifyURL(url);

    TQString *iconURL = d->faviconsCache.find( removeSlash(simplifiedURL) );
    if (iconURL)
        icon = *iconURL;
    else
        icon = d->config->readEntry( removeSlash(simplifiedURL) );

    if (!icon.isEmpty())
        icon = iconNameFromURL(KURL( icon ));
    else 
        icon = url.host();
        
    icon = "favicons/" + icon;

    if (TQFile::exists(d->faviconsDir+icon+".png"))
        return icon;

    return TQString::null;
}

TQString FaviconsModule::simplifyURL(const KURL &url)
{
    // splat any = in the URL so it can be safely used as a config key
    TQString result = url.host() + url.path();
    for (unsigned int i = 0; i < result.length(); ++i)
        if (result[i] == '=')
            result[i] = '_';
    return result;
}

TQString FaviconsModule::iconNameFromURL(const KURL &iconURL)
{
    if (iconURL.path() == "/favicon.ico")
       return iconURL.host();

    TQString result = simplifyURL(iconURL);
    // splat / so it can be safely used as a file name
    for (unsigned int i = 0; i < result.length(); ++i)
        if (result[i] == '/')
            result[i] = '_';

    TQString ext = result.right(4);
    if (ext == ".ico" || ext == ".png" || ext == ".xpm")
        result.remove(result.length() - 4, 4);

    return result;
}

bool FaviconsModule::isIconOld(const TQString &icon)
{
    struct stat st;
    if (stat(TQFile::encodeName(icon), &st) != 0)
        return true; // Trigger a new download on error

    return (time(0) - st.st_mtime) > 604800; // arbitrary value (one week)
}

void FaviconsModule::setIconForURL(const KURL &url, const KURL &iconURL)
{
    TQString simplifiedURL = simplifyURL(url);

    d->faviconsCache.insert(removeSlash(simplifiedURL), new TQString(iconURL.url()) );

    TQString iconName = "favicons/" + iconNameFromURL(iconURL);
    TQString iconFile = d->faviconsDir + iconName + ".png";

    if (!isIconOld(iconFile)) {
        emit iconChanged(false, simplifiedURL, iconName);
        return;
    }

    startDownload(simplifiedURL, false, iconURL);
}

void FaviconsModule::downloadHostIcon(const KURL &url)
{
    TQString iconFile = d->faviconsDir + "favicons/" + url.host() + ".png";
    if (!isIconOld(iconFile))
        return;

    startDownload(url.host(), true, KURL(url, "/favicon.ico"));
}

void FaviconsModule::startDownload(const TQString &hostOrURL, bool isHost, const KURL &iconURL)
{
    if (d->failedDownloads.contains(iconURL.url()))
        return;

    TDEIO::Job *job = TDEIO::get(iconURL, false, false);
    job->addMetaData(d->metaData);
    connect(job, TQ_SIGNAL(data(TDEIO::Job *, const TQByteArray &)), TQ_SLOT(slotData(TDEIO::Job *, const TQByteArray &)));
    connect(job, TQ_SIGNAL(result(TDEIO::Job *)), TQ_SLOT(slotResult(TDEIO::Job *)));
    connect(job, TQ_SIGNAL(infoMessage(TDEIO::Job *, const TQString &)), TQ_SLOT(slotInfoMessage(TDEIO::Job *, const TQString &)));
    FaviconsModulePrivate::DownloadInfo download;
    download.hostOrURL = hostOrURL;
    download.isHost = isHost;
    d->downloads.insert(job, download);
}

void FaviconsModule::slotData(TDEIO::Job *job, const TQByteArray &data)
{
    FaviconsModulePrivate::DownloadInfo &download = d->downloads[job];
    unsigned int oldSize = download.iconData.size();
    if (oldSize > 0x10000)
    {
        d->killJobs.append(job);
        TQTimer::singleShot(0, this, TQ_SLOT(slotKill()));
    }
    download.iconData.resize(oldSize + data.size());
    memcpy(download.iconData.data() + oldSize, data.data(), data.size());
}

void FaviconsModule::slotResult(TDEIO::Job *job)
{
    FaviconsModulePrivate::DownloadInfo download = d->downloads[job];
    d->downloads.remove(job);
    KURL iconURL = static_cast<TDEIO::TransferJob *>(job)->url();
    TQString iconName;
    if (!job->error())
    {
        TQBuffer buffer(download.iconData);
        buffer.open(IO_ReadOnly);
        TQImageIO io;
        io.setIODevice(&buffer);
        io.setParameters("size=16");
        // Check here too, the job might have had no error, but the downloaded
        // file contains just a 404 message sent with a 200 status.
        // microsoft.com does that... (malte)
        if (io.read())
        {
            // Some sites have nasty 32x32 icons, according to the MS docs
            // IE ignores them, well, we scale them, otherwise the location
            // combo / menu will look quite ugly
            if (io.image().width() != TDEIcon::SizeSmall || io.image().height() != TDEIcon::SizeSmall)
                io.setImage(io.image().smoothScale(TDEIcon::SizeSmall, TDEIcon::SizeSmall));

            if (download.isHost)
                iconName = download.hostOrURL;
            else
                iconName = iconNameFromURL(iconURL);

            iconName = "favicons/" + iconName;

            io.setIODevice(0);
            io.setFileName(d->faviconsDir + iconName + ".png");
            io.setFormat("PNG");
            if (!io.write())
                iconName = TQString::null;
            else if (!download.isHost)
                d->config->writeEntry( removeSlash(download.hostOrURL), iconURL.url());
        }
    }
    if (iconName.isEmpty())
        d->failedDownloads.append(iconURL.url());

    emit iconChanged(download.isHost, download.hostOrURL, iconName);
}

void FaviconsModule::slotInfoMessage(TDEIO::Job *job, const TQString &msg)
{
    emit infoMessage(static_cast<TDEIO::TransferJob *>( job )->url(), msg);
}

void FaviconsModule::slotKill()
{
    d->killJobs.clear();
}

extern "C" {
    TDE_EXPORT KDEDModule *create_favicons(const TQCString &obj)
    {
        KImageIO::registerFormats();
        return new FaviconsModule(obj);
    }
}
