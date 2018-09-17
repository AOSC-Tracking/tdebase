noinst_PROGRAMS = test_gravity

SUBDIRS = decobenchmark

INCLUDES = $(all_includes)

test_gravity_SOURCES = test_gravity.cpp
test_gravity_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
test_gravity_LDADD = $(LIB_X11)

METASOURCES = AUTO
