//
//
// Author: Andriy Rysin <rysin@kde.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _LAYOUTUNIT_H
#define _LAYOUTUNIT_H

#include <tqstring.h>

inline TQString createPair(TQString key, TQString value)
{
	if (value.isEmpty()) return key;
	return TQString("%1(%2)").arg(key, value);
}

struct LayoutUnit {
	TQString layout;
	TQString variant;
	TQString displayName;

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

#endif // _LAYOUTUNIT_H