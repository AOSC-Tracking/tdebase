INCLUDES = -I$(srcdir)/../../libkicker -I../../libkicker \
	 -I../ui -I$(srcdir)/../ui -I$(srcdir)/../buttons -I$(top_srcdir)/libkonq \
	 $(all_includes) $(GLIB_CFLAGS)

noinst_LTLIBRARIES = libkicker_core.la

libkicker_core_la_COMPILE_FIRST = kmenubase.h

libkicker_core_la_SOURCES = extensionSettings.kcfgc \
    main.cpp kicker.cpp kicker.skel \
    userrectsel.cpp containerarea.cpp kmenubase.ui \
    applethandle.cpp container_base.cpp container_button.cpp \
    container_applet.cpp container_extension.cpp extensionmanager.cpp \
    menumanager.cpp pluginmanager.cpp showdesktop.cpp \
    unhidetrigger.cpp containerarealayout.cpp \
    panelextension.cpp panelextension.skel


libkicker_core_la_LDFLAGS = $(all_libraries)
libkicker_core_la_LIBADD = $(top_builddir)/libkonq/libkonq.la $(LIB_TDEUI)
libkicker_core_la_METASOURCES = AUTO
libkicker_core_la_COMPILE_FIRST = ../../libkicker/kickerSettings.h

kicker_core_data_DATA = default-apps
kicker_core_datadir = $(kde_datadir)/kicker

lnkdir = $(kde_datadir)/kicker/extensions
lnk_DATA = childpanelextension.desktop

kicker.lo: ../../libkicker/kickerSettings.h
showdesktop.lo: ../../libkicker/kickerSettings.h
container_applet.lo: ../../libkicker/kickerSettings.h
containerarea.lo: ../../libkicker/kickerSettings.h
container_extension.lo: ../../libkicker/kickerSettings.h

