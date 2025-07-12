
/*  This file is part of the KDE project

    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

    Advanced web shortcuts:
    Copyright (C) 2001 Andreas Hochsteger <e9625392@student.tuwien.ac.at>


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

#include <unistd.h>

#include <tqregexp.h>
#include <tqtextcodec.h>

#include <kurl.h>
#include <kdebug.h>
#include <tdeconfig.h>
#include <tdeapplication.h>
#include <tdestandarddirs.h>
#include <kprotocolinfo.h>
#include <tdesimpleconfig.h>
#include <kstaticdeleter.h>

#include "kuriikwsfiltereng.h"
#include "searchprovider.h"

#define PIDDBG kdDebug(7023) << "(" << getpid() << ") "
#define PDVAR(n,v) PIDDBG << n << " = '" << v << "'\n"

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * tdelibs/tdeio/tests/kurifiltertest
 */

KURISearchFilterEngine *KURISearchFilterEngine::s_pSelf = 0;
static KStaticDeleter<KURISearchFilterEngine> kurisearchfilterengsd;

KURISearchFilterEngine::KURISearchFilterEngine()
{
  loadConfig();
}

TQString KURISearchFilterEngine::webShortcutQuery( const TQString& typedString ) const
{
  TQString result;

  if (m_bWebShortcutsEnabled)
  {
    TQString search = typedString;
    int pos = search.find(m_cKeywordDelimiter);

    TQString key;
    if (pos > -1)
      key = search.left(pos);
    else if (m_cKeywordDelimiter == ' ' && !search.isEmpty())
      key = search;

    if (!key.isEmpty() && !KProtocolInfo::isKnownProtocol( key ))
    {
      // Make web shortcut keywords case-insensitive, i.e.
      // kde == KDE == Kde == kDe == kdE
      SearchProvider *provider = SearchProvider::findByKey(key.lower());

      if (provider)
      {
        result = formatResult(provider->query(), provider->charset(),
                              TQString::null, search.mid(pos+1), true);
        delete provider;
      }
    }
  }

  return result;
}


TQString KURISearchFilterEngine::autoWebSearchQuery( const TQString& typedString ) const
{
  TQString result;

  if (m_bWebShortcutsEnabled && !m_defaultSearchEngine.isEmpty())
  {
    // Make sure we ignore supported protocols, e.g. "smb:", "http:"
    int pos = typedString.find(':');

    if (pos == -1 || !KProtocolInfo::isKnownProtocol(typedString.left(pos)))
    {
      SearchProvider *provider = SearchProvider::findByDesktopName(m_defaultSearchEngine);

      if (provider)
      {
        result = formatResult (provider->query(), provider->charset(),
                               TQString::null, typedString, true);
        delete provider;
      }
    }
  }

  return result;
}

TQCString KURISearchFilterEngine::name() const
{
  return "kuriikwsfilter";
}

KURISearchFilterEngine* KURISearchFilterEngine::self()
{
  if (!s_pSelf)
    kurisearchfilterengsd.setObject( s_pSelf, new KURISearchFilterEngine );
  return s_pSelf;
}

TQStringList KURISearchFilterEngine::modifySubstitutionMap(SubstMap& map,
                                                          const TQString& query) const
{
  // Returns the number of query words
  TQString userquery = query;

  // Do some pre-encoding, before we can start the work:
  {
    int start = 0;
    int pos = 0;
    TQRegExp qsexpr("\\\"[^\\\"]*\\\"");

    // Temporary substitute spaces in quoted strings (" " -> "%20")
    // Needed to split user query into StringList correctly.
    while ((pos = qsexpr.search(userquery, start)) >= 0)
    {
      int i = 0;
      int n = 0;
      TQString s = userquery.mid (pos, qsexpr.matchedLength());
      while ((i = s.find(" ")) != -1)
      {
        s = s.replace (i, 1, "%20");
        n++;
      }
      start = pos + qsexpr.matchedLength() + 2*n; // Move after last quote
      userquery = userquery.replace (pos, qsexpr.matchedLength(), s);
    }
  }

  // Split user query between spaces:
  TQStringList l = TQStringList::split(" ", userquery.simplifyWhiteSpace());

  // Back-substitute quoted strings (%20 -> " "):
  {
    int i = 0;
    while ((i = userquery.find("%20")) != -1)
      userquery = userquery.replace(i, 3, " ");

    for ( TQStringList::Iterator it = l.begin(); it != l.end(); ++it )
      *it = (*it).replace("%20", " ");
  }

  PIDDBG << "Generating substitution map:\n";
  // Generate substitution map from user query:
  for (unsigned int i=0; i<=l.count(); i++)
  {
    int j = 0;
    int pos = 0;
    TQString v = "";
    TQString nr = TQString::number(i);

    // Add whole user query (\{0}) to substitution map:
    if (i==0)
      v = userquery;
    // Add partial user query items to substitution map:
    else
      v = l[i-1];

    // Back-substitute quoted strings (%20 -> " "):
    while ((j = v.find("%20")) != -1)
      v = v.replace(j, 3, " ");

    // Insert partial queries (referenced by \1 ... \n) to map:
    map.replace(TQString::number(i), v);
    PDVAR ("  map['" + nr + "']", map[nr]);

    // Insert named references (referenced by \name) to map:
    j = 0;
    if ((i>0) && (pos = v.find("=")) > 0)
    {
      TQString s = v.mid(pos + 1);
      TQString k = v.left(pos);

      // Back-substitute references contained in references (e.g. '\refname' substitutes to 'thisquery=\0')
      while ((j = s.find("%5C")) != -1) s = s.replace(j, 3, "\\");
      map.replace(k, s);
      PDVAR ("  map['" + k + "']", map[k]);
    }
  }

  return l;
}

static TQString encodeString(const TQString &s, int mib)
{
  TQStringList l = TQStringList::split(" ", s, true);
  for(TQStringList::Iterator it = l.begin();
      it != l.end(); ++it)
  {
     *it = KURL::encode_string(*it, mib);
  }
  return l.join("+");
}

TQString KURISearchFilterEngine::substituteQuery(const TQString& url, SubstMap &map, const TQString& userquery, const int encodingMib) const
{
  TQString newurl = url;
  TQStringList ql = modifySubstitutionMap (map, userquery);
  int count = ql.count();

  // Check, if old style '\1' is found and replace it with \{@} (compatibility mode):
  {
    int pos = -1;
    if ((pos = newurl.find("\\1")) >= 0)
    {
      PIDDBG << "WARNING: Using compatibility mode for newurl='" << newurl
             << "'. Please replace old style '\\1' with new style '\\{0}' "
                "in the query definition.\n";
      newurl = newurl.replace(pos, 2, "\\{@}");
    }
  }

  PIDDBG << "Substitute references:\n";
  // Substitute references (\{ref1,ref2,...}) with values from user query:
  {
    int pos = 0;
    TQRegExp reflist("\\\\\\{[^\\}]+\\}");

    // Substitute reflists (\{ref1,ref2,...}):
    while ((pos = reflist.search(newurl, 0)) >= 0)
    {
      bool found = false;

      //bool rest = false;
      TQString v = "";
      TQString rlstring = newurl.mid(pos + 2, reflist.matchedLength() - 3);
      PDVAR ("  reference list", rlstring);

      // \{@} gets a special treatment later
      if (rlstring == "@")
      {
        v = "\\@";
        found = true;
      }

      // TODO: strip whitespaces around commas
      TQStringList rl = TQStringList::split(",", rlstring);
      unsigned int i = 0;

      while ((i<rl.count()) && !found)
      {
        TQString rlitem = rl[i];
        TQRegExp range("[0-9]*\\-[0-9]*");

        // Substitute a range of keywords
        if (range.search(rlitem, 0) >= 0)
        {
          int pos = rlitem.find("-");
          int first = rlitem.left(pos).toInt();
          int last  = rlitem.right(rlitem.length()-pos-1).toInt();

          if (first == 0)
            first = 1;

          if (last  == 0)
            last = count;

          for (int i=first; i<=last; i++)
          {
            v += map[TQString::number(i)] + " ";
            // Remove used value from ql (needed for \{@}):
            ql[i-1] = "";
          }

          v = v.stripWhiteSpace();
          if (!v.isEmpty())
            found = true;

          PDVAR ("    range", TQString::number(first) + "-" + TQString::number(last) + " => '" + v + "'");
          v = encodeString(v, encodingMib);
        }
        else if ( rlitem.startsWith("\"") && rlitem.endsWith("\"") )
        {
          // Use default string from query definition:
          found = true;
          TQString s = rlitem.mid(1, rlitem.length() - 2);
          v = encodeString(s, encodingMib);
          PDVAR ("    default", s);
        }
        else if (map.contains(rlitem))
        {
          // Use value from substitution map:
          found = true;
          PDVAR ("    map['" + rlitem + "']", map[rlitem]);
          v = encodeString(map[rlitem], encodingMib);

          // Remove used value from ql (needed for \{@}):
          TQString c = rlitem.left(1);
          if (c=="0")
          {
            // It's a numeric reference to '0'
            for (TQStringList::Iterator it = ql.begin(); it!=ql.end(); ++it)
              (*it) = "";
          }
          else if ((c>="0") && (c<="9"))
          {
            // It's a numeric reference > '0'
            int n = rlitem.toInt();
            ql[n-1] = "";
          }
          else
          {
            // It's a alphanumeric reference
            TQStringList::Iterator it = ql.begin();
            while ((it != ql.end()) && ((rlitem + "=") != (*it).left(rlitem.length()+1)))
              ++it;
            if ((rlitem + "=") == (*it).left(rlitem.length()+1))
              (*it) = "";
          }

          // Encode '+', otherwise it would be interpreted as space in the resulting url:
          int vpos = 0;
          while ((vpos = v.find('+')) != -1)
            v = v.replace (vpos, 1, "%2B");

        }
        else if (rlitem == "@")
        {
          v = "\\@";
          PDVAR ("    v", v);
        }

        i++;
      }

      newurl = newurl.replace(pos, reflist.matchedLength(), v);
    }

    // Special handling for \{@};
    {
      PDVAR ("  newurl", newurl);
      // Generate list of unmatched strings:
      TQString v = "";
      for (unsigned int i=0; i<ql.count(); i++) {
        v += " " + ql[i];
      }
      v = v.simplifyWhiteSpace();
      PDVAR ("    rest", v);
      v = encodeString(v, encodingMib);

      // Substitute \{@} with list of unmatched query strings
      int vpos = 0;
      while ((vpos = newurl.find("\\@")) != -1)
        newurl = newurl.replace (vpos, 2, v);
    }
  }

  return newurl;
}

TQString KURISearchFilterEngine::formatResult( const TQString& url,
                                              const TQString& cset1,
                                              const TQString& cset2,
                                              const TQString& query,
                                              bool isMalformed ) const
{
  SubstMap map;
  return formatResult (url, cset1, cset2, query, isMalformed, map);
}

TQString KURISearchFilterEngine::formatResult( const TQString& url,
                                              const TQString& cset1,
                                              const TQString& cset2,
                                              const TQString& query,
                                              bool /* isMalformed */,
                                              SubstMap& map ) const
{
  // Return nothing if userquery is empty and it contains
  // substitution strings...
  if (query.isEmpty() && url.find(TQRegExp(TQRegExp::escape("\\{"))) > 0)
    return TQString::null;

  // Debug info of map:
  if (!map.isEmpty())
  {
    PIDDBG << "Got non-empty substitution map:\n";
    for(SubstMap::Iterator it = map.begin(); it != map.end(); ++it)
      PDVAR ("    map['" + it.key() + "']", it.data());
  }

  // Create a codec for the desired encoding so that we can transcode the user's "url".
  TQString cseta = cset1;
  if (cseta.isEmpty())
    cseta = "iso-8859-1";

  TQTextCodec *csetacodec = TQTextCodec::codecForName(cseta.latin1());
  if (!csetacodec)
  {
    cseta = "iso-8859-1";
    csetacodec = TQTextCodec::codecForName(cseta.latin1());
  }

  // Decode user query:
  TQString userquery = KURL::decode_string(query, 106 /* utf-8*/);

  PDVAR ("user query", userquery);
  PDVAR ("query definition", url);

  // Add charset indicator for the query to substitution map:
  map.replace("ikw_charset", cseta);

  // Add charset indicator for the fallback query to substitution map:
  TQString csetb = cset2;
  if (csetb.isEmpty())
    csetb = "iso-8859-1";
  map.replace("wsc_charset", csetb);

  TQString newurl = substituteQuery (url, map, userquery, csetacodec->mibEnum());

  PDVAR ("substituted query", newurl);

  return newurl;
}

void KURISearchFilterEngine::loadConfig()
{
  // Migrate from the old format, this block should remain until
  // we can assume "every" user has upgraded to a KDE version that
  // contains the sycoca based search provider configuration (malte).
  // TODO: Remove in KDE 4 !!! This has been here a sufficient amount of time...
  {
    TDESimpleConfig oldConfig(tdeApp->dirs()->saveLocation("config") + TQString(name()) + "rc");
    oldConfig.setGroup("General");

    if (oldConfig.hasKey("SearchEngines"))
    {
      // User has an old config file in his local config dir
      PIDDBG << "Migrating config file to .desktop files..." << endl;
      TQString fallback = oldConfig.readEntry("InternetKeywordsSearchFallback");
      TQStringList engines = oldConfig.readListEntry("SearchEngines");
      for (TQStringList::ConstIterator it = engines.begin(); it != engines.end(); ++it)
      {
        if (!oldConfig.hasGroup(*it + " Search"))
            continue;

        oldConfig.setGroup(*it + " Search");
        TQString query = oldConfig.readEntry("Query");
        TQStringList keys = oldConfig.readListEntry("Keys");
        TQString charset = oldConfig.readEntry("Charset");
        oldConfig.deleteGroup(*it + " Search");

        TQString name;
        for (TQStringList::ConstIterator key = keys.begin(); key != keys.end(); ++key)
        {
            // take the longest key as name for the .desktop file
            if ((*key).length() > name.length())
                name = *key;
        }

        if (*it == fallback)
            fallback = name;
        SearchProvider *provider = SearchProvider::findByKey(name);

        if (provider)
        {
          // If this entry has a corresponding global entry
          // that comes with KDE's default configuration,
          // compare both and if thei're equal, don't
          // create a local copy
          if (provider->name() == *it && provider->query() == query &&
              provider->keys() == keys && (provider->charset() == charset ||
              (provider->charset().isEmpty() && charset.isEmpty())))
          {
              PIDDBG << *it << " is unchanged, skipping" << endl;
              continue;
          }

          delete provider;
        }

        TDESimpleConfig desktop(tdeApp->dirs()->saveLocation("services", "searchproviders/") + name + ".desktop");
        desktop.setGroup("Desktop Entry");
        desktop.writeEntry("Type", "Service");
        desktop.writeEntry("X-TDE-ServiceTypes", "SearchProvider");
        desktop.writeEntry("Name", *it);
        desktop.writeEntry("Query", query);
        desktop.writeEntry("Keys", keys);
        desktop.writeEntry("Charset", charset);

        PIDDBG << "Created searchproviders/" << name << ".desktop for " << *it << endl;
      }

      oldConfig.deleteEntry("SearchEngines", false);
      oldConfig.setGroup("General");
      oldConfig.writeEntry("InternetKeywordsSearchFallback", fallback);

      PIDDBG << "...completed" << endl;
    }
  }

  PIDDBG << "Keywords Engine: Loading config..." << endl;

  // Load the config.
  TDEConfig config( name() + "rc", false, false );
  config.setGroup( "General" );

  m_cKeywordDelimiter = config.readNumEntry("KeywordDelimiter", ':');
  m_bWebShortcutsEnabled = config.readBoolEntry("EnableWebShortcuts", true);
  m_defaultSearchEngine = config.readEntry("DefaultSearchEngine");
  m_bVerbose = config.readBoolEntry("Verbose", false);

  // Use either a white space or a : as the keyword delimiter...
  if (strchr (" :",m_cKeywordDelimiter) == 0)
    m_cKeywordDelimiter = ':';

  PIDDBG << "Keyword Delimiter: " << m_cKeywordDelimiter << endl;
  PIDDBG << "Default Search Engine: " << m_defaultSearchEngine << endl;
  PIDDBG << "Web Shortcuts Enabled: " << m_bWebShortcutsEnabled << endl;
  PIDDBG << "Verbose: " << m_bVerbose << endl;
}
