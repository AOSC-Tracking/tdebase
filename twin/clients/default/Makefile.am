
INCLUDES = -I$(srcdir)/../../lib $(all_includes)

SUBDIRS = . config

kde_module_LTLIBRARIES = twin_default.la

twin_default_la_SOURCES = kdedefault.cpp
twin_default_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -module
twin_default_la_LIBADD  = $(LIB_TDECORE) ../../lib/libtdecorations.la

METASOURCES = AUTO
noinst_HEADERS = kdedefault.h

