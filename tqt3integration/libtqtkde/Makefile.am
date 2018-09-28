tqtkdelib_LTLIBRARIES = libtqtkde.la

libtqtkde_la_SOURCES = tqtkde.cpp
libtqtkde_la_LIBADD = -lDCOP
libtqtkde_la_LDFLAGS = $(all_libraries) -module -no-undefined -avoid-version

CLEANFILES = tqtkde_functions.cpp

INCLUDES = $(all_includes)
METASOURCES = AUTO

tqtkde.lo : tqtkde_functions.cpp

tqtkde_functions.cpp : ../utils/tqtkde_functions.cpp
	cp -f ../utils/tqtkde_functions.cpp . || exit 1
