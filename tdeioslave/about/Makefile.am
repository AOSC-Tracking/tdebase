## Makefile.am of tdebase/tdeioslave/about

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

####### Files

kde_module_LTLIBRARIES = tdeio_about.la

tdeio_about_la_SOURCES = tdeio_about.cpp
tdeio_about_la_LIBADD = $(LIB_TDESYCOCA)
tdeio_about_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = tdeio_about.h

kdelnk_DATA = about.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/tdeio_about.pot
