SUBDIRS = apps

bin_PROGRAMS = kappfinder
kappfinder_SOURCES = main.cpp toplevel.cpp common.cpp
kappfinder_LDADD = $(LIB_TDEUI) $(LIB_TDEIO)
kappfinder_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

noinst_PROGRAMS = kappfinder_install
kappfinder_install_SOURCES = main_install.cpp common.cpp
kappfinder_install_LDADD = $(LIB_TDEUI)
kappfinder_install_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

INCLUDES = $(all_includes)
METASOURCES = AUTO

install-data-local: runkappfinder_$(RUN_KAPPFINDER)

runkappfinder_yes:
	echo "Installing non-TDE applications"
	pwd=`pwd`; \
	(cd $(srcdir) && $$pwd/kappfinder_install "$(DESTDIR)$(kde_appsdir)")

runkappfinder_no:

KDE_ICON = kappfinder

xdg_apps_DATA = kappfinder.desktop

messages:
	$(XGETTEXT) $(kappfinder_SOURCES) -o $(podir)/kappfinder.pot

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_appsdir)/System
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_appsdir)/System/kappfinder.desktop
