noinst_LTLIBRARIES = libtdefontinstprint.la
libtdefontinstprint_la_SOURCES = KfiPrint.cpp
libtdefontinstprint_la_LDFLAGS = $(all_libraries)
libtdefontinstprint_la_LIBADD = $(LIB_TDEPRINT) ../lib/libtdefontinst.la

kde_module_LTLIBRARIES = libtdefontviewpart.la

libtdefontviewpart_la_SOURCES = FontViewPart.cpp FontViewPartFactory.cpp FontPreview.cpp
libtdefontviewpart_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
libtdefontviewpart_la_LIBADD = $(LIB_TDEPARTS) libtdefontinstprint.la ../lib/libtdefontinst.la

noinst_HEADERS  = FontViewPart.h FontViewPartFactory.h FontPreview.h FontViewerApp.h KfiPrint.h

kde_services_DATA = tdefontviewpart.desktop

AM_CPPFLAGS = -I$(srcdir)/../lib -I$(srcdir)/../../fonts $(all_includes) $(LIBFREETYPE_CFLAGS) $(LIBFONTCONFIG_CFLAGS)
METASOURCES = AUTO

tdefontview_LDADD = $(LIB_TDEPARTS)
tdefontview_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

bin_PROGRAMS = tdefontview
xdg_apps_DATA = tdefontview.desktop

appdata_DATA = tdefontviewpart.rc tdefontviewui.rc
appdatadir = $(kde_datadir)/tdefontview

tdefontview_SOURCES = FontViewerApp.cpp


