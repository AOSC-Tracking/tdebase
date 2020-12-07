INCLUDES= $(all_includes)

bin_PROGRAMS = tdeprintfax
tdeprintfax_SOURCES = main.cpp tdeprintfax.cpp faxab.cpp faxctrl.cpp confgeneral.cpp configdlg.cpp \
		      conffax.cpp confsystem.cpp conffilters.cpp filterdlg.cpp defcmds.cpp
tdeprintfax_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_QT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
tdeprintfax_LDADD   = $(LIB_TDEUI) $(LIB_TDEIO) $(LIB_TDEPRINT) -ltdeabc
tdeprintfax_METASOURCES = AUTO

xdg_apps_DATA = tdeprintfax.desktop

uidir = $(kde_datadir)/tdeprintfax
ui_DATA = tdeprintfaxui.rc faxfilters

private_SCRIPTS = anytops
privatedir = $(kde_datadir)/tdeprintfax

messages: rc.cpp
	$(XGETTEXT) `find . -name \*.h -o -name \*.cpp` -o $(podir)/tdeprintfax.pot

KDE_ICON = tdeprintfax
pic_ICON = abentry
picdir = $(kde_datadir)/tdeprintfax/icons
