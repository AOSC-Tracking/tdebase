/*****************************************************************

Copyright (c) 1996-2000 the kicker authors. See file AUTHORS.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include <tqtooltip.h>
#include <tqfile.h>

#include <tdedesktopfile.h>
#include <tdefileitem.h>
#include <konq_operations.h>
#include <kicontheme.h>
#include <tdeglobal.h>
#include <kiconeffect.h>
#include <kpropertiesdialog.h>
#include <kiconloader.h>
#include <tdemessagebox.h>
#include <krun.h>
#include <tdelocale.h>

#include <kdebug.h>

#include "global.h"
#include "kicker.h"

#include "urlbutton.h"
#include "urlbutton.moc"

URLButton::URLButton( const TQString& url, TQWidget* parent )
  : PanelButton( parent, "URLButton" )
  , fileItem( 0 )
  , pDlg( 0 )
{
    initialize( url );
}

URLButton::URLButton( const TDEConfigGroup& config, TQWidget* parent )
  : PanelButton( parent, "URLButton" )
  , fileItem( 0 )
  , pDlg( 0 )
{
    initialize( config.readPathEntry("URL") );
}

URLButton::~URLButton()
{
    delete fileItem;
}

void URLButton::initialize( const TQString& _url )
{
    KURL url(_url);
    if (!url.isLocalFile() || !url.path().endsWith(".desktop"))
    {
       TQString file = KickerLib::newDesktopFile(url);
       TDEDesktopFile df(file);
       df.writeEntry("Encoding", "UTF-8");
       df.writeEntry("Type","Link");
       df.writeEntry("Name", url.prettyURL());
       if (url.isLocalFile())
       {
          KFileItem item( KFileItem::Unknown, KFileItem::Unknown, url );
          df.writeEntry("Icon", item.iconName() );
       }
       else
       {
          df.writeEntry("Icon", KMimeType::favIconForURL(url));
       }
       df.writeEntry("URL", url.url());
       url = KURL();
       url.setPath(file);
    }
    fileItem = new KFileItem( KFileItem::Unknown, KFileItem::Unknown, url );
    setIcon( fileItem->iconName() );
    connect( this, TQ_SIGNAL(clicked()), TQ_SLOT(slotExec()) );
    setToolTip();

    if (url.isLocalFile())
    {
        backedByFile(url.path());
    }
}

void URLButton::saveConfig( TDEConfigGroup& config ) const
{
    config.writePathEntry("URL", fileItem->url().prettyURL());
}

void URLButton::setToolTip()
{
    if (fileItem->isLocalFile()
        && TDEDesktopFile::isDesktopFile(fileItem->url().path()))
    {
        TDEDesktopFile df(fileItem->url().path());

        if (df.readComment().isEmpty())
        {
            TQToolTip::add(this, df.readName());
        }
        else
        {
            TQToolTip::add(this, df.readName() + " - " + df.readComment());
        }

        setTitle(df.readName());
    }
    else
    {
        TQToolTip::add(this, fileItem->url().prettyURL());
        setTitle(fileItem->url().prettyURL());
    }
}

void URLButton::dragEnterEvent(TQDragEnterEvent *ev)
{
    if ((ev->source() != this) && fileItem->acceptsDrops() && KURLDrag::canDecode(ev))
	ev->accept(rect());
    else
        ev->ignore(rect());
    PanelButton::dragEnterEvent(ev);
}

void URLButton::dropEvent(TQDropEvent *ev)
{
    tdeApp->propagateSessionManager();
    KURL::List execList;
    if(KURLDrag::decode(ev, execList)){
        KURL url( fileItem->url() );
        if(!execList.isEmpty())  {
             if (TDEDesktopFile::isDesktopFile(url.path())){
                TDEApplication::startServiceByDesktopPath(url.path(), execList.toStringList(),
                                                        0, 0, 0, "", true);
             }
             else // attempt to interpret path as directory
             {
                 KonqOperations::doDrop( fileItem, url, ev, this );
             }
        }
    }
    PanelButton::dropEvent(ev);
}

void URLButton::startDrag()
{
    emit dragme(KURL::List(fileItem->url()), labelIcon());
}

void URLButton::slotExec()
{
    tdeApp->propagateSessionManager();
    fileItem->run();
}

void URLButton::updateURL()
{
    if (pDlg->kurl() != fileItem->url()) {
	fileItem->setURL(pDlg->kurl());
	setIcon(fileItem->iconName());
	setToolTip();
	emit requestSave();
    } else {
	setIcon(fileItem->iconName());
	setToolTip();
    }

    pDlg = 0L;
}

void URLButton::properties()
{
    if ( (fileItem->isLocalFile() && !TQFile::exists(fileItem->url().path()) )
         || !fileItem->url().isValid())
    {
        KMessageBox::error( 0L, i18n("The file %1 does not exist")
	                            .arg(fileItem->url().prettyURL()) );
        return;
    }

    pDlg = new KPropertiesDialog(fileItem, 0L, 0L, false, false); // will delete itself
    pDlg->setFileNameReadOnly(true);
    connect(pDlg, TQ_SIGNAL(applied()), TQ_SLOT(updateURL()));
    pDlg->show();
}
