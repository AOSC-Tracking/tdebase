//
// C++ Interface: kxkbconfig
//
// Description:
//
//
// Author: Andriy Rysin <rysin@kde.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KXKBCONFIG_H
#define KXKBCONFIG_H

#include <tqstring.h>
#include <tqstringlist.h>
#include <tqcolor.h>
#include <tqfont.h>
#include <tqptrqueue.h>
#include <tqmap.h>


/* Utility classes for per-window/per-application layout implementation
*/
enum SwitchingPolicy {
	SWITCH_POLICY_GLOBAL = 0,
	SWITCH_POLICY_WIN_CLASS = 1,
	SWITCH_POLICY_WINDOW = 2,
	SWITCH_POLICY_COUNT = 3
};



inline TQString createPair(TQString key, TQString value)
{
	if( value.isEmpty() )
		return key;
	return TQString("%1(%2)").arg(key, value);
}

struct LayoutUnit {
	TQString layout;
	TQString variant;
	TQString includeGroup;
	TQString displayName;
 	int defaultGroup;

	LayoutUnit() {}

	LayoutUnit(TQString layout_, TQString variant_):
		layout(layout_),
		variant(variant_)
	{}

	LayoutUnit(TQString pair) {
		setFromPair( pair );
	}

	void setFromPair(const TQString& pair) {
		layout = parseLayout(pair);
		variant = parseVariant(pair);
	}

	TQString toPair() const {
		return createPair(layout, variant);
	}

	bool operator<(const LayoutUnit& lu) const {
		return layout<lu.layout ||
				(layout==lu.layout && variant<lu.variant);
	}

	bool operator!=(const LayoutUnit& lu) const {
		return layout!=lu.layout || variant!=lu.variant;
	}

	bool operator==(const LayoutUnit& lu) const {
// 		kdDebug() << layout << "==" << lu.layout << "&&" << variant << "==" << lu.variant << endl;
		return layout==lu.layout && variant==lu.variant;
	}

//private:
	static const TQString parseLayout(const TQString &layvar);
	static const TQString parseVariant(const TQString &layvar);
};

extern const LayoutUnit DEFAULT_LAYOUT_UNIT;
extern const char* DEFAULT_MODEL;


class KxkbConfig
{
public:
	enum { LOAD_INIT_OPTIONS, LOAD_ACTIVE_OPTIONS, LOAD_ALL };

	bool m_useKxkb;
	bool m_showSingle;
	bool m_showFlag;
	bool m_showLabel;
	bool m_enableXkbOptions;
	bool m_resetOldOptions;
	SwitchingPolicy m_switchingPolicy;
	bool m_stickySwitching;
	int m_stickySwitchingDepth;

	bool m_useThemeColors;
	TQColor m_colorBackground;
	TQColor m_colorLabel;
	TQFont m_labelFont;
	bool m_labelShadow;
	TQColor m_colorShadow;

	TQString m_model;
	TQString m_options;
	TQValueList<LayoutUnit> m_layouts;

	LayoutUnit getDefaultLayout();

	bool load(int loadMode);
	void save();
	void setDefaults();

	TQStringList getLayoutStringList(/*bool compact*/);
	static TQString getDefaultDisplayName(const TQString& code_);
	static TQString getDefaultDisplayName(const LayoutUnit& layoutUnit, bool single=false);

private:
	static const TQMap<TQString, TQString> parseIncludesMap(const TQStringList& pairList);
};


#endif
