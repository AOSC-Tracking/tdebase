INCLUDES = $(all_includes)

bin_PROGRAMS = twin_killer_helper

twin_killer_helper_SOURCES = killer.cpp
twin_killer_helper_LDADD = $(LIB_TDEUI)
twin_killer_helper_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

METASOURCES = AUTO
