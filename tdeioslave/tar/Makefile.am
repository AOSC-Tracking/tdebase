## Makefile.am of tdebase/tdeioslave/tar

INCLUDES = $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_QT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
METASOURCES = AUTO

kde_module_LTLIBRARIES = tdeio_tar.la

tdeio_tar_la_SOURCES = tar.cpp
tdeio_tar_la_LIBADD = $(LIB_TDESYCOCA)
tdeio_tar_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

check_PROGRAMS = ktartest

ktartest_SOURCES = ktartest.cpp
ktartest_LDADD = $(LIB_TDESYCOCA)

noinst_HEADERS = tar.h

kdelnk_DATA = tar.protocol ar.protocol zip.protocol
kdelnkdir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_tar.pot
