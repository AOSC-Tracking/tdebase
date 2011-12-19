/**
 *  kcmhtmlsearch.cpp
 *
 *  Copyright (c) 2000 Matthias Hölzer-Klüpfel <hoelzer@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqlayout.h>
#include <tqwhatsthis.h>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurllabel.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kurlrequester.h>
#include <klineedit.h>

#include "htmlsearchconfig.h"
#include "htmlsearchconfig.moc"

namespace KHC {

HtmlSearchConfig::HtmlSearchConfig(TQWidget *parent, const char *name)
  : TQWidget(parent, name)
{
  TQVBoxLayout *vbox = new TQVBoxLayout(this, 5);


  TQGroupBox *gb = new TQGroupBox(i18n("ht://dig"), this);
  vbox->addWidget(gb);

  TQGridLayout *grid = new TQGridLayout(gb, 3,2, 6,6);

  grid->addRowSpacing(0, gb->fontMetrics().lineSpacing());

  TQLabel *l = new TQLabel(i18n("The fulltext search feature makes use of the "
                  "ht://dig HTML search engine. "
                  "You can get ht://dig at the"), gb);
  l->setAlignment(TQLabel::WordBreak);
  l->setMinimumSize(l->sizeHint());
  grid->addMultiCellWidget(l, 1, 1, 0, 1);
  TQWhatsThis::add( gb, i18n( "Information about where to get the ht://dig package." ) );

  KURLLabel *url = new KURLLabel(gb);
  url->setURL("http://www.htdig.org");
  url->setText(i18n("ht://dig home page"));
  url->setAlignment(TQLabel::AlignHCenter);
  grid->addMultiCellWidget(url, 2,2, 0, 1);
  connect(url, TQT_SIGNAL(leftClickedURL(const TQString&)),
      this, TQT_SLOT(urlClicked(const TQString&)));

  gb = new TQGroupBox(i18n("Program Locations"), this);

  vbox->addWidget(gb);
  grid = new TQGridLayout(gb, 4,2, 6,6);
  grid->addRowSpacing(0, gb->fontMetrics().lineSpacing());

  mHtsearchUrl = new KURLRequester(gb);
  l = new TQLabel(mHtsearchUrl, i18n("htsearch:"), gb);
  l->setBuddy( mHtsearchUrl );
  grid->addWidget(l, 1,0);
  grid->addWidget(mHtsearchUrl, 1,1);
  connect( mHtsearchUrl->lineEdit(), TQT_SIGNAL( textChanged( const TQString & ) ),
           TQT_SIGNAL( changed() ) );
  TQString wtstr = i18n( "Enter the URL of the htsearch CGI program." );
  TQWhatsThis::add( mHtsearchUrl, wtstr );
  TQWhatsThis::add( l, wtstr );

  mIndexerBin = new KURLRequester(gb);
  l = new TQLabel(mIndexerBin, i18n("Indexer:"), gb);
  l->setBuddy( mIndexerBin );
  grid->addWidget(l, 2,0);
  grid->addWidget(mIndexerBin, 2,1);
  connect( mIndexerBin->lineEdit(), TQT_SIGNAL( textChanged( const TQString & ) ),
           TQT_SIGNAL( changed() ) );
  wtstr = i18n( "Enter the path to your htdig indexer program here." );
  TQWhatsThis::add( mIndexerBin, wtstr );
  TQWhatsThis::add( l, wtstr );

  mDbDir = new KURLRequester(gb);
  mDbDir->setMode( KFile::Directory | KFile::LocalOnly );
  l = new TQLabel(mDbDir, i18n("htdig database:"), gb);
  l->setBuddy( mDbDir );
  grid->addWidget(l, 3,0);
  grid->addWidget(mDbDir, 3,1);
  connect( mDbDir->lineEdit(), TQT_SIGNAL( textChanged( const TQString & ) ),
           TQT_SIGNAL( changed() ) );
  wtstr = i18n( "Enter the path to the htdig database folder." );
  TQWhatsThis::add( mDbDir, wtstr );
  TQWhatsThis::add( l, wtstr );
}

HtmlSearchConfig::~HtmlSearchConfig()
{
  kdDebug() << "~HtmlSearchConfig()" << endl;
}

void HtmlSearchConfig::makeReadOnly()
{
    mHtsearchUrl->setEnabled( false );
    mIndexerBin->setEnabled( false );
    mDbDir->setEnabled( false );
}

void HtmlSearchConfig::load( KConfig *config )
{
  config->setGroup("htdig");

  mHtsearchUrl->lineEdit()->setText(config->readPathEntry("htsearch", kapp->dirs()->findExe("htsearch")));
  mIndexerBin->lineEdit()->setText(config->readPathEntry("indexer"));
  mDbDir->lineEdit()->setText(config->readPathEntry("dbdir", "/opt/www/htdig/db/" ) );
}

void HtmlSearchConfig::save( KConfig *config )
{
  config->setGroup("htdig");

  config->writePathEntry("htsearch", mHtsearchUrl->lineEdit()->text());
  config->writePathEntry("indexer", mIndexerBin->lineEdit()->text());
  config->writePathEntry("dbdir", mDbDir->lineEdit()->text());
}

void HtmlSearchConfig::defaults()
{
    mHtsearchUrl->lineEdit()->setText(kapp->dirs()->findExe("htsearch"));
    mIndexerBin->lineEdit()->setText("");
    mDbDir->lineEdit()->setText("/opt/www/htdig/db/" );
}

void HtmlSearchConfig::urlClicked(const TQString &url)
{
  kapp->invokeBrowser(url);
}

} // End namespace KHC
// vim:ts=2:sw=2:et
