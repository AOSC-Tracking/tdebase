#include <string.h>
#include <errno.h>

#include <tqstring.h>
#include <tqmap.h>
#include <tqfile.h>
#include <tqdir.h>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>

#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKM.h>

#include "extension.h"


TQMap<TQString, FILE*> XKBExtension::fileCache;	//TODO: move to class?


static TQString getLayoutKey(const TQString& layout, const TQString& variant)
{
	return layout + "." + variant;
}

TQString XKBExtension::getPrecompiledLayoutFilename(const TQString& layoutKey)
{
	TQString compiledLayoutFileName = m_tempDir + layoutKey + ".xkm";
	return compiledLayoutFileName;
}

XKBExtension::XKBExtension(Display *d)
{
	if ( d == NULL )
		d = tqt_xdisplay();
	m_dpy = d;
	
//	TQStringList dirs = TDEGlobal::dirs()->findDirs ( "tmp", "" );
//	m_tempDir = dirs.count() == 0 ? "/tmp/" : dirs[0];
	m_tempDir = locateLocal("tmp", "");
}

bool XKBExtension::init()
{
    // Verify the Xlib has matching XKB extension.

    int major = XkbMajorVersion;
    int minor = XkbMinorVersion;
	
    if (!XkbLibraryVersion(&major, &minor))
    {
        kdError() << "[kxkb-extension] Xlib XKB extension " << major << '.' << minor <<
            " != " << XkbMajorVersion << '.' << XkbMinorVersion << endl;
        return false;
    }

    // Verify the X server has matching XKB extension.

    int opcode_rtrn;
    int error_rtrn;
    int xkb_opcode;
    if (!XkbQueryExtension(m_dpy, &opcode_rtrn, &xkb_opcode, &error_rtrn,
                         &major, &minor))
    {
        kdError() << "[kxkb-extension] X server XKB extension " << major << '.' << minor <<
            " != " << XkbMajorVersion << '.' << XkbMinorVersion << endl;
        return false;
    }

    // Do it, or face horrible memory corrupting bugs
    ::XkbInitAtoms(NULL);

    return true;
}

void XKBExtension::reset()
{
	for(TQMap<TQString, FILE*>::ConstIterator it = fileCache.begin(); it != fileCache.end(); it++) {
		fclose(*it);
//		remove( TQFile::encodeName(getPrecompiledLayoutFileName(*it)) );
	}
	fileCache.clear();
}

XKBExtension::~XKBExtension()
{
/*	if( m_compiledLayoutFileNames.isEmpty() == false )
		deletePrecompiledLayouts();*/
}

bool XKBExtension::setXkbOptions(const TQString& options, bool resetOld)
{
    if (options.isEmpty())
        return true;

    TQString exe = TDEGlobal::dirs()->findExe("setxkbmap");
    if (exe.isEmpty())
        return false;

    TDEProcess p;
    p << exe;
    if( resetOld )
        p << "-option";
    p << "-option" << options;

    p.start(TDEProcess::Block);

    return p.normalExit() && (p.exitStatus() == 0);
}

bool XKBExtension::setLayout(const TQString& model,
		const TQString& layout, const TQString& variant,
		const TQString& includeGroup, bool useCompiledLayouts)
{
	if( useCompiledLayouts == false ) {
		return setLayoutInternal( model, layout, variant, includeGroup );
	}
	
	const TQString layoutKey = getLayoutKey(layout, variant);
	
	bool res;
	if( fileCache.contains(layoutKey) ) {
		res = setCompiledLayout( layoutKey );
		kdDebug() << "[kxkb-extension] setCompiledLayout " << layoutKey << ": " << res << endl;

		if( res )
			return res;
	}
//	else {
		res = setLayoutInternal( model, layout, variant, includeGroup );
		kdDebug() << "[kxkb-extension] setRawLayout " << layoutKey << ": " << res << endl;
		if( res )
			compileCurrentLayout( layoutKey );
		
//	}
	return res;
}

// private
bool XKBExtension::setLayoutInternal(const TQString& model,
		const TQString& layout, const TQString& variant,
		const TQString& includeGroup)
{
    if ( layout.isEmpty() )
        return false;

	TQString exe = TDEGlobal::dirs()->findExe("setxkbmap");
	if( exe.isEmpty() ) {
		kdError() << "[kxkb-extension] Can't find setxkbmap" << endl;
		return false;
	}

    TQString fullLayout = layout;
    TQString fullVariant = variant;
	if( includeGroup.isEmpty() == false ) {
        fullLayout = includeGroup;
        fullLayout += ",";
        fullLayout += layout;
		
//    fullVariant = baseVar;
        fullVariant = ",";
        fullVariant += variant;
    }
 
    TDEProcess p;
    p << exe;
//  p << "-rules" << rule;
	if( model.isEmpty() == false )
		p << "-model" << model;
    p << "-layout" << fullLayout;
    if( !fullVariant.isNull() && !fullVariant.isEmpty() )
        p << "-variant" << fullVariant;

    p.start(TDEProcess::Block); 

    // reload system-wide hotkey-setup keycode -> keysym maps
    TQString modmapFileName = TDEGlobal::dirs()->findResource( "data", "kxkb/system.xmodmap" );
    if ( TQFile::exists( modmapFileName ) ) {
        TDEProcess pXmodmap;
        pXmodmap << "xmodmap" << modmapFileName;
        pXmodmap.start(TDEProcess::Block);
    }

    if ( TQFile::exists( TQDir::home().path() + "/.Xmodmap" ) ) {
        TDEProcess pXmodmapHome;
        pXmodmapHome << "xmodmap" << TQDir::home().path() + "/.Xmodmap";
        pXmodmapHome.start(TDEProcess::Block);
    }

    return p.normalExit() && (p.exitStatus() == 0);
}

bool XKBExtension::setGroup(unsigned int group)
{
	kdDebug() << "[kxkb-extension] Setting group " << group << endl;
	return XkbLockGroup( m_dpy, XkbUseCoreKbd, group );
}

unsigned int XKBExtension::getGroup() const
{
	XkbStateRec xkbState;
	XkbGetState( m_dpy, XkbUseCoreKbd, &xkbState );
	return xkbState.group;
}

/**
 * @brief Gets the current layout in its binary compiled form
 *		and write it to the file specified by 'fileName'
 * @param[in] fileName file to store compiled layout to
 * @return true if no problem, false otherwise
 */
bool XKBExtension::compileCurrentLayout(const TQString &layoutKey)
{
    XkbFileInfo result;
    memset(&result, 0, sizeof(result));
    result.type = XkmKeymapFile;
    XkbReadFromServer(m_dpy, XkbAllMapComponentsMask, XkbAllMapComponentsMask, &result);
	 
	const TQString fileName = getPrecompiledLayoutFilename(layoutKey);

	kdDebug() << "[kxkb-extension] compiling layout " << this << " cache size: " << fileCache.count() << endl;
	if( fileCache.contains(layoutKey) ) {
		kdDebug() << "[kxkb-extension] trashing old compiled layout for " << fileName << endl;
		if( fileCache[ layoutKey ] != NULL )
			fclose( fileCache[ layoutKey ] );	// recompiling - trash the old file
		fileCache.remove(fileName);
	}

	FILE *output = fopen(TQFile::encodeName(fileName), "w");
		
    if ( output == NULL )
    {
		kdWarning() << "[kxkb-extension] Could not open " << fileName << " to precompile: " << strerror(errno) << endl;
        XkbFreeKeyboard(result.xkb, XkbAllControlsMask, True);
        return false;
    }

	if( !XkbWriteXKMFile(output, &result) ) {
		kdWarning() << "[kxkb-extension] Could not write compiled layout to " << fileName << endl;
		fclose(output);
        return false;
	}
	
	fclose(output);	// TODO: can we change mode w/out reopening?
	FILE *input = fopen(TQFile::encodeName(fileName), "r");
	fileCache[ layoutKey ] = input;

	XkbFreeKeyboard(result.xkb, XkbAllControlsMask, True);
    return true;
}

/**
 * @brief takes layout from its compiled binary snapshot in file 
 *	and sets it as current
 * TODO: cache layout in memory rather than in file
 */
bool XKBExtension::setCompiledLayout(const TQString &layoutKey)
{
	FILE *input = NULL;
	
	if( fileCache.contains(layoutKey) ) {
		input = fileCache[ layoutKey ];
	}
	
	if( input == NULL ) {
		kdWarning() << "[kxkb-extension] setCompiledLayout trying to reopen xkb file" << endl;	// should never happen
		const TQString fileName = getPrecompiledLayoutFilename(layoutKey);
		input = fopen(TQFile::encodeName(fileName), "r");
		
		// 	FILE *input = fopen(TQFile::encodeName(fileName), "r");
		if ( input == NULL ) {
			kdDebug() << "[kxkb-extension] Unable to open " << fileName << ": " << strerror(errno) << endl;
			fileCache.remove(layoutKey);
			return false;
		}
	}
	else {
		rewind(input);
	}

    XkbFileInfo result;
    memset(&result, 0, sizeof(result));
	if ((result.xkb = XkbAllocKeyboard())==NULL) {
		kdWarning() << "[kxkb-extension] Unable to allocate memory for keyboard description" << endl;
//      fclose(input);
//		fileCache.remove(layoutKey);
    	return false;
	}
	
    unsigned retVal = XkmReadFile(input, 0, XkmKeymapLegal, &result);
    if (retVal == XkmKeymapLegal)
    {
        // this means reading the Xkm didn't manage to read any section
        kdWarning() << "[kxkb-extension] Unable to load map from file" << endl;
        XkbFreeKeyboard(result.xkb, XkbAllControlsMask, True);
        fclose(input);
		fileCache.remove(layoutKey);
		return false;
    }

	//    fclose(input);	// don't close - goes in cache

    if (XkbChangeKbdDisplay(m_dpy, &result) == Success)
    {
        if (!XkbWriteToServer(&result))
        {
            kdWarning() << "[kxkb-extension] Unable to write the keyboard layout to X display" << endl;
            XkbFreeKeyboard(result.xkb, XkbAllControlsMask, True);
            return false;
        }
    }
    else
    {
        kdWarning() << "[kxkb-extension] Unable prepare the keyboard layout for X display" << endl;
    }

    XkbFreeKeyboard(result.xkb, XkbAllControlsMask, True);
    return true;
}


// Deletes the precompiled layouts stored in temporary files
// void XKBExtension::deletePrecompiledLayouts()
// {
// 	TQMapConstIterator<LayoutUnit, TQString> it, end;
// 	end = m_compiledLayoutFileNames.end();
// 	for (it = m_compiledLayoutFileNames.begin(); it != end; ++it)
// 	{
// 		unlink(TQFile::encodeName(it.data()));
// 	}
// 	m_compiledLayoutFileNames.clear();
// }
