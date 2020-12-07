INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

kde_module_LTLIBRARIES = tdeio_pop3.la

tdeio_pop3_la_SOURCES = pop3.cpp
tdeio_pop3_la_LIBADD  = $(LIB_TDEIO) $(SASL2_LIBS)
tdeio_pop3_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = pop3.h

kdelnk_DATA = pop3.protocol pop3s.protocol
kdelnkdir = $(kde_servicesdir)

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_pop3.pot
