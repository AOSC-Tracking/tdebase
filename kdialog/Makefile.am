KDE_CXXFLAGS = -DQT_NO_CAST_ASCII -DQT_NO_ASCII_CAST
INCLUDES = $(all_includes)

bin_PROGRAMS = kdialog

kdialog_SOURCES = kdialog.cpp widgets.cpp tdelistboxdialog.cpp progressdialog.cpp progressdialogiface.skel
kdialog_LDADD = $(LIB_TDEIO)
kdialog_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIB_TQT) -lDCOP $(LIB_TDECORE) $(LIB_TDEUI) -ltdefx $(LIB_TDEIO) -ltdetexteditor

METASOURCES = AUTO

messages:        
	$(XGETTEXT) *.cpp -o $(podir)/kdialog.pot 
