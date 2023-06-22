
INCLUDES = -I$(srcdir)/../../lib $(all_includes) 

SUBDIRS = . config

kde_module_LTLIBRARIES = twin_b2.la

twin_b2_la_SOURCES = b2client.cpp
twin_b2_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
# twin_b2_la_LDFLAGS = $(all_libraries) -avoid-version -module $(KDE_RPATH) $(KDE_MT_LDFLAGS)
twin_b2_la_LIBADD  = ../../lib/libtdecorations.la

METASOURCES = AUTO
noinst_HEADERS = b2client.h

lnkdir = $(kde_datadir)/twin/
lnk_DATA = b2.desktop

EXTRA_DIST = $(lnk_DATA)

###KMAKE-start (don't edit or delete this block)

###KMAKE-end
