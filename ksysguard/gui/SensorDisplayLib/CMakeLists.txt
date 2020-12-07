#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ksysguard/gui/ksgrd
  ${CMAKE_SOURCE_DIR}/ksysguard/gui
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### sensordisplays (static) ###################

tde_add_library( sensordisplays STATIC_PIC AUTOMOC
  SOURCES
    SensorDisplay.cpp BarGraph.cpp DancingBars.cpp DancingBarsSettings.cpp
    DummyDisplay.cpp FancyPlotter.cpp FancyPlotterSettings.cpp
    ListView.cpp LogFile.cpp MultiMeter.cpp MultiMeterSettings.cpp
    ProcessController.cpp ProcessList.cpp ReniceDlg.cpp
    SensorLogger.cpp SensorLoggerDlg.cpp SensorLoggerSettings.cpp
    ListViewSettings.cpp SignalPlotter.cpp ListViewSettingsWidget.ui
    LogFileSettings.ui MultiMeterSettingsWidget.ui SensorLoggerDlgWidget.ui
    SensorLoggerSettingsWidget.ui
  LINK ksgrd-shared tdeio-shared
)
