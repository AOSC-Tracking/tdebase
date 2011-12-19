/*****************************************************************

Copyright (c) 2001 Matthias Elter <elter@kde.org>

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

#include <tqlayout.h>
#include <tqtimer.h>
#include <tqwmatrix.h>

#include <dcopclient.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <krootpixmap.h>
#include <kstandarddirs.h>

#include "global.h"
#include "kickerSettings.h"
#include "taskbarcontainer.h"

#include "taskbarextension.h"
#include "taskbarextension.moc"

extern "C"
{
    KDE_EXPORT KPanelExtension* init( TQWidget *parent, const TQString& configFile )
    {
        KGlobal::locale()->insertCatalogue( "taskbarextension" );
   	return new TaskBarExtension( configFile, KPanelExtension::Stretch,
				     KPanelExtension::Preferences, parent, "taskbarextension" );
    }
}

TaskBarExtension::TaskBarExtension(const TQString& configFile, Type type,
                                   int actions, TQWidget *parent, const char *name)
    : KPanelExtension(configFile, type, actions, parent, name),
      m_bgImage(0),
      m_bgFilename(0),
      m_rootPixmap(0)
{
    TQHBoxLayout *layout = new TQHBoxLayout(this);
    m_container = new TaskBarContainer(false, this);
    m_container->setBackgroundOrigin(AncestorOrigin);
    positionChange(position());
    layout->addWidget(m_container);

    connect(m_container, TQT_SIGNAL(containerCountChanged()),
            TQT_SIGNAL(updateLayout()));

    kapp->dcopClient()->setNotifications(true);
    connectDCOPSignal("kicker", "kicker", "configurationChanged()",
                      "configure()", false);

    connect(kapp, TQT_SIGNAL(kdisplayPaletteChanged()),
            TQT_SLOT(setBackgroundTheme()));

    TQTimer::singleShot(0, this, TQT_SLOT(setBackgroundTheme()));
}

TaskBarExtension::~TaskBarExtension()
{
    KGlobal::locale()->removeCatalogue( "taskbarextension" );
}

void TaskBarExtension::positionChange( Position p )
{

    m_container->orientationChange(orientation());

    switch (p)
    {
    case Bottom:
        m_container->popupDirectionChange(KPanelApplet::Up);
        break;
    case Top:
        m_container->popupDirectionChange(KPanelApplet::Down);
        break;
    case Right:
        m_container->popupDirectionChange(KPanelApplet::Left);
        break;
    case Left:
        m_container->popupDirectionChange(KPanelApplet::Right);
        break;
    case Floating:
        if (orientation() == Qt::Horizontal)
        {
            m_container->popupDirectionChange(KPanelApplet::Down);
        }
        else if (TQApplication::reverseLayout())
        {
            m_container->popupDirectionChange(KPanelApplet::Left);
        }
        else
        {
            m_container->popupDirectionChange(KPanelApplet::Right);
        }
        break;
    }
    setBackgroundTheme();
}

void TaskBarExtension::preferences()
{
    m_container->preferences();
}

TQSize TaskBarExtension::sizeHint(Position p, TQSize maxSize) const
{
    if (p == Left || p == Right)
        maxSize.setWidth(sizeInPixels());
    else
        maxSize.setHeight(sizeInPixels());

//    kdDebug(1210) << "TaskBarExtension::sizeHint( Position, TQSize )" << endl;
//    kdDebug(1210) << " width: " << size.width() << endl;
//    kdDebug(1210) << "height: " << size.height() << endl;
    return m_container->sizeHint(p, maxSize);
}

void TaskBarExtension::configure()
{
    setBackgroundTheme();
    update();
}

void TaskBarExtension::setBackgroundTheme()
{
    if (KickerSettings::transparent())
    {
        if (!m_rootPixmap)
        {
            m_rootPixmap = new KRootPixmap(this);
            m_rootPixmap->setCustomPainting(true);
            connect(m_rootPixmap, TQT_SIGNAL(backgroundUpdated(const TQPixmap&)),
                    TQT_SLOT(updateBackground(const TQPixmap&)));
        }
        else
        {
            m_rootPixmap->repaint(true);
        }

        double tint = double(KickerSettings::tintValue()) / 100;
        m_rootPixmap->setFadeEffect(tint, KickerSettings::tintColor());
        m_rootPixmap->start();
        return;
    }
    else if (m_rootPixmap)
    {
        delete m_rootPixmap;
        m_rootPixmap = 0;
    }

    unsetPalette();

    if (KickerSettings::useBackgroundTheme())
    {
        TQString bgFilename = locate("appdata", KickerSettings::backgroundTheme());

        if (m_bgFilename != bgFilename)
        {
            m_bgFilename = bgFilename;
            m_bgImage.load(m_bgFilename);
        }

        if (!m_bgImage.isNull())
        {
            TQImage bgImage = m_bgImage;

            if (orientation() == Qt::Vertical)
            {
                if (KickerSettings::rotateBackground())
                {
                    TQWMatrix matrix;
                    matrix.rotate(position() == KPanelExtension::Left ? 90: 270);
                    bgImage = bgImage.xForm(matrix);
                }

                bgImage = bgImage.scaleWidth(size().width());
            }
            else
            {
                if (position() == KPanelExtension::Top &&
                    KickerSettings::rotateBackground())
                {
                    TQWMatrix matrix;
                    matrix.rotate(180);
                    bgImage = bgImage.xForm(matrix);
                }

                bgImage = bgImage.scaleHeight(size().height());
            }

            if (KickerSettings::colorizeBackground())
            {
                KickerLib::colorize(bgImage);
            }
            setPaletteBackgroundPixmap(TQPixmap(bgImage));
        }
    }
    
    m_container->setBackground();
}

void TaskBarExtension::updateBackground(const TQPixmap& bgImage)
{
    unsetPalette();
    setPaletteBackgroundPixmap(bgImage);
    m_container->setBackground();
}

void TaskBarExtension::resizeEvent(TQResizeEvent *e)
{
    TQFrame::resizeEvent(e);
    setBackgroundTheme();
}

