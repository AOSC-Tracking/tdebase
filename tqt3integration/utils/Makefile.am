noinst_PROGRAMS = gen
gen_SOURCES = gen.cpp
gen_LDADD = $(LIB_TDECORE)
gen_LDFLAGS = $(all_libraries)
INCLUDES = $(all_includes)
CLEANFILES = tqtkde_functions.cpp module_functions.cpp module_functions.h generated
SUBDIRS = tqt

generate: generated

generated: gen gen.txt
	./gen || exit 1
	cat tqt/in/tqtkdeintegration_x11_0.cpp tqt/in/tqtkdeintegration_x11_1.cpp tqtkdeintegration_x11.cpp.gen tqt/in/tqtkdeintegration_x11_2.cpp >tqt/tqtkdeintegration_x11.cpp
	cat tqt/in/tqtkdeintegration_x11_p_1.h tqtkdeintegration_x11_p.h.gen tqt/in/tqtkdeintegration_x11_p_2.h >tqt/tqtkdeintegration_x11_p.h
	rm -f tqtkdeintegration_x11.cpp.gen tqtkdeintegration_x11_p.h.gen
	touch generated

tqtkde_functions.cpp module_functions.cpp module_functions.h : generated
