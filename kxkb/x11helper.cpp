#include <tqdom.h>
#include <tqdir.h>
#include <tqstring.h>
#include <tqwindowdefs.h>
#include <tqstring.h>
#include <tqstringlist.h>
#include <tqdict.h>
#include <tqregexp.h>

#include <kdebug.h>
#include <tdestandarddirs.h>
#include <tdelocale.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#define explicit int_explicit        // avoid compiler name clash in XKBlib.h
#include <X11/XKBlib.h>
#undef explicit
#include <X11/extensions/XKBrules.h>

#include "x11helper.h"
#include "config.h"


// Compiler will size array automatically.
static const char* X11DirList[] =
    {
#ifdef X11_XKB_RULES_DIR
        X11_XKB_RULES_DIR,
#endif
        XLIBDIR,
        "/usr/share/X11/",
        "/usr/lib/X11/",
        "/usr/lib64/X11/",
        "/usr/X11/share/X11/",
        "/usr/X11/lib/X11/",
        "/usr/X11/lib64/X11/",
        "/usr/X11R7/share/X11/",
        "/usr/X11R7/lib/X11/",
        "/usr/X11R7/lib64/X11/",
        "/usr/X11R6/share/X11/",
        "/usr/X11R6/lib/X11/",
        "/usr/X11R6/lib64/X11/",
        "/usr/local/X11/share/X11/",
        "/usr/local/X11/lib/X11/",
        "/usr/local/X11/lib64/X11/",
        "/usr/local/X11R7/share/X11/",
        "/usr/local/X11R7/lib/X11/",
        "/usr/local/X11R7/lib64/X11/",
        "/usr/local/X11R6/share/X11/",
        "/usr/local/X11R6/lib/X11/",
        "/usr/local/X11R6/lib64/X11/",
        "/usr/local/share/X11/",
        "/usr/local/lib/X11/",
        "/usr/local/lib64/X11/",
        "/usr/pkg/share/X11/",
        "/usr/pkg/xorg/lib/X11/",
        "/etc/X11/"
    };

// Compiler will size array automatically.
static const char* rulesFileList[] =
    {
	"xkb/rules/xorg",
	"xkb/rules/xfree86"
    };

// Macro will return number of elements in any static array as long as the
// array has at least one element.
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

static const int X11_DIR_COUNT = ARRAY_SIZE(X11DirList);
static const int X11_RULES_COUNT = ARRAY_SIZE(rulesFileList);

const TQString X11Helper::X11_WIN_CLASS_ROOT = "<root>";
const TQString X11Helper::X11_WIN_CLASS_UNKNOWN = "<unknown>";

static const TQRegExp NON_CLEAN_LAYOUT_REGEXP("[^a-z]");

bool X11Helper::m_layoutsClean = true;

const TQString
X11Helper::findX11Dir()
{
	for(int ii=0; ii<X11_DIR_COUNT; ii++) {
		const char* xDir = X11DirList[ii];
		if( xDir != NULL && TQDir(TQString(xDir) + "xkb").exists() ) {
//			for(int jj=0; jj<X11_RULES_COUNT; jj++) {
//				
//			}
			return TQString(xDir);
	    }
	
//	  if( X11_DIR.isEmpty() ) {
//	    return;
//	  }
	}
	return NULL;	
}

const TQString
X11Helper::findXkbRulesFile(TQString x11Dir, Display *dpy)
{
	TQString rulesFile;
	XkbRF_VarDefsRec vd;
	char *tmp = NULL;

	if (XkbRF_GetNamesProp(dpy, &tmp, &vd) && tmp != NULL ) {
// 			kdDebug() << "namesprop " << tmp  << endl;
	  	rulesFile = x11Dir + TQString("xkb/rules/%1").arg(tmp);
// 			kdDebug() << "rulesF " << rulesFile  << endl;
	}
	else {
    // old way
    	for(int ii=0; ii<X11_RULES_COUNT; ii++) {
    		const char* ruleFile = rulesFileList[ii];
     		TQString xruleFilePath = x11Dir + ruleFile;
//  			kdDebug() << "trying " << xruleFilePath << endl;
		    if( TQFile(xruleFilePath).exists() ) {
				rulesFile = xruleFilePath;
			break;
		    }
    	}
    }

	return rulesFile;
}

RulesInfo*
X11Helper::loadRules(const TQString& file, bool layoutsOnly) {
	XkbRF_RulesPtr xkbRules = XkbRF_Load(TQFile::encodeName(file).data(), "", true, true);

	if (xkbRules == NULL) {
// throw Exception
		return NULL;
	}

	RulesInfo* rulesInfo = new RulesInfo();

	for (int i = 0; i < xkbRules->layouts.num_desc; ++i) {
		TQString layoutName(xkbRules->layouts.desc[i].name);
		rulesInfo->layouts.replace( layoutName, tqstrdup( xkbRules->layouts.desc[i].desc ) );

		if( m_layoutsClean == true
				  && layoutName.find( NON_CLEAN_LAYOUT_REGEXP ) != -1 
				  && layoutName.endsWith("/jp") == false ) {
			kdDebug() << "Layouts are not clean (Xorg < 6.9.0 or XFree86)" << endl;
			m_layoutsClean = false;
		}
	}

	if( layoutsOnly == true ) {
		XkbRF_Free(xkbRules, true);
		return rulesInfo;
	}

  for (int i = 0; i < xkbRules->models.num_desc; ++i)
      rulesInfo->models.replace(xkbRules->models.desc[i].name, tqstrdup( xkbRules->models.desc[i].desc ) );

  // Prefer XML file for Xkb options
  if (TQFile(file + ".xml").exists()) {
      XkbRF_Free(xkbRules, true);

      TQDomDocument xmlrules("xkbrules");
      TQFile xmlfile(file + ".xml");
      if (!xmlfile.open(IO_ReadOnly)) {
          return NULL;
      }
      if (!xmlrules.setContent(&xmlfile)) {
          xmlfile.close();
          return NULL;
      }
      xmlfile.close();

      TQDomElement options = xmlrules.documentElement().namedItem("optionList").toElement();
      TQDomNode optGroupNode = options.firstChild();
      while (!optGroupNode.isNull()) {
          TQDomElement optGroupElem = optGroupNode.toElement();
          if (optGroupElem.tagName() == "group") {
              for (TQDomNode optNode = optGroupElem.firstChild();
                   !optNode.isNull();
                   optNode = optNode.nextSibling()
              ) {
                TQDomElement optElem = optNode.toElement();
                if (!optElem.isNull()) {
                    // This might be either a configItem (group) or an option tag
                    // If it is an option tag, it contains a configItem that describes
                    // the option
                    if (optElem.tagName() == "option") {
                        optElem = optElem.namedItem("configItem").toElement();
                    }

                    TQString optName = optElem.namedItem("name").toElement().text();
                    TQString optDesc = optElem.namedItem("description").toElement().text();
                    if (optDesc.isEmpty()) {
                        optDesc = optName;
                    }
                    // Items from these 'meta' groups fall into other groups.  They all will be
                    // added to the "cystom" group as orphans and to avoid displaying empty groups
                    // we won't be skipping the groups themself.
                    // Admittedly not the best way to handle this.
                    if (optName == "currencysign" || optName == "compat") continue;

                    // HACK this should be called "compose" or else the code breaks
                    if (optName == "Compose key") optName = "compose";

                    rulesInfo->options.replace(optName.ascii(), tqstrdup(optDesc.ascii()));
                }
              }
          }
          optGroupNode = optGroupNode.nextSibling();
      }
  }
  else {
      for (int i = 0; i < xkbRules->options.num_desc; ++i)
          rulesInfo->options.replace(xkbRules->options.desc[i].name, tqstrdup( xkbRules->options.desc[i].desc ) );

      XkbRF_Free(xkbRules, true);

      // workaround for empty 'compose' options group description
      if( rulesInfo->options.find("compose:menu") && !rulesInfo->options.find("compose") ) {
          rulesInfo->options.replace("compose", I18N_NOOP("Compose Key Position"));
      }
  }

//   // workaround for empty misc options group description in XFree86 4.4.0
//   if( rulesInfo->options.find("numpad:microsoft") && !rulesInfo->options.find("misc") ) {
//     rulesInfo->options.replace("misc", "Miscellaneous compatibility options" );
//   }

  return rulesInfo;
}

// check $oldlayouts and $nonlatin groups for XFree 4.3 and later
OldLayouts*
X11Helper::loadOldLayouts(const TQString& rulesFile)
{
  static const char* oldLayoutsTag = "! $oldlayouts";
  static const char* nonLatinLayoutsTag = "! $nonlatin";
  TQStringList m_oldLayouts;
  TQStringList m_nonLatinLayouts;
  
  TQFile f(rulesFile);
  
  if (f.open(IO_ReadOnly))
    {
      TQTextStream ts(&f);
      TQString line;

      while (!ts.eof()) {
	  line = ts.readLine().simplifyWhiteSpace();

	  if( line.find(oldLayoutsTag) == 0 ) {

	    line = line.mid(strlen(oldLayoutsTag));
	    line = line.mid(line.find('=')+1).simplifyWhiteSpace();
	    while( !ts.eof() && line.endsWith("\\") )
		line = line.left(line.length()-1) + ts.readLine();
	    line = line.simplifyWhiteSpace();

	    m_oldLayouts = TQStringList::split(TQRegExp("\\s"), line);
//	    kdDebug() << "oldlayouts " << m_oldLayouts.join("|") << endl;
	    if( !m_nonLatinLayouts.empty() )
	      break;
	    
	  }
	  else
	  if( line.find(nonLatinLayoutsTag) == 0 ) {

	    line = line.mid(strlen(nonLatinLayoutsTag)+1).simplifyWhiteSpace();
	    line = line.mid(line.find('=')+1).simplifyWhiteSpace();
	    while( !ts.eof() && line.endsWith("\\") )
		line = line.left(line.length()-1) + ts.readLine();
	    line = line.simplifyWhiteSpace();

	    m_nonLatinLayouts = TQStringList::split(TQRegExp("\\s"), line);
//	    kdDebug() << "nonlatin " << m_nonLatinLayouts.join("|") << endl;
	    if( !m_oldLayouts.empty() )
	      break;
	    
	  }
      }

      f.close();
    }
	
	OldLayouts* oldLayoutsStruct = new OldLayouts();
	oldLayoutsStruct->oldLayouts = m_oldLayouts;
	oldLayoutsStruct->nonLatinLayouts = m_nonLatinLayouts;
	
	return oldLayoutsStruct;
}


/* pretty simple algorithm - reads the layout file and
    tries to find "xkb_symbols"
    also checks whether previous line contains "hidden" to skip it
*/
TQStringList*
X11Helper::getVariants(const TQString& layout, const TQString& x11Dir, bool oldLayouts)
{
  TQStringList* result = new TQStringList();

  TQString file = x11Dir + "xkb/symbols/";
  // workaround for XFree 4.3 new directory for one-group layouts
  if( TQDir(file+"pc").exists() && !oldLayouts )
    file += "pc/";
    
  file += layout;

//  kdDebug() << "reading variants from " << file << endl;
  
  TQFile f(file);
  if (f.open(IO_ReadOnly))
    {
      TQTextStream ts(&f);

      TQString line;
      TQString prev_line;

      while (!ts.eof()) {
    	  prev_line = line;
	  line = ts.readLine().simplifyWhiteSpace();

	    if (line[0] == '#' || line.left(2) == "//" || line.isEmpty())
		continue;

	    int pos = line.find("xkb_symbols");
	    if (pos < 0)
		continue;

	    if( prev_line.find("hidden") >=0 )
		continue;

	    pos = line.find('"', pos) + 1;
	    int pos2 = line.find('"', pos);
	    if( pos < 0 || pos2 < 0 )
		continue;

	    result->append(line.mid(pos, pos2-pos));
//  kdDebug() << "adding variant " << line.mid(pos, pos2-pos) << endl;
      }

      f.close();
    }

	return result;
}

TQString 
X11Helper::getWindowClass(WId winId, Display* dpy)
{
  unsigned long nitems_ret, bytes_after_ret;
  unsigned char* prop_ret;
  Atom     type_ret;
  int      format_ret;
  Window w = (Window)winId;	// suppose WId == Window
  TQString  property;

  if( winId == X11Helper::UNKNOWN_WINDOW_ID ) {
	  kdDebug() << "Got window class for " << winId << ": '" << X11_WIN_CLASS_ROOT << "'" << endl;
	  return X11_WIN_CLASS_ROOT;
  }
  
//  kdDebug() << "Getting window class for " << winId << endl;
  if((XGetWindowProperty(dpy, w, XA_WM_CLASS, 0L, 256L, 0, XA_STRING,
			&type_ret, &format_ret, &nitems_ret,
			&bytes_after_ret, &prop_ret) == Success) && (type_ret != None)) {
    property = TQString::fromLocal8Bit(reinterpret_cast<char*>(prop_ret));
    XFree(prop_ret);
  }
  else {
	  property = X11_WIN_CLASS_UNKNOWN;
  }
  kdDebug() << "Got window class for " << winId << ": '" << property << "'" << endl;
  
  return property;
}

bool X11Helper::areSingleGroupsSupported()
{
	return true; //TODO:
}

void X11Helper::initializeTranslations() {
    // TDE is usually installed into some non-standard prefix and by default system-wide locale
    // dirs are not considered when searching for gettext message catalogues, so we have to add
    // it explicitly.
#ifdef WITH_XKB_TRANSLATIONS
    TDEGlobal::dirs()->addResourceDir("locale", XKB_CONFIG_LOCALE_DIR);
    TDEGlobal::locale()->insertCatalogue("xkeyboard-config");
#endif
}
