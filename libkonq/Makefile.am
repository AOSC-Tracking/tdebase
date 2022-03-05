#    This file is part of the KDE libraries
#    Copyright (C) 1997 David Faure <faure@kde.org>
#
#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Library General Public
#    License as published by the Free Software Foundation; either
#    version 2 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Library General Public License for more details.
#
#    You should have received a copy of the GNU Library General Public License
#    along with this library; see the file COPYING.LIB.  If not, write to
#    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
#    Boston, MA 02110-1301, USA.

INCLUDES = -I$(kde_includes)/arts $(all_includes)
SUBDIRS = pics favicons

lib_LTLIBRARIES = libkonq.la
libkonq_la_LDFLAGS = $(all_libraries) -version-info 6:0:2 -no-undefined
libkonq_la_LIBADD = $(LIB_TDEPARTS)

libkonq_la_SOURCES = konq_popupmenu.cpp knewmenu.cpp \
   konq_xmlguiclient.cpp\
   tdefileivi.cpp konq_iconviewwidget.cpp konq_settings.cpp konq_drag.cpp  \
   konq_operations.cpp \
   konq_dirpart.cpp konq_propsview.cpp konq_events.cpp konq_bgnddlg.cpp \
   konq_undo.cpp konq_undo.skel \
   konq_historymgr.cpp konq_historycomm.cpp konq_historycomm.skel \
   konq_pixmapprovider.cpp \
   kivdirectoryoverlay.cpp \
   kivfreespaceoverlay.cpp \
   konq_faviconmgr.cpp konq_faviconmgr.skel konq_filetip.cpp

directory_DATA = directory_bookmarkbar.desktop 
directorydir   = $(kde_datadir)/kbookmark

servicetype_DATA = konqpopupmenuplugin.desktop
servicetypedir = $(kde_servicetypesdir)

METASOURCES = AUTO

include_HEADERS = konq_popupmenu.h knewmenu.h \
    tdefileivi.h konq_drag.h konq_iconviewwidget.h \
    konq_defaults.h konq_settings.h \
    konq_operations.h libkonq_export.h \
    konq_dirpart.h konq_propsview.h konq_events.h \
    konq_undo.h konq_historymgr.h konq_historycomm.h \
    konq_pixmapprovider.h \
    kivdirectoryoverlay.h \
    kivfreespaceoverlay.h \
    konq_faviconmgr.h konq_xmlguiclient.h konqbookmarkmanager.h konq_filetip.h \
    konq_sort_constants.h konq_string_compare.h


if include_ARTS
ARTS_MODULE = konq_sound.la
endif

kde_module_LTLIBRARIES = $(ARTS_MODULE)
konq_sound_la_SOURCES = konq_sound.cpp
konq_sound_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
konq_sound_la_LIBADD = -lsoundserver_idl -lartskde

noinst_HEADERS = konq_sound.h

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/libkonq.pot

DOXYGEN_REFERENCES = dcop tdecore tdeio/bookmarks
include ../admin/Doxyfile.am

