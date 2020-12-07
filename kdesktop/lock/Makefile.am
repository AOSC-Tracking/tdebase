## Makefile.am of tdebase/kdesktop/lock

INCLUDES = -I.. -I$(top_srcdir)/kcheckpass -I$(top_srcdir)/tdmlib $(GLINC) $(all_includes)
kdesktop_lock_LDFLAGS  = $(all_libraries) $(KDE_RPATH) $(LIB_QT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
kdesktop_lock_LDADD    = ../libkdesktopsettings.la ../../tdmlib/libdmctl.la $(LIB_TDEIO) $(LIB_XF86MISC) $(GLLIB)

####### Files

bin_PROGRAMS = kdesktop_lock

kdesktop_lock_SOURCES = lockprocess.cpp lockdlg.cpp infodlg.cpp querydlg.cpp autologout.cpp main.cpp

noinst_HEADERS = lockprocess.h lockdlg.h infodlg.h querydlg.h autologout.h main.h

METASOURCES = AUTO

lockprocess.o: ../kdesktopsettings.h

####### Build rules

PAM = @TDESCREENSAVER_PAM_SERVICE@

install-data-local:
	-@test -n "$(DESTDIR)" || test -z "$(PAM)" || $(top_srcdir)/mkpamserv $(PAM)
