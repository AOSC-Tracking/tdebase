INCLUDES = -I$(top_srcdir)/libltdl -I.. $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = testnsplugin.h

check_PROGRAMS = testnsplugin
testnsplugin_SOURCES = testnsplugin.cpp
testnsplugin_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
testnsplugin_LDADD = $(LIB_TDEUI) ../libnsplugin.la

$(srcdir)/testnsplugin.cpp: ../NSPluginClassIface_stub.h

