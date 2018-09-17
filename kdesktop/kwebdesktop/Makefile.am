
INCLUDES= $(all_includes)
LDADD = $(LIB_TDEHTML)

bin_PROGRAMS = 	kwebdesktop

METASOURCES = AUTO

kwebdesktop_SOURCES = kwebdesktop.cpp kwebdesktopsettings.kcfgc 
kwebdesktop_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

kdesktop_kwebdesktop_data_DATA = kwebdesktop.desktop
kdesktop_kwebdesktop_datadir = $(kde_datadir)/kdesktop/programs

kde_kcfg_DATA = kwebdesktop.kcfg 
