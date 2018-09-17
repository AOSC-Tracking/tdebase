## Makfile.am for tdeio_mac

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

####### Files

kde_module_LTLIBRARIES = tdeio_mac.la

tdeio_mac_la_SOURCES = tdeio_mac.cpp
tdeio_mac_la_LIBADD = -ltdeio
tdeio_mac_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = tdeio_mac.h

kdelnk_DATA = mac.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO
KDE_ICON = AUTO

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_mac.pot
