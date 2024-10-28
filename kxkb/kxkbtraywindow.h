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

#include "layoutunit.h"

class XkbRules;
class KxkbConfig;
class LayoutIconManager;

class KxkbSystemTray : public KSystemTray
{
    TQ_OBJECT

    public:
        KxkbSystemTray(KxkbConfig *kxkbConfig);
        ~KxkbSystemTray();
        void initLayoutList(const TQValueList<LayoutUnit>& layouts, const XkbRules& rule);
        void setCurrentLayout(const LayoutUnit& layout);
        void setError(const TQString& layoutInfo = TQString::null);

        enum { START_MENU_ID = 100, CONFIG_MENU_ID = 130, HELP_MENU_ID = 131 };

    protected:
        void mouseReleaseEvent(TQMouseEvent *ev);

    private slots:
        void setToolTip(const TQString& tip);
        void setPixmap(const TQPixmap& pix);

    signals:
        void menuActivated(int);
        void toggled();

    private:
        LayoutIconManager *m_icoMgr;
        int m_prevLayoutCount;
        TQMap<TQString, TQString> m_descriptionMap;
};


#endif
