bin_PROGRAMS = tdeio_media_mounthelper

INCLUDES = -I$(srcdir)/../libmediacommon $(all_includes)
AM_LDFLAGS = $(all_libraries)

tdeio_media_mounthelper_SOURCES = tdeio_media_mounthelper.cpp unlockdialog.ui dialog.cpp

tdeio_media_mounthelper_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor
tdeio_media_mounthelper_LDADD = ../libmediacommon/libmediacommon.la $(LIB_TDEIO) $(LIB_TDEUI)

METASOURCES = AUTO

