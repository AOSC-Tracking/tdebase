INCLUDES = -I$(srcdir)/../core -I$(srcdir)/../../libkicker -I../../libkicker \
	-I../ui -I$(srcdir)/../ui -I$(top_srcdir)/libkonq $(all_includes) $(GLIB_CFLAGS)

noinst_LTLIBRARIES = libkicker_buttons.la

libkicker_buttons_la_SOURCES = servicebutton.cpp bookmarksbutton.cpp \
	browserbutton.cpp knewbutton.cpp \
	desktopbutton.cpp extensionbutton.cpp kbutton.cpp \
	nontdeappbutton.cpp servicemenubutton.cpp urlbutton.cpp \
	windowlistbutton.cpp

libkicker_buttons_la_LDFLAGS = $(all_libraries)
libkicker_buttons_la_LIBADD = $(top_builddir)/libkonq/libkonq.la $(LIB_TDEUI) $(XTESTLIB)
libkicker_buttons_la_METASOURCES = AUTO

desktopmenu_DATA = bookmarks.desktop browser.desktop desktop.desktop \
                   exec.desktop kmenu.desktop windowlist.desktop
desktopmenudir = $(kde_datadir)/kicker/builtins

browserbutton.lo: ../../libkicker/kickerSettings.h
desktopbutton.lo: ../../libkicker/kickerSettings.h
panelbutton.lo: ../../libkicker/kickerSettings.h
servicebutton.lo: ../../libkicker/kickerSettings.h
urlbutton.lo: ../../libkicker/kickerSettings.h
