####### Fiddle here

INCLUDES = $(all_includes)
LDADD    = $(LIB_TDEUI)

#######	Files

bin_PROGRAMS	= ksystraycmd
METASOURCES	= ksystraycmd.moc
noinst_HEADERS  = ksystraycmd.h

ksystraycmd_SOURCES = ksystraycmd.cpp main.cpp
ksystraycmd_LDFLAGS  = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

messages:
	$(XGETTEXT) $(ksystraycmd_SOURCES) -o $(podir)/ksystraycmd.pot


