// KDE Display color scheme setup module
//
// Copyright (c)  Mark Donohoe 1997
//
// Converted to a kcc module by Matthias Hoelzer 1997
// Ported to Qt-2.0 by Matthias Ettrich 1999
// Ported to kcontrol2 by Geert Jansen 1999
// Made maintainable by Waldo Bastian 2000

#include <assert.h>
#include <config.h>
#include <stdlib.h>
#include <unistd.h>

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqdir.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpainter.h>
#include <tqslider.h>
#include <tqvgroupbox.h>
#include <tqwhatsthis.h>

#include <kcolorbutton.h>
#include <kcursor.h>
#include <kfiledialog.h>
#include <kgenericfactory.h>
#include <kglobalsettings.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <kipc.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kaboutdata.h>

#if defined Q_WS_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "../krdb/krdb.h"

#include "colorscm.h"


/**** DLL Interface ****/
typedef KGenericFactory<KColorScheme , TQWidget> KolorFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_colors, KolorFactory("kcmcolors") )

class KColorSchemeEntry {
public:
    KColorSchemeEntry(const TQString &_path, const TQString &_name, bool _local)
    	: path(_path), name(_name), local(_local) { }

    TQString path;
    TQString name;
    bool local;
};

class KColorSchemeList : public TQPtrList<KColorSchemeEntry> {
public:
    KColorSchemeList()
        { setAutoDelete(true); }

    int compareItems(TQPtrCollection::Item item1, TQPtrCollection::Item item2)
        {
           KColorSchemeEntry *i1 = (KColorSchemeEntry*)item1;
           KColorSchemeEntry *i2 = (KColorSchemeEntry*)item2;
           if (i1->local != i2->local)
              return i1->local ? -1 : 1;
           return i1->name.localeAwareCompare(i2->name);
        }
};

#define SIZE 8

// make a 24 * 8 pixmap with the main colors in a scheme
TQPixmap mkColorPreview(const WidgetCanvas *cs)
{
   TQPixmap group(SIZE*3,SIZE);
   TQPixmap block(SIZE,SIZE);
   group.fill(TQColor(0,0,0));
   block.fill(cs->back);   bitBlt(&group,0*SIZE,0,&block,0,0,SIZE,SIZE);
   block.fill(cs->window); bitBlt(&group,1*SIZE,0,&block,0,0,SIZE,SIZE);
   block.fill(cs->aTitle); bitBlt(&group,2*SIZE,0,&block,0,0,SIZE,SIZE);
   TQPainter p(&group);
   p.drawRect(0,0,3*SIZE,SIZE);
   return group;
}

/**** KColorScheme ****/

KColorScheme::KColorScheme(TQWidget *parent, const char *name, const TQStringList &)
    : KCModule(KolorFactory::instance(), parent, name)
{
    nSysSchemes = 2;

    setQuickHelp( i18n("<h1>Colors</h1> This module allows you to choose"
       " the color scheme used for the KDE desktop. The different"
       " elements of the desktop, such as title bars, menu text, etc.,"
       " are called \"widgets\". You can choose the widget whose"
       " color you want to change by selecting it from a list, or by"
       " clicking on a graphical representation of the desktop.<p>"
       " You can save color settings as complete color schemes,"
       " which can also be modified or deleted. KDE comes with several"
       " predefined color schemes on which you can base your own.<p>"
       " All KDE applications will obey the selected color scheme."
       " Non-KDE applications may also obey some or all of the color"
       " settings, if this option is enabled."));

    KConfig *cfg = new KConfig("kcmdisplayrc");
    cfg->setGroup("X11");
    useRM = cfg->readBoolEntry("useResourceManager", true);
    delete cfg;

    cs = new WidgetCanvas( this );
    cs->setCursor( KCursor::handCursor() );

    // LAYOUT

    TQGridLayout *topLayout = new TQGridLayout( this, 3, 2, 0,
        KDialog::spacingHint() );
    topLayout->setRowStretch(0,0);
    topLayout->setRowStretch(1,0);
    topLayout->setColStretch(0,1);
    topLayout->setColStretch(1,1);

    cs->setFixedHeight(160);
    cs->setMinimumWidth(440);

    TQWhatsThis::add( cs, i18n("This is a preview of the color settings which"
       " will be applied if you click \"Apply\" or \"OK\". You can click on"
       " different parts of this preview image. The widget name in the"
       " \"Widget color\" box will change to reflect the part of the preview"
       " image you clicked.") );

    connect( cs, TQT_SIGNAL( widgetSelected( int ) ),
         TQT_SLOT( slotWidgetColor( int ) ) );
    connect( cs, TQT_SIGNAL( colorDropped( int, const TQColor&)),
         TQT_SLOT( slotColorForWidget( int, const TQColor&)));
    topLayout->addMultiCellWidget( cs, 0, 0, 0, 1 );

    TQGroupBox *group = new TQVGroupBox( i18n("Color Scheme"), this );
    topLayout->addWidget( group, 1, 0 );

    sList = new KListBox( group );
    mSchemeList = new KColorSchemeList();
    readSchemeNames();
    sList->setCurrentItem( 0 );
    connect(sList, TQT_SIGNAL(highlighted(int)), TQT_SLOT(slotPreviewScheme(int)));

    TQWhatsThis::add( sList, i18n("This is a list of predefined color schemes,"
       " including any that you may have created. You can preview an existing"
       " color scheme by selecting it from the list. The current scheme will"
       " be replaced by the selected color scheme.<p>"
       " Warning: if you have not yet applied any changes you may have made"
       " to the current scheme, those changes will be lost if you select"
       " another color scheme.") );

    addBt = new TQPushButton(i18n("&Save Scheme..."), group);
    connect(addBt, TQT_SIGNAL(clicked()), TQT_SLOT(slotAdd()));

    TQWhatsThis::add( addBt, i18n("Press this button if you want to save"
       " the current color settings as a color scheme. You will be"
       " prompted for a name.") );

    removeBt = new TQPushButton(i18n("R&emove Scheme"), group);
    removeBt->setEnabled(FALSE);
    connect(removeBt, TQT_SIGNAL(clicked()), TQT_SLOT(slotRemove()));

    TQWhatsThis::add( removeBt, i18n("Press this button to remove the selected"
       " color scheme. Note that this button is disabled if you do not have"
       " permission to delete the color scheme.") );

	importBt = new TQPushButton(i18n("I&mport Scheme..."), group);
	connect(importBt, TQT_SIGNAL(clicked()),TQT_SLOT(slotImport()));

	TQWhatsThis::add( importBt, i18n("Press this button to import a new color"
		" scheme. Note that the color scheme will only be available for the"
		" current user." ));


    TQBoxLayout *stackLayout = new TQVBoxLayout;
    topLayout->addLayout(stackLayout, 1, 1);

    group = new TQGroupBox(i18n("&Widget Color"), this);
    stackLayout->addWidget(group);
    TQBoxLayout *groupLayout = new TQVBoxLayout(group, 10);
    groupLayout->addSpacing(10);

    wcCombo = new TQComboBox(false, group);
    for(int i=0; i < CSM_LAST;i++)
    {
       wcCombo->insertItem(TQString::null);
    }

    setColorName(i18n("Inactive Title Bar") , CSM_Inactive_title_bar);
    setColorName(i18n("Inactive Title Text"), CSM_Inactive_title_text);
    setColorName(i18n("Inactive Title Blend"), CSM_Inactive_title_blend);
    setColorName(i18n("Active Title Bar"), CSM_Active_title_bar);
    setColorName(i18n("Active Title Text"), CSM_Active_title_text);
    setColorName(i18n("Active Title Blend"), CSM_Active_title_blend);
    setColorName(i18n("Window Background"), CSM_Background);
    setColorName(i18n("Window Text"), CSM_Text);
    setColorName(i18n("Selected Background"), CSM_Select_background);
    setColorName(i18n("Selected Text"), CSM_Select_text);
    setColorName(i18n("Standard Background"), CSM_Standard_background);
    setColorName(i18n("Standard Text"), CSM_Standard_text);
    setColorName(i18n("Button Background"), CSM_Button_background);
    setColorName(i18n("Button Text"), CSM_Button_text);
    setColorName(i18n("Active Title Button"), CSM_Active_title_button);
    setColorName(i18n("Inactive Title Button"), CSM_Inactive_title_button);
    setColorName(i18n("Active Window Frame"), CSM_Active_frame);
    setColorName(i18n("Active Window Handle"), CSM_Active_handle);
    setColorName(i18n("Inactive Window Frame"), CSM_Inactive_frame);
    setColorName(i18n("Inactive Window Handle"), CSM_Inactive_handle);
    setColorName(i18n("Link"), CSM_Link);
    setColorName(i18n("Followed Link"), CSM_Followed_Link);
    setColorName(i18n("Alternate Background in Lists"), CSM_Alternate_background);

    wcCombo->adjustSize();
    connect(wcCombo, TQT_SIGNAL(activated(int)), TQT_SLOT(slotWidgetColor(int)));
    groupLayout->addWidget(wcCombo);

    TQWhatsThis::add( wcCombo, i18n("Click here to select an element of"
       " the KDE desktop whose color you want to change. You may either"
       " choose the \"widget\" here, or click on the corresponding part"
       " of the preview image above.") );

    colorButton = new KColorButton( group );
    connect( colorButton, TQT_SIGNAL( changed(const TQColor &)),
             TQT_SLOT(slotSelectColor(const TQColor &)));

    groupLayout->addWidget( colorButton );

    TQWhatsThis::add( colorButton, i18n("Click here to bring up a dialog"
       " box where you can choose a color for the \"widget\" selected"
       " in the above list.") );

    cbShadeList = new TQCheckBox(i18n("Shade sorted column in lists"), this);
    stackLayout->addWidget(cbShadeList);
    connect(cbShadeList, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(slotShadeSortColumnChanged(bool)));

    TQWhatsThis::add(cbShadeList,
       i18n("Check this box to show the sorted column in a list with a shaded background"));

    group = new TQGroupBox(  i18n("Con&trast"), this );
    stackLayout->addWidget(group);

    TQVBoxLayout *groupLayout2 = new TQVBoxLayout(group, 10);
    groupLayout2->addSpacing(10);
    groupLayout = new TQHBoxLayout;
    groupLayout2->addLayout(groupLayout);

    sb = new TQSlider( Qt::Horizontal,group,"Slider" );
    sb->setRange( 0, 10 );
    sb->setFocusPolicy( TQ_StrongFocus );
    connect(sb, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(sliderValueChanged(int)));

    TQWhatsThis::add(sb, i18n("Use this slider to change the contrast level"
       " of the current color scheme. Contrast does not affect all of the"
       " colors, only the edges of 3D objects."));

    TQLabel *label = new TQLabel(sb, i18n("Low Contrast", "Low"), group);
    groupLayout->addWidget(label);
    groupLayout->addWidget(sb, 10);
    label = new TQLabel(group);
    label->setText(i18n("High Contrast", "High"));
    groupLayout->addWidget( label );

    cbExportColors = new TQCheckBox(i18n("Apply colors to &non-KDE applications"), this);
    topLayout->addMultiCellWidget( cbExportColors, 2, 2, 0, 1 );
    connect(cbExportColors, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(changed()));

    TQWhatsThis::add(cbExportColors, i18n("Check this box to apply the"
       " current color scheme to non-KDE applications."));

    load();

    KAboutData* about = new KAboutData("kcmcolors", I18N_NOOP("Colors"), 0, 0,
        KAboutData::License_GPL,
        I18N_NOOP("(c) 1997-2005 Colors Developers"), 0, 0);
    about->addAuthor("Mark Donohoe", 0, 0);
    about->addAuthor("Matthias Hoelzer", 0, 0);
    about->addAuthor("Matthias Ettrich", 0, 0);
    about->addAuthor("Geert Jansen", 0, 0);
    about->addAuthor("Waldo Bastian", 0, 0);
    setAboutData( about );
}


KColorScheme::~KColorScheme()
{
    delete mSchemeList;
}

void KColorScheme::setColorName( const TQString &name, int id )
{
    wcCombo->changeItem(name, id);
    cs->addToolTip( id, name );
}

void KColorScheme::load()
{
   load( false );
}
void KColorScheme::load( bool useDefaults )
{
    KConfig *config = KGlobal::config();
    config->setReadDefaults(  useDefaults );
    config->setGroup("KDE");
    sCurrentScheme = config->readEntry("colorScheme");

    sList->setCurrentItem(findSchemeByName(sCurrentScheme));
    readScheme(0);

    cbShadeList->setChecked(cs->shadeSortColumn);

    cs->drawSampleWidgets();
    slotWidgetColor(wcCombo->currentItem());
    sb->blockSignals(true);
    sb->setValue(cs->contrast);
    sb->blockSignals(false);

    KConfig cfg("kcmdisplayrc", true, false);
    cfg.setGroup("X11");
    bool exportColors = cfg.readBoolEntry("exportKDEColors", true);
    cbExportColors->setChecked(exportColors);

    emit changed( useDefaults );
}


void KColorScheme::save()
{
    KConfig *cfg = KGlobal::config();
    cfg->setGroup( "General" );
    cfg->writeEntry("background", cs->back, true, true);
    cfg->writeEntry("selectBackground", cs->select, true, true);
    cfg->writeEntry("foreground", cs->txt, true, true);
    cfg->writeEntry("windowForeground", cs->windowTxt, true, true);
    cfg->writeEntry("windowBackground", cs->window, true, true);
    cfg->writeEntry("selectForeground", cs->selectTxt, true, true);
    cfg->writeEntry("buttonBackground", cs->button, true, true);
    cfg->writeEntry("buttonForeground", cs->buttonTxt, true, true);
    cfg->writeEntry("linkColor", cs->link, true, true);
    cfg->writeEntry("visitedLinkColor", cs->visitedLink, true, true);
    cfg->writeEntry("alternateBackground", cs->alternateBackground, true, true);

    cfg->writeEntry("shadeSortColumn", cs->shadeSortColumn, true, true);

    cfg->setGroup( "WM" );
    cfg->writeEntry("activeForeground", cs->aTxt, true, true);
    cfg->writeEntry("inactiveBackground", cs->iaTitle, true, true);
    cfg->writeEntry("inactiveBlend", cs->iaBlend, true, true);
    cfg->writeEntry("activeBackground", cs->aTitle, true, true);
    cfg->writeEntry("activeBlend", cs->aBlend, true, true);
    cfg->writeEntry("inactiveForeground", cs->iaTxt, true, true);
    cfg->writeEntry("activeTitleBtnBg", cs->aTitleBtn, true, true);
    cfg->writeEntry("inactiveTitleBtnBg", cs->iTitleBtn, true, true);
    cfg->writeEntry("frame", cs->aFrame, true, true);
    cfg->writeEntry("inactiveFrame", cs->iaFrame, true, true);
    cfg->writeEntry("handle", cs->aHandle, true, true);
    cfg->writeEntry("inactiveHandle", cs->iaHandle, true, true);

    cfg->setGroup( "KDE" );
    cfg->writeEntry("contrast", cs->contrast, true, true);
    cfg->writeEntry("colorScheme", sCurrentScheme, true, true);
    cfg->sync();

    // KDE-1.x support
    KSimpleConfig *config =
    new KSimpleConfig( TQDir::homeDirPath() + "/.kderc" );
    config->setGroup( "General" );
    config->writeEntry("background", cs->back );
    config->writeEntry("selectBackground", cs->select );
    config->writeEntry("foreground", cs->txt, true, true);
    config->writeEntry("windowForeground", cs->windowTxt );
    config->writeEntry("windowBackground", cs->window );
    config->writeEntry("selectForeground", cs->selectTxt );
    config->sync();
    delete config;

    KConfig cfg2("kcmdisplayrc", false, false);
    cfg2.setGroup("X11");
    bool exportColors = cbExportColors->isChecked();
    cfg2.writeEntry("exportKDEColors", exportColors);
    cfg2.sync();
    TQApplication::syncX();

    // Notify all qt-only apps of the KDE palette changes
    uint flags = KRdbExportQtColors;
    if ( exportColors )
        flags |= KRdbExportColors;
    else
    {
#if defined Q_WS_X11 && !defined K_WS_QTONLY
        // Undo the property xrdb has placed on the root window (if any),
        // i.e. remove all entries, including ours
        XDeleteProperty( qt_xdisplay(), qt_xrootwin(), XA_RESOURCE_MANAGER );
#endif
    }
    runRdb( flags );	// Save the palette to qtrc for KStyles

    // Notify all KDE applications
    KIPC::sendMessageAll(KIPC::PaletteChanged);

    // Update the "Current Scheme"
    int current = findSchemeByName(sCurrentScheme);
    sList->setCurrentItem(0);
    readScheme(0);
    TQPixmap preview = mkColorPreview(cs);
    sList->changeItem(preview, sList->text(0), 0);
    sList->setCurrentItem(current);
    readScheme(current);
    preview = mkColorPreview(cs);
    sList->changeItem(preview, sList->text(current), current);

    emit changed(false);
}


void KColorScheme::defaults()
{
   load( true );
}

void KColorScheme::sliderValueChanged( int val )
{
    cs->contrast = val;
    cs->drawSampleWidgets();

    sCurrentScheme = TQString::null;

    emit changed(true);
}


void KColorScheme::slotSave( )
{
    KColorSchemeEntry *entry = mSchemeList->at(sList->currentItem()-nSysSchemes);
    if (!entry) return;
    sCurrentScheme = entry->path;
    KSimpleConfig *config = new KSimpleConfig(sCurrentScheme );
    int i = sCurrentScheme.findRev('/');
    if (i >= 0)
      sCurrentScheme = sCurrentScheme.mid(i+1);

    config->setGroup("Color Scheme" );
    config->writeEntry("background", cs->back );
    config->writeEntry("selectBackground", cs->select );
    config->writeEntry("foreground", cs->txt );
    config->writeEntry("activeForeground", cs->aTxt );
    config->writeEntry("inactiveBackground", cs->iaTitle );
    config->writeEntry("inactiveBlend", cs->iaBlend );
    config->writeEntry("activeBackground", cs->aTitle );
    config->writeEntry("activeBlend", cs->aBlend );
    config->writeEntry("inactiveForeground", cs->iaTxt );
    config->writeEntry("windowForeground", cs->windowTxt );
    config->writeEntry("windowBackground", cs->window );
    config->writeEntry("selectForeground", cs->selectTxt );
    config->writeEntry("contrast", cs->contrast );
    config->writeEntry("buttonForeground", cs->buttonTxt );
    config->writeEntry("buttonBackground", cs->button );
    config->writeEntry("activeTitleBtnBg", cs->aTitleBtn);
    config->writeEntry("inactiveTitleBtnBg", cs->iTitleBtn);
    config->writeEntry("frame", cs->aFrame);
    config->writeEntry("inactiveFrame", cs->iaFrame);
    config->writeEntry("handle", cs->aHandle);
    config->writeEntry("inactiveHandle", cs->iaHandle);
    config->writeEntry("linkColor", cs->link);
    config->writeEntry("visitedLinkColor", cs->visitedLink);
    config->writeEntry("alternateBackground", cs->alternateBackground);
    config->writeEntry("shadeSortColumn", cs->shadeSortColumn);

    delete config;
}


void KColorScheme::slotRemove()
{
    uint ind = sList->currentItem();
    KColorSchemeEntry *entry = mSchemeList->at(ind-nSysSchemes);
    if (!entry) return;

    if (unlink(TQFile::encodeName(entry->path).data())) {
        KMessageBox::error( 0,
          i18n("This color scheme could not be removed.\n"
           "Perhaps you do not have permission to alter the file"
           "system where the color scheme is stored." ));
        return;
    }

    sList->removeItem(ind);
    mSchemeList->remove(entry);

    ind = sList->currentItem();
    entry = mSchemeList->at(ind-nSysSchemes);
    if (!entry) return;
    removeBt->setEnabled(entry ? entry->local : false);
}


/*
 * Add a local color scheme.
 */
void KColorScheme::slotAdd()
{
    TQString sName;
    if (sList->currentItem() >= nSysSchemes)
       sName = sList->currentText();

    TQString sFile;

    bool valid = false;
    bool ok;
    int exists = -1;

    while (!valid)
    {
        sName = KInputDialog::getText( i18n( "Save Color Scheme" ),
            i18n( "Enter a name for the color scheme:" ), sName, &ok, this );
        if (!ok)
            return;

        sName = sName.simplifyWhiteSpace();
        sFile = sName;

        int i = 0;

        exists = -1;
        // Check if it's already there
        for (i=0; i < (int) sList->count(); i++)
        {
            if (sName == sList->text(i))
            {
                exists = i;
                int result = KMessageBox::warningContinueCancel( this,
                   i18n("A color scheme with the name '%1' already exists.\n"
                        "Do you want to overwrite it?\n").arg(sName),
                   i18n("Save Color Scheme"),
                   i18n("Overwrite"));
                if (result == KMessageBox::Cancel)
                    break;
            }
        }
        if (i == (int) sList->count())
            valid = true;
    }

    disconnect(sList, TQT_SIGNAL(highlighted(int)), this,
               TQT_SLOT(slotPreviewScheme(int)));

    if (exists != -1)
    {
       sList->setFocus();
       sList->setCurrentItem(exists);
    }
    else
    {
       sFile = KGlobal::dirs()->saveLocation("data", "kdisplay/color-schemes/") + sFile + ".kcsrc";
       KSimpleConfig *config = new KSimpleConfig(sFile);
       config->setGroup( "Color Scheme");
       config->writeEntry("Name", sName);
       delete config;

	   insertEntry(sFile, sName);

    }
    slotSave();

    TQPixmap preview = mkColorPreview(cs);
    int current = sList->currentItem();
    sList->changeItem(preview, sList->text(current), current);
    connect(sList, TQT_SIGNAL(highlighted(int)), TQT_SLOT(slotPreviewScheme(int)));
    slotPreviewScheme(current);
}

void KColorScheme::slotImport()
{
	TQString location = locateLocal( "data", "kdisplay/color-schemes/" );

	KURL file ( KFileDialog::getOpenFileName(TQString::null, "*.kcsrc", this) );
	if ( file.isEmpty() )
		return;

	//kdDebug() << "Location: " << location << endl;
	if (!KIO::NetAccess::file_copy(file, KURL( location+file.fileName( false ) ) ) )
	{
		KMessageBox::error(this, KIO::NetAccess::lastErrorString(),i18n("Import failed."));
		return;
	}
	else
	{
		TQString sFile = location + file.fileName( false );
		KSimpleConfig *config = new KSimpleConfig(sFile);
		config->setGroup( "Color Scheme");
		TQString sName = config->readEntry("Name", i18n("Untitled Theme"));
		delete config;


		insertEntry(sFile, sName);
		TQPixmap preview = mkColorPreview(cs);
		int current = sList->currentItem();
		sList->changeItem(preview, sList->text(current), current);
		connect(sList, TQT_SIGNAL(highlighted(int)), TQT_SLOT(slotPreviewScheme(int)));
		slotPreviewScheme(current);
	}
}

TQColor &KColorScheme::color(int index)
{
    switch(index) {
    case CSM_Inactive_title_bar:
    return cs->iaTitle;
    case CSM_Inactive_title_text:
    return cs->iaTxt;
    case CSM_Inactive_title_blend:
    return cs->iaBlend;
    case CSM_Active_title_bar:
    return cs->aTitle;
    case CSM_Active_title_text:
    return cs->aTxt;
    case CSM_Active_title_blend:
    return cs->aBlend;
    case CSM_Background:
    return cs->back;
    case CSM_Text:
    return cs->txt;
    case CSM_Select_background:
    return cs->select;
    case CSM_Select_text:
    return cs->selectTxt;
    case CSM_Standard_background:
    return cs->window;
    case CSM_Standard_text:
    return cs->windowTxt;
    case CSM_Button_background:
    return cs->button;
    case CSM_Button_text:
    return cs->buttonTxt;
    case CSM_Active_title_button:
    return cs->aTitleBtn;
    case CSM_Inactive_title_button:
    return cs->iTitleBtn;
    case CSM_Active_frame:
    return cs->aFrame;
    case CSM_Active_handle:
    return cs->aHandle;
    case CSM_Inactive_frame:
    return cs->iaFrame;
    case CSM_Inactive_handle:
    return cs->iaHandle;
    case CSM_Link:
    return cs->link;
    case CSM_Followed_Link:
    return cs->visitedLink;
    case CSM_Alternate_background:
    return cs->alternateBackground;
    }

    assert(0); // Should never be here!
    return cs->iaTxt; // Silence compiler
}


void KColorScheme::slotSelectColor(const TQColor &col)
{
    int selection;
    selection = wcCombo->currentItem();

    // Adjust the alternate background color if the standard color changes
    // Only if the previous alternate color was not a user-configured one
    // of course
    if ( selection == CSM_Standard_background &&
         color(CSM_Alternate_background) ==
         KGlobalSettings::calculateAlternateBackgroundColor(
             color(CSM_Standard_background) ) )
    {
        color(CSM_Alternate_background) =
            KGlobalSettings::calculateAlternateBackgroundColor( col );
    }

    color(selection) = col;

    cs->drawSampleWidgets();

    sCurrentScheme = TQString::null;

    emit changed(true);
}


void KColorScheme::slotWidgetColor(int indx)
{
    if (indx < 0)
        indx = 0;
    if (wcCombo->currentItem() != indx)
        wcCombo->setCurrentItem( indx );

    // Do not emit KCModule::changed()
    colorButton->blockSignals( true );

    TQColor col = color(indx);
    colorButton->setColor( col );
    colorPushColor = col;

    colorButton->blockSignals( false );
}


void KColorScheme::slotColorForWidget(int indx, const TQColor& col)
{
    if (wcCombo->currentItem() != indx)
        wcCombo->setCurrentItem( indx );

    slotSelectColor(col);
}

void KColorScheme::slotShadeSortColumnChanged(bool b)
{
    cs->shadeSortColumn = b;
    sCurrentScheme = TQString::null;

    emit changed(true);
}

/*
 * Read a color scheme into "cs".
 *
 * KEEP IN SYNC with thememgr!
 */
void KColorScheme::readScheme( int index )
{
    KConfigBase* config;

    TQColor widget(239, 239, 239);
    TQColor trinity4Blue(103,141,178);
    TQColor inactiveBackground(157,170,186);
    TQColor activeBackground(65,142,220);
    TQColor inactiveForeground(221,221,221);
    TQColor activeBlend(107,145,184);
    TQColor inactiveBlend(157,170,186);
    TQColor activeTitleBtnBg(220,220,220);
    TQColor inactiveTitleBtnBg(220,220,220);
    TQColor alternateBackground(237,244,249);

    TQColor button;
    if (TQPixmap::defaultDepth() > 8)
      button.setRgb(221, 223, 228 );
    else
      button.setRgb(220, 220, 220);

    TQColor link(0, 0, 238);
    TQColor visitedLink(82, 24,139);

    // note: keep default color scheme in sync with default Current Scheme
    if (index == 1) {
      sCurrentScheme  = "<default>";
      cs->txt         = black;
      cs->back        = widget;
      cs->select      = trinity4Blue;
      cs->selectTxt   = white;
      cs->window      = white;
      cs->windowTxt   = black;
      cs->iaTitle     = inactiveBackground;
      cs->iaTxt       = inactiveForeground;
      cs->iaBlend     = inactiveBlend;
      cs->aTitle      = activeBackground;
      cs->aTxt        = white;
      cs->aBlend      = activeBlend;
      cs->button      = button;
      cs->buttonTxt   = black;
      cs->aTitleBtn   = activeTitleBtnBg;
      cs->iTitleBtn   = inactiveTitleBtnBg;
      cs->aFrame      = cs->back;
      cs->aHandle     = cs->back;
      cs->iaFrame     = cs->back;
      cs->iaHandle    = cs->back;
      cs->link        = link;
      cs->visitedLink = visitedLink;
      cs->alternateBackground = alternateBackground;

      cs->contrast    = 7;
      cs->shadeSortColumn = KDE_DEFAULT_SHADE_SORT_COLUMN;

      return;
    }

    if (index == 0) {
      // Current scheme
      config = KGlobal::config();
      config->setGroup("General");
    } else {
      // Open scheme file
      KColorSchemeEntry *entry = mSchemeList->at(sList->currentItem()-nSysSchemes);
      if (!entry) return;
      sCurrentScheme = entry->path;
      config = new KSimpleConfig(sCurrentScheme, true);
      config->setGroup("Color Scheme");
      int i = sCurrentScheme.findRev('/');
      if (i >= 0)
        sCurrentScheme = sCurrentScheme.mid(i+1);
    }

    cs->shadeSortColumn = config->readBoolEntry( "shadeSortColumn", KDE_DEFAULT_SHADE_SORT_COLUMN );

    // note: defaults should be the same as the KDE default
    cs->txt = config->readColorEntry( "foreground", &black );
    cs->back = config->readColorEntry( "background", &widget );
    cs->select = config->readColorEntry( "selectBackground", &trinity4Blue );
    cs->selectTxt = config->readColorEntry( "selectForeground", &white );
    cs->window = config->readColorEntry( "windowBackground", &white );
    cs->windowTxt = config->readColorEntry( "windowForeground", &black );
    cs->button = config->readColorEntry( "buttonBackground", &button );
    cs->buttonTxt = config->readColorEntry( "buttonForeground", &black );
    cs->link = config->readColorEntry( "linkColor", &link );
    cs->visitedLink = config->readColorEntry( "visitedLinkColor", &visitedLink );
    TQColor alternate = KGlobalSettings::calculateAlternateBackgroundColor(cs->window);
    cs->alternateBackground = config->readColorEntry( "alternateBackground", &alternate );

    if (index == 0)
      config->setGroup( "WM" );

    cs->iaTitle = config->readColorEntry("inactiveBackground", &inactiveBackground);
    cs->iaTxt = config->readColorEntry("inactiveForeground", &inactiveForeground);
    cs->iaBlend = config->readColorEntry("inactiveBlend", &inactiveBackground);
    cs->iaFrame = config->readColorEntry("inactiveFrame", &cs->back);
    cs->iaHandle = config->readColorEntry("inactiveHandle", &cs->back);
    cs->aTitle = config->readColorEntry("activeBackground", &activeBackground);
    cs->aTxt = config->readColorEntry("activeForeground", &white);
    cs->aBlend = config->readColorEntry("activeBlend", &activeBlend);
    cs->aFrame = config->readColorEntry("frame", &cs->back);
    cs->aHandle = config->readColorEntry("handle", &cs->back);
    // hack - this is all going away. For now just set all to button bg
    cs->aTitleBtn = config->readColorEntry("activeTitleBtnBg", &activeTitleBtnBg);
    cs->iTitleBtn = config->readColorEntry("inactiveTitleBtnBg", &inactiveTitleBtnBg);

    if (index == 0)
      config->setGroup( "KDE" );

    cs->contrast = config->readNumEntry( "contrast", 7 );
    if (index != 0)
      delete config;
}


/*
 * Get all installed color schemes.
 */
void KColorScheme::readSchemeNames()
{
    mSchemeList->clear();
    sList->clear();
    // Always a current and a default scheme
    sList->insertItem( i18n("Current Scheme"), 0 );
    sList->insertItem( i18n("KDE Default"), 1 );
    nSysSchemes = 2;

    // Global + local schemes
    TQStringList list = KGlobal::dirs()->findAllResources("data",
            "kdisplay/color-schemes/*.kcsrc", false, true);

    // And add them
    for (TQStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
       KSimpleConfig *config = new KSimpleConfig(*it);
       config->setGroup("Color Scheme");
       TQString str = config->readEntry("Name");
       if (str.isEmpty()) {
          str =  config->readEntry("name");
          if (str.isEmpty())
             continue;
       }
       mSchemeList->append(new KColorSchemeEntry(*it, str, !config->isImmutable()));
       delete config;
    }

    mSchemeList->sort();

    for(KColorSchemeEntry *entry = mSchemeList->first(); entry; entry = mSchemeList->next())
    {
       sList->insertItem(entry->name);
    }

    for (uint i = 0; i < (nSysSchemes + mSchemeList->count()); i++)
    {
       sList->setCurrentItem(i);
       readScheme(i);
       TQPixmap preview = mkColorPreview(cs);
       sList->changeItem(preview, sList->text(i), i);
    }

}

/*
 * Find scheme based on filename
 */
int KColorScheme::findSchemeByName(const TQString &scheme)
{
   if (scheme.isEmpty())
      return 0;
   if (scheme == "<default>")
      return 1;

   TQString search = scheme;
   int i = search.findRev('/');
   if (i >= 0)
      search = search.mid(i+1);

   i = 0;

   for(KColorSchemeEntry *entry = mSchemeList->first(); entry; entry = mSchemeList->next())
   {
      KURL url;
      url.setPath(entry->path);
      if (url.fileName() == search)
         return i+nSysSchemes;
      i++;
   }

   return 0;
}


void KColorScheme::slotPreviewScheme(int indx)
{
    readScheme(indx);

    // Set various appropriate for the scheme

    cbShadeList->setChecked(cs->shadeSortColumn);

    cs->drawSampleWidgets();
    sb->blockSignals(true);
    sb->setValue(cs->contrast);
    sb->blockSignals(false);
    slotWidgetColor(wcCombo->currentItem());
    if (indx < nSysSchemes)
       removeBt->setEnabled(false);
    else
    {
       KColorSchemeEntry *entry = mSchemeList->at(indx-nSysSchemes);
       removeBt->setEnabled(entry ? entry->local : false);
    }

    emit changed((indx != 0));
}


/* this function should dissappear: colorscm should work directly on a Qt palette, since
   this will give us much more cusomization with qt-2.0.
   */
TQPalette KColorScheme::createPalette()
{
    TQColorGroup disabledgrp(cs->windowTxt, cs->back, cs->back.light(150),
                cs->back.dark(), cs->back.dark(120), cs->back.dark(120),
                cs->window);

    TQColorGroup colgrp(cs->windowTxt, cs->back, cs->back.light(150),
               cs->back.dark(), cs->back.dark(120), cs->txt, cs->window);

    colgrp.setColor(TQColorGroup::Highlight, cs->select);
    colgrp.setColor(TQColorGroup::HighlightedText, cs->selectTxt);
    colgrp.setColor(TQColorGroup::Button, cs->button);
    colgrp.setColor(TQColorGroup::ButtonText, cs->buttonTxt);
    return TQPalette( colgrp, disabledgrp, colgrp);
}

void KColorScheme::insertEntry(const TQString &sFile, const TQString &sName)
{
       KColorSchemeEntry *newEntry = new KColorSchemeEntry(sFile, sName, true);
       mSchemeList->inSort(newEntry);
       int newIndex = mSchemeList->findRef(newEntry)+nSysSchemes;
       sList->insertItem(sName, newIndex);
       sList->setCurrentItem(newIndex);
}

#include "colorscm.moc"
