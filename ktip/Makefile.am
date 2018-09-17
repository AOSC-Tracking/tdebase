INCLUDES = $(all_includes)

SUBDIRS = pics

bin_PROGRAMS = ktip
ktip_SOURCES = ktipwindow.cpp
ktip_LDADD = $(LIB_TDEUI)
ktip_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

METASOURCES = AUTO
KDE_ICON = AUTO

xdg_apps_DATA = ktip.desktop

autostart_DATA = ktip.desktop
autostartdir = $(datadir)/autostart

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/Toys
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/Toys/ktip.desktop

messages: 
	$(PREPARETIPS) > tips.txt
	$(XGETTEXT) *.txt *.cpp -o $(podir)/ktip.pot
	rm -f tips.txt

appdata_DATA = tips
appdatadir=$(kde_datadir)/tdewizard

