////
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998
// (c) 2001 Waldo Bastian <bastian@kde.org>

#include <tqlayout.h>//CT - 12Nov1998
#include <tqwhatsthis.h>
#include <tqvgroupbox.h>
#include <tqlabel.h>
#include <tqpushbutton.h>

#include "htmlopts.h"
#include "advancedTabDialog.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <kglobalsettings.h> // get default for DEFAULT_CHANGECURSOR
#include <klocale.h>
#include <kdialog.h>
#include <knuminput.h>
#include <kseparator.h>

#include <kapplication.h>
#include <dcopclient.h>


#include "htmlopts.moc"

enum UnderlineLinkType { UnderlineAlways=0, UnderlineNever=1, UnderlineHover=2 };
enum AnimationsType { AnimationsAlways=0, AnimationsNever=1, AnimationsLoopOnce=2 };
enum SmoothScrollingType { SmoothScrollingAlways=0, SmoothScrollingNever=1, SmoothScrollingWhenEfficient=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(KConfig *config, TQString group, TQWidget *parent, const char *)
    : KCModule( parent, "kcmkonqhtml" ), m_pConfig(config), m_groupname(group)
{
    int row = 0;
    TQGridLayout *lay = new TQGridLayout(this, 10, 2, 0, KDialog::spacingHint());

    // Bookmarks
    setQuickHelp( i18n("<h1>Konqueror Browser</h1> Here you can configure Konqueror's browser "
              "functionality. Please note that the file manager "
              "functionality has to be configured using the \"File Manager\" "
              "configuration module. You can make some "
              "settings how Konqueror should handle the HTML code in "
              "the web pages it loads. It is usually not necessary to "
              "change anything here."));

    TQVGroupBox *bgBookmarks = new TQVGroupBox( i18n("Boo&kmarks"), this );
    m_pAdvancedAddBookmarkCheckBox = new TQCheckBox(i18n( "Ask for name and folder when adding bookmarks" ), bgBookmarks);
    TQWhatsThis::add( m_pAdvancedAddBookmarkCheckBox, i18n( "If this box is checked, Konqueror will allow you to"
                                                        " change the title of the bookmark and choose a folder in which to store it when you add a new bookmark." ) );
    connect(m_pAdvancedAddBookmarkCheckBox, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));

    m_pOnlyMarkedBookmarksCheckBox = new TQCheckBox(i18n( "Show only marked bookmarks in bookmark toolbar" ), bgBookmarks);
    TQWhatsThis::add( m_pOnlyMarkedBookmarksCheckBox, i18n( "If this box is checked, Konqueror will show only those"
                                                         " bookmarks in the bookmark toolbar which you have marked to do so in the bookmark editor." ) );
    connect(m_pOnlyMarkedBookmarksCheckBox, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));

    lay->addMultiCellWidget( bgBookmarks, row, row, 0, 1 );
    row++;

     // Form completion

    TQVGroupBox *bgForm = new TQVGroupBox( i18n("Form Com&pletion"), this );
    m_pFormCompletionCheckBox = new TQCheckBox(i18n( "Enable completion of &forms" ), bgForm);
    TQWhatsThis::add( m_pFormCompletionCheckBox, i18n( "If this box is checked, Konqueror will remember"
                                                        " the data you enter in web forms and suggest it in similar fields for all forms." ) );
    connect(m_pFormCompletionCheckBox, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));

    m_pMaxFormCompletionItems = new KIntNumInput( bgForm );
    m_pMaxFormCompletionItems->setLabel( i18n( "&Maximum completions:" ) );
    m_pMaxFormCompletionItems->setRange( 0, 100 );
    TQWhatsThis::add( m_pMaxFormCompletionItems,
        i18n( "Here you can select how many values Konqueror will remember for a form field." ) );
    connect(m_pMaxFormCompletionItems, TQT_SIGNAL(valueChanged(int)), TQT_SLOT(slotChanged()));

    lay->addMultiCellWidget( bgForm, row, row, 0, 1 );
    row++;

    // Tabbed Browsing

    TQGroupBox *bgTabbedBrowsing = new TQGroupBox( 0, Qt::Vertical, i18n("Tabbed Browsing"), this );
    TQVBoxLayout *laygroup = new TQVBoxLayout(bgTabbedBrowsing->layout(), KDialog::spacingHint() );

    m_pShowMMBInTabs = new TQCheckBox( i18n( "Open &links in new tab instead of in new window" ), bgTabbedBrowsing );
    TQWhatsThis::add( m_pShowMMBInTabs, i18n("This will open a new tab instead of a new window in various situations, "
                          "such as choosing a link or a folder with the middle mouse button.") );
    connect(m_pShowMMBInTabs, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));
    laygroup->addWidget(m_pShowMMBInTabs);

    m_pDynamicTabbarHide = new TQCheckBox( i18n( "Hide the tab bar when only one tab is open" ), bgTabbedBrowsing );
    TQWhatsThis::add( m_pDynamicTabbarHide, i18n("This will display the tab bar only if there are two or more tabs. Otherwise it will always be displayed.") );
    connect(m_pDynamicTabbarHide, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));
    laygroup->addWidget(m_pDynamicTabbarHide);

    TQHBoxLayout *laytab = new TQHBoxLayout(laygroup, KDialog::spacingHint());
    TQPushButton *advancedTabButton = new TQPushButton( i18n( "Advanced Options"), bgTabbedBrowsing );
    laytab->addWidget(advancedTabButton);
    laytab->addStretch();
    connect(advancedTabButton, TQT_SIGNAL(clicked()), this, TQT_SLOT(launchAdvancedTabDialog()));

    lay->addMultiCellWidget( bgTabbedBrowsing, row, row, 0, 1 );
    row++;

    // Mouse behavior

    TQVGroupBox *bgMouse = new TQVGroupBox( i18n("Mouse Beha&vior"), this );

    m_cbCursor = new TQCheckBox(i18n("Chan&ge cursor over links"), bgMouse );
    TQWhatsThis::add( m_cbCursor, i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );
    connect(m_cbCursor, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));

    m_pOpenMiddleClick = new TQCheckBox( i18n ("M&iddle click opens URL in selection" ), bgMouse );
    TQWhatsThis::add( m_pOpenMiddleClick, i18n (
      "If this box is checked, you can open the URL in the selection by middle clicking on a "
      "Konqueror view." ) );
    connect(m_pOpenMiddleClick, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));

    m_pBackRightClick = new TQCheckBox( i18n( "Right click goes &back in history" ), bgMouse );
    TQWhatsThis::add( m_pBackRightClick, i18n(
      "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
      "To access the context menu, press the right mouse button and move." ) );
    connect(m_pBackRightClick, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));

    lay->addMultiCellWidget( bgMouse, row, row, 0, 1 );
    row++;

    // Misc

    m_pAutoLoadImagesCheckBox = new TQCheckBox( i18n( "A&utomatically load images"), this );
    TQWhatsThis::add( m_pAutoLoadImagesCheckBox, i18n( "If this box is checked, Konqueror will automatically load any images that are embedded in a web page. Otherwise, it will display placeholders for the images, and you can then manually load the images by clicking on the image button.<br>Unless you have a very slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pAutoLoadImagesCheckBox, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAutoLoadImagesCheckBox, row, row, 0, 1 );
    row++;

    m_pUnfinishedImageFrameCheckBox = new TQCheckBox( i18n( "Dra&w frame around not completely loaded images"), this );
    TQWhatsThis::add( m_pUnfinishedImageFrameCheckBox, i18n( "If this box is checked, Konqueror will draw a frame as placeholder around not yet fully loaded images that are embedded in a web page.<br>Especially if you have a slow network connection, you will probably want to check this box to enhance your browsing experience." ) );
    connect(m_pUnfinishedImageFrameCheckBox, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pUnfinishedImageFrameCheckBox, row, row, 0, 1 );
    row++;

    m_pAutoRedirectCheckBox = new TQCheckBox( i18n( "Allow automatic delayed &reloading/redirecting"), this );
    TQWhatsThis::add( m_pAutoRedirectCheckBox,
    i18n( "Some web pages request an automatic reload or redirection after a certain period of time. By unchecking this box Konqueror will ignore these requests." ) );
    connect(m_pAutoRedirectCheckBox, TQT_SIGNAL(clicked()), TQT_SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAutoRedirectCheckBox, row, row, 0, 1 );
    row++;


    // More misc

    KSeparator *sep = new KSeparator(this);
    lay->addMultiCellWidget(sep, row, row, 0, 1);
    row++;

    TQLabel *label = new TQLabel( i18n("Und&erline links:"), this);
    m_pUnderlineCombo = new TQComboBox( false, this );
    label->setBuddy(m_pUnderlineCombo);
    m_pUnderlineCombo->insertItem(i18n("underline","Enabled"), UnderlineAlways);
    m_pUnderlineCombo->insertItem(i18n("underline","Disabled"), UnderlineNever);
    m_pUnderlineCombo->insertItem(i18n("Only on Hover"), UnderlineHover);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pUnderlineCombo, row, 1);
    row++;
    TQString whatsThis = i18n("Controls how Konqueror handles underlining hyperlinks:<br>"
	    "<ul><li><b>Enabled</b>: Always underline links</li>"
	    "<li><b>Disabled</b>: Never underline links</li>"
	    "<li><b>Only on Hover</b>: Underline when the mouse is moved over the link</li>"
	    "</ul><br><i>Note: The site's CSS definitions can override this value</i>");
    TQWhatsThis::add( label, whatsThis);
    TQWhatsThis::add( m_pUnderlineCombo, whatsThis);
    connect(m_pUnderlineCombo, TQT_SIGNAL(activated(int)), TQT_SLOT(slotChanged()));



    label = new TQLabel( i18n("A&nimations:"), this);
    m_pAnimationsCombo = new TQComboBox( false, this );
    label->setBuddy(m_pAnimationsCombo);
    m_pAnimationsCombo->insertItem(i18n("animations","Enabled"), AnimationsAlways);
    m_pAnimationsCombo->insertItem(i18n("animations","Disabled"), AnimationsNever);
    m_pAnimationsCombo->insertItem(i18n("Show Only Once"), AnimationsLoopOnce);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pAnimationsCombo, row, 1);
    row++;
    whatsThis = i18n("Controls how Konqueror shows animated images:<br>"
	    "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
	    "<li><b>Disabled</b>: Never show animations, show the start image only.</li>"
	    "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li></ul>");
    TQWhatsThis::add( label, whatsThis);
    TQWhatsThis::add( m_pAnimationsCombo, whatsThis);
    connect(m_pAnimationsCombo, TQT_SIGNAL(activated(int)), TQT_SLOT(slotChanged()));

    label = new TQLabel( i18n("Sm&ooth scrolling"), this);
    m_pSmoothScrollingCombo = new TQComboBox( false, this );
    label->setBuddy(m_pSmoothScrollingCombo);
    m_pSmoothScrollingCombo->insertItem(i18n("SmoothScrolling","Enabled"), SmoothScrollingAlways);
    m_pSmoothScrollingCombo->insertItem(i18n("SmoothScrolling","Disabled"), SmoothScrollingNever);
    // not implemented: m_pSmoothScrollingCombo->insertItem(i18n("SmoothScrolling","WhenEfficient"), SmoothScrollingWhenEfficient);
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pSmoothScrollingCombo, row, 1);
    row++;
    whatsThis = i18n("Determines whether Konqueror should use smooth steps to scroll HTML pages, or whole steps:<br>"
	    "<ul><li><b>Always</b>: Always use smooth steps when scrolling.</li>"
	    "<li><b>Never</b>: Never use smooth scrolling, scroll with whole steps instead.</li>"
	    // not implemented: "<li><b>When Efficient</b>: Only use smooth scrolling on pages where it can be achieved with moderate usage of system resources.</li>"
	    "</ul>");
    TQWhatsThis::add( label, whatsThis);
    TQWhatsThis::add( m_pSmoothScrollingCombo, whatsThis);
    connect(m_pSmoothScrollingCombo, TQT_SIGNAL(activated(int)), TQT_SLOT(slotChanged()));


    lay->setRowStretch(row, 1);

    load();
    emit changed(false);
}

KMiscHTMLOptions::~KMiscHTMLOptions()
{
    delete m_pConfig;
}

void KMiscHTMLOptions::load()
{
	load( false );
}

void KMiscHTMLOptions::load( bool useDefaults )
{
    KConfig khtmlrc("khtmlrc", true, false);
    khtmlrc.setReadDefaults( useDefaults );
	 m_pConfig->setReadDefaults( useDefaults );

#define SET_GROUP(x) m_pConfig->setGroup(x); khtmlrc.setGroup(x)
#define READ_BOOL(x,y) m_pConfig->readBoolEntry(x, khtmlrc.readBoolEntry(x, y))
#define READ_ENTRY(x) m_pConfig->readEntry(x, khtmlrc.readEntry(x))


    // *** load ***
    SET_GROUP( "MainView Settings" );
    bool bOpenMiddleClick = READ_BOOL( "OpenMiddleClick", true );
    bool bBackRightClick = READ_BOOL( "BackRightClick", false );
    SET_GROUP( "HTML Settings" );
    bool changeCursor = READ_BOOL("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
    bool underlineLinks = READ_BOOL("UnderlineLinks", DEFAULT_UNDERLINELINKS);
    bool hoverLinks = READ_BOOL("HoverLinks", true);
    bool bAutoLoadImages = READ_BOOL( "AutoLoadImages", true );
    bool bUnfinishedImageFrame = READ_BOOL( "UnfinishedImageFrame", true );
    TQString strAnimations = READ_ENTRY( "ShowAnimations" ).lower();

    bool bAutoRedirect = m_pConfig->readBoolEntry( "AutoDelayedActions", true );

    // *** apply to GUI ***
    m_cbCursor->setChecked( changeCursor );
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );
    m_pUnfinishedImageFrameCheckBox->setChecked( bUnfinishedImageFrame );
    m_pAutoRedirectCheckBox->setChecked( bAutoRedirect );
    m_pOpenMiddleClick->setChecked( bOpenMiddleClick );
    m_pBackRightClick->setChecked( bBackRightClick );

    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (hoverLinks)
    {
        m_pUnderlineCombo->setCurrentItem( UnderlineHover );
    }
    else
    {
        if (underlineLinks)
            m_pUnderlineCombo->setCurrentItem( UnderlineAlways );
        else
            m_pUnderlineCombo->setCurrentItem( UnderlineNever );
    }
    if (strAnimations == "disabled")
       m_pAnimationsCombo->setCurrentItem( AnimationsNever );
    else if (strAnimations == "looponce")
       m_pAnimationsCombo->setCurrentItem( AnimationsLoopOnce );
    else
       m_pAnimationsCombo->setCurrentItem( AnimationsAlways );

    m_pFormCompletionCheckBox->setChecked( m_pConfig->readBoolEntry( "FormCompletion", true ) );
    m_pMaxFormCompletionItems->setValue( m_pConfig->readNumEntry( "MaxFormCompletionItems", 10 ) );
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );

    m_pConfig->setGroup("FMSettings");
    m_pShowMMBInTabs->setChecked( m_pConfig->readBoolEntry( "MMBOpensTab", false ) );
    m_pDynamicTabbarHide->setChecked( ! (m_pConfig->readBoolEntry( "AlwaysTabbedMode", false )) );

    KConfig config("kbookmarkrc", true, false);
    config.setReadDefaults( useDefaults );
	 config.setGroup("Bookmarks");
    m_pAdvancedAddBookmarkCheckBox->setChecked( config.readBoolEntry("AdvancedAddBookmarkDialog", false) );
    m_pOnlyMarkedBookmarksCheckBox->setChecked( config.readBoolEntry("FilteredToolbar", false) );

    KConfig kdeglobals("kdeglobals", true, false);
    kdeglobals.setReadDefaults( useDefaults );
	 kdeglobals.setGroup("KDE");
    bool smoothScrolling = kdeglobals.readBoolEntry("SmoothScroll", DEFAULT_SMOOTHSCROLL);
    if (smoothScrolling)
	m_pSmoothScrollingCombo->setCurrentItem( SmoothScrollingAlways );
    else
	m_pSmoothScrollingCombo->setCurrentItem( SmoothScrollingNever );

	 emit changed( useDefaults );

#undef READ_ENTRY
#undef READ_BOOL
#undef SET_GROUP
}

void KMiscHTMLOptions::defaults()
{
	load( true );
}

void KMiscHTMLOptions::save()
{
    m_pConfig->setGroup( "MainView Settings" );
    m_pConfig->writeEntry( "OpenMiddleClick", m_pOpenMiddleClick->isChecked() );
    m_pConfig->writeEntry( "BackRightClick", m_pBackRightClick->isChecked() );
    m_pConfig->setGroup( "HTML Settings" );
    m_pConfig->writeEntry( "ChangeCursor", m_cbCursor->isChecked() );
    m_pConfig->writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
    m_pConfig->writeEntry( "UnfinishedImageFrame", m_pUnfinishedImageFrameCheckBox->isChecked() );
    m_pConfig->writeEntry( "AutoDelayedActions", m_pAutoRedirectCheckBox->isChecked() );
    switch(m_pUnderlineCombo->currentItem())
    {
      case UnderlineAlways:
        m_pConfig->writeEntry( "UnderlineLinks", true );
        m_pConfig->writeEntry( "HoverLinks", false );
        break;
      case UnderlineNever:
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", false );
        break;
      case UnderlineHover:
        m_pConfig->writeEntry( "UnderlineLinks", false );
        m_pConfig->writeEntry( "HoverLinks", true );
        break;
    }
    switch(m_pAnimationsCombo->currentItem())
    {
      case AnimationsAlways:
        m_pConfig->writeEntry( "ShowAnimations", "Enabled" );
        break;
      case AnimationsNever:
        m_pConfig->writeEntry( "ShowAnimations", "Disabled" );
        break;
      case AnimationsLoopOnce:
        m_pConfig->writeEntry( "ShowAnimations", "LoopOnce" );
        break;
    }

    m_pConfig->writeEntry( "FormCompletion", m_pFormCompletionCheckBox->isChecked() );
    m_pConfig->writeEntry( "MaxFormCompletionItems", m_pMaxFormCompletionItems->value() );

    m_pConfig->setGroup("FMSettings");
    m_pConfig->writeEntry( "MMBOpensTab", m_pShowMMBInTabs->isChecked() );
    m_pConfig->writeEntry( "AlwaysTabbedMode", !(m_pDynamicTabbarHide->isChecked()) );
    m_pConfig->sync();

    KConfig config("kbookmarkrc", false, false);
    config.setGroup("Bookmarks");
    config.writeEntry("AdvancedAddBookmarkDialog", m_pAdvancedAddBookmarkCheckBox->isChecked());
    config.writeEntry("FilteredToolbar", m_pOnlyMarkedBookmarksCheckBox->isChecked());
    config.sync();

    KConfig kdeglobals("kdeglobals", false, false);
    kdeglobals.setGroup("KDE");
    switch(m_pSmoothScrollingCombo->currentItem())
    {
      case SmoothScrollingAlways:
        kdeglobals.writeEntry( "SmoothScroll", true );
        break;
      case SmoothScrollingNever:
        kdeglobals.writeEntry( "SmoothScroll", false );
        break;
      // case SmoothScrollingWhenEfficient:
        // kdeglobals.writeEntry( "SmoothScroll", somethingelse );
        // break;
    }
    kdeglobals.sync();

  TQByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

    emit changed(false);
}


void KMiscHTMLOptions::slotChanged()
{
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
    emit changed(true);
}


void KMiscHTMLOptions::launchAdvancedTabDialog()
{
    advancedTabDialog* dialog = new advancedTabDialog(this, m_pConfig, "advancedTabDialog");
    dialog->exec();
}


