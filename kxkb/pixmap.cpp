#include <tqimage.h>
#include <tqbitmap.h>
#include <tqfont.h>
#include <tqpainter.h>
#include <tqregexp.h>
#include <tqdict.h>

#include <kstandarddirs.h>
#include <tdeglobalsettings.h>
#include <tdelocale.h>
#include <kdebug.h>

#include "pixmap.h"
#include "x11helper.h"


static const int FLAG_MAX_DIM = 24;

const TQString LayoutIcon::flagTemplate("l10n/%1/flag.png");
const TQString& LayoutIcon::ERROR_CODE("error");
LayoutIcon* LayoutIcon::instance;


LayoutIcon& LayoutIcon::getInstance() {
	if( instance == NULL ) {
		instance = new LayoutIcon();
	}
	return *instance;
}

LayoutIcon::LayoutIcon():
		m_pixmapCache(80)
{
}

const TQPixmap&
LayoutIcon::findPixmap(const TQString& code_, int pixmapStyle, const TQString& displayName_)
{
	m_kxkbConfig.load(KxkbConfig::LOAD_ALL);  // (re)load settings

	if (m_kxkbConfig.m_useThemeColors) { // use colors from color scheme
		m_bgColor = TDEGlobalSettings::highlightColor();
		m_fgColor = TDEGlobalSettings::highlightedTextColor();
	} else {
		m_bgColor = m_kxkbConfig.m_colorBackground;
		m_fgColor = m_kxkbConfig.m_colorLabel;
	}

	m_labelFont = m_kxkbConfig.m_labelFont;
	m_labelShadow = m_kxkbConfig.m_labelShadow;
	m_shColor = m_kxkbConfig.m_colorShadow;
	m_bgTransparent = m_kxkbConfig.m_bgTransparent;

	// Decide on how to style the pixmap
	switch(pixmapStyle) {
		case PIXMAP_STYLE_NORMAL:
			m_fitToBox = true;
			m_showFlag = true;
			m_showLabel = false;
			break;

		case PIXMAP_STYLE_INDICATOR:
			m_fitToBox = true;
			m_showFlag = m_kxkbConfig.m_showFlag;
			m_showLabel = m_kxkbConfig.m_showLabel;
			break;

		case PIXMAP_STYLE_CONTEXTMENU:
			m_fitToBox = false; // causes white color loss
			m_showFlag = true;
			m_showLabel = false;
			break;
	}

	// Label only mode is always 'fit to box'
	if( m_showLabel && !m_showFlag )
		m_fitToBox = true;

	TQPixmap* pm = NULL;

	if( code_ == ERROR_CODE ) {
		pm = m_pixmapCache[ERROR_CODE];
		if( pm == NULL ) {
			pm = createErrorPixmap();
			m_pixmapCache.insert(ERROR_CODE, pm);
		}
		return *pm;
	}

	TQString displayName(displayName_);

	if( displayName.isEmpty() ) {
		displayName = KxkbConfig::getDefaultDisplayName(code_);
	}
	if( displayName.length() > 3 )
		displayName = displayName.left(3);

	const TQString pixmapKey(
		TQString( m_showFlag ? "f" : "" ) + TQString( m_showLabel ? "l" : "" ) + TQString( m_labelShadow ? "s" : "" ) + "." +
		m_labelFont.key() + "." + ( m_bgTransparent ? "x" : m_bgColor.name() ) + "." + m_fgColor.name() + "." + m_shColor.name() + '.' + code_ + "." + displayName
	);

	// Only use cache for indicator
	if( pixmapStyle == PIXMAP_STYLE_INDICATOR ) {
		pm = m_pixmapCache[pixmapKey];
		if( pm )
			return *pm;
	}

	// Need to create new pixmap
	pm = new TQPixmap();

	if( m_fitToBox ) // Resize to box size
		pm->resize(FLAG_MAX_DIM, FLAG_MAX_DIM);

	if( m_showFlag ) {
		TQString countryCode = getCountryFromLayoutName( code_ );
		TQString flag = locate("locale", flagTemplate.arg(countryCode));

		if( flag.isEmpty() ) {
			pm->fill(m_bgColor);
			m_showLabel = true;
		} else {
			if( m_fitToBox ) { // Resize flag
				TQPainter p_(pm);
				p_.drawPixmap(TQRect(0, 0, FLAG_MAX_DIM, FLAG_MAX_DIM), flag);
			} else { // Show the flag as is
				pm->load(flag);
			}

			if( m_showLabel ) // only dim for label
				dimPixmap( *pm );
		}
	} else {
		pm->fill(m_bgColor);
	}

	if( m_showLabel ) {
		TQPainter p(pm);
		p.setFont(m_labelFont);

		if( m_labelShadow ) {
			p.setPen(m_shColor);
			p.drawText(1, 1, pm->width(), pm->height(), TQt::AlignCenter, displayName);
		}

		p.setPen(m_fgColor);
		p.drawText(0, 0, pm->width(), pm->height(), TQt::AlignCenter, displayName);

		if( m_bgTransparent && !m_showFlag )
		{
			TQPixmap maskpix(pm->width(), pm->height());
			TQPainter maskp(&maskpix);

			maskpix.fill(TQt::white);
			maskp.setPen(TQt::black);
			maskp.setFont(m_labelFont);

			maskp.drawText(0, 0, maskpix.width(), maskpix.height(), TQt::AlignCenter, displayName);
			if( m_labelShadow )
			{
				maskp.drawText(1, 1, maskpix.width(), maskpix.height(), TQt::AlignCenter, displayName);
			}

			TQBitmap mask;
			mask = maskpix;
			pm->setMask(mask);
		}
	}


	if( pixmapStyle == PIXMAP_STYLE_INDICATOR )
		m_pixmapCache.insert(pixmapKey, pm);

	return *pm;
}

/**
@brief Try to get country code from layout name in xkb before xorg 6.9.0
*/
TQString LayoutIcon::getCountryFromLayoutName(const TQString& layoutName)
{
	TQString flag;

	if( X11Helper::areLayoutsClean() ) { // >= Xorg 6.9.0
		if( layoutName == "mkd" )
			flag = "mk";
		else
		  if( layoutName == "srp" ) {
			  TQString csFlagFile = locate("locale", flagTemplate.arg("cs"));
			  flag = csFlagFile.isEmpty() ? "yu" : "cs";
		}
		else
			if( layoutName.endsWith("/jp") )
				flag = "jp";
    else
      if( layoutName == "trq" || layoutName == "trf" || layoutName == "tralt" )
        flag = "tr";
    else
      if( layoutName == "epo" ) // Esperanto
        flag = "eo";
    else
      if( layoutName == "mao" ) // Maori
        flag = "mi";
    else
      if( layoutName == "brai" ) // Braille
        flag = "braille";
		else
			if( layoutName.length() > 2 )
				flag = "";
		else
				flag = layoutName;
	}
	else {
		if( layoutName == "ar" )	// Arabic - not argentina
      ;
    else
      if( layoutName == "epo" ) // Esperanto
          flag = "eo";
    else
      if( layoutName == "mao" ) // Maori
          flag = "mi";
    else
      if( layoutName == "brai" ) // Braille
          flag = "braille";
    else
      if( layoutName == "sr" || layoutName == "cs") // Serbian language - Yugoslavia
          flag = "yu";
		else
			if( layoutName == "bs" )	// Bosnian language - Bosnia
				flag = "ba";
		else
			if( layoutName == "la" )	// Latin America
				;
		else
			if( layoutName == "lo" )	// Lao
				flag = "la";
		else
			if( layoutName == "pl2" )	// Poland
				flag = "pl";
		else
			if( layoutName == "iu" )	// Inuktitut - Canada
				flag = "ca";
		else
			if( layoutName == "syr" )	// Syriac
				flag = "sy";
		else
			if( layoutName == "dz" )	// Dzongka/Tibetian - Buthan
				flag = "bt";
		else
			if( layoutName == "ogham" )	// Ogham - Ireland
				flag = "ie";
		else
			if( layoutName == "ge_la" || layoutName == "ge_ru" )
				flag = "ge";
		else
			if( layoutName == "el" )
				flag = "gr";
		else
			if( layoutName.endsWith("/jp") )
				flag = "jp";
		else
			if( layoutName == "ml" || layoutName == "dev" || layoutName == "gur"
						 || layoutName == "guj" || layoutName == "kan" || layoutName == "ori"
						 || layoutName == "tel" || layoutName == "tml" || layoutName == "ben" ) // some Indian languages
				flag = "in";
		else {
			int sepPos = layoutName.find(TQRegExp("[-_]"));
			TQString leftCode = layoutName.mid(0, sepPos);
			TQString rightCode;
			if( sepPos != -1 )
				rightCode = layoutName.mid(sepPos+1);
//			kdDebug() << "layout name breakup: " << leftCode << ":" << rightCode << endl;

			if( rightCode.length() == 2
					&& TQRegExp("[A-Z][A-Z]").exactMatch(rightCode) ) {
				flag = rightCode.lower();
			}
			else {
				flag = leftCode.length() == 2 ? leftCode : "";
			}
		}
	}

    return flag;
}


void LayoutIcon::dimPixmap(TQPixmap& pm)
{
	TQImage image = pm.convertToImage();
	for (int y=0; y<image.height(); y++)
		for(int x=0; x<image.width(); x++)
	{
		TQRgb rgb = image.pixel(x,y);
		TQRgb dimRgb(tqRgb(tqRed(rgb)*3/4, tqGreen(rgb)*3/4, tqBlue(rgb)*3/4));
		image.setPixel(x, y, dimRgb);
	}
	pm.convertFromImage(image);
}

static const char* ERROR_LABEL = "err";

//private
TQPixmap* LayoutIcon::createErrorPixmap()
{
	TQPixmap* pm = new TQPixmap(21, 14);
	pm->fill(TQt::white);

	TQPainter p(pm);

	p.setFont(m_labelFont);
	p.setPen(TQt::red);
	p.drawText(1, 1, pm->width(), pm->height()-2, TQt::AlignCenter, ERROR_LABEL);
	p.setPen(TQt::blue);
	p.drawText(0, 0, pm->width(), pm->height()-2, TQt::AlignCenter, ERROR_LABEL);
	m_pixmapCache.insert(ERROR_CODE, pm);

	return pm;
}


// Note: this seems stupid, but allows for translations
#if 0
   I18N_NOOP("Belgian");
   I18N_NOOP("Bulgarian");
   I18N_NOOP("Brazilian");
   I18N_NOOP("Canadian");
   I18N_NOOP("Czech");
   I18N_NOOP("Czech (qwerty)");
   I18N_NOOP("Danish");
   I18N_NOOP("Estonian");
   I18N_NOOP("Finnish");
   I18N_NOOP("French");
   I18N_NOOP("German");
   I18N_NOOP("Hungarian");
   I18N_NOOP("Hungarian (qwerty)");
   I18N_NOOP("Italian");
   I18N_NOOP("Japanese");
   I18N_NOOP("Lithuanian");
   I18N_NOOP("Norwegian");
   I18N_NOOP("PC-98xx Series");
   I18N_NOOP("Polish");
   I18N_NOOP("Portuguese");
   I18N_NOOP("Romanian");
   I18N_NOOP("Russian");
   I18N_NOOP("Slovak");
   I18N_NOOP("Slovak (qwerty)");
   I18N_NOOP("Spanish");
   I18N_NOOP("Swedish");
   I18N_NOOP("Swiss German");
   I18N_NOOP("Swiss French");
   I18N_NOOP("Thai");
   I18N_NOOP("United Kingdom");
   I18N_NOOP("U.S. English");
   I18N_NOOP("U.S. English w/ deadkeys");
   I18N_NOOP("U.S. English w/ISO9995-3");

  //lukas: these seem to be new in XF 4.0.2
   I18N_NOOP("Armenian");
   I18N_NOOP("Azerbaijani");
   I18N_NOOP("Icelandic");
   I18N_NOOP("Israeli");
   I18N_NOOP("Lithuanian azerty standard");
   I18N_NOOP("Lithuanian querty \"numeric\"");	     //for bw compatibility
   I18N_NOOP("Lithuanian querty \"programmer's\"");
   I18N_NOOP("Macedonian");
   I18N_NOOP("Serbian");
   I18N_NOOP("Slovenian");
   I18N_NOOP("Vietnamese");

  //these seem to be new in XFree86 4.1.0
   I18N_NOOP("Arabic");
   I18N_NOOP("Belarusian");
   I18N_NOOP("Bengali");
   I18N_NOOP("Croatian");
   I18N_NOOP("Greek");
   I18N_NOOP("Latvian");
   I18N_NOOP("Lithuanian qwerty \"numeric\"");
   I18N_NOOP("Lithuanian qwerty \"programmer's\"");
   I18N_NOOP("Turkish");
   I18N_NOOP("Ukrainian");

  //these seem to be new in XFree86 4.2.0
   I18N_NOOP("Albanian");
   I18N_NOOP("Burmese");
   I18N_NOOP("Dutch");
   I18N_NOOP("Georgian (latin)");
   I18N_NOOP("Georgian (russian)");
   I18N_NOOP("Gujarati");
   I18N_NOOP("Gurmukhi");
   I18N_NOOP("Hindi");
   I18N_NOOP("Inuktitut");
   I18N_NOOP("Iranian");
//   I18N_NOOP("Iranian"); // should be not Iranian but Farsi
   I18N_NOOP("Latin America");
   I18N_NOOP("Maltese");
   I18N_NOOP("Maltese (US layout)");
   I18N_NOOP("Northern Saami (Finland)");
   I18N_NOOP("Northern Saami (Norway)");
   I18N_NOOP("Northern Saami (Sweden)");
   I18N_NOOP("Polish (qwertz)");
   I18N_NOOP("Russian (cyrillic phonetic)");
   I18N_NOOP("Tajik");
   I18N_NOOP("Turkish (F)");
   I18N_NOOP("U.S. English w/ ISO9995-3");
   I18N_NOOP("Yugoslavian");

  //these seem to be new in XFree86 4.3.0
   I18N_NOOP("Bosnian");
   I18N_NOOP("Croatian (US)");
   I18N_NOOP("Dvorak");
   I18N_NOOP("French (alternative)");
   I18N_NOOP("French Canadian");
   I18N_NOOP("Kannada");
   I18N_NOOP("Lao");
   I18N_NOOP("Malayalam");
   I18N_NOOP("Mongolian");
   I18N_NOOP("Ogham");
   I18N_NOOP("Oriya");
   I18N_NOOP("Syriac");
   I18N_NOOP("Telugu");
   I18N_NOOP("Thai (Kedmanee)");
   I18N_NOOP("Thai (Pattachote)");
   I18N_NOOP("Thai (TIS-820.2538)");

  //these seem to be new in XFree86 4.4.0
   I18N_NOOP("Uzbek");
   I18N_NOOP("Faroese");

  //these seem to be new in XOrg 6.8.2
   I18N_NOOP("Dzongkha / Tibetan");
   I18N_NOOP("Hungarian (US)");
   I18N_NOOP("Irish");
   I18N_NOOP("Israeli (phonetic)");
   I18N_NOOP("Serbian (Cyrillic)");
   I18N_NOOP("Serbian (Latin)");
   I18N_NOOP("Swiss");
#endif
