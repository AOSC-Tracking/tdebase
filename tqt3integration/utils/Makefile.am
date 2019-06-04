noinst_PROGRAMS = gen
gen_SOURCES = gen.cpp
gen_LDADD = $(LIB_TDECORE)
gen_LDFLAGS = $(all_libraries)
INCLUDES = $(all_includes)
CLEANFILES = tqttde_functions.cpp module_functions.cpp module_functions.h generated
SUBDIRS = tqt

generate: generated

generated: gen gen.txt
	./gen || exit 1
	cat tqt/in/tqttdeintegration_x11_0.cpp tqt/in/tqttdeintegration_x11_1.cpp tqttdeintegration_x11.cpp.gen tqt/in/tqttdeintegration_x11_2.cpp >tqt/tqttdeintegration_x11.cpp
	cat tqt/in/tqttdeintegration_x11_p_1.h tqttdeintegration_x11_p.h.gen tqt/in/tqttdeintegration_x11_p_2.h >tqt/tqttdeintegration_x11_p.h
	rm -f tqttdeintegration_x11.cpp.gen tqttdeintegration_x11_p.h.gen
	touch generated

tqttde_functions.cpp module_functions.cpp module_functions.h : generated
