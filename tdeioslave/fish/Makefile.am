kde_module_LTLIBRARIES = tdeio_fish.la

INCLUDES = $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

tdeio_fish_la_SOURCES = fish.cpp
tdeio_fish_la_LIBADD = $(LIB_TDESYCOCA) $(LIBUTIL) 
tdeio_fish_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -lutil
noinst_HEADERS = fishcode.h fish.h

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO FAQ fish.pl

DISTCLEANFILES = fishcode.h

kdelnk_DATA = fish.protocol nxfish.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

fish.lo: fishcode.h

fishcode.h: fish.pl
	SUM=`$(MD5SUM) $(srcdir)/fish.pl | cut -d ' ' $(MD5SUM_CUT)`; \
	echo '#define CHECKSUM "'$$SUM'"' > $@; \
	echo 'static const char *fishCode(' >> $@; \
	sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^[ 	]*/"/;/^"# /d;s/[ 	]*$$/\\n"/;/^"\\n"$$/d;s/{CHECKSUM}/'$$SUM'/;' $(srcdir)/fish.pl >> $@; \
	echo ');' >> $@;

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_fish.pot



