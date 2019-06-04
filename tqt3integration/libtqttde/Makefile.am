tqttdelib_LTLIBRARIES = libtqttde.la

libtqttde_la_SOURCES = tqttde.cpp
libtqttde_la_LIBADD = -lDCOP
libtqttde_la_LDFLAGS = $(all_libraries) -module -no-undefined -avoid-version

CLEANFILES = tqttde_functions.cpp

INCLUDES = $(all_includes)
METASOURCES = AUTO

tqttde.lo : tqttde_functions.cpp

tqttde_functions.cpp : ../utils/tqttde_functions.cpp
	cp -f ../utils/tqttde_functions.cpp . || exit 1
