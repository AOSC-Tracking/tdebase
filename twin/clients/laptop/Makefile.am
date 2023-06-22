
INCLUDES = -I$(srcdir)/../../lib $(all_includes)

kde_module_LTLIBRARIES = twin_laptop.la

twin_laptop_la_SOURCES = laptopclient.cpp
twin_laptop_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module 
twin_laptop_la_LIBADD  = ../../lib/libtdecorations.la

METASOURCES = AUTO
noinst_HEADERS = laptopclient.h

lnkdir = $(kde_datadir)/twin
lnk_DATA = laptop.desktop

EXTRA_DIST = $(lnk_DATA)

