SUBDIRS=cli_installer
INCLUDES = $(all_includes) 

kde_module_LTLIBRARIES = twin_kwmtheme.la

twin_kwmtheme_la_SOURCES = kwmthemeclient.cpp
twin_kwmtheme_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
twin_kwmtheme_la_LIBADD  = -ltdecorations

METASOURCES = AUTO
noinst_HEADERS = kwmthemeclient.h

lnkdir = $(kde_datadir)/twin
lnk_DATA = kwmtheme.desktop

