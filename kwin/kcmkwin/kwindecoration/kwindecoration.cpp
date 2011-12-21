/*
	This is the new kwindecoration kcontrol module

	Copyright (c) 2001
		Karol Szwed <gallium@kde.org>
		http://gallium.n3.net/

	Supports new kwin configuration plugins, and titlebar button position
	modification via dnd interface.

	Based on original "kwintheme" (Window Borders)
	Copyright (C) 2001 Rik Hemsley (rikkus) <rik@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include <assert.h>
#include <math.h>

#include <tqdir.h>
#include <tqfileinfo.h>
#include <tqlayout.h>
#include <tqwhatsthis.h>
#include <tqgroupbox.h>
#include <tqcheckbox.h>
#include <tqtabwidget.h>
#include <tqvbox.h>
#include <tqlabel.h>
#include <tqfile.h>
#include <tqslider.h>
#include <tqspinbox.h>

#include <kapplication.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <kaboutdata.h>
#include <dcopclient.h>

#include "kwindecoration.h"
#include "preview.h"
#include <kdecoration_plugins_p.h>
#include <kdecorationfactory.h>

// KCModule plugin interface
// =========================
typedef KGenericFactory<KWinDecorationModule, TQWidget> KWinDecoFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_kwindecoration, KWinDecoFactory("kcmkwindecoration") )

KWinDecorationModule::KWinDecorationModule(TQWidget* parent, const char* name, const TQStringList &)
	: DCOPObject("KWinClientDecoration"),
	  KCModule(KWinDecoFactory::instance(), parent, name),
          kwinConfig("kwinrc"),
          pluginObject(0)
{
	kwinConfig.setGroup("Style");
        plugins = new KDecorationPreviewPlugins( &kwinConfig );

	TQVBoxLayout* layout = new TQVBoxLayout(this, 0, KDialog::spacingHint()); 

// Save this for later...
//	cbUseMiniWindows = new TQCheckBox( i18n( "Render mini &titlebars for all windows"), checkGroup );
//	TQWhatsThis::add( cbUseMiniWindows, i18n( "Note that this option is not available on all styles yet!" ) );

	tabWidget = new TQTabWidget( this );
	layout->addWidget( tabWidget );

	// Page 1 (General Options)
	TQWidget *pluginPage = new TQWidget( tabWidget );

	TQVBoxLayout* pluginLayout = new TQVBoxLayout(pluginPage, KDialog::marginHint(), KDialog::spacingHint());

	// decoration chooser
	decorationList = new KComboBox( pluginPage );
	TQString whatsThis = i18n("Select the window decoration. This is the look and feel of both "
                             "the window borders and the window handle.");
	TQWhatsThis::add(decorationList, whatsThis);
	pluginLayout->addWidget(decorationList);

	TQGroupBox *pluginSettingsGrp = new TQGroupBox( i18n("Decoration Options"), pluginPage );
	pluginSettingsGrp->setColumnLayout( 0, Qt::Vertical );
	pluginSettingsGrp->setFlat( true );
	pluginSettingsGrp->layout()->setMargin( 0 );
	pluginSettingsGrp->layout()->setSpacing( KDialog::spacingHint() );
	pluginLayout->addWidget( pluginSettingsGrp );

	pluginLayout->addStretch();

	// Border size chooser
	lBorder = new TQLabel (i18n("B&order size:"), pluginSettingsGrp);
	cBorder = new TQComboBox(pluginSettingsGrp);
	lBorder->setBuddy(cBorder);
	TQWhatsThis::add( cBorder, i18n( "Use this combobox to change the border size of the decoration." ));
	lBorder->hide();
	cBorder->hide();
	TQHBoxLayout *borderSizeLayout = new TQHBoxLayout(pluginSettingsGrp->layout() );
	borderSizeLayout->addWidget(lBorder);
	borderSizeLayout->addWidget(cBorder);
	borderSizeLayout->addStretch();

	pluginConfigWidget = new TQVBox(pluginSettingsGrp);
	pluginSettingsGrp->layout()->add( pluginConfigWidget );

	// Page 2 (Button Selector)
	TQWidget* buttonPage = new TQWidget( tabWidget );
	TQVBoxLayout* buttonLayout = new TQVBoxLayout(buttonPage, KDialog::marginHint(), KDialog::spacingHint());

	cbShowToolTips = new TQCheckBox(
			i18n("&Show window button tooltips"), buttonPage );
	TQWhatsThis::add( cbShowToolTips,
			i18n(  "Enabling this checkbox will show window button tooltips. "
				   "If this checkbox is off, no window button tooltips will be shown."));

	cbUseCustomButtonPositions = new TQCheckBox(
			i18n("Use custom titlebar button &positions"), buttonPage );
	TQWhatsThis::add( cbUseCustomButtonPositions,
			i18n(  "The appropriate settings can be found in the \"Buttons\" Tab; "
				   "please note that this option is not available on all styles yet." ) );

	buttonLayout->addWidget( cbShowToolTips );
	buttonLayout->addWidget( cbUseCustomButtonPositions );

	// Add nifty dnd button modification widgets
	buttonPositionWidget = new ButtonPositionWidget(buttonPage, "button_position_widget");
	buttonPositionWidget->setDecorationFactory(plugins->factory() );
	TQHBoxLayout* buttonControlLayout = new TQHBoxLayout(buttonLayout);
	buttonControlLayout->addSpacing(20);
	buttonControlLayout->addWidget(buttonPositionWidget);
// 	buttonLayout->addStretch();

	// preview
	TQVBoxLayout* previewLayout = new TQVBoxLayout(layout, KDialog::spacingHint() );
	previewLayout->setMargin( KDialog::marginHint() );

	preview = new KDecorationPreview( this );
	previewLayout->addWidget(preview);

	preview->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	tabWidget->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Maximum);

	// Page 3 (Window Shadows)
	TQHBox *inactiveShadowColourHBox, *shadowColourHBox;
	TQHBox *inactiveShadowOpacityHBox, *shadowOpacityHBox;
	TQHBox *inactiveShadowXOffsetHBox, *shadowXOffsetHBox;
	TQHBox *inactiveShadowYOffsetHBox, *shadowYOffsetHBox;
	TQHBox *inactiveShadowThicknessHBox, *shadowThicknessHBox;
	TQLabel *inactiveShadowColourLabel, *shadowColourLabel;
	TQLabel *inactiveShadowOpacityLabel, *shadowOpacityLabel;
	TQLabel *inactiveShadowXOffsetLabel, *shadowXOffsetLabel;
	TQLabel *inactiveShadowYOffsetLabel, *shadowYOffsetLabel;
	TQLabel *inactiveShadowThicknessLabel, *shadowThicknessLabel;

	shadowPage = new TQVBox(tabWidget);
	shadowPage->setSpacing(KDialog::spacingHint());
	shadowPage->setMargin(KDialog::marginHint());

	cbWindowShadow = new TQCheckBox(
			i18n("&Draw a drop shadow under windows"), shadowPage);
	TQWhatsThis::add(cbWindowShadow,
			i18n("Enabling this checkbox will allow you to choose a kind of "
				 "drop shadow to draw under each window."));

	activeShadowSettings = new TQGroupBox(1, Qt::Horizontal,
			i18n("Active Window Shadow"), shadowPage);
	inactiveShadowSettings = new TQGroupBox(1, Qt::Horizontal,
			i18n("Inactive Window Shadows"), shadowPage);
	whichShadowSettings = new TQGroupBox(3, Qt::Horizontal,
			i18n("Draw Shadow Under Normal Windows And..."), shadowPage);

	cbShadowDocks = new TQCheckBox(i18n("Docks and &panels"),
			whichShadowSettings);
	connect(cbShadowDocks, TQT_SIGNAL(toggled(bool)),
			TQT_SLOT(slotSelectionChanged()));
	cbShadowOverrides = new TQCheckBox(i18n("O&verride windows"),
			whichShadowSettings);
	connect(cbShadowOverrides, TQT_SIGNAL(toggled(bool)),
			TQT_SLOT(slotSelectionChanged()));
	cbShadowTopMenus = new TQCheckBox(i18n("&Top menu"),
			whichShadowSettings);
	connect(cbShadowTopMenus, TQT_SIGNAL(toggled(bool)),
			TQT_SLOT(slotSelectionChanged()));
	cbInactiveShadow = new TQCheckBox(
			i18n("Draw shadow under &inactive windows"), inactiveShadowSettings);
	connect(cbInactiveShadow, TQT_SIGNAL(toggled(bool)),
			TQT_SLOT(slotSelectionChanged()));

	shadowColourHBox = new TQHBox(activeShadowSettings);
	shadowColourHBox->setSpacing(KDialog::spacingHint());
	shadowColourLabel = new TQLabel(i18n("Colour:"), shadowColourHBox);
	shadowColourButton = new KColorButton(shadowColourHBox);
	connect(shadowColourButton, TQT_SIGNAL(changed(const TQColor &)), TQT_SLOT(slotSelectionChanged()));

	inactiveShadowColourHBox = new TQHBox(inactiveShadowSettings);
	inactiveShadowColourHBox->setSpacing(KDialog::spacingHint());
	inactiveShadowColourLabel = new TQLabel(i18n("Colour:"), inactiveShadowColourHBox);
	inactiveShadowColourButton = new KColorButton(inactiveShadowColourHBox);
	connect(inactiveShadowColourButton, TQT_SIGNAL(changed(const TQColor &)), TQT_SLOT(slotSelectionChanged()));

	shadowOpacityHBox = new TQHBox(activeShadowSettings);
	shadowOpacityHBox->setSpacing(KDialog::spacingHint());
	shadowOpacityLabel = new TQLabel(i18n("Maximum opacity:"), shadowOpacityHBox);
	shadowOpacitySlider = new TQSlider(1, 100, 10, 50, Qt::Horizontal,
			shadowOpacityHBox);
	shadowOpacitySlider->setTickmarks(TQSlider::Below);
	shadowOpacitySlider->setTickInterval(10);
	shadowOpacitySpinBox = new TQSpinBox(1, 100, 1, shadowOpacityHBox);
	shadowOpacitySpinBox->setSuffix(" %");
	connect(shadowOpacitySlider, TQT_SIGNAL(valueChanged(int)), shadowOpacitySpinBox,
			TQT_SLOT(setValue(int)));
	connect(shadowOpacitySpinBox, TQT_SIGNAL(valueChanged(int)), shadowOpacitySlider,
			TQT_SLOT(setValue(int)));
	connect(shadowOpacitySlider, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	inactiveShadowOpacityHBox = new TQHBox(inactiveShadowSettings);
	inactiveShadowOpacityHBox->setSpacing(KDialog::spacingHint());
	inactiveShadowOpacityLabel = new TQLabel(i18n("Maximum opacity:"),
			inactiveShadowOpacityHBox);
	inactiveShadowOpacitySlider = new TQSlider(1, 100, 10, 50, Qt::Horizontal,
			inactiveShadowOpacityHBox);
	inactiveShadowOpacitySlider->setTickmarks(TQSlider::Below);
	inactiveShadowOpacitySlider->setTickInterval(10);
	inactiveShadowOpacitySpinBox = new TQSpinBox(1, 100, 1,
			inactiveShadowOpacityHBox);
	inactiveShadowOpacitySpinBox->setSuffix(" %");
	connect(inactiveShadowOpacitySlider, TQT_SIGNAL(valueChanged(int)),
			inactiveShadowOpacitySpinBox,
			TQT_SLOT(setValue(int)));
	connect(inactiveShadowOpacitySpinBox, TQT_SIGNAL(valueChanged(int)),
			inactiveShadowOpacitySlider,
			TQT_SLOT(setValue(int)));
	connect(inactiveShadowOpacitySlider, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	shadowXOffsetHBox = new TQHBox(activeShadowSettings);
	shadowXOffsetHBox->setSpacing(KDialog::spacingHint());
	shadowXOffsetLabel = new TQLabel(
			i18n("Offset rightward (may be negative):"),
			shadowXOffsetHBox);
	shadowXOffsetSpinBox = new TQSpinBox(-1024, 1024, 1, shadowXOffsetHBox);
	shadowXOffsetSpinBox->setSuffix(i18n(" pixels"));
	connect(shadowXOffsetSpinBox, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	inactiveShadowXOffsetHBox = new TQHBox(inactiveShadowSettings);
	inactiveShadowXOffsetHBox->setSpacing(KDialog::spacingHint());
	inactiveShadowXOffsetLabel = new TQLabel(
			i18n("Offset rightward (may be negative):"),
			inactiveShadowXOffsetHBox);
	inactiveShadowXOffsetSpinBox = new TQSpinBox(-1024, 1024, 1,
			inactiveShadowXOffsetHBox);
	inactiveShadowXOffsetSpinBox->setSuffix(i18n(" pixels"));
	connect(inactiveShadowXOffsetSpinBox, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	shadowYOffsetHBox = new TQHBox(activeShadowSettings);
	shadowYOffsetHBox->setSpacing(KDialog::spacingHint());
	shadowYOffsetLabel = new TQLabel(
			i18n("Offset downward (may be negative):"),
			shadowYOffsetHBox);
	shadowYOffsetSpinBox = new TQSpinBox(-1024, 1024, 1, shadowYOffsetHBox);
	shadowYOffsetSpinBox->setSuffix(i18n(" pixels"));
	connect(shadowYOffsetSpinBox, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	inactiveShadowYOffsetHBox = new TQHBox(inactiveShadowSettings);
	inactiveShadowYOffsetHBox->setSpacing(KDialog::spacingHint());
	inactiveShadowYOffsetLabel = new TQLabel(
			i18n("Offset downward (may be negative):"),
			inactiveShadowYOffsetHBox);
	inactiveShadowYOffsetSpinBox = new TQSpinBox(-1024, 1024, 1,
			inactiveShadowYOffsetHBox);
	inactiveShadowYOffsetSpinBox->setSuffix(i18n(" pixels"));
	connect(inactiveShadowYOffsetSpinBox, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	shadowThicknessHBox = new TQHBox(activeShadowSettings);
	shadowThicknessHBox->setSpacing(KDialog::spacingHint());
	shadowThicknessLabel = new TQLabel(
			i18n("Thickness to either side of window:"),
			shadowThicknessHBox);
	shadowThicknessSpinBox = new TQSpinBox(1, 100, 1,
			shadowThicknessHBox);
	shadowThicknessSpinBox->setSuffix(i18n(" pixels"));
	connect(shadowThicknessSpinBox, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	inactiveShadowThicknessHBox = new TQHBox(inactiveShadowSettings);
	inactiveShadowThicknessHBox->setSpacing(KDialog::spacingHint());
	inactiveShadowThicknessLabel = new TQLabel(
			i18n("Thickness to either side of window:"),
			inactiveShadowThicknessHBox);
	inactiveShadowThicknessSpinBox = new TQSpinBox(1, 100, 1,
			inactiveShadowThicknessHBox);
	inactiveShadowThicknessSpinBox->setSuffix(i18n(" pixels"));
	connect(inactiveShadowThicknessSpinBox, TQT_SIGNAL(valueChanged(int)),
			TQT_SLOT(slotSelectionChanged()));

	// Load all installed decorations into memory
	// Set up the decoration lists and other UI settings
	findDecorations();
	createDecorationList();
	readConfig( &kwinConfig );
	resetPlugin( &kwinConfig );

	tabWidget->insertTab( pluginPage, i18n("&Window Decoration") );
	tabWidget->insertTab( buttonPage, i18n("&Buttons") );
	tabWidget->insertTab( shadowPage, i18n("&Shadows") );

	connect( buttonPositionWidget, TQT_SIGNAL(changed()), this, TQT_SLOT(slotButtonsChanged()) ); // update preview etc.
	connect( buttonPositionWidget, TQT_SIGNAL(changed()), this, TQT_SLOT(slotSelectionChanged()) ); // emit changed()...
	connect( decorationList, TQT_SIGNAL(activated(const TQString&)), TQT_SLOT(slotSelectionChanged()) );
	connect( decorationList, TQT_SIGNAL(activated(const TQString&)),
								TQT_SLOT(slotChangeDecoration(const TQString&)) );
	connect( cbUseCustomButtonPositions, TQT_SIGNAL(clicked()), TQT_SLOT(slotSelectionChanged()) );
	connect(cbUseCustomButtonPositions, TQT_SIGNAL(toggled(bool)), buttonPositionWidget, TQT_SLOT(setEnabled(bool)));
	connect(cbUseCustomButtonPositions, TQT_SIGNAL(toggled(bool)), this, TQT_SLOT(slotButtonsChanged()) );
 	connect(cbWindowShadow, TQT_SIGNAL(toggled(bool)), activeShadowSettings, TQT_SLOT(setEnabled(bool)));
 	connect(cbWindowShadow, TQT_SIGNAL(toggled(bool)), inactiveShadowSettings, TQT_SLOT(setEnabled(bool)));
 	connect(cbWindowShadow, TQT_SIGNAL(toggled(bool)), whichShadowSettings, TQT_SLOT(setEnabled(bool)));

	connect( cbShowToolTips, TQT_SIGNAL(clicked()), TQT_SLOT(slotSelectionChanged()) );
	connect( cbWindowShadow, TQT_SIGNAL(clicked()), TQT_SLOT(slotSelectionChanged()) );
	connect( cBorder, TQT_SIGNAL( activated( int )), TQT_SLOT( slotBorderChanged( int )));
//	connect( cbUseMiniWindows, TQT_SIGNAL(clicked()), TQT_SLOT(slotSelectionChanged()) );

	// Allow kwin dcop signal to update our selection list
	connectDCOPSignal("kwin", 0, "dcopResetAllClients()", "dcopUpdateClientList()", false);
	
	KAboutData *about =
		new KAboutData(I18N_NOOP("kcmkwindecoration"),
				I18N_NOOP("Window Decoration Control Module"),
				0, 0, KAboutData::License_GPL,
				I18N_NOOP("(c) 2001 Karol Szwed"));
	about->addAuthor("Karol Szwed", 0, "gallium@kde.org");
	setAboutData(about);
}


KWinDecorationModule::~KWinDecorationModule()
{
        delete preview; // needs to be destroyed before plugins
        delete plugins;
}


// Find all theme desktop files in all 'data' dirs owned by kwin.
// And insert these into a DecorationInfo structure
void KWinDecorationModule::findDecorations()
{
	TQStringList dirList = KGlobal::dirs()->findDirs("data", "kwin");
	TQStringList::ConstIterator it;

	for (it = dirList.begin(); it != dirList.end(); it++)
	{
		TQDir d(*it);
		if (d.exists())
			for (TQFileInfoListIterator it(*d.entryInfoList()); it.current(); ++it)
			{
				TQString filename(it.current()->absFilePath());
				if (KDesktopFile::isDesktopFile(filename))
				{
					KDesktopFile desktopFile(filename);
					TQString libName = desktopFile.readEntry("X-KDE-Library");

					if (!libName.isEmpty() && libName.startsWith( "kwin3_" ))
					{
						DecorationInfo di;
						di.name = desktopFile.readName();
						di.libraryName = libName;
						decorations.append( di );
					}
				}
			}
	}
}


// Fills the decorationList with a list of available kwin decorations
void KWinDecorationModule::createDecorationList()
{
	TQValueList<DecorationInfo>::ConstIterator it;

	// Sync with kwin hardcoded KDE2 style which has no desktop item
    TQStringList decorationNames;
	decorationNames.append( i18n("KDE 2") );
	for (it = decorations.begin(); it != decorations.end(); ++it)
	{
		decorationNames.append((*it).name);
	}
	decorationNames.sort();
    decorationList->insertStringList(decorationNames);
}


// Reset the decoration plugin to what the user just selected
void KWinDecorationModule::slotChangeDecoration( const TQString & text)
{
	KConfig kwinConfig("kwinrc");
	kwinConfig.setGroup("Style");

	// Let the user see config options for the currently selected decoration
	resetPlugin( &kwinConfig, text );
}


// This is the selection handler setting
void KWinDecorationModule::slotSelectionChanged()
{
	emit KCModule::changed(true);
}

static const char* const border_names[ KDecorationDefines::BordersCount ] =
    {
    I18N_NOOP( "Tiny" ),
    I18N_NOOP( "Normal" ),
    I18N_NOOP( "Large" ),
    I18N_NOOP( "Very Large" ),
    I18N_NOOP( "Huge" ),
    I18N_NOOP( "Very Huge" ),
    I18N_NOOP( "Oversized" )
    };

int KWinDecorationModule::borderSizeToIndex( BorderSize size, TQValueList< BorderSize > sizes )
{
        int pos = 0;
        for( TQValueList< BorderSize >::ConstIterator it = sizes.begin();
             it != sizes.end();
             ++it, ++pos )
            if( size <= *it )
                break;
        return pos;
}

KDecorationDefines::BorderSize KWinDecorationModule::indexToBorderSize( int index,
    TQValueList< BorderSize > sizes )
{
        TQValueList< BorderSize >::ConstIterator it = sizes.begin();
        for(;
             it != sizes.end();
             ++it, --index )
            if( index == 0 )
                break;
        return *it;
}

void KWinDecorationModule::slotBorderChanged( int size )
{
        if( lBorder->isHidden())
            return;
        emit KCModule::changed( true );
        TQValueList< BorderSize > sizes;
        if( plugins->factory() != NULL )
            sizes = plugins->factory()->borderSizes();
        assert( sizes.count() >= 2 );
        border_size = indexToBorderSize( size, sizes );

	// update preview
	preview->setTempBorderSize(plugins, border_size);
}

void KWinDecorationModule::slotButtonsChanged()
{
	// update preview
	preview->setTempButtons(plugins, cbUseCustomButtonPositions->isChecked(), buttonPositionWidget->buttonsLeft(), buttonPositionWidget->buttonsRight() );
}

TQString KWinDecorationModule::decorationName( TQString& libName )
{
	TQString decoName;

	TQValueList<DecorationInfo>::Iterator it;
	for( it = decorations.begin(); it != decorations.end(); ++it )
		if ( (*it).libraryName == libName )
		{
			decoName = (*it).name;
			break;
		}

	return decoName;
}


TQString KWinDecorationModule::decorationLibName( const TQString& name )
{
	TQString libName;

	// Find the corresponding library name to that of
	// the current plugin name
	TQValueList<DecorationInfo>::Iterator it;
	for( it = decorations.begin(); it != decorations.end(); ++it )
		if ( (*it).name == name )
		{
			libName = (*it).libraryName;
			break;
		}

	if (libName.isEmpty())
		libName = "kwin_default";	// KDE 2

	return libName;
}


// Loads/unloads and inserts the decoration config plugin into the
// pluginConfigWidget, allowing for dynamic configuration of decorations
void KWinDecorationModule::resetPlugin( KConfig* conf, const TQString& currentDecoName )
{
	// Config names are "kwin_icewm_config"
	// for "kwin3_icewm" kwin client

	TQString oldName = styleToConfigLib( oldLibraryName );

	TQString currentName;
	if (!currentDecoName.isEmpty())
		currentName = decorationLibName( currentDecoName ); // Use what the user selected
	else
		currentName = currentLibraryName; // Use what was read from readConfig()

        if( plugins->loadPlugin( currentName )
            && preview->recreateDecoration( plugins ))
            preview->enablePreview();
        else
            preview->disablePreview();
        plugins->destroyPreviousPlugin();

        checkSupportedBorderSizes();

	// inform buttonPositionWidget about the new factory...
	buttonPositionWidget->setDecorationFactory(plugins->factory() );

	currentName = styleToConfigLib( currentName );

	// Delete old plugin widget if it exists
	delete pluginObject;
	pluginObject = 0;

	// Use klibloader for library manipulation
	KLibLoader* loader = KLibLoader::self();

	// Free the old library if possible
	if (!oldLibraryName.isNull())
		loader->unloadLibrary( TQFile::encodeName(oldName) );

	KLibrary* library = loader->library( TQFile::encodeName(currentName) );
	if (library != NULL)
	{
		void* alloc_ptr = library->symbol("allocate_config");

		if (alloc_ptr != NULL)
		{
			allocatePlugin = (TQObject* (*)(KConfig* conf, TQWidget* parent))alloc_ptr;
			pluginObject = (TQObject*)(allocatePlugin( conf, pluginConfigWidget ));

			// connect required signals and slots together...
			connect( pluginObject, TQT_SIGNAL(changed()), this, TQT_SLOT(slotSelectionChanged()) );
			connect( this, TQT_SIGNAL(pluginLoad(KConfig*)), pluginObject, TQT_SLOT(load(KConfig*)) );
			connect( this, TQT_SIGNAL(pluginSave(KConfig*)), pluginObject, TQT_SLOT(save(KConfig*)) );
			connect( this, TQT_SIGNAL(pluginDefaults()), pluginObject, TQT_SLOT(defaults()) );
			pluginConfigWidget->show();
			return;
		}
	}

	pluginConfigWidget->hide();
}


// Reads the kwin config settings, and sets all UI controls to those settings
// Updating the config plugin if required
void KWinDecorationModule::readConfig( KConfig* conf )
{
	// General tab
	// ============
	cbShowToolTips->setChecked( conf->readBoolEntry("ShowToolTips", true ));
//	cbUseMiniWindows->setChecked( conf->readBoolEntry("MiniWindowBorders", false));

	// Find the corresponding decoration name to that of
	// the current plugin library name

	oldLibraryName = currentLibraryName;
	currentLibraryName = conf->readEntry("PluginLib",
					((TQPixmap::defaultDepth() > 8) ? "kwin_plastik" : "kwin_quartz"));
	TQString decoName = decorationName( currentLibraryName );

	// If we are using the "default" kde client, use the "default" entry.
	if (decoName.isEmpty())
		decoName = i18n("KDE 2");

    int numDecos = decorationList->count();
	for (int i = 0; i < numDecos; ++i)
    {
		 if (decorationList->text(i) == decoName)
		 {
		 		 decorationList->setCurrentItem(i);
		 		 break;
		 }
	}

	// Buttons tab
	// ============
	bool customPositions = conf->readBoolEntry("CustomButtonPositions", false);
	cbUseCustomButtonPositions->setChecked( customPositions );
	buttonPositionWidget->setEnabled( customPositions );
	// Menu and onAllDesktops buttons are default on LHS
	buttonPositionWidget->setButtonsLeft( conf->readEntry("ButtonsOnLeft", "MS") );
	// Help, Minimize, Maximize and Close are default on RHS
	buttonPositionWidget->setButtonsRight( conf->readEntry("ButtonsOnRight", "HIAX") );

        int bsize = conf->readNumEntry( "BorderSize", BorderNormal );
        if( bsize >= BorderTiny && bsize < BordersCount )
            border_size = static_cast< BorderSize >( bsize );
        else
            border_size = BorderNormal;
        checkSupportedBorderSizes();

	// Shadows tab
	// ===========
	bool shadowEnabled = conf->readBoolEntry("ShadowEnabled", false);
 	cbWindowShadow->setChecked(shadowEnabled);
	activeShadowSettings->setEnabled(shadowEnabled);
	inactiveShadowSettings->setEnabled(shadowEnabled);
	whichShadowSettings->setEnabled(shadowEnabled);
	shadowColourButton->setColor(conf->readColorEntry("ShadowColour", &TQt::black));
 	shadowOpacitySlider->setValue((int)ceil(conf->readDoubleNumEntry("ShadowOpacity", 0.70) * 100));
 	shadowXOffsetSpinBox->setValue(conf->readNumEntry("ShadowXOffset", 0));
 	shadowYOffsetSpinBox->setValue(conf->readNumEntry("ShadowYOffset", 10));
 	cbShadowDocks->setChecked(conf->readBoolEntry("ShadowDocks", false));
 	cbShadowOverrides->setChecked(conf->readBoolEntry("ShadowOverrides", false));
 	cbShadowTopMenus->setChecked(conf->readBoolEntry("ShadowTopMenus", false));
 	shadowThicknessSpinBox->setValue(conf->readNumEntry("ShadowThickness", 10));
 	cbInactiveShadow->setChecked(conf->readBoolEntry("InactiveShadowEnabled", false));
 	inactiveShadowColourButton->setColor(conf->readColorEntry("InactiveShadowColour", &TQt::black));
 	inactiveShadowOpacitySlider->setValue((int)ceil(conf->readDoubleNumEntry("InactiveShadowOpacity", 0.70) * 100));
 	inactiveShadowXOffsetSpinBox->setValue(conf->readNumEntry("InactiveShadowXOffset", 0));
 	inactiveShadowYOffsetSpinBox->setValue(conf->readNumEntry("InactiveShadowYOffset", 5));
 	inactiveShadowThicknessSpinBox->setValue(conf->readNumEntry("InactiveShadowThickness", 5));

	emit KCModule::changed(false);
}


// Writes the selected user configuration to the kwin config file
void KWinDecorationModule::writeConfig( KConfig* conf )
{
	TQString name = decorationList->currentText();
	TQString libName = decorationLibName( name );

	KConfig kwinConfig("kwinrc");
	kwinConfig.setGroup("Style");

	// General settings
	conf->writeEntry("PluginLib", libName);
	conf->writeEntry("CustomButtonPositions", cbUseCustomButtonPositions->isChecked());
	conf->writeEntry("ShowToolTips", cbShowToolTips->isChecked());
//	conf->writeEntry("MiniWindowBorders", cbUseMiniWindows->isChecked());

	// Button settings
	conf->writeEntry("ButtonsOnLeft", buttonPositionWidget->buttonsLeft() );
	conf->writeEntry("ButtonsOnRight", buttonPositionWidget->buttonsRight() );
        conf->writeEntry("BorderSize", border_size );

	// Shadow settings
	conf->writeEntry("ShadowEnabled", cbWindowShadow->isChecked());
	conf->writeEntry("ShadowColour", shadowColourButton->color());
	conf->writeEntry("ShadowOpacity", shadowOpacitySlider->value() / 100.0);
	conf->writeEntry("ShadowXOffset", shadowXOffsetSpinBox->value());
	conf->writeEntry("ShadowYOffset", shadowYOffsetSpinBox->value());
	conf->writeEntry("ShadowThickness", shadowThicknessSpinBox->value());
	conf->writeEntry("ShadowDocks", cbShadowDocks->isChecked());
	conf->writeEntry("ShadowOverrides", cbShadowOverrides->isChecked());
	conf->writeEntry("ShadowTopMenus", cbShadowTopMenus->isChecked());
	conf->writeEntry("InactiveShadowEnabled", cbInactiveShadow->isChecked());
	conf->writeEntry("InactiveShadowColour", inactiveShadowColourButton->color());
	conf->writeEntry("InactiveShadowOpacity",
			inactiveShadowOpacitySlider->value() / 100.0);
	conf->writeEntry("InactiveShadowXOffset",
			inactiveShadowXOffsetSpinBox->value());
	conf->writeEntry("InactiveShadowYOffset",
			inactiveShadowYOffsetSpinBox->value());
	conf->writeEntry("InactiveShadowThickness",
			inactiveShadowThicknessSpinBox->value());

	oldLibraryName = currentLibraryName;
	currentLibraryName = libName;

	// We saved, so tell kcmodule that there have been  no new user changes made.
	emit KCModule::changed(false);
}


void KWinDecorationModule::dcopUpdateClientList()
{
	// Changes the current active ListBox item, and
	// Loads a new plugin configuration tab if required.
	KConfig kwinConfig("kwinrc");
	kwinConfig.setGroup("Style");

	readConfig( &kwinConfig );
	resetPlugin( &kwinConfig );
}


// Virutal functions required by KCModule
void KWinDecorationModule::load()
{
	KConfig kwinConfig("kwinrc");
	kwinConfig.setGroup("Style");

	// Reset by re-reading the config
	readConfig( &kwinConfig );
        resetPlugin( &kwinConfig );
}


void KWinDecorationModule::save()
{
	KConfig kwinConfig("kwinrc");
	kwinConfig.setGroup("Style");

	writeConfig( &kwinConfig );
	emit pluginSave( &kwinConfig );

	kwinConfig.sync();
	resetKWin();
	// resetPlugin() will get called via the above DCOP function
}


void KWinDecorationModule::defaults()
{
	// Set the KDE defaults
	cbUseCustomButtonPositions->setChecked( false );
	buttonPositionWidget->setEnabled( false );
	cbShowToolTips->setChecked( true );
	cbWindowShadow->setChecked( false );
//	cbUseMiniWindows->setChecked( false);
// Don't set default for now
//	decorationList->setSelected(
//		decorationList->findItem( i18n("KDE 2") ), true );  // KDE classic client

	buttonPositionWidget->setButtonsLeft("MS");
	buttonPositionWidget->setButtonsRight("HIAX");

        border_size = BorderNormal;
        checkSupportedBorderSizes();

	shadowColourButton->setColor(Qt::black);
	shadowOpacitySlider->setValue(70);
	shadowXOffsetSpinBox->setValue(0);
	shadowYOffsetSpinBox->setValue(10);
	shadowThicknessSpinBox->setValue(10);
	cbShadowDocks->setChecked(false);
	cbShadowOverrides->setChecked(false);
	cbShadowTopMenus->setChecked(false);
	cbInactiveShadow->setChecked(false);
	inactiveShadowColourButton->setColor(Qt::black);
	inactiveShadowOpacitySlider->setValue(70);
	inactiveShadowXOffsetSpinBox->setValue(0);
	inactiveShadowYOffsetSpinBox->setValue(5);
	inactiveShadowThicknessSpinBox->setValue(5);

	// Set plugin defaults
	emit pluginDefaults();
}

void KWinDecorationModule::checkSupportedBorderSizes()
{
        TQValueList< BorderSize > sizes;
        if( plugins->factory() != NULL )
            sizes = plugins->factory()->borderSizes();
	if( sizes.count() < 2 ) {
		lBorder->hide();
		cBorder->hide();
	} else {
		cBorder->clear();
		for (TQValueList<BorderSize>::const_iterator it = sizes.begin(); it != sizes.end(); ++it) {
			BorderSize size = *it;
			cBorder->insertItem(i18n(border_names[size]), borderSizeToIndex(size,sizes) );
		}
		int pos = borderSizeToIndex( border_size, sizes );
		lBorder->show();
		cBorder->show();
		cBorder->setCurrentItem(pos);
		slotBorderChanged( pos );
	}
}

TQString KWinDecorationModule::styleToConfigLib( TQString& styleLib )
{
        if( styleLib.startsWith( "kwin3_" ))
            return "kwin_" + styleLib.mid( 6 ) + "_config";
        else
            return styleLib + "_config";
}

TQString KWinDecorationModule::quickHelp() const
{
	return i18n( "<h1>Window Manager Decoration</h1>"
		"<p>This module allows you to choose the window border decorations, "
		"as well as titlebar button positions and custom decoration options.</p>"
		"To choose a theme for your window decoration click on its name and apply your choice by clicking the \"Apply\" button below."
		" If you do not want to apply your choice you can click the \"Reset\" button to discard your changes."
		"<p>You can configure each theme in the \"Configure [...]\" tab. There are different options specific for each theme.</p>"
		"<p>In \"General Options (if available)\" you can activate the \"Buttons\" tab by checking the \"Use custom titlebar button positions\" box."
		" In the \"Buttons\" tab you can change the positions of the buttons to your liking.</p>" );
}


void KWinDecorationModule::resetKWin()
{
	bool ok = kapp->dcopClient()->send("kwin*", "KWinInterface",
                        "reconfigure()", TQByteArray());
	if (!ok)
		kdDebug() << "kcmkwindecoration: Could not reconfigure kwin" << endl;
}

#include "kwindecoration.moc"
// vim: ts=4
// kate: space-indent off; tab-width 4;

