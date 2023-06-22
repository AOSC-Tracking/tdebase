INCLUDES                  = -I$(top_srcdir) $(all_includes)
kde_module_LTLIBRARIES    = twin_web.la

twin_web_la_SOURCES      = Web.cpp WebButton.cpp

noinst_HEADERS            = Web.h WebButton.h

twin_web_la_LDFLAGS      = $(all_libraries) $(KDE_PLUGIN) -module
twin_web_la_LIBADD       = $(LIB_TDEUI) ../../lib/libtdecorations.la
METASOURCES               = AUTO

linkdir                   = $(kde_datadir)/twin/
link_DATA                 = web.desktop
EXTRA_DIST                = $(link_DATA)

