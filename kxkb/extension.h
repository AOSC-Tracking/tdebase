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

#ifndef __EXTENSION_H__
#define __EXTENSION_H__

#include <X11/Xlib.h>

#include <tqobject.h>

#include "kxkbconfig.h"

class XKBExtension : public TQObject
{
    TQ_OBJECT

public:
	static XKBExtension *the();
	~XKBExtension();

	XKBExtension(XKBExtension const&) = delete;
	void operator=(XKBExtension const&) = delete;

	bool init();

	bool setXkbOptions(const XkbOptions options);
	bool setGroup(uint group);

	uint getGroup() const;
	XkbOptions getServerOptions();

	void processXEvent(XEvent *ev);

	void enableConfigureFilter();
	void disableConfigureFilter();

	bool kcmlayoutRunning();

private slots:
	void slotReleaseConfigureLock();

protected:
	XKBExtension() {}

private:
	Display *m_dpy;
	TQString m_tempDir;
	int m_keycode;
	static TQMap<TQString, FILE*> fileCache;
	int m_configureFilterCounter;
	int m_xkb_opcode;

signals:
	void groupChanged(uint group);
	void optionsChanged();
};

#endif
