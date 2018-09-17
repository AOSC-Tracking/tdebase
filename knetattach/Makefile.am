INCLUDES= $(all_includes)

bin_PROGRAMS = knetattach
knetattach_SOURCES = knetattach.ui main.cpp
knetattach_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
knetattach_LDADD   = $(LIB_TDEIO)
METASOURCES = AUTO
xdg_apps_DATA = knetattach.desktop
KDE_ICON = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/knetattach.pot

knetattach.lo: knetattach.ui knetattach.ui.h
