
#include <tqfontdatabase.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqvgroupbox.h>
#include <tqwhatsthis.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kcharsets.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kfontcombo.h>
#include <kglobal.h>
#include <khtmldefaults.h>
#include <klocale.h>
#include <knuminput.h>

#if defined Q_WS_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#endif


#include "appearance.moc"

KAppearanceOptions::KAppearanceOptions(KConfig *config, TQString group, TQWidget *parent, const char *)
    : KCModule( parent, "kcmkonqhtml" ), m_pConfig(config), m_groupname(group),
      fSize( 10 ), fMinSize( HTML_DEFAULT_MIN_FONT_SIZE )

{
  setQuickHelp( i18n("<h1>Konqueror Fonts</h1>On this page, you can configure "
              "which fonts Konqueror should use to display the web "
              "pages you view."));

  TQString wtstr;

  TQGridLayout *lay = new TQGridLayout(this, 1 ,1 , 0, KDialog::spacingHint());
  int r = 0;
  int E = 0, M = 1, W = 3; //CT 3 (instead 2) allows smaller color buttons

  TQGroupBox* gb = new TQGroupBox( 1, Qt::Horizontal, i18n("Font Si&ze"), this );
  lay->addMultiCellWidget(gb, r, r, E, W);

  TQWhatsThis::add( gb, i18n("This is the relative font size Konqueror uses to display web sites.") );

  m_minSize = new KIntNumInput( fMinSize, gb );
  m_minSize->setLabel( i18n( "M&inimum font size:" ) );
  m_minSize->setRange( 2, 30 );
  connect( m_minSize, TQT_SIGNAL( valueChanged( int ) ), this, TQT_SLOT( slotMinimumFontSize( int ) ) );
  connect( m_minSize, TQT_SIGNAL( valueChanged( int ) ), this, TQT_SLOT( changed() ) );
  TQWhatsThis::add( m_minSize, i18n( "Konqueror will never display text smaller than "
                                    "this size,<br>overriding any other settings" ) );

  m_MedSize = new KIntNumInput( m_minSize, fSize, gb );
  m_MedSize->setLabel( i18n( "&Medium font size:" ) );
  m_MedSize->setRange( 2, 30 );
  connect( m_MedSize, TQT_SIGNAL( valueChanged( int ) ), this, TQT_SLOT( slotFontSize( int ) ) );
  connect( m_MedSize, TQT_SIGNAL( valueChanged( int ) ), this, TQT_SLOT( changed() ) );
  TQWhatsThis::add( m_MedSize,
                   i18n("This is the relative font size Konqueror uses "
                        "to display web sites.") );

  TQStringList emptyList;

  TQLabel* label = new TQLabel( i18n("S&tandard font:"), this );
  lay->addWidget( label , ++r, E);

  m_pFonts[0] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[0] );
  lay->addMultiCellWidget(m_pFonts[0], r, r, M, W);

  wtstr = i18n("This is the font used to display normal text in a web page.");
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pFonts[0], wtstr );

  connect( m_pFonts[0], TQT_SIGNAL( activated(const TQString&) ),
	   TQT_SLOT( slotStandardFont(const TQString&) ) );
  connect( m_pFonts[0], TQT_SIGNAL( activated(const TQString&) ),
	   TQT_SLOT(changed() ) );
  connect( m_pFonts[0]->lineEdit(), TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT( slotStandardFont(const TQString&) ) );
  connect( m_pFonts[0], TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT(changed() ) );

  label = new TQLabel( i18n( "&Fixed font:"), this );
  lay->addWidget( label, ++r, E );

  m_pFonts[1] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[1] );
  lay->addMultiCellWidget(m_pFonts[1], r, r, M, W);

  wtstr = i18n("This is the font used to display fixed-width (i.e. non-proportional) text.");
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pFonts[1], wtstr );

  connect( m_pFonts[1], TQT_SIGNAL( activated(const TQString&) ),
	   TQT_SLOT( slotFixedFont(const TQString&) ) );
  connect( m_pFonts[1], TQT_SIGNAL( activated(const TQString&) ),
	   TQT_SLOT(changed() ) );
  connect( m_pFonts[1]->lineEdit(), TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT( slotFixedFont(const TQString&) ) );
  connect( m_pFonts[1], TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT(changed() ) );

  label = new TQLabel( i18n( "S&erif font:" ), this );
  lay->addWidget( label, ++r, E );

  m_pFonts[2] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[2] );
  lay->addMultiCellWidget( m_pFonts[2], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as serif." );
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pFonts[2], wtstr );

  connect( m_pFonts[2], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( slotSerifFont( const TQString& ) ) );
  connect( m_pFonts[2], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( changed() ) );
  connect( m_pFonts[2]->lineEdit(), TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT( slotSerifFont(const TQString&) ) );
  connect( m_pFonts[2], TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT(changed() ) );

  label = new TQLabel( i18n( "Sa&ns serif font:" ), this );
  lay->addWidget( label, ++r, E );

  m_pFonts[3] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[3] );
  lay->addMultiCellWidget( m_pFonts[3], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as sans-serif." );
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pFonts[3], wtstr );

  connect( m_pFonts[3], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( slotSansSerifFont( const TQString& ) ) );
  connect( m_pFonts[3], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( changed() ) );
  connect( m_pFonts[3]->lineEdit(), TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT( slotSansSerifFont(const TQString&) ) );
  connect( m_pFonts[3], TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT(changed() ) );


  label = new TQLabel( i18n( "C&ursive font:" ), this );
  lay->addWidget( label, ++r, E );

  m_pFonts[4] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[4] );
  lay->addMultiCellWidget( m_pFonts[4], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as italic." );
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pFonts[4], wtstr );

  connect( m_pFonts[4], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( slotCursiveFont( const TQString& ) ) );
  connect( m_pFonts[4], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( changed() ) );
  connect( m_pFonts[4]->lineEdit(), TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT( slotCursiveFont(const TQString&) ) );
  connect( m_pFonts[4], TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT(changed() ) );


  label = new TQLabel( i18n( "Fantas&y font:" ), this );
  lay->addWidget( label, ++r, E );

  m_pFonts[5] = new KFontCombo( emptyList, this );

  label->setBuddy( m_pFonts[5] );
  lay->addMultiCellWidget( m_pFonts[5], r, r, M, W );

  wtstr= i18n( "This is the font used to display text that is marked up as a fantasy font." );
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pFonts[5], wtstr );

  connect( m_pFonts[5], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( slotFantasyFont( const TQString& ) ) );
  connect( m_pFonts[5], TQT_SIGNAL( activated( const TQString& ) ),
	   TQT_SLOT( changed() ) );
  connect( m_pFonts[5]->lineEdit(), TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT( slotFantasyFont(const TQString&) ) );
  connect( m_pFonts[5], TQT_SIGNAL( textChanged(const TQString&) ),
	   TQT_SLOT(changed() ) );


  label = new TQLabel( i18n( "Font &size adjustment for this encoding:" ), this );
  lay->addWidget( label, ++r, M );

  m_pFontSizeAdjust = new TQSpinBox( -5, 5, 1, this );
  label->setBuddy( m_pFontSizeAdjust );
  lay->addMultiCellWidget( m_pFontSizeAdjust, r, r, M+1, W );

  connect( m_pFontSizeAdjust, TQT_SIGNAL( valueChanged( int ) ),
	   TQT_SLOT( slotFontSizeAdjust( int ) ) );
  connect( m_pFontSizeAdjust, TQT_SIGNAL( valueChanged( int ) ),
	   TQT_SLOT( changed() ) );

  label = new TQLabel( i18n( "Default encoding:"), this );
  //++r;
  //lay->addMultiCellWidget( label, r, r, E, E+1);
  lay->addWidget( label, ++r, E);

  m_pEncoding = new TQComboBox( false, this );
  label->setBuddy( m_pEncoding );
  encodings = KGlobal::charsets()->availableEncodingNames();
  encodings.prepend(i18n("Use Language Encoding"));
  m_pEncoding->insertStringList( encodings );
  lay->addMultiCellWidget(m_pEncoding,r, r, M, W);

  wtstr = i18n( "Select the default encoding to be used; normally, you will be fine with 'Use language encoding' "
	       "and should not have to change this.");
  TQWhatsThis::add( label, wtstr );
  TQWhatsThis::add( m_pEncoding, wtstr );

  connect( m_pEncoding, TQT_SIGNAL( activated(const TQString& ) ),
	   TQT_SLOT( slotEncoding(const TQString&) ) );
  connect( m_pEncoding, TQT_SIGNAL( activated(const TQString& ) ),
	   TQT_SLOT( changed() ) );

  ++r; lay->setRowStretch(r, 8);

  TQFontDatabase db;

  m_families = db.families();

  m_pFonts[0]->setFonts( m_families );
  m_pFonts[1]->setFonts( m_families );
  m_pFonts[2]->setFonts( m_families );
  m_pFonts[3]->setFonts( m_families );
  m_pFonts[4]->setFonts( m_families );
  m_pFonts[5]->setFonts( m_families );

  load();
}

KAppearanceOptions::~KAppearanceOptions()
{
delete m_pConfig;
}

void KAppearanceOptions::slotFontSize( int i )
{
    fSize = i;
    if ( fSize < fMinSize ) {
        m_minSize->setValue( fSize );
        fMinSize = fSize;
    }
}


void KAppearanceOptions::slotMinimumFontSize( int i )
{
    fMinSize = i;
    if ( fMinSize > fSize ) {
        m_MedSize->setValue( fMinSize );
        fSize = fMinSize;
    }
}


void KAppearanceOptions::slotStandardFont(const TQString& n )
{
    fonts[0] = n;
}


void KAppearanceOptions::slotFixedFont(const TQString& n )
{
    fonts[1] = n;
}


void KAppearanceOptions::slotSerifFont( const TQString& n )
{
    fonts[2] = n;
}


void KAppearanceOptions::slotSansSerifFont( const TQString& n )
{
    fonts[3] = n;
}


void KAppearanceOptions::slotCursiveFont( const TQString& n )
{
    fonts[4] = n;
}


void KAppearanceOptions::slotFantasyFont( const TQString& n )
{
    fonts[5] = n;
}

void KAppearanceOptions::slotFontSizeAdjust( int value )
{
    fonts[6] = TQString::number( value );
}

void KAppearanceOptions::slotEncoding(const TQString& n)
{
    encodingName = n;
}

void KAppearanceOptions::load()
{
	load( false );
}

void KAppearanceOptions::load( bool useDefaults )
{
    KConfig khtmlrc("khtmlrc", true, false);
	 m_pConfig->setReadDefaults( useDefaults );

	 khtmlrc.setReadDefaults( useDefaults );

#define SET_GROUP(x) m_pConfig->setGroup(x); khtmlrc.setGroup(x)
#define READ_NUM(x,y) m_pConfig->readNumEntry(x, khtmlrc.readNumEntry(x, y))
#define READ_ENTRY(x,y) m_pConfig->readEntry(x, khtmlrc.readEntry(x, y))
#define READ_LIST(x) m_pConfig->readListEntry(x, khtmlrc.readListEntry(x))

    SET_GROUP(m_groupname);
    fSize = READ_NUM( "MediumFontSize", 12 );
    fMinSize = READ_NUM( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE );
    if (fSize < fMinSize)
      fSize = fMinSize;

    defaultFonts = TQStringList();
    defaultFonts.append( READ_ENTRY( "StandardFont", KGlobalSettings::generalFont().family() ) );
    defaultFonts.append( READ_ENTRY( "FixedFont", KGlobalSettings::fixedFont().family() ) );
    defaultFonts.append( READ_ENTRY( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT ) );
    defaultFonts.append( READ_ENTRY( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT ) );
    defaultFonts.append( READ_ENTRY( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT ) );
    defaultFonts.append( READ_ENTRY( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT ) );
    defaultFonts.append( TQString("0") ); // default font size adjustment

    if (m_pConfig->hasKey("Fonts"))
       fonts = m_pConfig->readListEntry( "Fonts" );
    else
       fonts = khtmlrc.readListEntry( "Fonts" );
    while (fonts.count() < 7)
       fonts.append(TQString::null);

    encodingName = READ_ENTRY( "DefaultEncoding", TQString::null );
    //kdDebug(0) << "encoding = " << encodingName << endl;

    updateGUI();
    emit changed( useDefaults );

#undef SET_GROUP
#undef READ_NUM
#undef READ_ENTRY
#undef READ_LIST
}

void KAppearanceOptions::defaults()
{
	load( true );
}

void KAppearanceOptions::updateGUI()
{
    //kdDebug() << "KAppearanceOptions::updateGUI " << charset << endl;
    for ( int f = 0; f < 6; f++ ) {
        TQString ff = fonts[f];
        if (ff.isEmpty())
           ff = defaultFonts[f];
        m_pFonts[f]->setCurrentFont(ff);
    }

    int i = 0;
    for ( TQStringList::Iterator it = encodings.begin(); it != encodings.end(); ++it, ++i )
        if ( encodingName == *it )
            m_pEncoding->setCurrentItem( i );
    if(encodingName.isEmpty())
        m_pEncoding->setCurrentItem( 0 );
    m_pFontSizeAdjust->setValue( fonts[6].toInt() );
    m_MedSize->blockSignals(true);
    m_MedSize->setValue( fSize );
    m_MedSize->blockSignals(false);
    m_minSize->blockSignals(true);
    m_minSize->setValue( fMinSize );
    m_minSize->blockSignals(false);
}

void KAppearanceOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "MediumFontSize", fSize );
    m_pConfig->writeEntry( "MinimumFontSize", fMinSize );
    m_pConfig->writeEntry( "Fonts", fonts );

    // If the user chose "Use language encoding", write an empty string
    if (encodingName == i18n("Use Language Encoding"))
        encodingName = "";
    m_pConfig->writeEntry( "DefaultEncoding", encodingName );
    m_pConfig->sync();

  TQByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

  emit changed(false);
}

