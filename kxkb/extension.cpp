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


static TQString getLayoutKey(const TQString& layout, const TQString& variant)
{
	return layout + "." + variant;
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

    // watch group change events
    XkbSelectEventDetails(m_dpy, XkbUseCoreKbd, XkbStateNotify,
                          XkbAllStateComponentsMask, XkbGroupStateMask);

    return true;
}

XKBExtension::~XKBExtension()
{
/*	if( m_compiledLayoutFileNames.isEmpty() == false )
		deletePrecompiledLayouts();*/
}

bool XKBExtension::setXkbOptions(const XkbOptions options)
{
	TQString exe = TDEGlobal::dirs()->findExe("setxkbmap");
	if (exe.isEmpty())
		return false;

	TDEProcess p;
	p << exe;

	p << "-layout";
	p << options.layouts;
	
	p << "-variant";
	p << options.variants;
	
	if (!options.model.isEmpty()) {
		p << "-model";
		p << options.model;
	}

	if (options.resetOld) {
		p << "-option";
	}
	if (!options.options.isEmpty()) {
		p << "-option" << options.options;
	}

	kdDebug() << "[kxkb-extension] Command: " << p.args() << endl;

	p.start(TDEProcess::Block);

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

/** Examines an X Event passed to it and takes actions if the event is of
  * interest to KXkb */
void XKBExtension::processXEvent(XEvent *event) {
	XkbEvent* xkb_event = (XkbEvent*)event;
    if (xkb_event->any.xkb_type == XkbStateNotify) {
		emit groupChanged(xkb_event->state.group);
    }
}

#include "extension.moc"