/*
 *
 * This file is part of the KDE project, module kcmdisplay.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 * with minor additions and based on ideas from
 * Torsten Rahn <torsten@kde.org>
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#ifndef __icons_h__
#define __icons_h__

#include <tqcolor.h>
#include <tqimage.h>
#include <tqvaluelist.h>

#include <tdecmodule.h>
#include <kdialogbase.h>
#include <ksimpleconfig.h>

class TQCheckBox;
class TQColor;
class TQComboBox;
class TQGridLayout;
class TQGroupBox;
class TQIconView;
class TQLabel;
class TQListBox;
class TQListView;
class TQPushButton;
class TQSlider;
class TQTabWidget;
class TQWidget;

class KColorButton;
class TDEConfig;
class TDEIconEffect;
class TDEIconLoader;
class TDEIconTheme;

struct Effect 
{
    int type;
    float value;
    TQColor color;
    TQColor color2;
    bool transparant;
};


/**
 * The General Icons tab in kcontrol.
 */
class TDEIconConfig: public TDECModule
{
    TQ_OBJECT

public:
    TDEIconConfig(TQWidget *parent, const char *name=0);
    ~TDEIconConfig();

    virtual void load();
    virtual void load( bool useDefaults );
    virtual void save();
    virtual void defaults();
    void preview();

private slots:
    void slotEffectSetup0() { EffectSetup(0); }
    void slotEffectSetup1() { EffectSetup(1); }
    void slotEffectSetup2() { EffectSetup(2); }
    
    void slotUsage(int index);
    void slotSize(int index);
    void slotDPCheck(bool check);
    void slotAnimatedCheck(bool check);
    void slotRoundedCheck(bool check);
    void slotActiveEffect(bool check);
    void QLSizeLockedChanged(bool checked);

private:
    void preview(int i);
    void EffectSetup(int state);
    TQPushButton *addPreviewIcon(int i, const TQString &str, TQWidget *parent, TQGridLayout *lay);
    void init();
    void initDefaults();
    void read();
    void apply();


    bool mbDP[6], mbChanged[6], mbAnimated[6];
    int mSizes[6];
    TQValueList<int> mAvSizes[6];

    Effect mEffects[6][3];
    Effect mDefaultEffect[3];
    
    int mUsage;
    TQString mTheme, mExample;
    TQStringList mGroups, mStates;
    int mSysTraySize;
    int mQuickLaunchSize;

    TDEIconEffect *mpEffect;
    TDEIconTheme *mpTheme;
    TDEIconLoader *mpLoader;
    TDEConfig *mpConfig;
    KSimpleConfig *mpSystrayConfig;
    KSimpleConfig *mpKickerConfig;

    typedef TQLabel *QLabelPtr;
    QLabelPtr mpPreview[3];

    TQListBox *mpUsageList;
    TQComboBox *mpSizeBox;
    TQCheckBox *mpDPCheck, *wordWrapCB, *underlineCB, *mpAnimatedCheck, *mpRoundedCheck, *mpActiveEffectCheck;
    TQTabWidget *m_pTabWidget;
    TQWidget *m_pTab1;
    TQPushButton *mPreviewButton1, *mPreviewButton2, *mPreviewButton3;
};

class TDEIconEffectSetupDialog: public KDialogBase
{
    TQ_OBJECT
     
public:
    TDEIconEffectSetupDialog(const Effect &, const Effect &,
                           const TQString &, const TQImage &,
			   TQWidget *parent=0L, char *name=0L);
    ~TDEIconEffectSetupDialog();
    Effect effect() { return mEffect; }

protected:
    void preview();
    void init();

protected slots:
    void slotEffectValue(int value);
    void slotEffectColor(const TQColor &col);
    void slotEffectColor2(const TQColor &col);
    void slotEffectType(int type);
    void slotSTCheck(bool b);
    void slotDefault();

private:
    TDEIconEffect *mpEffect;
    TQListBox *mpEffectBox;
    TQCheckBox *mpSTCheck;
    TQSlider *mpEffectSlider;
    KColorButton *mpEColButton;
    KColorButton *mpECol2Button;
    Effect mEffect;
    Effect mDefaultEffect;
    TQImage mExample;
    TQGroupBox *mpEffectGroup;
    TQLabel *mpPreview, *mpEffectLabel, *mpEffectColor, *mpEffectColor2;
};

#endif
