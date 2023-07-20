//-----------------------------------------------------------------------------
//
// KDE Display color scheme setup module
//
// Copyright (c)  Mark Donohoe 1997
//

#ifndef __COLORSCM_H__
#define __COLORSCM_H__

#include <tqcolor.h>
#include <tqobject.h>
#include <tqstring.h>
#include <tqstringlist.h>

#include <tdecmodule.h>
#include <kdialogbase.h>

#include "widgetcanvas.h"

class TQSlider;
class TQComboBox;
class TQPushButton;
class TQCheckBox;
class TQResizeEvent;
class KLineEdit;
class TQPalette;
class TDEListBox;
class KColorButton;
class TDEConfig;
class KStdDirs;
class KColorSchemeList;

/**
 * The Desktop/Colors tab in kcontrol.
 */
class KColorScheme: public TDECModule
{
    TQ_OBJECT

public:
    KColorScheme(TQWidget *parent, const char *name, const TQStringList &);
    ~KColorScheme();

    virtual void load();
    virtual void load( bool useDefaults );
    virtual void save();
    virtual void defaults();

private slots:
    void sliderValueChanged(int val);
    void slotSave();
    void slotAdd();
    void slotRemove();
    void slotImport();
    void slotSelectColor(const TQColor &col);
    void slotWidgetColor(int);
    void slotColorForWidget(int, const TQColor &);
    void slotPreviewScheme(int);
    void slotShadeSortColumnChanged(bool);

private:
    void setColorName( const TQString &name, int id );
    void readScheme(int index=0);
    void readSchemeNames();
	void insertEntry(const TQString &sFile, const TQString &sName);
    int findSchemeByName(const TQString &scheme);
    TQPalette createPalette();
    
    TQColor &color(int index);

    int nSysSchemes;
    bool useRM;

    TQColor colorPushColor;
    TQSlider *sb;
    TQComboBox *wcCombo;
    TQPushButton *addBt, *removeBt, *importBt;
    TDEListBox *sList;
    KColorSchemeList *mSchemeList;
    TQString sCurrentScheme;

    KColorButton *colorButton;
    WidgetCanvas *cs;
    
    TQCheckBox *cbExportColors;
    TQCheckBox *cbShadeList;
};

#endif
