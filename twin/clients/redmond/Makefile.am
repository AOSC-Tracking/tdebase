
INCLUDES = -I$(srcdir)/../../lib $(all_includes)

kde_module_LTLIBRARIES = twin_redmond.la

twin_redmond_la_SOURCES = redmond.cpp
twin_redmond_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
twin_redmond_la_LIBADD = $(LIB_TDECORE) ../../lib/libtdecorations.la

METASOURCES = AUTO
noinst_HEADERS = redmond.h

lnkdir = $(kde_datadir)/twin
lnk_DATA = redmond.desktop

EXTRA_DIST = $(lnk_DATA)

