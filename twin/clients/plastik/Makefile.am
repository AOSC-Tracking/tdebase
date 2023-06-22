AUTOMAKE_OPTIONS = foreign

SUBDIRS = config

KDE_CXXFLAGS = -DQT_PLUGIN

INCLUDES = -I$(srcdir)/../../lib $(all_includes)

twindir = $(kde_datadir)/twin/
twin_DATA = plastik.desktop

kde_module_LTLIBRARIES = twin_plastik.la
twin_plastik_la_SOURCES = plastik.cpp plastikclient.cpp plastikbutton.cpp misc.cpp
twin_plastik_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
twin_plastik_la_LIBADD = $(LIB_TDEUI) ../../lib/libtdecorations.la
twin_plastik_la_METASOURCES = AUTO

DISTCLEANFILES = $(twin_plastik_la_METASOURCES)

