SUBDIRS = themeengine pics kcmksplash

INCLUDES = -I$(srcdir)/themeengine -I$(srcdir)/themeengine/default $(all_includes)

METASOURCES=AUTO

bin_PROGRAMS = ksplash
ksplash_SOURCES = wndmain.cpp ksplashiface.skel main.cpp
ksplash_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
ksplash_LDADD = $(LIB_TDEUI) themeengine/default/libthemedefault.la themeengine/libksplashthemes.la $(LIB_TDEIO)

noinst_HEADERS = ksplashiface.h wndmain.h

servicesdir = $(kde_servicesdir)
services_DATA = ksplash.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp themeengine/*.cpp -o $(podir)/ksplash.pot
