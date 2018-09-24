####### Fiddle here

AM_CPPFLAGS = -DQT_NO_CAST_ASCII -DQT_NO_COMPAT

INCLUDES = $(all_includes)

#######	Files

bin_PROGRAMS	= kstart
kstart_SOURCES	= kstart.cpp
kstart_LDFLAGS  = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
kstart_LDADD    = $(LIB_TDECORE)
METASOURCES =	kstart.moc 

noinst_HEADERS = kstart.h version.h

messages:
	$(XGETTEXT) $(kstart_SOURCES) -o $(podir)/kstart.pot

EXTRA_DIST = CHANGES LICENSE.readme

