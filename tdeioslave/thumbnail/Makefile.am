## Makefile.am of tdebase/tdeioslave/thumbnail

INCLUDES = $(all_includes) $(EXR_FLAGS)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
METASOURCES = AUTO

if have_xcursor
CURSOR_MODULE = cursorthumbnail.la
CURSOR_DATAFILE = cursorthumbnail.desktop
endif

if include_EXR_MODULES
EXR_MODULE = exrthumbnail.la
EXR_DATAFILE = exrthumbnail.desktop
endif

kde_module_LTLIBRARIES = tdeio_thumbnail.la imagethumbnail.la \
	textthumbnail.la htmlthumbnail.la \
	djvuthumbnail.la $(CURSOR_MODULE) \
	$(EXR_MODULE)

tdeio_thumbnail_la_SOURCES = thumbnail.cpp
tdeio_thumbnail_la_LIBADD = $(LIB_TDEIO)
tdeio_thumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

imagethumbnail_la_SOURCES = imagecreator.cpp
imagethumbnail_la_LIBADD = $(LIB_TDEIO) # for kimageio
imagethumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

textthumbnail_la_SOURCES = textcreator.cpp
textthumbnail_la_LIBADD = $(LIB_TDEIO) # for kmimetype
textthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

htmlthumbnail_la_SOURCES = htmlcreator.cpp
htmlthumbnail_la_LIBADD = $(LIB_TDEHTML)
htmlthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

djvuthumbnail_la_SOURCES = djvucreator.cpp
djvuthumbnail_la_LIBADD = $(LIB_TDECORE)
djvuthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

cursorthumbnail_la_SOURCES = cursorcreator.cpp
cursorthumbnail_la_LIBADD = $(LIB_TDECORE) $(LIB_XCURSOR)
cursorthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

exrthumbnail_la_SOURCES = exrcreator.cpp
exrthumbnail_la_LIBADD = $(LIB_TDECORE) $(LIB_EXR)
exrthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = thumbnail.h imagecreator.h textcreator.h htmlcreator.h \
                 djvucreator.h cursorcreator.h \
		 exrcreator.h

kdelnk_DATA = thumbnail.protocol
kdelnkdir = $(kde_servicesdir)

servicetypes_DATA = thumbcreator.desktop
servicetypesdir = $(kde_servicetypesdir)

services_DATA = imagethumbnail.desktop textthumbnail.desktop \
                htmlthumbnail.desktop \
                djvuthumbnail.desktop \
                $(CURSOR_DATAFILE) $(EXR_DATAFILE)

servicesdir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_thumbnail.pot

