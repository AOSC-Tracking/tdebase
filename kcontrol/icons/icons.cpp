/*
 *
 * This file is part of the KDE project, module kcmdisplay.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 * with minor additions and based on ideas from
 * Torsten Rahn <torsten@kde.org>                                                *
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#include <stdlib.h>

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqgroupbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqslider.h>

#include <tdeapplication.h>
#include <kcolorbutton.h>
#include <tdeconfig.h>
#include <kdebug.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <kipc.h>
#include <tdelocale.h>
#include <kseparator.h>
#include <tdeglobalsettings.h>
#include <dcopclient.h>

#include "icons.h"

/**** TDEIconConfig ****/

TDEIconConfig::TDEIconConfig(TQWidget *parent, const char *name)
    : TDECModule(parent, name)
{

    TQGridLayout *top = new TQGridLayout(this, 4, 2,
                                       KDialog::marginHint(),
                                       KDialog::spacingHint());
    top->setColStretch(0, 1);
    top->setColStretch(1, 1);

    // Use of Icon at (0,0) - (1, 0)
    TQGroupBox *gbox = new TQGroupBox(i18n("Use of Icon"), this);
    top->addMultiCellWidget(gbox, 0, 1, 0, 0);
    TQBoxLayout *g_vlay = new TQVBoxLayout(gbox,
                                        KDialog::marginHint(),
                                        KDialog::spacingHint());
    g_vlay->addSpacing(fontMetrics().lineSpacing());
    mpUsageList = new TQListBox(gbox);
    connect(mpUsageList, TQT_SIGNAL(highlighted(int)), TQT_SLOT(slotUsage(int)));
    g_vlay->addWidget(mpUsageList);

    KSeparator *sep = new KSeparator( KSeparator::HLine, this );
    top->addWidget(sep, 1, 1);
    // Preview at (2,0) - (2, 1)
    TQGridLayout *g_lay = new TQGridLayout(4, 3, KDialog::marginHint(), 0);
    top->addMultiCellLayout(g_lay, 2, 2, 0, 1);
    g_lay->addRowSpacing(0, fontMetrics().lineSpacing());

    TQPushButton *push;

    mPreviewButton1 = addPreviewIcon(0, i18n("Default"), this, g_lay);
    connect(mPreviewButton1, TQT_SIGNAL(clicked()), TQT_SLOT(slotEffectSetup0()));
    mPreviewButton2 = addPreviewIcon(1, i18n("Active"), this, g_lay);
    connect(mPreviewButton2, TQT_SIGNAL(clicked()), TQT_SLOT(slotEffectSetup1()));
    mPreviewButton3 = addPreviewIcon(2, i18n("Disabled"), this, g_lay);
    connect(mPreviewButton3, TQT_SIGNAL(clicked()), TQT_SLOT(slotEffectSetup2()));

    m_pTab1 = new TQWidget(this, "General Tab");
    top->addWidget(m_pTab1, 0, 1);

    TQGridLayout *grid = new TQGridLayout(m_pTab1, 4, 3, 10, 10);
    grid->setColStretch(1, 1);
    grid->setColStretch(2, 1);

    // Size
    TQLabel *lbl = new TQLabel(i18n("Size:"), m_pTab1);
    lbl->setFixedSize(lbl->sizeHint());
    grid->addWidget(lbl, 0, 0, TQt::AlignLeft);
    mpSizeBox = new TQComboBox(m_pTab1);
    connect(mpSizeBox, TQT_SIGNAL(activated(int)), TQT_SLOT(slotSize(int)));
    lbl->setBuddy(mpSizeBox);
    grid->addWidget(mpSizeBox, 0, 1, TQt::AlignLeft);

    mpDPCheck = new TQCheckBox(i18n("Double-sized pixels"), m_pTab1);
    connect(mpDPCheck, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotDPCheck(bool)));
    grid->addMultiCellWidget(mpDPCheck, 1, 1, 0, 1, TQt::AlignLeft);

    mpAnimatedCheck = new TQCheckBox(i18n("Animate icons"), m_pTab1);
    connect(mpAnimatedCheck, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotAnimatedCheck(bool)));
    grid->addMultiCellWidget(mpAnimatedCheck, 2, 2, 0, 1, TQt::AlignLeft);

    mpRoundedCheck = new TQCheckBox(i18n("Rounded text selection"), m_pTab1);
    connect(mpRoundedCheck, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotRoundedCheck(bool)));
    grid->addMultiCellWidget(mpRoundedCheck, 3, 3, 0, 1, TQt::AlignLeft);

    mpActiveEffectCheck = new TQCheckBox(i18n("Show icon activation effect"), m_pTab1);
    connect(mpActiveEffectCheck, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotActiveEffect(bool)));
    grid->addMultiCellWidget(mpActiveEffectCheck, 4, 4, 0, 1, TQt::AlignLeft);

    top->activate();

    mpSystrayConfig = new KSimpleConfig( TQString::fromLatin1( "systemtray_panelappletrc" ));
    mpKickerConfig = new KSimpleConfig( TQString::fromLatin1( "kickerrc" ));

    init();
    read();
    apply();
    preview();
}

TDEIconConfig::~TDEIconConfig()
{
  delete mpSystrayConfig;
  delete mpEffect;
}

TQPushButton *TDEIconConfig::addPreviewIcon(int i, const TQString &str, TQWidget *parent, TQGridLayout *lay)
{
    TQLabel *lab = new TQLabel(str, parent);
    lay->addWidget(lab, 1, i, TQt::AlignCenter);
    mpPreview[i] = new TQLabel(parent);
    mpPreview[i]->setAlignment(AlignCenter);
    mpPreview[i]->setMinimumSize(105, 105);
    lay->addWidget(mpPreview[i], 2, i);
    TQPushButton *push = new TQPushButton(i18n("Set Effect..."), parent);
    lay->addWidget(push, 3, i, TQt::AlignCenter);
    return push;
}

void TDEIconConfig::init()
{
    mpLoader = TDEGlobal::iconLoader();
    mpConfig = TDEGlobal::config();
    mpEffect = new TDEIconEffect;
    mpTheme = mpLoader->theme();
    mUsage = 0;
    for (int i=0; i<TDEIcon::LastGroup; i++)
	mbChanged[i] = false;

    // Fill list/checkboxen
    mpUsageList->insertItem(i18n("Desktop/File Manager"));
    mpUsageList->insertItem(i18n("Toolbar"));
    mpUsageList->insertItem(i18n("Main Toolbar"));
    mpUsageList->insertItem(i18n("Small Icons"));
    mpUsageList->insertItem(i18n("Panel"));
    mpUsageList->insertItem(i18n("All Icons"));
    mpUsageList->insertItem(i18n("Panel Buttons"));
    mpUsageList->insertItem(i18n("System Tray Icons"));

    // For reading the configuration
    mGroups += "Desktop";
    mGroups += "Toolbar";
    mGroups += "MainToolbar";
    mGroups += "Small";
    mGroups += "Panel";

    mStates += "Default";
    mStates += "Active";
    mStates += "Disabled";
}

void TDEIconConfig::initDefaults()
{
    mDefaultEffect[0].type = TDEIconEffect::NoEffect;
    mDefaultEffect[1].type = TDEIconEffect::NoEffect;
    mDefaultEffect[2].type = TDEIconEffect::ToGray;
    mDefaultEffect[0].transparant = false;
    mDefaultEffect[1].transparant = false;
    mDefaultEffect[2].transparant = true;
    mDefaultEffect[0].value = 1.0;
    mDefaultEffect[1].value = 1.0;
    mDefaultEffect[2].value = 1.0;
    mDefaultEffect[0].color = TQColor(144,128,248);
    mDefaultEffect[1].color = TQColor(169,156,255);
    mDefaultEffect[2].color = TQColor(34,202,0);
    mDefaultEffect[0].color2 = TQColor(0,0,0);
    mDefaultEffect[1].color2 = TQColor(0,0,0);
    mDefaultEffect[2].color2 = TQColor(0,0,0);

    const int defDefSizes[] = { 32, 22, 22, 16, 32 };

    TDEIcon::Group i;
    TQStringList::ConstIterator it;
    for(it=mGroups.begin(), i=TDEIcon::FirstGroup; it!=mGroups.end(); ++it, i++)
    {
	mbDP[i] = false;
	mbChanged[i] = true;
	mbAnimated[i] = false;
	if (mpTheme)
	    mSizes[i] = mpTheme->defaultSize(i);
	else
	    mSizes[i] = defDefSizes[i];

	mEffects[i][0] = mDefaultEffect[0];
	mEffects[i][1] = mDefaultEffect[1];
	mEffects[i][2] = mDefaultEffect[2];
    }
    // Animate desktop icons by default
    int group = mGroups.findIndex( "Desktop" );
    if ( group != -1 )
        mbAnimated[group] = true;

    // This is the new default in KDE 2.2, in sync with the kiconeffect of tdelibs Nolden 2001/06/11
    int activeState = mStates.findIndex( "Active" );
    if ( activeState != -1 )
    {
        int group = mGroups.findIndex( "Desktop" );
        if ( group != -1 )
        {
            mEffects[ group ][ activeState ].type = TDEIconEffect::ToGamma;
            mEffects[ group ][ activeState ].value = 0.7;
        }

        group = mGroups.findIndex( "Panel" );
        if ( group != -1 )
        {
            mEffects[ group ][ activeState ].type = TDEIconEffect::ToGamma;
            mEffects[ group ][ activeState ].value = 0.7;
        }
    }
}

void TDEIconConfig::read()
{
    if (mpTheme)
    {
        for (TDEIcon::Group i=TDEIcon::FirstGroup; i<TDEIcon::LastGroup; i++)
            mAvSizes[i] = mpTheme->querySizes(i);

        mTheme = mpTheme->current();
        mExample = mpTheme->example();
    }
    else
    {
        for (TDEIcon::Group i=TDEIcon::FirstGroup; i<TDEIcon::LastGroup; i++)
            mAvSizes[i] = TQValueList<int>();

        mTheme = TQString::null;
        mExample = TQString::null;
    }

    initDefaults();

    int i, j, effect;
    TQStringList::ConstIterator it, it2;
    for (it=mGroups.begin(), i=0; it!=mGroups.end(); ++it, i++)
    {
        mbChanged[i] = false;

	mpConfig->setGroup(*it + "Icons");
	mSizes[i] = mpConfig->readNumEntry("Size", mSizes[i]);
	mbDP[i] = mpConfig->readBoolEntry("DoublePixels", mbDP[i]);
	mbAnimated[i] = mpConfig->readBoolEntry("Animated", mbAnimated[i]);

	for (it2=mStates.begin(), j=0; it2!=mStates.end(); ++it2, j++)
	{
	    TQString tmp = mpConfig->readEntry(*it2 + "Effect");
	    if (tmp == "togray")
		effect = TDEIconEffect::ToGray;
	    else if (tmp == "colorize")
		effect = TDEIconEffect::Colorize;
	    else if (tmp == "togamma")
		effect = TDEIconEffect::ToGamma;
	    else if (tmp == "desaturate")
		effect = TDEIconEffect::DeSaturate;
	    else if (tmp == "tomonochrome")
		effect = TDEIconEffect::ToMonochrome;
	    else if (tmp == "none")
		effect = TDEIconEffect::NoEffect;
	    else continue;
	    mEffects[i][j].type = effect;
	    mEffects[i][j].value = mpConfig->readDoubleNumEntry(*it2 + "Value");
	    mEffects[i][j].color = mpConfig->readColorEntry(*it2 + "Color");
	    mEffects[i][j].color2 = mpConfig->readColorEntry(*it2 + "Color2");
	    mEffects[i][j].transparant = mpConfig->readBoolEntry(*it2 + "SemiTransparent");
	}
    }


    mpSystrayConfig->setGroup("System Tray");
    mSysTraySize = mpSystrayConfig->readNumEntry("systrayIconWidth", 22);

    mpKickerConfig->setGroup("General");
    mQuickLaunchSize = mpKickerConfig->readNumEntry("panelIconWidth", TDEIcon::SizeLarge);

    TDEConfigGroup g( TDEGlobal::config(), "KDE" );
    mpRoundedCheck->setChecked(g.readBoolEntry("IconUseRoundedRect", KDE_DEFAULT_ICONTEXTROUNDED));
    mpActiveEffectCheck->setChecked(g.readBoolEntry("ShowKonqIconActivationEffect", KDE_DEFAULT_KONQ_ACTIVATION_EFFECT));
}

void TDEIconConfig::apply()
{
    int i;

    mpUsageList->setCurrentItem(mUsage);

    if (mpUsageList->currentText() == i18n("Panel Buttons")) {
        mpSizeBox->clear();
        mpSizeBox->insertItem(TQString().setNum(16));
        mpSizeBox->insertItem(TQString().setNum(22));
        mpSizeBox->insertItem(TQString().setNum(32));
        mpSizeBox->insertItem(TQString().setNum(48));
        mpSizeBox->insertItem(TQString().setNum(64));
        mpSizeBox->insertItem(TQString().setNum(128));
        for (i=0;i<(mpSizeBox->count());i++) {
            if (mpSizeBox->text(i) == TQString().setNum(mQuickLaunchSize)) {
                mpSizeBox->setCurrentItem(i);
            }
        }
    }
    else if (mpUsageList->currentText() == i18n("System Tray Icons")) {
        mpSizeBox->clear();
        mpSizeBox->insertItem(TQString().setNum(16));
        mpSizeBox->insertItem(TQString().setNum(22));
        mpSizeBox->insertItem(TQString().setNum(32));
        mpSizeBox->insertItem(TQString().setNum(48));
        mpSizeBox->insertItem(TQString().setNum(64));
        mpSizeBox->insertItem(TQString().setNum(128));
        for (i=0;i<(mpSizeBox->count());i++) {
            if (mpSizeBox->text(i) == TQString().setNum(mSysTraySize)) {
                mpSizeBox->setCurrentItem(i);
            }
        }
    }
    else {
        int delta = 1000, dw, index = -1, size = 0, i;
        TQValueList<int>::Iterator it;
        mpSizeBox->clear();
        if (mUsage < TDEIcon::LastGroup) {
            for (it=mAvSizes[mUsage].begin(), i=0; it!=mAvSizes[mUsage].end(); ++it, i++)
            {
                mpSizeBox->insertItem(TQString().setNum(*it));
                dw = abs(mSizes[mUsage] - *it);
                if (dw < delta)
                {
                    delta = dw;
                    index = i;
                    size = *it;
                }
            }
            if (index != -1)
            {
                mpSizeBox->setCurrentItem(index);
                mSizes[mUsage] = size; // best or exact match
            }
            mpDPCheck->setChecked(mbDP[mUsage]);
            mpAnimatedCheck->setChecked(mbAnimated[mUsage]);
        }
    }
}

void TDEIconConfig::preview(int i)
{
    // Apply effects ourselves because we don't want to sync
    // the configuration every preview.

    int viewedGroup;
    if (mpUsageList->text(mUsage) == i18n("Panel Buttons")) {
        viewedGroup = TDEIcon::FirstGroup;
    }
    else if (mpUsageList->text(mUsage) == i18n("System Tray Icons")) {
        viewedGroup = TDEIcon::FirstGroup;
    }
    else {
        viewedGroup = (mUsage == TDEIcon::LastGroup) ? TDEIcon::FirstGroup : mUsage;
    }

    TQPixmap pm;
    if (mpUsageList->text(mUsage) == i18n("Panel Buttons")) {
        pm = mpLoader->loadIcon(mExample, TDEIcon::NoGroup, mQuickLaunchSize);
    }
    else if (mpUsageList->text(mUsage) == i18n("System Tray Icons")) {
        pm = mpLoader->loadIcon(mExample, TDEIcon::NoGroup, mSysTraySize);
    }
    else {
        pm = mpLoader->loadIcon(mExample, TDEIcon::NoGroup, mSizes[viewedGroup]);
    }
    TQImage img = pm.convertToImage();
    if (mbDP[viewedGroup])
    {
        int w = img.width() * 2;
        img = img.smoothScale(w, w);
    }

    Effect &effect = mEffects[viewedGroup][i];

    img = mpEffect->apply(img, effect.type,
        effect.value, effect.color, effect.color2, effect.transparant);
    pm.convertFromImage(img);
    mpPreview[i]->setPixmap(pm);
}

void TDEIconConfig::preview()
{
    preview(0);
    preview(1);
    preview(2);
}

void TDEIconConfig::load()
{
    load( false );
}

void TDEIconConfig::load( bool useDefaults )
{
    mpConfig = TDEGlobal::config();
    mpConfig->setReadDefaults( useDefaults );
    read();
    apply();
    for (int i=0; i<TDEIcon::LastGroup; i++)
	mbChanged[i] = false;
    preview();
    emit changed( useDefaults );
}


void TDEIconConfig::save()
{
    int i, j;
    TQStringList::ConstIterator it, it2;
    for (it=mGroups.begin(), i=0; it!=mGroups.end(); ++it, i++)
    {
	mpConfig->setGroup(*it + "Icons");
	mpConfig->writeEntry("Size", mSizes[i], true, true);
	mpConfig->writeEntry("DoublePixels", mbDP[i], true, true);
	mpConfig->writeEntry("Animated", mbAnimated[i], true, true);
	for (it2=mStates.begin(), j=0; it2!=mStates.end(); ++it2, j++)
	{
	    TQString tmp;
	    switch (mEffects[i][j].type)
	    {
	    case TDEIconEffect::ToGray:
		tmp = "togray";
		break;
	    case TDEIconEffect::ToGamma:
		tmp = "togamma";
		break;
	    case TDEIconEffect::Colorize:
		tmp = "colorize";
		break;
	    case TDEIconEffect::DeSaturate:
		tmp = "desaturate";
		break;
	    case TDEIconEffect::ToMonochrome:
		tmp = "tomonochrome";
		break;
	    default:
		tmp = "none";
		break;
	    }
	    mpConfig->writeEntry(*it2 + "Effect", tmp, true, true);
	    mpConfig->writeEntry(*it2 + "Value", mEffects[i][j].value, true, true);
            mpConfig->writeEntry(*it2 + "Color", mEffects[i][j].color, true, true);
            mpConfig->writeEntry(*it2 + "Color2", mEffects[i][j].color2, true, true);
            mpConfig->writeEntry(*it2 + "SemiTransparent", mEffects[i][j].transparant, true, true);
	}
    }

    // Reload kicker/systray configuration files; we have no way of knowing if any other parameters changed
    // from initial read to this write request
    mpSystrayConfig->reparseConfiguration();
    mpKickerConfig->reparseConfiguration();

    mpSystrayConfig->setGroup("System Tray");
    mpSystrayConfig->writeEntry("systrayIconWidth", mSysTraySize);
    mpKickerConfig->setGroup("General");
    mpKickerConfig->writeEntry("panelIconWidth", mQuickLaunchSize);

    TDEConfigGroup g( TDEGlobal::config(), "KDE" );
    g.writeEntry("IconUseRoundedRect", mpRoundedCheck->isChecked(), true, true);
    g.writeEntry("ShowKonqIconActivationEffect", mpActiveEffectCheck->isChecked(), true, true);

    kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", TQString("") );
    kapp->dcopClient()->send( "kdesktop", "KDesktopIface", "configure()", TQString("") );

    mpConfig->sync();
    mpSystrayConfig->sync();
    mpKickerConfig->sync();

    emit changed(false);

    // Emit KIPC change message.
    for (int i=0; i<TDEIcon::LastGroup; i++)
    {
	if (mbChanged[i])
	{
	    KIPC::sendMessageAll(KIPC::IconChanged, i);
	    mbChanged[i] = false;
	}
    }

    // Signal kicker to reload icon configuration
    kapp->dcopClient()->send("kicker", "kicker", "configure()", TQByteArray());

    // Signal system tray to reload icon configuration
    kapp->dcopClient()->send("kicker", "SystemTrayApplet", "iconSizeChanged()", TQByteArray());
}

void TDEIconConfig::defaults()
{
    load( true );
}

void TDEIconConfig::QLSizeLockedChanged(bool checked) {
    emit changed();
}

void TDEIconConfig::slotUsage(int index)
{
    mUsage = index;
    if (mpUsageList->text(index) == i18n("Panel Buttons")) {
        mpSizeBox->setEnabled(true);
        mpDPCheck->setEnabled(false);
        mpAnimatedCheck->setEnabled(false);
        mPreviewButton1->setEnabled(false);
        mPreviewButton2->setEnabled(false);
        mPreviewButton3->setEnabled(false);
    }
    else if (mpUsageList->text(index) == i18n("System Tray Icons")) {
        mpSizeBox->setEnabled(true);
        mpDPCheck->setEnabled(false);
        mpAnimatedCheck->setEnabled(false);
        mPreviewButton1->setEnabled(false);
        mPreviewButton2->setEnabled(false);
        mPreviewButton3->setEnabled(false);
    }
    else if ( mUsage == TDEIcon::Panel || mUsage == TDEIcon::LastGroup )
    {
        mpSizeBox->setEnabled(false);
        mpDPCheck->setEnabled(false);
	mpAnimatedCheck->setEnabled( mUsage == TDEIcon::Panel );
        mPreviewButton1->setEnabled(true);
        mPreviewButton2->setEnabled(true);
        mPreviewButton3->setEnabled(true);
    }
    else
    {
        mpSizeBox->setEnabled(true);
        mpDPCheck->setEnabled(true);
	mpAnimatedCheck->setEnabled( mUsage == TDEIcon::Desktop );
        mPreviewButton1->setEnabled(true);
        mPreviewButton2->setEnabled(true);
        mPreviewButton3->setEnabled(true);
    }

    apply();
    preview();
}

void TDEIconConfig::EffectSetup(int state)
{
    int viewedGroup = (mUsage == TDEIcon::LastGroup) ? TDEIcon::FirstGroup : mUsage;

    if (mpUsageList->currentText() == i18n("Panel Buttons")) {
        return;
    }
    if (mpUsageList->currentText() == i18n("System Tray Icons")) {
        return;
    }

    TQPixmap pm = mpLoader->loadIcon(mExample, TDEIcon::NoGroup, mSizes[viewedGroup]);
    TQImage img = pm.convertToImage();
    if (mbDP[viewedGroup])
    {
	int w = img.width() * 2;
	img = img.smoothScale(w, w);
    }

    TQString caption;
    switch (state)
    {
    case 0 : caption = i18n("Setup Default Icon Effect"); break;
    case 1 : caption = i18n("Setup Active Icon Effect"); break;
    case 2 : caption = i18n("Setup Disabled Icon Effect"); break;
    }

    TDEIconEffectSetupDialog dlg(mEffects[viewedGroup][state], mDefaultEffect[state], caption, img);

    if (dlg.exec() == TQDialog::Accepted)
    {
        if (mUsage == TDEIcon::LastGroup) {
            for (int i=0; i<TDEIcon::LastGroup; i++)
                mEffects[i][state] = dlg.effect();
        } else {
            mEffects[mUsage][state] = dlg.effect();
        }

        // AK - can this call be moved therefore removing
        //      code duplication?

        emit changed(true);

        if (mUsage == TDEIcon::LastGroup) {
            for (int i=0; i<TDEIcon::LastGroup; i++)
                mbChanged[i] = true;
        } else {
            mbChanged[mUsage] = true;
        }
    }
    preview(state);
}

void TDEIconConfig::slotSize(int index)
{
    if (mpUsageList->currentText() == i18n("Panel Buttons")) {
        mQuickLaunchSize = mpSizeBox->currentText().toInt();
        preview();
        emit changed(true);
    }
    else if (mpUsageList->currentText() == i18n("System Tray Icons")) {
        mSysTraySize = mpSizeBox->currentText().toInt();
        preview();
        emit changed(true);
    }
    else {
        Q_ASSERT(mUsage < TDEIcon::LastGroup);
        mSizes[mUsage] = mAvSizes[mUsage][index];
        preview();
        emit changed(true);
        mbChanged[mUsage] = true;
    }
}

void TDEIconConfig::slotDPCheck(bool check)
{
    Q_ASSERT(mUsage < TDEIcon::LastGroup);
    if (mbDP[mUsage] != check)
    {
        mbDP[mUsage] = check;
        emit changed(true);
        mbChanged[mUsage] = true;
    }
    preview();

}

void TDEIconConfig::slotAnimatedCheck(bool check)
{
    Q_ASSERT(mUsage < TDEIcon::LastGroup);
    if (mbAnimated[mUsage] != check)
    {
        mbAnimated[mUsage] = check;
        emit changed(true);
        mbChanged[mUsage] = true;
    }
}

void TDEIconConfig::slotRoundedCheck(bool check)
{
    emit changed(true);
}

void TDEIconConfig::slotActiveEffect(bool check)
{
    emit changed(true);
}

TDEIconEffectSetupDialog::TDEIconEffectSetupDialog(const Effect &effect,
    const Effect &defaultEffect,
    const TQString &caption, const TQImage &image,
    TQWidget *parent, char *name)
    : KDialogBase(parent, name, true, caption,
	Default|Ok|Cancel, Ok, true),
      mEffect(effect),
      mDefaultEffect(defaultEffect),
      mExample(image)
{
    mpEffect = new TDEIconEffect;

    TQLabel *lbl;
    TQGroupBox *frame;
    TQGridLayout *grid;

    TQWidget *page = new TQWidget(this);
    setMainWidget(page);

    TQGridLayout *top = new TQGridLayout(page, 4, 2, 0, spacingHint());
    top->setColStretch(0,1);
    top->addColSpacing(1,10);
    top->setColStretch(2,2);
    top->setRowStretch(1,1);

    lbl = new TQLabel(i18n("&Effect:"), page);
    lbl->setFixedSize(lbl->sizeHint());
    top->addWidget(lbl, 0, 0, TQt::AlignLeft);
    mpEffectBox = new TQListBox(page);
    mpEffectBox->insertItem(i18n("No Effect"));
    mpEffectBox->insertItem(i18n("To Gray"));
    mpEffectBox->insertItem(i18n("Colorize"));
    mpEffectBox->insertItem(i18n("Gamma"));
    mpEffectBox->insertItem(i18n("Desaturate"));
    mpEffectBox->insertItem(i18n("To Monochrome"));
    mpEffectBox->setMinimumWidth( 100 );
    connect(mpEffectBox, TQT_SIGNAL(highlighted(int)), TQT_SLOT(slotEffectType(int)));
    top->addMultiCellWidget(mpEffectBox, 1, 2, 0, 0, TQt::AlignLeft);
    lbl->setBuddy(mpEffectBox);

    mpSTCheck = new TQCheckBox(i18n("&Semi-transparent"), page);
    connect(mpSTCheck, TQT_SIGNAL(toggled(bool)), TQT_SLOT(slotSTCheck(bool)));
    top->addWidget(mpSTCheck, 3, 0, TQt::AlignLeft);

    frame = new TQGroupBox(i18n("Preview"), page);
    top->addMultiCellWidget(frame, 0, 1, 1, 1);
    grid = new TQGridLayout(frame, 2, 1, marginHint(), spacingHint());
    grid->addRowSpacing(0, fontMetrics().lineSpacing());
    grid->setRowStretch(1, 1);

    mpPreview = new TQLabel(frame);
    mpPreview->setAlignment(AlignCenter);
    mpPreview->setMinimumSize(105, 105);
    grid->addWidget(mpPreview, 1, 0);

    mpEffectGroup = new TQGroupBox(i18n("Effect Parameters"), page);
    top->addMultiCellWidget(mpEffectGroup, 2, 3, 1, 1);
    grid = new TQGridLayout(mpEffectGroup, 3, 2, marginHint(), spacingHint());
    grid->addRowSpacing(0, fontMetrics().lineSpacing());

    mpEffectLabel = new TQLabel(i18n("&Amount:"), mpEffectGroup);
    grid->addWidget(mpEffectLabel, 1, 0);
    mpEffectSlider = new TQSlider(0, 100, 5, 10, TQt::Horizontal, mpEffectGroup);
    mpEffectLabel->setBuddy( mpEffectSlider );
    connect(mpEffectSlider, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(slotEffectValue(int)));
    grid->addWidget(mpEffectSlider, 1, 1);

    mpEffectColor = new TQLabel(i18n("Co&lor:"), mpEffectGroup);
    grid->addWidget(mpEffectColor, 2, 0);
    mpEColButton = new KColorButton(mpEffectGroup);
    mpEffectColor->setBuddy( mpEColButton );
    connect(mpEColButton, TQT_SIGNAL(changed(const TQColor &)),
		TQT_SLOT(slotEffectColor(const TQColor &)));
    grid->addWidget(mpEColButton, 2, 1);

    mpEffectColor2 = new TQLabel(i18n("&Second color:"), mpEffectGroup);
    grid->addWidget(mpEffectColor2, 3, 0);
    mpECol2Button = new KColorButton(mpEffectGroup);
    mpEffectColor2->setBuddy( mpECol2Button );
    connect(mpECol2Button, TQT_SIGNAL(changed(const TQColor &)),
		TQT_SLOT(slotEffectColor2(const TQColor &)));
    grid->addWidget(mpECol2Button, 3, 1);

    init();
    preview();
}

TDEIconEffectSetupDialog::~TDEIconEffectSetupDialog()
{
  delete mpEffect;
}

void TDEIconEffectSetupDialog::init()
{
    mpEffectBox->setCurrentItem(mEffect.type);
    mpEffectSlider->setEnabled(mEffect.type != TDEIconEffect::NoEffect);
    mpEColButton->setEnabled(mEffect.type == TDEIconEffect::Colorize || mEffect.type == TDEIconEffect::ToMonochrome);
    mpECol2Button->setEnabled(mEffect.type == TDEIconEffect::ToMonochrome);
    mpEffectSlider->setValue((int) (100.0 * mEffect.value + 0.5));
    mpEColButton->setColor(mEffect.color);
    mpECol2Button->setColor(mEffect.color2);
    mpSTCheck->setChecked(mEffect.transparant);
}

void TDEIconEffectSetupDialog::slotEffectValue(int value)
{
     mEffect.value = 0.01 * value;
     preview();
}

void TDEIconEffectSetupDialog::slotEffectColor(const TQColor &col)
{
     mEffect.color = col;
     preview();
}

void TDEIconEffectSetupDialog::slotEffectColor2(const TQColor &col)
{
     mEffect.color2 = col;
     preview();
}

void TDEIconEffectSetupDialog::slotEffectType(int type)
{
    mEffect.type = type;
    mpEffectGroup->setEnabled(mEffect.type != TDEIconEffect::NoEffect);
    mpEffectSlider->setEnabled(mEffect.type != TDEIconEffect::NoEffect);
    mpEffectColor->setEnabled(mEffect.type == TDEIconEffect::Colorize || mEffect.type == TDEIconEffect::ToMonochrome);
    mpEColButton->setEnabled(mEffect.type == TDEIconEffect::Colorize || mEffect.type == TDEIconEffect::ToMonochrome);
    mpEffectColor2->setEnabled(mEffect.type == TDEIconEffect::ToMonochrome);
    mpECol2Button->setEnabled(mEffect.type == TDEIconEffect::ToMonochrome);
    preview();
}

void TDEIconEffectSetupDialog::slotSTCheck(bool b)
{
     mEffect.transparant = b;
     preview();
}

void TDEIconEffectSetupDialog::slotDefault()
{
     mEffect = mDefaultEffect;
     init();
     preview();
}

void TDEIconEffectSetupDialog::preview()
{
    TQPixmap pm;
    TQImage img = mExample.copy();
    img = mpEffect->apply(img, mEffect.type,
          mEffect.value, mEffect.color, mEffect.color2, mEffect.transparant);
    pm.convertFromImage(img);
    mpPreview->setPixmap(pm);
}

#include "icons.moc"
