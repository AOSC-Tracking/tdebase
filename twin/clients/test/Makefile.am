INCLUDES                  = -I$(srcdir)/../../lib $(all_includes)

kde_module_LTLIBRARIES    = twin_test.la

twin_test_la_SOURCES   = test.cpp
twin_test_la_LDFLAGS   = $(all_libraries) $(KDE_PLUGIN) -module
twin_test_la_LIBADD    = $(LIB_TDEUI) ../../lib/libtdecorations.la
#twin_keramik_la_LDFLAGS   = $(all_libraries) -avoid-version -module $(KDE_RPATH) $(KDE_MT_LDFLAGS)

METASOURCES               = AUTO

twin_test_lnkdir                    = $(kde_datadir)/twin
twin_test_lnk_DATA                  = test.desktop

EXTRA_DIST                = $(twin_test_lnk_DATA)
