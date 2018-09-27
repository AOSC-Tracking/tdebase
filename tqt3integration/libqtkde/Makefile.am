qtkdelib_LTLIBRARIES = libtqtkde.la

libtqtkde_la_SOURCES = qtkde.cpp
libtqtkde_la_LIBADD = -lDCOP
libtqtkde_la_LDFLAGS = $(all_libraries) -module -no-undefined -avoid-version

CLEANFILES = qtkde_functions.cpp

INCLUDES = $(all_includes)
METASOURCES = AUTO

qtkde.lo : qtkde_functions.cpp

qtkde_functions.cpp : ../utils/qtkde_functions.cpp
	cp -f ../utils/qtkde_functions.cpp . || exit 1
