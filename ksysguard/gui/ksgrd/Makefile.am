
# set the include path for X, qt and KDE
INCLUDES= -I$(srcdir)/../SensorDisplayLib $(all_includes)
 
lib_LTLIBRARIES = libksgrd.la

libksgrd_la_LDFLAGS = -no-undefined -version-info 3:0:2 $(all_libraries)
libksgrd_la_LIBADD = $(LIB_TDEUI)

# Which sources should be compiled for ksysguard.
libksgrd_la_SOURCES = \
	HostConnector.cpp \
	SensorAgent.cpp \
	SensorManager.cpp \
	SensorShellAgent.cpp \
	SensorSocketAgent.cpp \
	StyleEngine.cpp \
	StyleSettings.cpp \
	TimerSettings.cpp

ksgrdincludedir = $(includedir)/ksgrd
ksgrdinclude_HEADERS = \
	HostConnector.h \
	SensorAgent.h \
	SensorClient.h \
	SensorManager.h \
	SensorShellAgent.h \
	SensorSocketAgent.h \
	StyleEngine.h \
	StyleSettings.h \
	TimerSettings.h

# just to make sure, automake makes them 
libksgrd_la_METASOURCES = AUTO
