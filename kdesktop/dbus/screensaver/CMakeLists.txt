#################################################
#
#  (C) 2024 Emanoil Kotsev
#  deloptes (AT) gmail.com
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/kdesktop
  ${TQT_INCLUDE_DIRS}
  ${TDE_INCLUDE_DIR}
  ${DBUS_1_TQT_INCLUDE_DIRS}
)


##### headers ###################################

install( FILES dbusscreensaverservice.h screensaverInterfaceImpl.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/kdesktop/dbus 
)

##### dbusscreensaverservice ####################

set( target dbusscreensaverservice )

set( ${target}_SRCS
  dbusscreensaverservice.cpp screensaverInterfaceImpl.cpp
)

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK ${DBUS_TQT_LIBRARIES} 
)
