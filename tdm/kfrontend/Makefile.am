# use 'make GENTDMCONF_FLAGS=... install' to override
GENTDMCONF_FLAGS = --no-old

SUBDIRS = themer themes pics sessions

AM_CPPFLAGS = -I$(srcdir)/../backend -I.. -I$(top_srcdir)/kcontrol/background \
    -I$(top_srcdir)/tdmlib $(all_includes)

bin_PROGRAMS = tdm_config tdm_greet krootimage gentdmconf tdmctl

tdm_config_SOURCES = tdm_config.c
tdm_config_LDADD = $(LIBRESOLV) $(LIBSOCKET) $(LIBPOSIX4)

tdm_greet_SOURCES = \
    tdm_greet.c \
    tdmconfig.cpp \
    tdmclock.cpp \
    kconsole.cpp \
    kfdialog.cpp \
    kgdialog.cpp \
    kchooser.cpp \
    kgverify.cpp \
    tdmshutdown.cpp \
    tdmadmindialog.cpp \
    kgreeter.cpp \
    kgapp.cpp
tdm_greet_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
tdm_greet_LDADD = themer/libtdmthemer.a $(LIB_TDEUI) $(XTESTLIB) $(LIBPOSIX4)

krootimage_SOURCES = krootimage.cpp
krootimage_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
krootimage_LDADD = $(top_builddir)/kcontrol/background/libbgnd.la $(LIB_TDEIO)

METASOURCES = AUTO

gentdmconf_SOURCES = gentdmconf.c
gentdmconf_LDFLAGS = $(X_LDFLAGS) $(X_RPATH)
gentdmconf_LDADD = $(LIB_X11)

tdmctl_SOURCES = tdmctl.c
tdmctl_LDADD = $(LIBSOCKET)

install-data-local: gentdmconf
	./gentdmconf --in $(DESTDIR)$(kde_confdir)/tdm --no-in-notice --face-src $(srcdir)/pics $(GENTDMCONF_FLAGS)

messages:
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/tdmgreet.pot

noinst_HEADERS = \
    tdm_greet.h \
    tdmconfig.h \
    tdmclock.h \
    kconsole.h \
    kfdialog.h \
    kgdialog.h \
    kchooser.h \
    kgverify.h \
    tdmshutdown.h \
    kgreeter.h \
    kgapp.h \
    \
    krootimage.h 

tdm_greet_COMPILE_FIRST = ../config.ci
tdm_config_COMPILE_FIRST = ../config.ci
gentdmconf_COMPILE_FIRST = ../config.ci

