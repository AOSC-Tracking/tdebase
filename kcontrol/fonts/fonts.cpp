// KDE Display fonts setup tab
//
// Copyright (c)  Mark Donohoe 1997
//                lars Knoll 1999
//                Rik Hemsley 2000
//
// Ported to kcontrol2 by Geert Jansen.

#include <config.h>

#include <tqcheckbox.h>
#include <tqcombobox.h>
#include <tqdir.h>
#include <tqgroupbox.h>
#include <tqhbox.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqpushbutton.h>
#include <tqsettings.h>
#include <tqtooltip.h>
#include <tqwhatsthis.h>

#include <dcopclient.h>

#include <tdeaccelmanager.h>
#include <tdeapplication.h>
#include <kgenericfactory.h>
#include <kipc.h>
#include <tdemessagebox.h>
#include <knuminput.h>
#include <kprocio.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <stdlib.h>

#ifdef HAVE_FREETYPE2
#include <ft2build.h>
#ifdef FT_LCD_FILTER_H
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#endif
#endif

#include "../krdb/krdb.h"
#include "fonts.h"
#include "fonts.moc"

#include <kdebug.h>

#include <X11/Xlib.h>

// X11 headers
#undef Bool
#undef Unsorted
#undef None

static const char *aa_rgb_xpm[]={
"12 12 3 1",
"a c #0000ff",
"# c #00ff00",
". c #ff0000",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa"};
static const char *aa_bgr_xpm[]={
"12 12 3 1",
". c #0000ff",
"# c #00ff00",
"a c #ff0000",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa",
"....####aaaa"};
static const char *aa_vrgb_xpm[]={
"12 12 3 1",
"a c #0000ff",
"# c #00ff00",
". c #ff0000",
"............",
"............",
"............",
"............",
"############",
"############",
"############",
"############",
"aaaaaaaaaaaa",
"aaaaaaaaaaaa",
"aaaaaaaaaaaa",
"aaaaaaaaaaaa"};
static const char *aa_vbgr_xpm[]={
"12 12 3 1",
". c #0000ff",
"# c #00ff00",
"a c #ff0000",
"............",
"............",
"............",
"............",
"############",
"############",
"############",
"############",
"aaaaaaaaaaaa",
"aaaaaaaaaaaa",
"aaaaaaaaaaaa",
"aaaaaaaaaaaa"};

static const char** aaPixmaps[]={ aa_rgb_xpm, aa_bgr_xpm, aa_vrgb_xpm, aa_vbgr_xpm };

/**** DLL Interface ****/
typedef KGenericFactory<TDEFonts, TQWidget> FontFactory;
K_EXPORT_COMPONENT_FACTORY( kcm_fonts, FontFactory("kcmfonts") )

/**** FontUseItem ****/

FontUseItem::FontUseItem(
  TQWidget * parent,
  const TQString &name,
  const TQString &grp,
  const TQString &key,
  const TQString &rc,
  const TQFont &default_fnt,
  bool f
)
  : TDEFontRequester(parent, 0L, f),
    _rcfile(rc),
    _rcgroup(grp),
    _rckey(key),
    _default(default_fnt)
{
  TDEAcceleratorManager::setNoAccel( this );
  setTitle( name );
  readFont( false );
}

void FontUseItem::setDefault()
{
  setFont( _default, isFixedOnly() );
}

void FontUseItem::readFont( bool useDefaults )
{
  TDEConfigBase *config;

  bool deleteme = false;
  if (_rcfile.isEmpty())
    config = TDEGlobal::config();
  else
  {
    config = new TDEConfig(_rcfile, true);
    deleteme = true;
  }

  config->setReadDefaults( useDefaults );

  config->setGroup(_rcgroup);
  TQFont tmpFnt(_default);
  setFont( config->readFontEntry(_rckey, &tmpFnt), isFixedOnly() );
  if (deleteme) delete config;
}

void FontUseItem::writeFont()
{
  TDEConfigBase *config;

  if (_rcfile.isEmpty()) {
    config = TDEGlobal::config();
    config->setGroup(_rcgroup);
    config->writeEntry(_rckey, font(), true, true);
  } else {
    config = new KSimpleConfig(locateLocal("config", _rcfile));
    config->setGroup(_rcgroup);
    config->writeEntry(_rckey, font());
    config->sync();
    delete config;
  }
}

void FontUseItem::applyFontDiff( const TQFont &fnt, int fontDiffFlags )
{
  TQFont _font( font() );

  if (fontDiffFlags & TDEFontChooser::FontDiffSize) {
    _font.setPointSize( fnt.pointSize() );
  }
  if (fontDiffFlags & TDEFontChooser::FontDiffFamily) {
    if (!isFixedOnly()) _font.setFamily( fnt.family() );
  }
  if (fontDiffFlags & TDEFontChooser::FontDiffStyle) {
    _font.setBold( fnt.bold() );
    _font.setItalic( fnt.italic() );
    _font.setUnderline( fnt.underline() );
  }

  setFont( _font, isFixedOnly() );
}

/**** FontAASettings ****/

FontAASettings::FontAASettings(TQWidget *parent)
              : KDialogBase(parent, "FontAASettings", true, i18n("Configure Anti-Alias Settings"), Ok|Cancel, Ok, true),
                changesMade(false)
{
  TQWidget     *mw=new TQWidget(this);
  TQGridLayout *layout=new TQGridLayout(mw, 1, 1, 0, KDialog::spacingHint());

  excludeRange=new TQCheckBox(i18n("E&xclude range:"), mw),
  layout->addWidget(excludeRange, 0, 0);
  excludeFrom=new KDoubleNumInput(0, 72, 8.0, 1, 1, mw),
  excludeFrom->setSuffix(i18n(" pt"));
  layout->addWidget(excludeFrom, 0, 1);
  excludeToLabel=new TQLabel(i18n(" to "), mw);
  layout->addWidget(excludeToLabel, 0, 2);
  excludeTo=new KDoubleNumInput(0, 72, 15.0, 1, 1, mw);
  excludeTo->setSuffix(i18n(" pt"));
  layout->addWidget(excludeTo, 0, 3);

  useSubPixel=new TQCheckBox(i18n("&Use sub-pixel hinting:"), mw);
  layout->addWidget(useSubPixel, 1, 0);

  TQWhatsThis::add(useSubPixel, i18n("If you have a TFT or LCD screen you"
       " can further improve the quality of displayed fonts by selecting"
       " this option.<br>Sub-pixel hinting is also known as ClearType(tm).<br>"
       "<br><b>This will not work with CRT monitors.</b>"));

  subPixelType=new TQComboBox(false, mw);
  layout->addMultiCellWidget(subPixelType, 1, 1, 1, 3);

  TQWhatsThis::add(subPixelType, i18n("In order for sub-pixel hinting to"
       " work correctly you need to know how the sub-pixels of your display"
       " are aligned.<br>"
       " On TFT or LCD displays a single pixel is actually composed of"
       " three sub-pixels, red, green and blue. Most displays"
       " have a linear ordering of RGB sub-pixel, some have BGR."));

  for(int t=KXftConfig::SubPixel::None+1; t<=KXftConfig::SubPixel::Vbgr; ++t)
    subPixelType->insertItem(TQPixmap(aaPixmaps[t-1]), KXftConfig::description((KXftConfig::SubPixel::Type)t));

#ifdef HAVE_FONTCONFIG
  TQLabel *hintingLabel=new TQLabel(i18n("Hinting style: "), mw);
  layout->addWidget(hintingLabel, 2, 0);
  hintingStyle=new TQComboBox(false, mw);
  layout->addMultiCellWidget(hintingStyle, 2, 2, 1, 3);
  for(int s=KXftConfig::Hint::NotSet+1; s<=KXftConfig::Hint::Full; ++s)
    hintingStyle->insertItem(KXftConfig::description((KXftConfig::Hint::Style)s));

  TQString hintingText(i18n("Hinting is a process used to enhance the quality of fonts at small sizes."));
  TQWhatsThis::add(hintingStyle, hintingText);
  TQWhatsThis::add(hintingLabel, hintingText);
#endif
  load();
  enableWidgets();
  setMainWidget(mw);

  connect(excludeRange, TQ_SIGNAL(toggled(bool)), TQ_SLOT(changed()));
  connect(useSubPixel, TQ_SIGNAL(toggled(bool)), TQ_SLOT(changed()));
  connect(excludeFrom, TQ_SIGNAL(valueChanged(double)), TQ_SLOT(changed()));
  connect(excludeTo, TQ_SIGNAL(valueChanged(double)), TQ_SLOT(changed()));
  connect(subPixelType, TQ_SIGNAL(activated(const TQString &)), TQ_SLOT(changed()));
#ifdef HAVE_FONTCONFIG
  connect(hintingStyle, TQ_SIGNAL(activated(const TQString &)), TQ_SLOT(changed()));
#endif
}

bool FontAASettings::load()
{
  return load( false );
}


bool FontAASettings::load( bool useDefaults )
{
  double     from, to;
  KXftConfig xft(KXftConfig::constStyleSettings);


  if(xft.getExcludeRange(from, to))
     excludeRange->setChecked(true);
  else
  {
    excludeRange->setChecked(false);
    from=8.0;
    to=15.0;
  }

  excludeFrom->setValue(from);
  excludeTo->setValue(to);

  KXftConfig::SubPixel::Type spType;

  if(!xft.getSubPixelType(spType) || KXftConfig::SubPixel::None==spType)
    useSubPixel->setChecked(false);
  else
  {
    int idx=getIndex(spType);

    if(idx>-1)
    {
      useSubPixel->setChecked(true);
      subPixelType->setCurrentItem(idx);
    }
    else
      useSubPixel->setChecked(false);
  }

#ifdef HAVE_FONTCONFIG
  KXftConfig::Hint::Style hStyle;

  if(!xft.getHintStyle(hStyle) || KXftConfig::Hint::NotSet==hStyle)
  {
    TDEConfig kglobals("kdeglobals", false, false);

    kglobals.setReadDefaults( useDefaults );

    kglobals.setGroup("General");
    hStyle=KXftConfig::Hint::Full;
    xft.setHintStyle(hStyle);
    xft.apply();  // Save this setting
    kglobals.writeEntry("XftHintStyle", KXftConfig::toStr(hStyle));
    kglobals.sync();
    runRdb(KRdbExportXftSettings);
  }

  hintingStyle->setCurrentItem(getIndex(hStyle));
#endif

  enableWidgets();

  return xft.getAntiAliasing();
}

bool FontAASettings::save( bool useAA )
{
  KXftConfig xft(KXftConfig::constStyleSettings);
  TDEConfig    kglobals("kdeglobals", false, false);

  kglobals.setGroup("General");

  xft.setAntiAliasing( useAA );

  if(excludeRange->isChecked())
    xft.setExcludeRange(excludeFrom->value(), excludeTo->value());
  else
    xft.setExcludeRange(0, 0);

  KXftConfig::SubPixel::Type spType(useSubPixel->isChecked()
                                        ? getSubPixelType()
                                        : KXftConfig::SubPixel::None);

  xft.setSubPixelType(spType);
  kglobals.writeEntry("XftSubPixel", KXftConfig::toStr(spType));
  kglobals.writeEntry("XftAntialias", useAA);

  bool mod=false;
#ifdef HAVE_FONTCONFIG
  KXftConfig::Hint::Style hStyle(getHintStyle());

  xft.setHintStyle(hStyle);

  TQString hs(KXftConfig::toStr(hStyle));

  if(!hs.isEmpty() && hs!=kglobals.readEntry("XftHintStyle"))
  {
    kglobals.writeEntry("XftHintStyle", hs);
    mod=true;
  }
#endif
  kglobals.sync();

  if(!mod)
    mod=xft.changed();

  xft.apply();

  return mod;
}

void FontAASettings::defaults()
{
  load( true );
}

int FontAASettings::getIndex(KXftConfig::SubPixel::Type spType)
{
  int pos=-1;
  int index;

  for(index=0; index<subPixelType->count(); ++index)
    if(subPixelType->text(index)==KXftConfig::description(spType))
    {
      pos=index;
      break;
    }

  return pos;
}

KXftConfig::SubPixel::Type FontAASettings::getSubPixelType()
{
  int t;

  for(t=KXftConfig::SubPixel::None; t<=KXftConfig::SubPixel::Vbgr; ++t)
    if(subPixelType->currentText()==KXftConfig::description((KXftConfig::SubPixel::Type)t))
      return (KXftConfig::SubPixel::Type)t;

  return KXftConfig::SubPixel::None;
}

#ifdef HAVE_FONTCONFIG
int FontAASettings::getIndex(KXftConfig::Hint::Style hStyle)
{
  int pos=-1;
  int index;

  for(index=0; index<hintingStyle->count(); ++index)
    if(hintingStyle->text(index)==KXftConfig::description(hStyle))
    {
      pos=index;
      break;
    }

  return pos;
}


KXftConfig::Hint::Style FontAASettings::getHintStyle()
{
  int s;

  for(s=KXftConfig::Hint::NotSet; s<=KXftConfig::Hint::Full; ++s)
    if(hintingStyle->currentText()==KXftConfig::description((KXftConfig::Hint::Style)s))
      return (KXftConfig::Hint::Style)s;

  return KXftConfig::Hint::Full;
}
#endif

void FontAASettings::enableWidgets()
{
  excludeFrom->setEnabled(excludeRange->isChecked());
  excludeTo->setEnabled(excludeRange->isChecked());
  excludeToLabel->setEnabled(excludeRange->isChecked());
  subPixelType->setEnabled(useSubPixel->isChecked());
#ifdef FT_LCD_FILTER_H
  static int ft_has_subpixel = -1;
  if( ft_has_subpixel == -1 ) {
    FT_Library            ftLibrary;
    if(FT_Init_FreeType(&ftLibrary) == 0) {
      ft_has_subpixel = ( FT_Library_SetLcdFilter(ftLibrary, FT_LCD_FILTER_DEFAULT )
        == FT_Err_Unimplemented_Feature ) ? 0 : 1;
      FT_Done_FreeType(ftLibrary);
    }
  }
  useSubPixel->setEnabled(ft_has_subpixel);
  subPixelType->setEnabled(ft_has_subpixel);
#endif
}

void FontAASettings::changed()
{
    changesMade=true;
    enableWidgets();
}

int FontAASettings::exec()
{
    int i=KDialogBase::exec();

    if(!i)
        load(); // Reset settings...

    return i && changesMade;
}

/**** TDEFonts ****/

static TQCString desktopConfigName()
{
  int desktop=0;
  if (tqt_xdisplay())
    desktop = DefaultScreen(tqt_xdisplay());
  TQCString name;
  if (desktop == 0)
    name = "kdesktoprc";
  else
    name.sprintf("kdesktop-screen-%drc", desktop);

  return name;
}

TDEFonts::TDEFonts(TQWidget *parent, const char *name, const TQStringList &)
    :   TDECModule(FontFactory::instance(), parent, name)
{
  TQStringList nameGroupKeyRc;

  nameGroupKeyRc
    << i18n("General")        << "General"    << "font"         << ""
    << i18n("Fixed width")    << "General"    << "fixed"        << ""
    << i18n("Toolbar")        << "General"    << "toolBarFont"  << ""
    << i18n("Menu")           << "General"    << "menuFont"     << ""
    << i18n("Window title")   << "WM"         << "activeFont"   << ""
    << i18n("Taskbar")        << "General"    << "taskbarFont"  << ""
    << i18n("Desktop")        << "FMSettings" << "StandardFont" << desktopConfigName();

  TQValueList<TQFont> defaultFontList;

  // Keep in sync with tdelibs/tdecore/tdeglobalsettings.cpp

  TQFont f0("Sans Serif", 10);
  TQFont f1("Monospace", 10);
  TQFont f2("Sans Serif", 10);
  TQFont f3("Sans Serif", 10, TQFont::Bold);
  TQFont f4("Sans Serif", 10);

  f0.setPointSize(10);
  f1.setPointSize(10);
  f2.setPointSize(10);
  f3.setPointSize(10);
  f4.setPointSize(10);

  defaultFontList << f0 << f1 << f2 << f0 << f3 << f4 << f0;

  TQValueList<bool> fixedList;

  fixedList
    <<  false
    <<  true
    <<  false
    <<  false
    <<  false
    <<  false
    <<  false;

  TQStringList quickHelpList;

  quickHelpList
    << i18n("Used for normal text (e.g. button labels, list items).")
    << i18n("A non-proportional font (i.e. typewriter font).")
    << i18n("Used to display text beside toolbar icons.")
    << i18n("Used by menu bars and popup menus.")
    << i18n("Used by the window titlebar.")
    << i18n("Used by the taskbar.")
    << i18n("Used for desktop icons.");

  TQVBoxLayout * layout =
    new TQVBoxLayout(this, 0, KDialog::spacingHint());

  TQGridLayout * fontUseLayout =
    new TQGridLayout(layout, nameGroupKeyRc.count() / 4, 3);

  fontUseLayout->setColStretch(0, 0);
  fontUseLayout->setColStretch(1, 1);
  fontUseLayout->setColStretch(2, 0);

  TQValueList<TQFont>::ConstIterator defaultFontIt(defaultFontList.begin());
  TQValueList<bool>::ConstIterator fixedListIt(fixedList.begin());
  TQStringList::ConstIterator quickHelpIt(quickHelpList.begin());
  TQStringList::ConstIterator it(nameGroupKeyRc.begin());

  unsigned int count = 0;

  while (it != nameGroupKeyRc.end()) {

    TQString name = *it; it++;
    TQString group = *it; it++;
    TQString key = *it; it++;
    TQString file = *it; it++;

    FontUseItem * i =
      new FontUseItem(
        this,
        name,
        group,
        key,
        file,
        *defaultFontIt++,
        *fixedListIt++
      );

    fontUseList.append(i);
    connect(i, TQ_SIGNAL(fontSelected(const TQFont &)), TQ_SLOT(fontSelected()));

    TQLabel * fontUse = new TQLabel(name+":", this);
    TQWhatsThis::add(fontUse, *quickHelpIt++);

    fontUseLayout->addWidget(fontUse, count, 0);
    fontUseLayout->addWidget(i, count, 1);

    ++count;
  }

   TQHBoxLayout *hblay = new TQHBoxLayout(layout, KDialog::spacingHint());
   hblay->addStretch();
   TQPushButton * fontAdjustButton = new TQPushButton(i18n("Ad&just All Fonts..."), this);
   TQWhatsThis::add(fontAdjustButton, i18n("Click to change all fonts"));
   hblay->addWidget( fontAdjustButton );
   connect(fontAdjustButton, TQ_SIGNAL(clicked()), TQ_SLOT(slotApplyFontDiff()));

   layout->addSpacing(KDialog::spacingHint());

   TQGridLayout* lay = new TQGridLayout(layout, 2, 4, KDialog::spacingHint());
   lay->setColStretch( 3, 10 );
   TQLabel* label = new TQLabel( i18n( "Use a&nti-aliasing:" ), this );
   lay->addWidget( label, 0, 0 );
   cbAA = new TQComboBox( this );
   cbAA->insertItem( i18n( "Enabled" )); // change AASetting type if order changes
   cbAA->insertItem( i18n( "System settings" ));
   cbAA->insertItem( i18n( "Disabled" ));
   TQWhatsThis::add(cbAA, i18n("If this option is selected, TDE will smooth the edges of curves in "
                              "fonts."));
   aaSettingsButton = new TQPushButton( i18n( "Configure..." ), this);
   connect(aaSettingsButton, TQ_SIGNAL(clicked()), TQ_SLOT(slotCfgAa()));
   label->setBuddy( cbAA );
   lay->addWidget( cbAA, 0, 1 );
   lay->addWidget( aaSettingsButton, 0, 2 );
   connect(cbAA, TQ_SIGNAL(activated(int)), TQ_SLOT(slotUseAntiAliasing()));

   label = new TQLabel( i18n( "Force fonts DPI:" ), this );
   lay->addWidget( label, 1, 0 );
   comboForceDpi = new TQComboBox( this );
   label->setBuddy( comboForceDpi );
   comboForceDpi->insertItem( i18n( "Disabled" ));
   comboForceDpi->insertItem( i18n( "Enabled" ));
   TQString whatsthis = i18n(
       "<p>This option forces a specific DPI value for fonts. It may be useful"
       " when the real DPI of the hardware is not detected properly and it"
       " is also often misused when poor quality fonts are used that do not"
       " look well with DPI values other than 96 or 120 DPI.</p>");
   TQWhatsThis::add(comboForceDpi, whatsthis);
   connect(comboForceDpi, TQ_SIGNAL(activated(int)), TQ_SLOT(slotUseFontDPI()));
   sbDpiValue = new KIntSpinBox(64, 512, 1, 96, 10, this);
   TQWhatsThis::add(sbDpiValue, whatsthis);
   connect(sbDpiValue, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(changed()));
   lay->addWidget( comboForceDpi, 1, 1 );
   lay->addWidget( sbDpiValue, 1, 2 );

   layout->addStretch(1);

   aaSettings=new FontAASettings(this);

   load();
}

TDEFonts::~TDEFonts()
{
  fontUseList.setAutoDelete(true);
  fontUseList.clear();
}

void TDEFonts::fontSelected()
{
  emit changed(true);
}

void TDEFonts::defaults()
{
  load( true );
  aaSettings->defaults();
}

void TDEFonts::load()
{
  load( false );
}


void TDEFonts::load( bool useDefaults )
{
  for ( uint i = 0; i < fontUseList.count(); i++ )
    fontUseList.at( i )->readFont( useDefaults );

  useAA_original = useAA = aaSettings->load( useDefaults ) ? AAEnabled : AADisabled;
  cbAA->setCurrentItem( useAA );

  TDEConfig cfgfonts("kcmfonts", true);
  cfgfonts.setGroup("General");
  int dpicfg = cfgfonts.readNumEntry("forceFontDPI", 0);
  // "forceFontDPIEnable" must be read after "forceFontDPI" to make sure it is
  // correctly initialized on the first run when upgrading to the new format,
  // without the user even noticying it. The first time "forceFontDPIEnable"
  // will not exist and its correct value will be deduced by the existing value
  // of "forceFontDPI", which contains the value prior to the update.
  bool dpiEnable = cfgfonts.readBoolEntry("forceFontDPIEnable", dpicfg > 0);
  dpi_original = dpiEnable ? DPIValue : DPINone;
  dpi_value_original = dpicfg;
  if (dpi_value_original < 64 || dpi_value_original > 512)
  {
    dpi_value_original = 96;
  }
  comboForceDpi->setCurrentItem(dpi_original);
  sbDpiValue->setValue(dpi_value_original);
  sbDpiValue->setEnabled(dpi_original != DPINone);
  if( cfgfonts.readBoolEntry( "dontChangeAASettings", true )) {
      useAA_original = useAA = AASystem;
      cbAA->setCurrentItem( useAA );
  }
  aaSettingsButton->setEnabled( cbAA->currentItem() == AAEnabled );

  emit changed( useDefaults );
}

void TDEFonts::save()
{

  for ( FontUseItem* i = fontUseList.first(); i; i = fontUseList.next() )
      i->writeFont();
  TDEGlobal::config()->sync();

  TDEConfig cfgfonts("kcmfonts");
  cfgfonts.setGroup("General");
  DPISetting dpi = (DPISetting)comboForceDpi->currentItem();
  int dpival = sbDpiValue->value();
  cfgfonts.writeEntry( "forceFontDPIEnable", dpi != DPINone );
  cfgfonts.writeEntry( "forceFontDPI", dpival );
  cfgfonts.writeEntry( "dontChangeAASettings", cbAA->currentItem() == AASystem );
  cfgfonts.sync();
  // if the setting is reset in the module, remove the dpi value,
  // otherwise don't explicitly remove it and leave any possible system-wide value
  if(dpi == DPINone)
  {
      KProcIO proc;
      proc << "xrdb" << "-quiet" << "-remove" << "-nocpp";
      proc.writeStdin( TQCString( "Xft.dpi" ), true );
      proc.closeWhenDone();
      proc.start( TDEProcess::Block );
  }

  // KDE-1.x support
  KSimpleConfig* config = new KSimpleConfig( TQDir::homeDirPath() + "/.tderc" );
  config->setGroup( "General" );
  for ( FontUseItem* i = fontUseList.first(); i; i = fontUseList.next() ) {
      if("font"==i->rcKey())
          TQSettings().writeEntry("/qt/font", i->font().toString());
      kdDebug(1208) << "write entry " <<  i->rcKey() << endl;
      config->writeEntry( i->rcKey(), i->font() );
  }
  config->sync();
  delete config;

  KIPC::sendMessageAll(KIPC::FontChanged);

  kapp->processEvents(); // Process font change ourselves

  bool aaSave = false;
  // Don't overwrite global settings unless explicitly asked for - e.g. the system
  // fontconfig setup may be much more complex than this module can provide.
  // TODO: With NoChange the changes already made by this module should be reverted somehow.
  if( cbAA->currentItem() != AASystem )
      aaSave = aaSettings->save( useAA == AAEnabled );

  if( aaSave || (useAA != useAA_original) || dpival != dpi_value_original || dpi != dpi_original)
  {
    KMessageBox::information(this,
      i18n( "<p>Some changes such as anti-aliasing will only affect newly started applications.</p>"),
      i18n("Font Settings Changed"), "FontSettingsChanged", false);
    useAA_original = useAA;
    dpi_original = dpi;
  }

  runRdb(KRdbExportXftSettings);

  emit changed(false);
}


void TDEFonts::slotApplyFontDiff()
{
  TQFont font = TQFont(fontUseList.first()->font());
  int fontDiffFlags = 0;
  int ret = TDEFontDialog::getFontDiff(font,fontDiffFlags);

  if (ret == KDialog::Accepted && fontDiffFlags)
  {
    for ( int i = 0; i < (int) fontUseList.count(); i++ )
      fontUseList.at( i )->applyFontDiff( font,fontDiffFlags );
    emit changed(true);
  }
}

void TDEFonts::slotUseAntiAliasing()
{
    useAA = static_cast< AASetting >( cbAA->currentItem());
    aaSettingsButton->setEnabled( cbAA->currentItem() == AAEnabled );
    emit changed(true);
}

void TDEFonts::slotUseFontDPI()
{
    DPISetting dpi = (DPISetting)(comboForceDpi->currentItem());
    sbDpiValue->setEnabled(dpi != DPINone);
    emit changed(true);
}

void TDEFonts::slotCfgAa()
{
  if(aaSettings->exec())
  {
    emit changed(true);
  }
}
