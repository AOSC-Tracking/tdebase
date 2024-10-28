//
// C++ Implementation: kxkbconfig
//
// Description:
//
//
// Author: Andriy Rysin <rysin@kde.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <assert.h>

#include <tqregexp.h>
#include <tqstringlist.h>
#include <tqdict.h>

#include <tdeconfig.h>
#include <kdebug.h>

#include "kxkbconfig.h"
#include "x11helper.h"



static const char* switchModes[SWITCH_POLICY_COUNT] = {
  "Global", "WinClass", "Window"
};

const LayoutUnit DEFAULT_LAYOUT_UNIT = LayoutUnit("us", "");
const char* DEFAULT_MODEL = "pc104";

void KxkbConfig::load(int loadMode)
{
	// INITIAL OPTIONS (loaded regardless of whether KXkb is enabled)

	TDEConfig *config = new TDEConfig("kxkbrc", true, false);
	config->setGroup("Layout");

	m_useKxkb = config->readBoolEntry("Use", false);

	m_resetOldOptions = config->readBoolEntry("ResetOldOptions", true);
	m_options = config->readEntry("Options", "");

	if (loadMode == LOAD_INIT_OPTIONS)
	{
		return;
	}

	// BASIC OPTIONS (passed to setxkbmap)

	m_model = config->readEntry("Model", DEFAULT_MODEL);

	// Layouts
	TQStringList layoutList;
	if (config->hasKey("LayoutList"))
	{
		layoutList = config->readListEntry("LayoutList");
	}
	else
	{ // old config
		TQString mainLayout = config->readEntry("Layout", DEFAULT_LAYOUT_UNIT.toPair());
		layoutList = config->readListEntry("Additional");
		layoutList.prepend(mainLayout);
	}

	if (layoutList.count() == 0)
	{
		layoutList.append("us");
	}

	TQStringList::ConstIterator it;

	m_layouts.clear();
	for (it = layoutList.begin(); it != layoutList.end(); ++it)
	{
		m_layouts.append( LayoutUnit(*it) );
		kdDebug() << " layout " << LayoutUnit(*it).toPair() << " in list: " << m_layouts.contains(LayoutUnit(*it)) << endl;
	}

	kdDebug() << "Found " << m_layouts.count() << " layouts" << endl;

	// Display names
	TQStringList displayNamesList = config->readListEntry("DisplayNames", ',');
	for (it = displayNamesList.begin(); it != displayNamesList.end() ; ++it)
	{
		TQStringList displayNamePair = TQStringList::split(':', *it );
		if (displayNamePair.count() == 2)
		{
			LayoutUnit layoutUnit(displayNamePair[0]);
			if (m_layouts.contains(layoutUnit))
			{
				m_layouts[m_layouts.findIndex(layoutUnit)].displayName = displayNamePair[1].left(3);
			}
		}
	}

	if (loadMode == LOAD_BASIC_OPTIONS)
	{
		return;
	}

	// ALL OTHER OPTIONS (of interest only to KXkb itself)

	// Tray indicator
	m_showSingle = config->readBoolEntry("ShowSingle", false);

	m_showFlag = config->readBoolEntry("ShowFlag", true);
	m_showLabel = config->readBoolEntry("ShowLabel", true);

	m_fitToBox = config->readBoolEntry("FitFlagToBox", true);

	m_useThemeColors = config->readBoolEntry("UseThemeColors", false);
	m_colorBackground = config->readColorEntry("ColorBackground", new TQColor(TQt::gray));
	m_bgTransparent = config->readBoolEntry("BgTransparent", false);
	m_colorLabel = config->readColorEntry("ColorLabel", new TQColor(TQt::white));

	m_labelFont = config->readFontEntry("LabelFont", new TQFont("sans", 10, TQFont::Bold));
	m_labelShadow = config->readBoolEntry("LabelShadow", true);
	m_colorShadow = config->readColorEntry("ColorShadow", new TQColor(TQt::black));

	m_dimFlag = config->readBoolEntry("DimFlag", true);
	m_bevel = config->readBoolEntry("IndicatorBevel", false);

	// Switching policy
	TQString layoutOwner = config->readEntry("SwitchMode", "Global");

	if (layoutOwner == "WinClass")
	{
		m_switchingPolicy = SWITCH_POLICY_WIN_CLASS;
	}
	else if (layoutOwner == "Window")
	{
		m_switchingPolicy = SWITCH_POLICY_WINDOW;
	}
	else
	{
		m_switchingPolicy = SWITCH_POLICY_GLOBAL;
	}

	if (m_layouts.count() < 2 && m_switchingPolicy != SWITCH_POLICY_GLOBAL)
	{
		kdWarning() << "Layout count is less than 2, using Global switching policy" << endl;
		m_switchingPolicy = SWITCH_POLICY_GLOBAL;
	}

	kdDebug() << "Layout owner mode " << layoutOwner << endl;

	// Sticky switching
	m_stickySwitching = config->readBoolEntry("StickySwitching", false);
	m_stickySwitchingDepth = config->readNumEntry("StickySwitchingDepth", 2);
	if (m_stickySwitchingDepth < 2)
	{
		m_stickySwitchingDepth = 2;
	}

	if (m_stickySwitching)
	{
		if (m_layouts.count() < 3)
		{
			kdWarning() << "Layout count is less than 3, sticky switching will be off" << endl;
			m_stickySwitching = false;
		}
		else if (m_layouts.count() - 1 < m_stickySwitchingDepth)
		{
			kdWarning() << "Sticky switching depth is more than layout count -1, adjusting..." << endl;
			m_stickySwitchingDepth = m_layouts.count() - 1;
		}
	}

	// Notifications
	config->setGroup("Notifications");
	m_enableNotify = config->readBoolEntry("Enable", false);
	m_notifyUseKMilo = config->readBoolEntry("UseKMilo", true);

	delete config;
}

void KxkbConfig::save()
{
	TDEConfig *config = new TDEConfig("kxkbrc", false, false);
	config->setGroup("Layout");

	config->writeEntry("Use", m_useKxkb);

	config->writeEntry("ResetOldOptions", m_resetOldOptions);
	config->writeEntry("Options", m_options );

	config->writeEntry("Model", m_model);

	// Layouts
	TQStringList layoutList;
	TQStringList displayNamesList;

	TQValueList<LayoutUnit>::ConstIterator it;
	for (it = m_layouts.begin(); it != m_layouts.end(); ++it) {
		const LayoutUnit& layoutUnit = *it;
		layoutList.append(layoutUnit.toPair());

		// Display name
		TQString displayName(layoutUnit.displayName);
		if (!displayName.isEmpty() && displayName != layoutUnit.layout)
		{
			displayName = TQString("%1:%2").arg(layoutUnit.toPair(), displayName);
			displayNamesList.append(displayName);
		}
	}
	config->writeEntry("LayoutList", layoutList);
	config->writeEntry("DisplayNames", displayNamesList);

	// Tray indicator
	config->writeEntry("ShowSingle", m_showSingle);

	config->writeEntry("ShowFlag", m_showFlag);
	config->writeEntry("ShowLabel", m_showLabel);

	config->writeEntry("FitFlagToBox", m_fitToBox);

	config->writeEntry("UseThemeColors", m_useThemeColors);
	config->writeEntry("ColorBackground", m_colorBackground);
	config->writeEntry("BgTransparent", m_bgTransparent);
	config->writeEntry("ColorLabel", m_colorLabel);

	config->writeEntry("LabelFont", m_labelFont);
	config->writeEntry("LabelShadow", m_labelShadow);
	config->writeEntry("ColorShadow", m_colorShadow);

	config->writeEntry("DimFlag", m_dimFlag);
	config->writeEntry("IndicatorBevel", m_bevel);

	// Switching policy
	config->writeEntry("SwitchMode", switchModes[m_switchingPolicy]);

	// Sticky switching
	config->writeEntry("StickySwitching", m_stickySwitching);
	config->writeEntry("StickySwitchingDepth", m_stickySwitchingDepth);

	// Notifications
	config->setGroup("Notifications");
	config->writeEntry("Enable", m_enableNotify);
	config->writeEntry("UseKMilo", m_notifyUseKMilo);

	// remove old options
 	config->deleteEntry("Variants");
	config->deleteEntry("Includes");
	config->deleteEntry("Encoding");
	config->deleteEntry("AdditionalEncodings");
	config->deleteEntry("Additional");
	config->deleteEntry("Layout");

	config->sync();
	delete config;
}

void KxkbConfig::setDefaults()
{
	m_model = DEFAULT_MODEL;

	m_resetOldOptions = true;
	m_options = "";

	m_layouts.clear();
	m_layouts.append( DEFAULT_LAYOUT_UNIT );

	m_useKxkb = false;
	m_showSingle = false;
	m_showFlag = true;
	m_fitToBox = true;
	m_dimFlag = true;

	m_bevel = false;

	m_switchingPolicy = SWITCH_POLICY_GLOBAL;

	m_stickySwitching = false;
	m_stickySwitchingDepth = 2;
}

bool KxkbConfig::setFromXkbOptions(XkbOptions options)
{
	XkbOptions curOptions = getKXkbOptions();

	bool modified = false;

	// We need to fix the variants string if it is empty, otherwise the
	// comparison below will often wrongly assume that the variants have
	// changed
	if (options.variants.isEmpty())
	{
		options.variants = ""; // ensure the string is empty but not null
		for (int i = 0; i < options.layouts.contains(","); ++i)
		{
			options.variants += ",";
		}
	}

	// Check if keyboard layout options have changed
	if ((options.model != curOptions.model && !options.model.isNull()))
	{
		modified = true;
		m_model = options.model;
	}

	if ((options.layouts != curOptions.layouts) || (options.variants != curOptions.variants))
	{
		modified = true;
		m_layouts.clear();

		TQStringList layouts = TQStringList::split(",", options.layouts, true);
		TQStringList variants = TQStringList::split(",", options.variants, true);
		TQStringList::Iterator lit = layouts.begin();
		TQStringList::Iterator vit = variants.begin();

		if (layouts.empty())
		{
			layouts << "us";
		}

		while (lit != layouts.end())
		{
			TQString layout = *lit;
			TQString variant = vit != variants.end() ? *vit : TQString::null;
			m_layouts.append(LayoutUnit(layout, variant));

			++lit;
			if (vit != variants.end())
			{
				++vit;
			}
		}
	}

	TQStringList serverOpts = TQStringList::split(",", options.options);
	TQStringList kxkbOpts = TQStringList::split(",", curOptions.options);
	serverOpts.sort();
	kxkbOpts.sort();

	if (serverOpts != kxkbOpts)
	{
		modified = true;
		m_options = options.options;
	}

	return modified;
}

TQStringList KxkbConfig::getLayoutStringList(/*bool compact*/)
{
	TQStringList layoutList;
	for(TQValueList<LayoutUnit>::ConstIterator it = m_layouts.begin(); it != m_layouts.end(); ++it) {
		const LayoutUnit& layoutUnit = *it;
		layoutList.append( layoutUnit.toPair() );
	}
	return layoutList;
}


TQString KxkbConfig::getDefaultDisplayName(const TQString& code_)
{
	TQString displayName;

	if( code_.length() <= 2 ) {
		displayName = code_;
	}
	else {
		int sepPos = code_.find(TQRegExp("[-_]"));
		TQString leftCode = code_.mid(0, sepPos);
		TQString rightCode;
		if( sepPos != -1 )
			rightCode = code_.mid(sepPos+1);

		if( rightCode.length() > 0 )
			displayName = leftCode.left(2) + rightCode.left(1).lower();
		else
			displayName = leftCode.left(3);
	}

	return displayName;
}

TQString KxkbConfig::getDefaultDisplayName(const LayoutUnit& layoutUnit, bool single)
{
	if( layoutUnit.variant == "" )
		return getDefaultDisplayName( layoutUnit.layout );

	TQString displayName = layoutUnit.layout.left(2);
	if( single == false )
		displayName += layoutUnit.variant.left(1);
	return displayName;
}

const XkbOptions KxkbConfig::getKXkbOptions() {
	XkbOptions options;
	TQStringList layouts;
	TQStringList variants;
	for (TQValueList<LayoutUnit>::ConstIterator it = m_layouts.begin(); it != m_layouts.end(); ++it) {
		const LayoutUnit& layoutUnit = *it;
		layouts << layoutUnit.layout;
		variants << layoutUnit.variant;
	}
	options.layouts  = layouts.join(",");
	options.variants = variants.join(",");
	options.options  = m_options;
	options.resetOld = m_resetOldOptions;
	options.model    = m_model;

	if (options.model.isEmpty())
	{
		options.model = DEFAULT_MODEL;
	}

	return options;
}

/**
 * @brief Gets the single layout part of a layout(variant) string
 * @param[in] layvar String in form layout(variant) to parse
 * @return The layout found in the string
 */
const TQString LayoutUnit::parseLayout(const TQString &layvar)
{
	static const char* LAYOUT_PATTERN = "[a-zA-Z0-9_/-]*";
	TQString varLine = layvar.stripWhiteSpace();
	TQRegExp rx(LAYOUT_PATTERN);
	int pos = rx.search(varLine, 0);
	int len = rx.matchedLength();
  // check for errors
	if( pos < 0 || len < 2 )
		return "";
//	kdDebug() << "getLayout: " << varLine.mid(pos, len) << endl;
	return varLine.mid(pos, len);
}

/**
 * @brief Gets the single variant part of a layout(variant) string
 * @param[in] layvar String in form layout(variant) to parse
 * @return The variant found in the string, no check is performed
 */
const TQString LayoutUnit::parseVariant(const TQString &layvar)
{
	static const char* VARIANT_PATTERN = "\\([a-zA-Z0-9_-]*\\)";
	TQString varLine = layvar.stripWhiteSpace();
	TQRegExp rx(VARIANT_PATTERN);
	int pos = rx.search(varLine, 0);
	int len = rx.matchedLength();
  // check for errors
	if( pos < 2 || len < 2 )
		return "";
	return varLine.mid(pos+1, len-2);
}
