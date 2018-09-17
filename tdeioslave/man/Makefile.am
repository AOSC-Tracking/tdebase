## Makefile.am of tdebase/tdeioslave/man

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

EXTRA_PROGRAMS = tdeio_man_test man2html

####### just for testing (j.habenicht@europemail.com, 15.02.2001)

tdeio_man_test_SOURCES = tdeio_man_test.cpp
tdeio_man_test_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
tdeio_man_test_LDADD = man2html.lo tdeio_man.lo $(LIB_TDEIO) $(LIB_TDEUI) $(LIB_TDECORE) $(LIB_TQT)

####### Files

kde_module_LTLIBRARIES = tdeio_man.la  libkmanpart.la

tdeio_man_la_SOURCES = man2html.cpp tdeio_man.cpp
tdeio_man_la_LIBADD = $(LIB_TDESYCOCA)
tdeio_man_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = tdeio_man.h
### TODO Why is man2htmk.h distributed?

libkmanpart_la_SOURCES = kmanpart.cpp
libkmanpart_la_LIBADD = -ltdehtml $(LIB_TDEPARTS)
libkmanpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

kdelnk_DATA = man.protocol kmanpart.desktop
kdelnkdir = $(kde_servicesdir)

tdeio_man_data_DATA = tdeio_man.css
tdeio_man_datadir = $(kde_datadir)/tdeio_man
EXTRA_DIST=$(tdeio_man_data_DATA)

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/tdeio_man.pot

man2html_SOURCES = dummy.cpp
man2html_LDADD = man2html_simple.o $(LIB_TQT)
man2html_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

dummy.cpp:
	echo > $@

man2html_simple.o: $(srcdir)/man2html.cpp
	-rm -f man2html_simple.cpp
	$(LN_S) $(srcdir)/man2html.cpp man2html_simple.cpp
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) -DSIMPLE_MAN2HTML $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c man2html_simple.cpp

