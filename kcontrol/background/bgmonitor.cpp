/* vi: ts=8 sts=4 sw=4
   kate: space-indent on; indent-width 4; indent-mode cstyle;

   This file is part of the KDE project, module kcmbackground.

   Copyright (C) 2002 Laurent Montel <montell@club-internet.fr>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2005 David Saxton <david@bluehaze.org>
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License 
   version 2 as published by the Free Software Foundation.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include <kdebug.h>
#include <klocale.h>
#include <kpixmap.h>
#include <kstandarddirs.h>
#include <kurldrag.h>
#include <tqapplication.h>
#include <tqpixmap.h>
#include <tqwhatsthis.h>

#include "bgmonitor.h"

// Constants used (should they be placed somewhere?)
// Size of monitor image: 200x186
// Geometry of "display" part of monitor image: (23,14)-[151x115]

//BEGIN class BGMonitorArrangement
BGMonitorArrangement::BGMonitorArrangement(TQWidget *parent, const char *name)
    : TQWidget(parent, name)
{
    m_pBGMonitor.resize( TQApplication::desktop()->numScreens(), 0l );
    
    for (int screen = 0; screen < TQApplication::desktop()->numScreens(); ++screen)
    {
        BGMonitorLabel * label = new BGMonitorLabel(this);
        m_pBGMonitor[screen] = label;
        
        connect( label->monitor(), TQT_SIGNAL(imageDropped(const TQString &)), this, TQT_SIGNAL(imageDropped(const TQString &)) );
    }
    
    parent->setFixedSize(200, 186);
    setFixedSize(200, 186);
    updateArrangement();
}


BGMonitor * BGMonitorArrangement::monitor( unsigned screen ) const
{
    return m_pBGMonitor[screen]->monitor();
}


TQRect BGMonitorArrangement::expandToPreview( TQRect r ) const
{
    double scaleX = 200.0 / 151.0;
    double scaleY = 186.0 / 115.0;
    return TQRect( int(r.x()*scaleX), int(r.y()*scaleY), int(r.width()*scaleX), int(r.height()*scaleY) );
}


TQSize BGMonitorArrangement::expandToPreview( TQSize s ) const
{
    double scaleX = 200.0 / 151.0;
    double scaleY = 186.0 / 115.0;
    return TQSize( int(s.width()*scaleX), int(s.height()*scaleY) );
}


TQPoint BGMonitorArrangement::expandToPreview( TQPoint p ) const
{
    double scaleX = 200.0 / 151.0;
    double scaleY = 186.0 / 115.0;
    return TQPoint( int(p.x()*scaleX), int(p.y()*scaleY) );
}


void BGMonitorArrangement::updateArrangement()
{
    // In this function, sizes, etc have a normal value, and their "expanded"
    // value. The expanded value is used for setting the size of the monitor
    // image that contains the preview of the background. The monitor image
    // will set the background preview back to the normal value.
    
    TQRect overallGeometry;
    for (int screen = 0; screen < TQApplication::desktop()->numScreens(); ++screen)
        overallGeometry |= TQApplication::desktop()->screenGeometry(screen);
    
    TQRect expandedOverallGeometry = expandToPreview(overallGeometry);
    
    double scale = QMIN(
                double(width()) / double(expandedOverallGeometry.width()),
                double(height()) / double(expandedOverallGeometry.height())
                       );
    
    m_combinedPreviewSize = overallGeometry.size() * scale;
    
    m_maxPreviewSize = TQSize(0,0);
    int previousMax = 0;
    
    for (int screen = 0; screen < TQApplication::desktop()->numScreens(); ++screen)
    {
        TQPoint topLeft = (TQApplication::desktop()->screenGeometry(screen).topLeft() - overallGeometry.topLeft()) * scale;
        TQPoint expandedTopLeft = expandToPreview(topLeft);
        
        TQSize previewSize = TQApplication::desktop()->screenGeometry(screen).size() * scale;
        TQSize expandedPreviewSize = expandToPreview(previewSize);
        
        if ( (previewSize.width() * previewSize.height()) > previousMax )
        {
            previousMax = previewSize.width() * previewSize.height();
            m_maxPreviewSize = previewSize;
        }
        
        m_pBGMonitor[screen]->setPreviewPosition( TQRect( topLeft, previewSize ) );
        m_pBGMonitor[screen]->setGeometry( TQRect( expandedTopLeft, expandedPreviewSize ) );
        m_pBGMonitor[screen]->updateMonitorGeometry();
    }
}


void BGMonitorArrangement::resizeEvent( TQResizeEvent * e )
{
    TQWidget::resizeEvent(e);
    updateArrangement();
}


void BGMonitorArrangement::setPixmap( const KPixmap & pm )
{
    for (unsigned screen = 0; screen < m_pBGMonitor.size(); ++screen)
    {
        TQRect position = m_pBGMonitor[screen]->previewPosition();
        
        TQPixmap monitorPixmap( position.size(), pm.depth() );
        copyBlt( &monitorPixmap, 0, 0, &pm, position.x(), position.y(), position.width(), position.height() );
        m_pBGMonitor[screen]->monitor()->setPixmap(monitorPixmap);
    }
}
//END class BGMonitorArrangement



//BEGIN class BGMonitorLabel
BGMonitorLabel::BGMonitorLabel(TQWidget *parent, const char *name)
    : TQLabel(parent, name)
{
    setAlignment(AlignCenter);
    setScaledContents(true);
    setPixmap( TQPixmap( locate("data",  "kcontrol/pics/monitor.png") ) );
    m_pBGMonitor = new BGMonitor(this);
    
    TQWhatsThis::add( this, i18n("This picture of a monitor contains a preview of what the current settings will look like on your desktop.") );
}


void BGMonitorLabel::updateMonitorGeometry()
{
    double scaleX = double(width()) / double(sizeHint().width());
    double scaleY = double(height()) / double(sizeHint().height());
    
    kdDebug() << k_funcinfo << " Setting geometry to " << TQRect( int(23*scaleX), int(14*scaleY), int(151*scaleX), int(115*scaleY) ) << endl;
    m_pBGMonitor->setGeometry( int(23*scaleX), int(14*scaleY), int(151*scaleX), int(115*scaleY) );
}


void BGMonitorLabel::resizeEvent( TQResizeEvent * e )
{
    TQWidget::resizeEvent(e);
    updateMonitorGeometry();
}
//END class BGMonitorLabel



//BEGIN class BGMonitor
BGMonitor::BGMonitor(TQWidget *parent, const char *name)
    : TQLabel(parent, name)
{
    setAlignment(AlignCenter);
    setScaledContents(true);
    setAcceptDrops(true);
}


void BGMonitor::dropEvent(TQDropEvent *e)
{
    if (!KURLDrag::canDecode(e))
        return;

    KURL::List uris;
    if (KURLDrag::decode(e, uris) && (uris.count() > 0)) {
        // TODO: Download remote file
        if (uris.first().isLocalFile())
           emit imageDropped(uris.first().path());
    }
}

void BGMonitor::dragEnterEvent(TQDragEnterEvent *e)
{
    if (KURLDrag::canDecode(e))
        e->accept(rect());
    else
        e->ignore(rect());
}
//END class BGMonitor

#include "bgmonitor.moc"
