INCLUDES = $(all_includes)
METASOURCES = AUTO

SUBDIRS = . tdefile-plugin

kde_module_LTLIBRARIES = tdeio_trash.la

tdeio_trash_la_SOURCES = tdeio_trash.cpp
tdeio_trash_la_LIBADD  = libtrashcommon.la $(LIB_TDEIO)
tdeio_trash_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

bin_PROGRAMS = ktrash
ktrash_SOURCES = ktrash.cpp
ktrash_LDADD = $(LIB_TDEIO)
ktrash_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_QT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

kde_services_DATA = trash.protocol

noinst_LTLIBRARIES = libtrashcommon.la
libtrashcommon_la_SOURCES = trashimpl.cpp

check_PROGRAMS = testtrash
testtrash_SOURCES = testtrash.cpp
testtrash_LDADD = libtrashcommon.la $(LIB_TDEIO)
testtrash_LDFLAGS = $(all_libraries)

TESTS = testtrash

messages:
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.h"` -o $(podir)/tdeio_trash.pot

