#include <tqwindowdefs.h>
#include <tqfile.h>
#include <tqtextstream.h>
#include <tqregexp.h>
#include <tqstringlist.h>
#include <tqdir.h>

#include <kstandarddirs.h>
#include <tdeglobal.h>
#include <tdelocale.h>
#include <kdebug.h>
#include <config.h>

#include "x11helper.h"
#include "rules.h"


XkbRules::XkbRules(bool layoutsOnly):
    m_layouts(90)
{
	X11_DIR = X11Helper::findX11Dir();

   	if( X11_DIR == NULL ) {
        kdError() << "Cannot find X11 directory!" << endl;
//        throw Exception();
		return;
   	}

	TQString rulesFile = X11Helper::findXkbRulesFile(X11_DIR, tqt_xdisplay());
	
	if( rulesFile.isEmpty() ) {
  		kdError() << "Cannot find rules file in " << X11_DIR << endl;
//		throw Exception();
		return;
	}

	loadRules(rulesFile, layoutsOnly);
}


void XkbRules::loadRules(TQString file, bool layoutsOnly)
{
	RulesInfo* rules = X11Helper::loadRules(file, layoutsOnly);

	if (rules == NULL) {
		kdDebug() << "Unable to load rules" << endl;
		return;
	}

	m_layouts= rules->layouts;
	if( layoutsOnly == false ) {
		m_models = rules->models;
		m_options = rules->options;
	}

	//  fixLayouts();
}

TQStringList
XkbRules::getAvailableVariants(const TQString& layout)
{
    if( layout.isEmpty() || !layouts().find(layout) )
	return TQStringList();

    TQStringList* result1 = m_varLists[layout];
    if( result1 )
        return *result1;

    TQStringList* result = X11Helper::getVariants(layout, X11_DIR);

    m_varLists.insert(layout, result);

    return *result;
}

TQString XkbRules::getLayoutName(LayoutUnit layout) const {
	TQString fullName = i18n(m_layouts[layout.layout]);
	if (!layout.variant.isEmpty()) {
		fullName += " (" + layout.variant + ")";
	}
	return fullName;
}