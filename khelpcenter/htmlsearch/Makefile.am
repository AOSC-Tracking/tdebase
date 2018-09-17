khelpcenter_en_data_DATA = long.html short.html wrapper.html nomatch.html syntax.html 
khelpcenter_en_datadir = $(kde_datadir)/khelpcenter/en

star_DATA = star.png star_blank.png
stardir = $(kde_datadir)/khelpcenter/pics

INCLUDES = $(all_includes)
METASOURCES = AUTO

lib_LTLIBRARIES = libhtmlsearch.la libkcm_htmlsearch.la

libhtmlsearch_la_SOURCES = htmlsearch.cpp progressdialog.cpp
libhtmlsearch_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
libhtmlsearch_la_LIBADD = $(LIB_TDEIO)

libkcm_htmlsearch_la_SOURCES = kcmhtmlsearch.cpp klangcombo.cpp ktagcombobox.cpp
libkcm_htmlsearch_la_LDFLAGS  = $(all_libraries) -module -avoid-version -no-undefined
libkcm_htmlsearch_la_LIBADD = $(LIB_TDEIO)

noinst_HEADERS = kcmhtmlsearch.h htmlsearch.h

xdg_apps_DATA = htmlsearch.desktop

bin_PROGRAMS = tdehtmlindex

wrapperdir = $(kde_datadir)/khelpcenter/
wrapper_SCRIPTS = meinproc_wrapper

tdehtmlindex_SOURCES = index.cpp
tdehtmlindex_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
tdehtmlindex_LDADD = libhtmlsearch.la $(LIB_TDEUI)

xpm_DATA = unchecked.xpm checked.xpm
xpmdir = $(kde_datadir)/khelpcenter/pics

messages:
	$(XGETTEXT) index.cpp $(libhtmlsearch_la_SOURCES) -o $(podir)/htmlsearch.pot
	$(XGETTEXT) $(libkcm_htmlsearch_la_SOURCES) -o $(podir)/kcmhtmlsearch.pot

