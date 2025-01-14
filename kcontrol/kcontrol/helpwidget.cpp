/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <tqwhatsthis.h>

#include <tdelocale.h>
#include <kdebug.h>
#include <tdeprocess.h>
#include <tdeapplication.h>
#include <krun.h>

#include "global.h"
#include "dockcontainer.h"
#include "proxywidget.h"
#include "modules.h"

#include "helpwidget.h"

HelpWidget::HelpWidget(DockContainer *parent) : TQWhatsThis(parent), _dock(parent)
{
  setBaseText();
}

void HelpWidget::setText(const TQString& docPath, const TQString& text)
{
  docpath = docPath;
  if (text.isEmpty() && docPath.isEmpty())
    setBaseText();
  else if (docPath.isEmpty())
    helptext = text;
  else
    helptext = (text + i18n("<p>Use the \"What's This?\" (Shift+F1) to get help on specific options.</p><p>To read the full manual click <a href=\"%1\">here</a>.</p>")
		      .arg(static_cast<const char *>(docPath.local8Bit())));
}

void HelpWidget::setBaseText()
{
	 helptext = (i18n("<h1>Trinity Control Center</h1>"
		 "There is no quick help available for the active control module."
		 "<br><br>"
		 "Click <a href = \"kcontrol/index.html\">here</a> to read the general Control Center manual.") );
}

TQString HelpWidget::text() const
{
    return helptext;
}

bool HelpWidget::clicked(const TQString & _url)
{
    TQString textUrl = _url;
    ConfigModule* dockModule = _dock->module();
    if (dockModule) {
        TQString localDocPath = dockModule->module()->handbookDocPath();        
        if (localDocPath != "")
          textUrl = localDocPath;       
        if (textUrl.isNull())
          return true;
        
        TQString section = dockModule->module()->handbookSection();
        if (section != "") {
            textUrl = TQString( "%1#%2" ).arg( textUrl ).arg( section );
        }
    }

    if ( textUrl.find('@') > -1 ) {
        tdeApp->invokeMailer(textUrl);
        return true;
    }

    TDEProcess process;
    KURL url(KURL("help:/"), textUrl);

    if (url.protocol() == "help" || url.protocol() == "man" || url.protocol() == "info") {
        process << "khelpcenter"
                << url.url();
        process.start(TDEProcess::DontCare);
    } else {
        new KRun(url);
    }
    return true;
}

void HelpWidget::handbookRequest()
{
    if (docpath.isEmpty())
        kdWarning() << "No handbook defined" << endl;
 
    clicked(docpath);
}
