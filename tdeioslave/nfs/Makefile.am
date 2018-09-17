## Makefile.am of tdebase/tdeioslave/man

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

####### Files

kde_module_LTLIBRARIES = tdeio_nfs.la

tdeio_nfs_la_SOURCES = tdeio_nfs.cpp mount_xdr.c nfs_prot_xdr.c
tdeio_nfs_la_LIBADD = -ltdeio $(LIBRPC)
tdeio_nfs_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = nfs_prot.h mount.h

kdelnk_DATA = nfs.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

$(srcdir)/mount_xdr.c: $(srcdir)/mount.x
	cd $(srcdir) && rpcgen ./mount.x


$(srcdir)/nfs_prot_xdr.c: $(srcdir)/nfs_prot.x
	cd $(srcdir) && rpcgen ./nfs_prot.x

messages:
	$(XGETTEXT) *.cpp -o $(podir)/tdeio_nfs.pot
