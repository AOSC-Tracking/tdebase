
# set the include path for X, qt and KDE
INCLUDES= -I$(srcdir)/.. -I$(srcdir)/../ksgrd -I../ksgrd $(all_includes)
 
noinst_LTLIBRARIES = libsensordisplays.la

libsensordisplays_la_LDFLAGS = $(all_libraries) -no-undefined
libsensordisplays_la_LIBADD = ../ksgrd/libksgrd.la $(LIB_TDEIO)

# Which sources should be compiled for the sensor display lib.
libsensordisplays_la_SOURCES = \
	SensorDisplay.cpp \
	BarGraph.cpp \
	DancingBars.cpp \
	DancingBarsSettings.cpp \
	DummyDisplay.cpp \
	FancyPlotter.cpp \
	FancyPlotterSettings.cpp \
	ListView.cpp \
	LogFile.cpp \
	MultiMeter.cpp \
	MultiMeterSettings.cpp \
	ProcessController.cpp \
	ProcessList.cpp \
	ReniceDlg.cpp \
	SensorLogger.cpp \
	SensorLoggerDlg.cpp \
	SensorLoggerSettings.cpp \
	ListViewSettings.cpp \
	SignalPlotter.cpp \
	ListViewSettingsWidget.ui \
	LogFileSettings.ui \
	MultiMeterSettingsWidget.ui \
	SensorLoggerDlgWidget.ui \
	SensorLoggerSettingsWidget.ui

# just to make sure, automake makes them 
METASOURCES = AUTO

