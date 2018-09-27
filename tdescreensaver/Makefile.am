# Makefile.am for tdescreensaver.
#

AM_CPPFLAGS = -UTQT_NO_ASCII_CAST

INCLUDES = $(GLINC) $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

bin_PROGRAMS = krandom.kss kblankscrn.kss

krandom_kss_SOURCES = random.cpp
krandom_kss_LDADD = $(LIB_TDEUI) $(GLLIB) -lm

kblankscrn_kss_SOURCES = blankscrn.cpp
kblankscrn_kss_LDADD = $(LIB_TDEUI) -ltdescreensaver -lm 

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/tdescreensaver.pot

########## Meta objects ##########

desktop_DATA = KRandom.desktop KBlankscreen.desktop
desktopdir = $(kde_appsdir)/System/ScreenSavers

EXTRA_DIST = $(desktop_DATA)
