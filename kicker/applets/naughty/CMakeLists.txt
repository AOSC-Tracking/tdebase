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

# FIXME seems that on freebsd is needed smth named LIB_KVM

if ( ${CMAKE_SYSTEM_NAME} MATCHES "NetBSD" )
  set( LIB_KVM "-lkvm")
else()
  set( LIB_KVM "")
endif()

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/kicker/libkicker
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE naughtyapplet.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kicker/applets
  PO_DIR kicker-desktops
)
install( FILES naughty-happy.png naughty-sad.png DESTINATION ${DATA_INSTALL_DIR}/naughtyapplet/pics )


##### naughty_panelapplet (module) ##############

tde_add_kpart( naughty_panelapplet AUTOMOC
  SOURCES
    NaughtyProcessMonitor.cpp NaughtyConfigDialog.cpp
    NaughtyApplet.cpp
  LINK kickermain-shared ${LIB_KVM}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
