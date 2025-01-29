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
  ${DBUS_TQT_INCLUDE_DIRS}
)

set( ScreenSaver_HDRS dbusbaseNode.h introspectableInterface.h screensaverInterface.h screensaverNode.h screensaverProxy.h)
set( ScreenSaver_SRCS dbusbaseNode.cpp introspectableInterface.cpp screensaverInterface.cpp screensaverNode.cpp screensaverProxy.cpp )

##### DbusScreenSaver #########################
add_custom_command( 
    OUTPUT  ${ScreenSaver_HDRS} ${ScreenSaver_SRCS}
    COMMAND ${DBUSXML2QT3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/kdesktop/dbus/interfaces/org.freedesktop.ScreenSaver.xml 2>/dev/null
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/kdesktop/dbus/screensaver
)

##### dbusscreensaverservice ####################

set( target dbusscreensaverservice )

set( ${target}_SRCS
  dbusscreensaverservice.cpp screensaverInterfaceImpl.cpp 
)

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES ${${target}_SRCS} ${ScreenSaver_SRCS}
  LINK ${DBUS_TQT_LIBRARIES} 
)

##### headers ###################################

install( FILES dbusscreensaverservice.h screensaverInterfaceImpl.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/kdesktop/dbus 
)
