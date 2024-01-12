/**************************************************************************

    config.cpp  - KPager config dialog
    Copyright (C) 2000  Antonio Larrosa Jimenez

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    Send comments and bug fixes to larrosa@kde.org

***************************************************************************/

#include <tqcheckbox.h>
#include <tqradiobutton.h>
#include <tqbuttongroup.h>
#include <tqwidget.h>
#include <tqvbox.h>
#include <tqhbox.h>
#include <tqlayout.h>

#include <kdialogbase.h>
#include <tdelocale.h>
#include <tdeconfig.h>
#include <kseparator.h>
#include <tdeapplication.h>
#include <kdebug.h>

#include "config.h"
#include "config.moc"
#include "desktop.h"
#include "kpager.h"


KPagerConfigDialog::KPagerConfigDialog (TQWidget *parent)
 : KDialogBase ( parent, "configdialog", true, i18n("Configuration"), Ok|Cancel, Ok, true )
{
    TQVBox *box = new TQVBox( this );
    m_chkWindowDragging=new TQCheckBox(i18n("Enable window dragging"),box,0);
    (void ) new KSeparator( box );
    connect(m_chkWindowDragging, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(enableWindowDragging(bool)));

    TQHBox *page = new TQHBox( box );
    TQVBox *lpage = new TQVBox( page );
    setMainWidget(box);

    m_chkShowName=new TQCheckBox(i18n("Show name"),lpage,0);
    connect(m_chkShowName, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(setShowName(bool)));
    m_chkShowNumber=new TQCheckBox(i18n("Show number"),lpage,0);
    connect(m_chkShowNumber, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(setShowNumber(bool)));
    m_chkShowBackground=new TQCheckBox(i18n("Show background"),lpage,0);
    connect(m_chkShowBackground, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(setShowBackground(bool)));
    m_chkShowWindows=new TQCheckBox(i18n("Show windows"),lpage,0);
    connect(m_chkShowWindows, TQ_SIGNAL(toggled(bool)), this, TQ_SLOT(setShowWindows(bool)));

    m_grpWindowDrawMode=new TQButtonGroup(i18n("Type of Window"),page);
    m_grpWindowDrawMode->setExclusive(true);
    TQVBoxLayout *vbox = new TQVBoxLayout(m_grpWindowDrawMode, KDialog::marginHint(),
					KDialog::spacingHint());
    vbox->addSpacing(fontMetrics().lineSpacing());
    vbox->addWidget(new TQRadioButton(i18n("Plain"),m_grpWindowDrawMode));
    vbox->addWidget(new TQRadioButton(i18n("Icon"),m_grpWindowDrawMode));

    TQRadioButton *rbpix = new TQRadioButton(i18n("Pixmap"),m_grpWindowDrawMode);
//    rbpix->setEnabled(false);
    vbox->addWidget(rbpix);

    connect(m_grpWindowDrawMode, TQ_SIGNAL(clicked(int)), this, TQ_SLOT(setWindowDrawMode(int)));

    m_grpLayoutType=new TQButtonGroup(i18n("Layout"),page);
    m_grpLayoutType->setExclusive(true);
    vbox = new TQVBoxLayout(m_grpLayoutType, KDialog::marginHint(), KDialog::spacingHint());
    vbox->addSpacing(fontMetrics().lineSpacing());
    vbox->addWidget(new TQRadioButton(i18n("Classical"),m_grpLayoutType));
    vbox->addWidget(new TQRadioButton(i18n("Horizontal"),m_grpLayoutType));
    vbox->addWidget(new TQRadioButton(i18n("Vertical"),m_grpLayoutType));

    connect(m_grpLayoutType, TQ_SIGNAL(clicked(int)), this, TQ_SLOT(setLayout(int)));
    connect(this,TQ_SIGNAL(okClicked()),this,TQ_SLOT(slotOk()));
    loadConfiguration();
    setMinimumSize(360, 160);
}

void KPagerConfigDialog::setShowName(bool show)
{
    m_tmpShowName=show;
}

void KPagerConfigDialog::setShowNumber(bool show)
{
    m_tmpShowNumber=show;
}

void KPagerConfigDialog::setShowBackground(bool show)
{
    m_tmpShowBackground=show;
}

void KPagerConfigDialog::setShowWindows(bool show)
{
    m_tmpShowWindows=show;
}

void KPagerConfigDialog::enableWindowDragging(bool enable)
{
    m_tmpWindowDragging = enable;
}

void KPagerConfigDialog::setWindowDrawMode(int type)
{
    m_tmpWindowDrawMode=type;
}

void KPagerConfigDialog::setLayout(int layout)
{
    m_tmpLayoutType=layout;
}

void KPagerConfigDialog::loadConfiguration()
{
    m_chkShowName->setChecked(m_showName);
    m_chkShowNumber->setChecked(m_showNumber);
    m_chkShowBackground->setChecked(m_showBackground);
    m_chkShowWindows->setChecked(m_showWindows);
    m_grpWindowDrawMode->setButton(m_windowDrawMode);
    m_grpLayoutType->setButton(m_layoutType);
    m_chkWindowDragging->setChecked( m_windowDragging );
    m_tmpShowName=m_showName;
    m_tmpShowNumber=m_showNumber;
    m_tmpShowBackground=m_showBackground;
    m_tmpShowWindows=m_showWindows;
    m_tmpWindowDrawMode=m_windowDrawMode;
    m_tmpLayoutType=m_layoutType;
    m_tmpWindowDragging=m_windowDragging;
}

void KPagerConfigDialog::initConfiguration(void)
{
  TDEConfig *cfg= kapp->config();
  cfg->setGroup("KPager");

  m_windowDrawMode=cfg->readNumEntry("windowDrawMode", Desktop::c_defWindowDrawMode);
  m_showName=cfg->readBoolEntry("showName", Desktop::c_defShowName);
  m_showNumber=cfg->readBoolEntry("showNumber", Desktop::c_defShowNumber);
  m_showBackground=cfg->readBoolEntry("showBackground", Desktop::c_defShowBackground);
  m_showWindows=cfg->readBoolEntry("showWindows", Desktop::c_defShowWindows);
  m_layoutType=cfg->readNumEntry("layoutType", KPager::c_defLayout);
  m_windowDragging=cfg->readBoolEntry("windowDragging", true );
}

void KPagerConfigDialog::slotOk()
{
  m_showName=m_tmpShowName;
  m_showNumber=m_tmpShowNumber;
  m_showBackground=m_tmpShowBackground;
  m_showWindows=m_tmpShowWindows;
  m_windowDrawMode=m_tmpWindowDrawMode;
  m_layoutType=m_tmpLayoutType;
  m_windowDragging=m_tmpWindowDragging;  
  accept();
}

bool KPagerConfigDialog::m_showName=Desktop::c_defShowName;
bool KPagerConfigDialog::m_showNumber=Desktop::c_defShowNumber;
bool KPagerConfigDialog::m_showBackground=Desktop::c_defShowBackground;
bool KPagerConfigDialog::m_showWindows=Desktop::c_defShowWindows;
bool KPagerConfigDialog::m_windowDragging=Desktop::c_defWindowDragging;
int  KPagerConfigDialog::m_windowDrawMode=Desktop::c_defWindowDrawMode;
int  KPagerConfigDialog::m_layoutType=KPager::c_defLayout;

