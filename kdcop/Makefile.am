#
#                           KDCOP Makefile.am
#
#  Copyright 2000 Matthias Kalle Dalheimer, kalle@dalheimer.de
#
#

INCLUDES = $(all_includes)

bin_PROGRAMS = kdcop

kdcop_SOURCES = kdcop.cpp kdcopwindow.cpp kdcoplistview.cpp kdcopview.ui
kdcop_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TDEUI) $(LIB_TDECORE) $(LIB_TDEIO) -lDCOP $(LIB_TQT)

noinst_HEADERS = kdcopwindow.h
METASOURCES =	AUTO

rcdir = $(kde_datadir)/kdcop
rc_DATA = kdcopui.rc

messages: rc.cpp
	$(XGETTEXT) rc.cpp *.cpp -o $(podir)/kdcop.pot

