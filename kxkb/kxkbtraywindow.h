//
// C++ Interface: kxkbtraywindow
//
// Description: 
//
//
// Author: Andriy Rysin <rysin@kde.org>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KXKBSYSTEMTRAY_H
#define KXKBSYSTEMTRAY_H

#include <ksystemtray.h>

#include <tqstring.h>
#include <tqvaluelist.h>

#include "kxkbconfig.h"


class TQLabel;
class TDEPopupMenu;
class XkbRules;

/* This class is responsible for displaying flag/label for the layout,
    catching keyboard/mouse events and displaying menu when selected
*/

class KxkbLabelController: public QObject
{
// 	Q_OBJECT
			
public:
	enum { START_MENU_ID = 100, CONFIG_MENU_ID = 130, HELP_MENU_ID = 131 };

    KxkbLabelController(TQLabel *label, TDEPopupMenu* contextMenu);

    void initLayoutList(const TQValueList<LayoutUnit>& layouts, const XkbRules& rule);
    void setCurrentLayout(const LayoutUnit& layout);
// 	void setCurrentLayout(const TQString& layout, const TQString &variant);
	void setError(const TQString& layoutInfo="");
    void setShowFlag(bool showFlag) { m_showFlag = showFlag; }
	void show() { label->show(); }
	
// signals:
// 
// 	void menuActivated(int);
//     void toggled();

// protected:
// 
//     void mouseReleaseEvent(TQMouseEvent *);

private:
	TQLabel* label;
	TDEPopupMenu* contextMenu;
	
	const int m_menuStartIndex;
	bool m_showFlag;
	int m_prevLayoutCount;
    TQMap<TQString, TQString> m_descriptionMap;
	
	void setToolTip(const TQString& tip);
	void setPixmap(const TQPixmap& pixmap);
};


class KxkbSystemTray : public KSystemTray
{
	Q_OBJECT 
			
	public:
	KxkbSystemTray():
		KSystemTray(NULL)
	{}
	
	void mouseReleaseEvent(TQMouseEvent *ev)
	{
		if (ev->button() == Qt::LeftButton)
			emit toggled();
		KSystemTray::mouseReleaseEvent(ev);
	}

	signals:
 		void menuActivated(int);
		void toggled();
};


#endif
