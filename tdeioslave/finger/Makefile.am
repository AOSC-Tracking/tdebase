## Makfile.am for tdeio_finger
## Edit from Makefile.am of tdebase/tdeioslave/man

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

####### Files

kde_module_LTLIBRARIES = tdeio_finger.la

tdeio_finger_la_SOURCES = tdeio_finger.cpp 
tdeio_finger_la_LIBADD = -ltdeio
tdeio_finger_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = tdeio_finger.h 

kdelnk_DATA = finger.protocol
kdelnkdir = $(kde_servicesdir)

tdeio_finger_data_DATA = tdeio_finger.pl tdeio_finger.css
tdeio_finger_datadir = $(kde_datadir)/tdeio_finger
EXTRA_DIST=$(tdeio_finger_data_DATA)   

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_finger.pot
