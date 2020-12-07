## Makefile.am of tdebase/kdesktop

INCLUDES = -I$(top_srcdir)/kcontrol/background -I$(top_srcdir)/libkonq -I$(top_srcdir)/tdmlib $(all_includes)
kdesktop_la_LDFLAGS  = $(all_libraries) -module $(KDE_PLUGIN)
kdesktop_la_LIBADD   = libkdesktopsettings.la $(top_builddir)/libkonq/libkonq.la $(top_builddir)/kcontrol/background/libbgnd.la $(top_builddir)/tdmlib/libdmctl.la $(LIB_XSS) $(LIB_TDEUTILS) $(LIB_XCURSOR)

SUBDIRS = . lock pics patterns programs init kwebdesktop

####### Files

bin_PROGRAMS = kcheckrunning
lib_LTLIBRARIES =
tdeinit_LTLIBRARIES = kdesktop.la
noinst_LTLIBRARIES = libkdesktopsettings.la

libkdesktopsettings_la_LDFLAGS = $(all_libraries) -no-undefined
libkdesktopsettings_la_LIBADD = $(LIB_TDECORE)
libkdesktopsettings_la_SOURCES = kdesktopsettings.kcfgc tdelaunchsettings.kcfgc 

kdesktop_la_SOURCES = main.cpp krootwm.cpp xautolock.cpp kdiconview.cpp desktop.cpp \
	lockeng.cpp KDesktopIface.skel \
	bgmanager.cpp init.cpp KScreensaverIface.skel \
	minicli.cpp KBackgroundIface.skel pixmapserver.cpp kcustommenu.cpp \
	startupid.cpp minicli_ui.ui xautolock_diy.c xautolock_engine.c \
	kshadowengine.cpp kshadowsettings.cpp \
	kdesktopshadowsettings.cpp tdefileividesktop.cpp \
	kdesktopapp.cpp

include_HEADERS = KDesktopIface.h KScreensaverIface.h KBackgroundIface.h

noinst_HEADERS = desktop.h bgmanager.h krootwm.h \
	xautolock.h lockeng.h init.h minicli.h \
	pixmapserver.h startupid.h xautolock_c.h \
	kdesktopapp.h

kcheckrunning_SOURCES = kcheckrunning.cpp
kcheckrunning_LDFLAGS = $(all_libraries)
kcheckrunning_LDADD = $(LIB_X11) $(LIB_QT)

METASOURCES = AUTO

####### Build rules

# List made with: grep -l kdesktopsettings.h *[pc] | sed -e 's/\.c.*$/.lo: kdesktopsettings.h/'
bgmanager.lo: kdesktopsettings.h
desktop.lo: kdesktopsettings.h
init.lo: kdesktopsettings.h
kdesktopsettings.lo: kdesktopsettings.h
kdiconview.lo: kdesktopsettings.h
krootwm.lo: kdesktopsettings.h
lockeng.lo: kdesktopsettings.h
main.lo: kdesktopsettings.h
minicli.lo: kdesktopsettings.h

# List made with: grep -l tdelaunchsettings.h *[pc] | sed -e 's/\.c.*$/.lo: tdelaunchsettings.h/'
desktop.lo: tdelaunchsettings.h
tdelaunchsettings.lo: tdelaunchsettings.h
startupid.lo: tdelaunchsettings.h

messages: rc.cpp
	$(EXTRACTRC) lock/*.ui >> rc.cpp
	$(XGETTEXT) lock/*.cpp *.cpp *.h -o $(podir)/kdesktop.pot

menu_DATA = kdesktop_custom_menu1 kdesktop_custom_menu2
menudir = $(kde_confdir)

autostart_DATA = kdesktop.desktop
autostartdir = $(datadir)/autostart

konqservice_DATA = kdesktopSetAsBackground.desktop
konqservicedir = $(kde_datadir)/konqueror/servicemenus

kde_kcfg_DATA = kdesktop.kcfg tdelaunch.kcfg
