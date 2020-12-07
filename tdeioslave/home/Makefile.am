SUBDIRS= . kdedmodule
# wizard

INCLUDES = $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = tdeio_home.la

tdeio_home_la_SOURCES = dummy.cpp
tdeio_home_la_LIBADD  = libtdeiohome.la $(LIB_TDEIO)
tdeio_home_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = home.protocol

noinst_LTLIBRARIES = libtdeiohome.la
libtdeiohome_la_SOURCES = tdeio_home.cpp homeimpl.cpp

check_PROGRAMS = testhome
testhome_SOURCES = testhome.cpp
testhome_LDADD = libtdeiohome.la $(LIB_TDEIO)
testhome_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testhome
check: testhome
	./testhome

messages:
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.h"` -o $(podir)/tdeio_home.pot

