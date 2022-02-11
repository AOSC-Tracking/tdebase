SUBDIRS = libmediacommon . mediamanager medianotifier mounthelper \
          tdefile-plugin tdecmodule mimetypes services $(PROPSDLGPLUGINDIR)

INCLUDES = -I$(srcdir)/libmediacommon $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = tdeio_media.la

tdeio_media_la_SOURCES = dummy.cpp
tdeio_media_la_LIBADD  = libtdeiomedia.la libmediacommon/libmediacommon.la $(LIB_TDEIO)
tdeio_media_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = media.protocol

noinst_LTLIBRARIES = libtdeiomedia.la
libtdeiomedia_la_SOURCES = tdeio_media.cpp mediaimpl.cpp mediaimpl.skel

check_PROGRAMS = testmedia
testmedia_SOURCES = testmedia.cpp
testmedia_LDADD = libtdeiomedia.la $(LIB_TDEIO)
testmedia_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testmedia
check: testmedia
	./testmedia

messages: rc.cpp
	$(EXTRACTRC) propsdlgplugin/propertiespagegui.ui >> rc.cpp
	$(XGETTEXT) *.cpp *.h tdefile-plugin/*.cpp libmediacommon/*.cpp mediamanager/*.cpp mounthelper/*.cpp propsdlgplugin/*.cpp -o $(podir)/tdeio_media.pot
