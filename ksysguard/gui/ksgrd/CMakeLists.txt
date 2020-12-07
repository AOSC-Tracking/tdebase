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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### headers ###################################

install( FILES
    HostConnector.h SensorAgent.h SensorClient.h
    SensorManager.h SensorShellAgent.h SensorSocketAgent.h
    StyleEngine.h StyleSettings.h TimerSettings.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/ksgrd )


##### ksgrd (shared) ############################

tde_add_library( ksgrd SHARED AUTOMOC
  SOURCES
    HostConnector.cpp SensorAgent.cpp SensorManager.cpp
    SensorShellAgent.cpp SensorSocketAgent.cpp StyleEngine.cpp
    StyleSettings.cpp TimerSettings.cpp
  VERSION 1.2.0
  LINK tdeui-shared
  DESTINATION ${LIB_INSTALL_DIR}
)
