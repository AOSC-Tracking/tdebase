/*******************************************************************************

  Xkb extension for KXkb
  Copyright © 2009-2025 Trinity Desktop project
  Copyright © 2001      S.R. Haque <srhaque@iee.org>

  Derived from an original by Matthias H�zer-Klpfel released under the QPL.

  Some portions come from kkbswitch released under the GNU GPL v2 (or later).
  Copyright © 2001      Leonid Zeitlin <lz@europe.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <tqstring.h>
#include <tqmap.h>
#include <tqfile.h>
#include <tqdir.h>
#include <tqtimer.h>

#include <kdebug.h>
#include <tdeapplication.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <dcopclient.h>

#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBfile.h>
#include <X11/extensions/XKBrules.h>
#include <X11/extensions/XKBgeom.h>
#include <X11/extensions/XKM.h>

#include "extension.h"

extern "C"
{
	static int IgnoreXError(Display *, XErrorEvent *) { return 0; }
}

static TQString getLayoutKey(const TQString& layout, const TQString& variant)
{
	return layout + "." + variant;
}

static XKBExtension *xkbExtension = nullptr;

XKBExtension *XKBExtension::the()
{
	if (!xkbExtension)
	{
		xkbExtension = new XKBExtension;
		if (!xkbExtension->init())
		{
			kdFatal() << "xkb initialization failed, exiting..." << endl;
			::exit(1);
		}
	}
	return xkbExtension;
}

bool XKBExtension::init()
{
	m_configureFilterCounter = 0;

	kdDebug() << "[kxkb-extension] Initializing Xkb extension" << endl;
	m_dpy = tqt_xdisplay();

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
	if (!XkbQueryExtension(m_dpy, &opcode_rtrn, &m_xkb_opcode, &error_rtrn, &major, &minor))
	{
		kdError() << "[kxkb-extension] X server XKB extension " << major << '.' << minor <<
				" != " << XkbMajorVersion << '.' << XkbMinorVersion << endl;
		return false;
	}

	enableConfigureFilter();

	// Do it, or face horrible memory corrupting bugs
	::XkbInitAtoms(nullptr);

	// Watch for interesting events
	XkbSelectEventDetails(m_dpy, XkbUseCoreKbd, XkbStateNotify,
	                      XkbAllStateComponentsMask, XkbGroupStateMask);

	XkbSelectEventDetails(m_dpy, XkbUseCoreKbd, XkbNewKeyboardNotify,
	                      XkbAllNewKeyboardEventsMask, XkbAllNewKeyboardEventsMask);


	m_tempDir = locateLocal("tmp", "");

	disableConfigureFilter();
	return true;
}

XKBExtension::~XKBExtension()
{
/*	if( m_compiledLayoutFileNames.isEmpty() == false )
		deletePrecompiledLayouts();*/
}

void XKBExtension::enableConfigureFilter()
{
	++m_configureFilterCounter;
}

void XKBExtension::disableConfigureFilter()
{
	// Without this protection in place KXkb would react to configuration
	// changes caused by itself
	TQTimer::singleShot(500, this, TQ_SLOT(slotReleaseConfigureLock()));
}

void XKBExtension::slotReleaseConfigureLock()
{
	--m_configureFilterCounter;
}

bool XKBExtension::setXkbOptions(const XkbOptions options)
{
	enableConfigureFilter();

	TQString exe = TDEGlobal::dirs()->findExe("setxkbmap");
	if (exe.isEmpty())
	{
		return false;
	}

	TDEProcess p;
	p << exe;

	if (!options.layouts.isEmpty())
	{
		p << "-layout";
		p << options.layouts;
	}

	if (!options.variants.isEmpty())
	{
		p << "-variant";
		p << options.variants;
	}

	if (!options.model.isEmpty()) {
		p << "-model";
		p << options.model;
	}

	if (options.resetOld) {
		p << "-option";
	}

	if (!options.options.isEmpty()) {
		if (options.resetOld)
		{
			p << "-option" << options.options;
		}
		else
		{
			// Avoid duplication of options in Append mode
			XkbOptions _opt = getServerOptions();
			TQStringList srvOptions = TQStringList::split(",", _opt.options);
			TQStringList kxkbOptions = TQStringList::split(",", options.options);
			TQStringList newOptions;
			for (TQStringList::Iterator it = kxkbOptions.begin(); it != kxkbOptions.end(); ++it)
			{
				TQString option(*it);
				if (!srvOptions.contains(option))
				{
					newOptions << option;
				}
			}
			if (!newOptions.isEmpty()) {
				p <<  "-option" <<  newOptions.join(",");
			}
		}
	}

	if (p.args().count() < 2)
	{
		// Either the user has not configured any Xkb options or these options
		// are already set and we are in append mode so we want to avoid
		// duplicates
		kdWarning() << "[setXkbOptions] No options need to be set" << endl;
		slotReleaseConfigureLock(); // immediately release the lock
		return true;
	}

	p << "-synch";

	kdDebug() << "[setXkbOptions] Command: " << p.args() << endl;

	p.start(TDEProcess::Block);

	disableConfigureFilter();

	return p.normalExit() && (p.exitStatus() == 0);
}

XkbOptions XKBExtension::getServerOptions()
{
	XkbOptions options;
	XkbRF_VarDefsRec vd;
	if (XkbRF_GetNamesProp(tqt_xdisplay(), nullptr, &vd))
	{
		options.model = vd.model;
		options.layouts = vd.layout;
		options.variants = vd.variant;
		options.options = vd.options;
	}
	return options;
}

bool XKBExtension::setGroup(unsigned int group)
{
	kdDebug() << "[kxkb-extension] Setting group " << group << endl;
	return XkbLockGroup(m_dpy, XkbUseCoreKbd, group);
}

uint XKBExtension::getGroup() const
{
	XkbStateRec xkbState;
	XkbGetState(m_dpy, XkbUseCoreKbd, &xkbState);
	return xkbState.group;
}

bool XKBExtension::kcmlayoutRunning()
{
	return kApp->dcopClient()->isApplicationRegistered("TDECModuleProxy-keyboard_layout");
}

// Examines an X Event passed to it and takes actions if the event is of
// interest to KXkb
void XKBExtension::processXEvent(XEvent *event) {
	if (event->type == m_xkb_opcode)
	{
		XkbEvent *xkb_event = (XkbEvent*)event;
		if (xkb_event->any.xkb_type == XkbStateNotify && xkb_event->state.changed & XkbGroupStateMask)
		{
			emit groupChanged((uint)xkb_event->state.group);
		}

		else if (xkb_event->any.xkb_type == XkbNewKeyboardNotify)
		{
			if (m_configureFilterCounter > 0 || kcmlayoutRunning())
			{
				return;
			}
			enableConfigureFilter();
			emit optionsChanged();
			disableConfigureFilter();
		}
	}
}

#include "extension.moc"
