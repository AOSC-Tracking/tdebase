INCLUDES= $(all_includes)

####### Files

kde_module_LTLIBRARIES = tdeio_settings.la

tdeio_settings_la_SOURCES = tdeio_settings.cpp
tdeio_settings_la_LIBADD  = $(LIB_TDESYCOCA)
tdeio_settings_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = tdeio_settings.h

kdelnk_DATA = settings.protocol programs.protocol applications.protocol
kdelnkdir = $(kde_servicesdir)


SUBDIRS=. 

messages:
	$(XGETTEXT) `find . -name "*.cpp" -o -name "*.h"` -o $(podir)/tdeio_settings.pot

