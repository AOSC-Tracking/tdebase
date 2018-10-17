## Makefile.am of tdebase/kcontrol/ebrowsing/plugins/shorturi

AM_CPPFLAGS   =  $(all_includes) -DTQT_NO_CAST_ASCII

kde_module_LTLIBRARIES = libtdeshorturifilter.la

libtdeshorturifilter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
libtdeshorturifilter_la_LIBADD = $(LIB_TDESYCOCA) $(LIB_TDEUI)

libtdeshorturifilter_la_SOURCES = tdeshorturifilter.cpp tdeshorturifilter.skel

service_DATA = tdeshorturifilter.desktop
servicedir = $(kde_servicesdir)

rc_DATA = tdeshorturifilterrc
rcdir = $(kde_confdir)

METASOURCES = AUTO
