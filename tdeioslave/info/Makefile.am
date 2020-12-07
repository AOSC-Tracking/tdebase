## Makefile.am of tdebase/tdeioslave/info

INCLUDES = $(all_includes)

METASOURCES = AUTO

####### Files

kde_module_LTLIBRARIES = tdeio_info.la

tdeio_info_la_SOURCES = info.cpp
tdeio_info_la_LIBADD = $(LIB_TDEIO)
tdeio_info_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = info.h

kdelnk_DATA = info.protocol
kdelnkdir = $(kde_servicesdir)

tdeio_info_data_DATA = kde-info2html.conf
tdeio_info_data_SCRIPTS = kde-info2html
tdeio_info_datadir = $(kde_datadir)/tdeio_info
