INCLUDES = $(all_includes)
METASOURCES = AUTO

SUBDIRS = . tdefile-plugin

kde_module_LTLIBRARIES = tdeio_trash.la kcm_trash ktrashpropsdlgplugin.la

tdeio_trash_la_SOURCES = tdeio_trash.cpp
tdeio_trash_la_LIBADD  = libtrashcommon.la $(LIB_TDEIO)
tdeio_trash_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

kcm_trash_la_SOURCES = kcm_trash.cpp
kcm_trash_la_LIBADD  = libtrashcommon.la $(LIB_TDEIO)
kcm_trash_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

bin_PROGRAMS = ktrash
ktrash_SOURCES = ktrash.cpp
ktrash_LDADD = $(LIB_TDEIO)
ktrash_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

kde_services_DATA = trash.protocol

noinst_LTLIBRARIES = libtrashcommon.la
libtrashcommon_la_SOURCES = trashimpl.cpp discspaceutil.cpp ktrashpropswidget.cpp

check_PROGRAMS = testtrash
testtrash_SOURCES = testtrash.cpp
testtrash_LDADD = libtrashcommon.la $(LIB_TDEIO)
testtrash_LDFLAGS = $(all_libraries)

TESTS = testtrash

messages:
	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/tdeio_trash.pot

# ktrashpropsdlgplugin target
noinst_HEADERS = ktrashpropsdlgplugin.h discspaceutil.h trash_constant.h

ktrashpropsdlgplugin_la_SOURCES = ktrashpropsdlgplugin.cpp discspaceutil.cpp trashimpl.cpp
ktrashpropsdlgplugin_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
ktrashpropsdlgplugin_la_LIBADD = $(LIB_TDEIO)

services_DATA = ktrashpropsdlgplugin.desktop
servicesdir = $(kde_servicesdir)

xdg_apps_DATA = kcm_trash.desktop
