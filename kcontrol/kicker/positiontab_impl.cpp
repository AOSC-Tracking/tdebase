/*
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 *  Copyright (c) 2002 Aaron Seigo <aseigo@olympusproject.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#include <stdlib.h>

#include <tqbuttongroup.h>
#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqslider.h>
#include <tqtooltip.h>
#include <tqtimer.h>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>
#include <kpanelextension.h>
#include <kpixmap.h>
#include <kstandarddirs.h>
#include <kwin.h>

#include "main.h"
#include "../background/bgrender.h"

#include "positiontab_impl.h"
#include "positiontab_impl.moc"


// magic numbers for the preview widget layout
extern const int offsetX = 23;
extern const int offsetY = 14;
extern const int maxX = 150;
extern const int maxY = 114;
extern const int margin = 1;

PositionTab::PositionTab(TQWidget *parent, const char* name)
  : PositionTabBase(parent, name),
    m_pretendPanel(0),
    m_desktopPreview(0),
    m_panelInfo(0),
    m_panelPos(PosBottom),
    m_panelAlign(AlignLeft)
{
    TQPixmap monitor(locate("data", "kcontrol/pics/monitor.png"));
    m_monitorImage->setPixmap(monitor);
    m_monitorImage->setFixedSize(m_monitorImage->sizeHint());

    m_pretendDesktop = new TQWidget(m_monitorImage, "pretendBG");
    m_pretendDesktop->setGeometry(offsetX, offsetY, maxX, maxY);
    m_pretendPanel = new TQFrame(m_monitorImage, "pretendPanel");
    m_pretendPanel->setGeometry(offsetX + margin, maxY + offsetY - 10,
                                maxX - margin, 10 - margin);
    m_pretendPanel->setFrameShape(TQFrame::MenuBarPanel);

    /*
     * set the tooltips on the buttons properly for RTL langs
     */
    if (kapp->reverseLayout())
    {
        TQToolTip::add(locationTopRight,     i18n("Top left"));
        TQToolTip::add(locationTop,          i18n("Top center"));
        TQToolTip::add(locationTopLeft,      i18n("Top right" ) );
        TQToolTip::add(locationRightTop,     i18n("Left top"));
        TQToolTip::add(locationRight,        i18n("Left center"));
        TQToolTip::add(locationRightBottom,  i18n("Left bottom"));
        TQToolTip::add(locationBottomRight,  i18n("Bottom left"));
        TQToolTip::add(locationBottom,       i18n("Bottom center"));
        TQToolTip::add(locationBottomLeft,   i18n("Bottom right"));
        TQToolTip::add(locationLeftTop,      i18n("Right top"));
        TQToolTip::add(locationLeft,         i18n("Right center"));
        TQToolTip::add(locationLeftBottom,   i18n("Right bottom"));
    }
    else
    {
        TQToolTip::add(locationTopLeft,      i18n("Top left"));
        TQToolTip::add(locationTop,          i18n("Top center"));
        TQToolTip::add(locationTopRight,     i18n("Top right" ) );
        TQToolTip::add(locationLeftTop,      i18n("Left top"));
        TQToolTip::add(locationLeft,         i18n("Left center"));
        TQToolTip::add(locationLeftBottom,   i18n("Left bottom"));
        TQToolTip::add(locationBottomLeft,   i18n("Bottom left"));
        TQToolTip::add(locationBottom,       i18n("Bottom center"));
        TQToolTip::add(locationBottomRight,  i18n("Bottom right"));
        TQToolTip::add(locationRightTop,     i18n("Right top"));
        TQToolTip::add(locationRight,        i18n("Right center"));
        TQToolTip::add(locationRightBottom,  i18n("Right bottom"));
    }

    // connections
    connect(m_locationGroup, TQT_SIGNAL(clicked(int)), TQT_SIGNAL(changed()));
    connect(m_xineramaScreenComboBox, TQT_SIGNAL(highlighted(int)), TQT_SIGNAL(changed()));

    connect(m_identifyButton,TQT_SIGNAL(clicked()),TQT_SLOT(showIdentify()));

    for(int s=0; s < TQApplication::desktop()->numScreens(); s++)
    {   /* populate the combobox for the available screens */
        m_xineramaScreenComboBox->insertItem(TQString::number(s+1));
    }
    m_xineramaScreenComboBox->insertItem(i18n("All Screens"));

    // hide the xinerama chooser widgets if there is no need for them
    if (TQApplication::desktop()->numScreens() < 2)
    {
        m_identifyButton->hide();
        m_xineramaScreenComboBox->hide();
        m_xineramaScreenLabel->hide();
    }

    connect(m_percentSlider, TQT_SIGNAL(valueChanged(int)), TQT_SIGNAL(changed()));
    connect(m_percentSpinBox, TQT_SIGNAL(valueChanged(int)), TQT_SIGNAL(changed()));
    connect(m_expandCheckBox, TQT_SIGNAL(clicked()), TQT_SIGNAL(changed()));

    connect(m_sizeGroup, TQT_SIGNAL(clicked(int)), TQT_SIGNAL(changed()));
    connect(m_customSlider, TQT_SIGNAL(valueChanged(int)), TQT_SIGNAL(changed()));
    connect(m_customSpinbox, TQT_SIGNAL(valueChanged(int)), TQT_SIGNAL(changed()));

    m_desktopPreview = new KVirtualBGRenderer(0);
    connect(m_desktopPreview, TQT_SIGNAL(imageDone(int)),
            TQT_SLOT(slotBGPreviewReady(int)));

    connect(KickerConfig::the(), TQT_SIGNAL(extensionInfoChanged()),
            TQT_SLOT(infoUpdated()));
    connect(KickerConfig::the(), TQT_SIGNAL(extensionAdded(ExtensionInfo*)),
            TQT_SLOT(extensionAdded(ExtensionInfo*)));
    connect(KickerConfig::the(), TQT_SIGNAL(extensionRemoved(ExtensionInfo*)),
            TQT_SLOT(extensionRemoved(ExtensionInfo*)));
    connect(KickerConfig::the(), TQT_SIGNAL(extensionChanged(const TQString&)),
            TQT_SLOT(extensionChanged(const TQString&)));
    connect(KickerConfig::the(), TQT_SIGNAL(extensionAboutToChange(const TQString&)),
            TQT_SLOT(extensionAboutToChange(const TQString&)));
    // position tab tells hiding tab about extension selections and vice versa
    connect(KickerConfig::the(), TQT_SIGNAL(hidingPanelChanged(int)),
            TQT_SLOT(jumpToPanel(int)));
    connect(m_panelList, TQT_SIGNAL(activated(int)),
            KickerConfig::the(), TQT_SIGNAL(positionPanelChanged(int)));

    connect(m_panelSize, TQT_SIGNAL(activated(int)),
            TQT_SLOT(sizeChanged(int)));
    connect(m_panelSize, TQT_SIGNAL(activated(int)),
            TQT_SIGNAL(changed()));
}

PositionTab::~PositionTab()
{
    delete m_desktopPreview;
}

void PositionTab::load()
{
    m_panelInfo = 0;
    KickerConfig::the()->populateExtensionInfoList(m_panelList);
    m_panelsGroupBox->setHidden(m_panelList->count() < 2);

    switchPanel(KickerConfig::the()->currentPanelIndex());
    m_desktopPreview->setPreview(m_pretendDesktop->size());
    m_desktopPreview->start();
}

void PositionTab::extensionAdded(ExtensionInfo* info)
{
    m_panelList->insertItem(info->_name);
    m_panelsGroupBox->setHidden(m_panelList->count() < 2);
}

void PositionTab::save()
{
    storeInfo();
    KickerConfig::the()->saveExtentionInfo();
}

void PositionTab::defaults()
{
    m_panelPos= PosBottom; // bottom of the screen
    m_percentSlider->setValue( 100 ); // use all space available
    m_percentSpinBox->setValue( 100 ); // use all space available
    m_expandCheckBox->setChecked( true ); // expand as required
    m_xineramaScreenComboBox->setCurrentItem(TQApplication::desktop()->primaryScreen());

    if (TQApplication::reverseLayout())
    {
        // RTL lang aligns right
        m_panelAlign = AlignRight;
    }
    else
    {
        // everyone else aligns left
        m_panelAlign = AlignLeft;
    }

    m_panelSize->setCurrentItem(KPanelExtension::SizeSmall);

    // update the magic drawing
    lengthenPanel(-1);
    switchPanel(KickerConfig::the()->currentPanelIndex());
}

void PositionTab::sizeChanged(int which)
{
    bool custom = which == KPanelExtension::SizeCustom;
    m_customSlider->setEnabled(custom);
    m_customSpinbox->setEnabled(custom);
}

void PositionTab::movePanel(int whichButton)
{
    TQPushButton* pushed = reinterpret_cast<TQPushButton*>(m_locationGroup->find(whichButton));

    if (pushed == locationTopLeft)
    {
	if (!(m_panelInfo->_allowedPosition[PosTop])) 
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = kapp->reverseLayout() ? AlignRight : AlignLeft;
        m_panelPos = PosTop;
    }
    else if (pushed == locationTop)
    {
	if (!(m_panelInfo->_allowedPosition[PosTop]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignCenter;
        m_panelPos = PosTop;
    }
    else if (pushed == locationTopRight)
    {
	if (!(m_panelInfo->_allowedPosition[PosTop]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = kapp->reverseLayout() ? AlignLeft : AlignRight;
        m_panelPos = PosTop;
    }
    else if (pushed == locationLeftTop)
    {
	if (!(m_panelInfo->_allowedPosition[kapp->reverseLayout() ? PosRight : PosLeft]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignLeft;
        m_panelPos = kapp->reverseLayout() ? PosRight : PosLeft;
    }
    else if (pushed == locationLeft)
    {
	if (!(m_panelInfo->_allowedPosition[kapp->reverseLayout() ? PosRight : PosLeft]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignCenter;
        m_panelPos = kapp->reverseLayout() ? PosRight : PosLeft;
    }
    else if (pushed == locationLeftBottom)
    {
	if (!(m_panelInfo->_allowedPosition[kapp->reverseLayout() ? PosRight : PosLeft]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignRight;
        m_panelPos = kapp->reverseLayout() ? PosRight : PosLeft;
    }
    else if (pushed == locationBottomLeft)
    {
	if (!(m_panelInfo->_allowedPosition[PosBottom]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = kapp->reverseLayout() ? AlignRight : AlignLeft;
        m_panelPos = PosBottom;
    }
    else if (pushed == locationBottom)
    {
	if (!(m_panelInfo->_allowedPosition[PosBottom]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignCenter;
        m_panelPos = PosBottom;
    }
    else if (pushed == locationBottomRight)
    {
	if (!(m_panelInfo->_allowedPosition[PosBottom]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = kapp->reverseLayout() ? AlignLeft : AlignRight;
        m_panelPos = PosBottom;
    }
    else if (pushed == locationRightTop)
    {
	if (!(m_panelInfo->_allowedPosition[kapp->reverseLayout() ? PosLeft : PosRight]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignLeft;
        m_panelPos = kapp->reverseLayout() ? PosLeft : PosRight;
    }
    else if (pushed == locationRight)
    {
	if (!(m_panelInfo->_allowedPosition[kapp->reverseLayout() ? PosLeft : PosRight]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignCenter;
        m_panelPos = kapp->reverseLayout() ? PosLeft : PosRight;
    }
    else if (pushed == locationRightBottom)
    {
	if (!(m_panelInfo->_allowedPosition[kapp->reverseLayout() ? PosLeft : PosRight]))
	{
	   setPositionButtons();
	   return;
	}
        m_panelAlign = AlignRight;
        m_panelPos = kapp->reverseLayout() ? PosLeft : PosRight;
    }

    lengthenPanel(-1);
    emit panelPositionChanged(m_panelPos);
}

void PositionTab::lengthenPanel(int sizePercent)
{
    if (sizePercent < 0)
    {
        sizePercent = m_percentSlider->value();
    }

    unsigned int x(0), y(0), x2(0), y2(0);
    unsigned int diff = 0;
    unsigned int panelSize = 4;

    switch (m_panelSize->currentItem())
    {
        case KPanelExtension::SizeTiny:
        case KPanelExtension::SizeSmall:
            panelSize = panelSize * 3 / 2;
            break;
        case KPanelExtension::SizeNormal:
            panelSize *= 2;
            break;
        case KPanelExtension::SizeLarge:
            panelSize = panelSize * 5 / 2;
            break;
        default:
            panelSize = panelSize * m_customSlider->value() / 24;
            break;
    }

    switch (m_panelPos)
    {
        case PosTop:
            x  = offsetX + margin;
            x2 = maxX - margin;
            y  = offsetY + margin;
            y2 = panelSize;

            diff =  x2 - ((x2 * sizePercent) / 100);
            if (m_panelAlign == AlignLeft)
            {
                x2  -= diff;
            }
            else if (m_panelAlign == AlignCenter)
            {
                x  += diff / 2;
                x2 -= diff;
            }
            else // m_panelAlign == AlignRight
            {
                x  += diff;
                x2 -= diff;
            }
            break;
        case PosLeft:
            x  = offsetX + margin;
            x2 = panelSize;
            y  = offsetY + margin;
            y2 = maxY - margin;

            diff =  y2 - ((y2 * sizePercent) / 100);
            if (m_panelAlign == AlignLeft)
            {
                y2  -= diff;
            }
            else if (m_panelAlign == AlignCenter)
            {
                y  += diff / 2;
                y2 -= diff;
            }
            else // m_panelAlign == AlignRight
            {
                y  += diff;
                y2 -= diff;
            }
            break;
        case PosBottom:
            x  = offsetX + margin;
            x2 = maxX - margin;
            y  = offsetY + maxY - panelSize;
            y2 = panelSize;

            diff =  x2 - ((x2 * sizePercent) / 100);
            if (m_panelAlign == AlignLeft)
            {
                x2  -= diff;
            }
            else if (m_panelAlign == AlignCenter)
            {
                x  += diff / 2;
                x2 -= diff;
            }
            else // m_panelAlign == AlignRight
            {
                x  += diff;
                x2 -= diff;
            }
            break;
        default: // case PosRight:
            x  = offsetX + maxX - panelSize;
            x2 = panelSize;
            y  = offsetY + margin;
            y2 = maxY - margin;

            diff =  y2 - ((y2 * sizePercent) / 100);
            if (m_panelAlign == AlignLeft)
            {
                y2  -= diff;
            }
            else if (m_panelAlign == AlignCenter)
            {
                y  += diff / 2;
                y2 -= diff;
            }
            else // m_panelAlign == AlignRight
            {
                y  += diff;
                y2 -= diff;
            }
            break;
    }

    if (x2 < 3)
    {
        x2 = 3;
    }

    if (y2 < 3)
    {
        y2 = 3;
    }

    m_pretendPanel->setGeometry(x, y, x2, y2);
}

void PositionTab::panelDimensionsChanged()
{
    lengthenPanel(-1);
}

void PositionTab::slotBGPreviewReady(int)
{
    m_pretendDesktop->setBackgroundPixmap(m_desktopPreview->pixmap());
#if 0
    KPixmap pm;
    if (TQPixmap::defaultDepth() < 15)
    {
        pm.convertFromImage(*m_desktopPreview->image(), KPixmap::LowColor);
    }
    else
    {
        pm.convertFromImage(*m_desktopPreview->image());
    }

    m_pretendDesktop->setBackgroundPixmap(pm);
#endif
}

void PositionTab::switchPanel(int panelItem)
{
    blockSignals(true);
    ExtensionInfo* panelInfo = (KickerConfig::the()->extensionsInfo())[panelItem];

    if (!panelInfo)
    {
        m_panelList->setCurrentItem(0);
        panelInfo = (KickerConfig::the()->extensionsInfo())[panelItem];

        if (!panelInfo)
        {
            return;
        }
    }

    if (m_panelInfo)
    {
        storeInfo();
    }

    m_panelInfo = panelInfo;

    // because this changes when panels come and go, we have 
    // to be overly pedantic and remove the custom item every time and
    // decide to add it back again, or not
    m_panelSize->removeItem(KPanelExtension::SizeCustom);
    if (m_panelInfo->_customSizeMin != m_panelInfo->_customSizeMax)
    {
        m_panelSize->insertItem(i18n("Custom"), KPanelExtension::SizeCustom);
    }

    if (m_panelInfo->_size >= KPanelExtension::SizeCustom ||
        (!m_panelInfo->_useStdSizes &&
         m_panelInfo->_customSizeMin != m_panelInfo->_customSizeMax)) // compat
    {
        m_panelSize->setCurrentItem(KPanelExtension::SizeCustom);
        sizeChanged(KPanelExtension::SizeCustom);
    }
    else
    {
        m_panelSize->setCurrentItem(m_panelInfo->_size);
        sizeChanged(0);
    }
    m_panelSize->setEnabled(m_panelInfo->_useStdSizes);

    m_customSlider->setMinValue(m_panelInfo->_customSizeMin);
    m_customSlider->setMaxValue(m_panelInfo->_customSizeMax);
    m_customSlider->setTickInterval(m_panelInfo->_customSizeMax / 6);
    m_customSlider->setValue(m_panelInfo->_customSize);
    m_customSpinbox->setMinValue(m_panelInfo->_customSizeMin);
    m_customSpinbox->setMaxValue(m_panelInfo->_customSizeMax);
    m_customSpinbox->setValue(m_panelInfo->_customSize);
    m_sizeGroup->setEnabled(m_panelInfo->_resizeable);
    m_panelPos = m_panelInfo->_position;
    m_panelAlign = m_panelInfo->_tqalignment;
    if(m_panelInfo->_xineramaScreen >= 0 && m_panelInfo->_xineramaScreen < TQApplication::desktop()->numScreens())
        m_xineramaScreenComboBox->setCurrentItem(m_panelInfo->_xineramaScreen);
    else if(m_panelInfo->_xineramaScreen == -2) /* the All Screens option: qt uses -1 for default, so -2 for all */
        m_xineramaScreenComboBox->setCurrentItem(m_xineramaScreenComboBox->count()-1);
    else
        m_xineramaScreenComboBox->setCurrentItem(TQApplication::desktop()->primaryScreen());

    setPositionButtons();

    m_percentSlider->setValue(m_panelInfo->_sizePercentage);
    m_percentSpinBox->setValue(m_panelInfo->_sizePercentage);

    m_expandCheckBox->setChecked(m_panelInfo->_expandSize);

    lengthenPanel(m_panelInfo->_sizePercentage);
    blockSignals(false);
}


void PositionTab::setPositionButtons() {
    if (m_panelPos == PosTop)
    {
        if (m_panelAlign == AlignLeft)
            kapp->reverseLayout() ? locationTopRight->setOn(true) :
                                    locationTopLeft->setOn(true);
        else if (m_panelAlign == AlignCenter)
            locationTop->setOn(true);
        else // if (m_panelAlign == AlignRight
            kapp->reverseLayout() ? locationTopLeft->setOn(true) :
                                    locationTopRight->setOn(true);
    }
    else if (m_panelPos == PosRight)
    {
        if (m_panelAlign == AlignLeft)
            kapp->reverseLayout() ? locationLeftTop->setOn(true) :
                                    locationRightTop->setOn(true);
        else if (m_panelAlign == AlignCenter)
            kapp->reverseLayout() ? locationLeft->setOn(true) :
                                    locationRight->setOn(true);
        else // if (m_panelAlign == AlignRight
            kapp->reverseLayout() ? locationLeftBottom->setOn(true) :
                                    locationRightBottom->setOn(true);
    }
    else if (m_panelPos == PosBottom)
    {
        if (m_panelAlign == AlignLeft)
            kapp->reverseLayout() ? locationBottomRight->setOn(true) :
                                    locationBottomLeft->setOn(true);
        else if (m_panelAlign == AlignCenter)
            locationBottom->setOn(true);
        else // if (m_panelAlign == AlignRight
            kapp->reverseLayout() ? locationBottomLeft->setOn(true) :
                                    locationBottomRight->setOn(true);
    }
    else // if (m_panelPos == PosLeft
    {
        if (m_panelAlign == AlignLeft)
            kapp->reverseLayout() ? locationRightTop->setOn(true) :
                                    locationLeftTop->setOn(true);
        else if (m_panelAlign == AlignCenter)
            kapp->reverseLayout() ? locationRight->setOn(true) :
                                    locationLeft->setOn(true);
        else // if (m_panelAlign == AlignRight
            kapp->reverseLayout() ? locationRightBottom->setOn(true) :
                                    locationLeftBottom->setOn(true);
    }

}

void PositionTab::infoUpdated()
{
    switchPanel(0);
}

void PositionTab::extensionAboutToChange(const TQString& configPath)
{
    ExtensionInfo* extension = (KickerConfig::the()->extensionsInfo())[m_panelList->currentItem()];
    if (extension && extension->_configPath == configPath)
    {
        storeInfo();
    }
}

void PositionTab::extensionChanged(const TQString& configPath)
{
    ExtensionInfo* extension = (KickerConfig::the()->extensionsInfo())[m_panelList->currentItem()];
    if (extension && extension->_configPath == configPath)
    {
        m_panelInfo = 0;
        switchPanel(m_panelList->currentItem());
    }
}

void PositionTab::storeInfo()
{
    if (!m_panelInfo)
    {
        return;
    }

    // Magic numbers stolen from kdebase/kicker/core/global.cpp
    // PGlobal::sizeValue()
    if (m_panelSize->currentItem() < KPanelExtension::SizeCustom)
    {
        m_panelInfo->_size = m_panelSize->currentItem();
    }
    else
    {
        m_panelInfo->_size = KPanelExtension::SizeCustom;
        m_panelInfo->_customSize = m_customSlider->value();
    }

    m_panelInfo->_position = m_panelPos;
    m_panelInfo->_tqalignment = m_panelAlign;
    if(m_xineramaScreenComboBox->currentItem() == m_xineramaScreenComboBox->count()-1)
        m_panelInfo->_xineramaScreen = -2; /* all screens */
    else
        m_panelInfo->_xineramaScreen = m_xineramaScreenComboBox->currentItem();

    m_panelInfo->_sizePercentage = m_percentSlider->value();
    m_panelInfo->_expandSize = m_expandCheckBox->isChecked();
}

void PositionTab::showIdentify()
{
    for(int s=0; s < TQApplication::desktop()->numScreens();s++)
    {

        TQLabel *screenLabel = new TQLabel(0,"Screen Identify", (WFlags)(WDestructiveClose | WStyle_Customize | WX11BypassWM) );

        TQFont identifyFont(KGlobalSettings::generalFont());
        identifyFont.setPixelSize(100);
        screenLabel->setFont(identifyFont);

        screenLabel->setFrameStyle(TQFrame::Panel);
        screenLabel->setFrameShadow(TQFrame::Plain);

        screenLabel->setAlignment(Qt::AlignCenter);
        screenLabel->setNum(s + 1);
        // BUGLET: we should not allow the identification to be entered again
        //         until the timer fires.
        TQTimer::singleShot(1500, screenLabel, TQT_SLOT(close()));

        TQPoint screenCenter(TQApplication::desktop()->screenGeometry(s).center());
        TQRect targetGeometry(TQPoint(0,0),screenLabel->sizeHint());
        targetGeometry.moveCenter(screenCenter);

        screenLabel->setGeometry(targetGeometry);

        screenLabel->show();
    }
}

void PositionTab::extensionRemoved(ExtensionInfo* info)
{
    int count = m_panelList->count();
    int extensionCount = KickerConfig::the()->extensionsInfo().count();
    int index = 0;
    for (; index < count && index < extensionCount; ++index)
    {
        if (KickerConfig::the()->extensionsInfo()[index] == info)
        {
            break;
        }
    }

    bool isCurrentlySelected = index == m_panelList->currentItem();
    m_panelList->removeItem(index);
    m_panelsGroupBox->setHidden(m_panelList->count() < 2);

    if (isCurrentlySelected)
    {
        m_panelList->setCurrentItem(0);
    }
}

void PositionTab::jumpToPanel(int index)
{
    m_panelList->setCurrentItem(index);
    switchPanel(index);
}
