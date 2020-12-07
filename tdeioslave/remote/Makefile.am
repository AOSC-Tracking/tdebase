SUBDIRS= . kdedmodule
# wizard

INCLUDES = $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = tdeio_remote.la

tdeio_remote_la_SOURCES = dummy.cpp
tdeio_remote_la_LIBADD  = libtdeioremote.la $(LIB_TDEIO)
tdeio_remote_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = remote.protocol

noinst_LTLIBRARIES = libtdeioremote.la
libtdeioremote_la_SOURCES = tdeio_remote.cpp remoteimpl.cpp

check_PROGRAMS = testremote
testremote_SOURCES = testremote.cpp
testremote_LDADD = libtdeioremote.la $(LIB_TDEIO)
testremote_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testremote
check: testremote
	./testremote

messages:
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.h"` -o $(podir)/tdeio_remote.pot

