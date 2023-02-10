#ifndef __EXTENSION_H__
#define __EXTENSION_H__

#include <X11/Xlib.h>
#include <tqobject.h>

#include "kxkbconfig.h"

class XKBExtension : public TQObject
{
    TQ_OBJECT

public:
	XKBExtension(Display *display=NULL);
	~XKBExtension();
	bool init();

	static bool setXkbOptions(const XkbOptions options);
	static TQString getServerOptions();
	bool setGroup(unsigned int group);
	unsigned int getGroup() const;
	void processXEvent(XEvent *ev);

private:
    Display *m_dpy;
	TQString m_tempDir;
	int m_keycode;
	static TQMap<TQString, FILE*> fileCache;

signals:
	void groupChanged(uint group);
};

#endif
