SUBDIRS= . kdedmodule entries mimetypes

INCLUDES = $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = tdeio_system.la

tdeio_system_la_SOURCES = dummy.cpp
tdeio_system_la_LIBADD  = libtdeiosystem.la $(LIB_TDEIO)
tdeio_system_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = system.protocol

noinst_LTLIBRARIES = libtdeiosystem.la
libtdeiosystem_la_SOURCES = tdeio_system.cpp systemimpl.cpp

check_PROGRAMS = testsystem
testsystem_SOURCES = testsystem.cpp
testsystem_LDADD = libtdeiosystem.la $(LIB_TDEIO)
testsystem_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testsystem
check: testsystem
	./testsystem

messages:
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.h"` -o $(podir)/tdeio_system.pot

