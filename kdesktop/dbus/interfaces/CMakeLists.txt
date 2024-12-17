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
  ${TQT_INCLUDE_DIRS}
  ${DBUS_TQT_INCLUDE_DIRS}
)

set( INTROSPECTIONPATH ${CMAKE_SOURCE_DIR}/kdesktop/dbus/screensaver )

set( ScreenSaver_HDRS dbusbaseNode.h introspectableInterface.h screensaverInterface.h screensaverNode.h screensaverProxy.h)
set( ScreenSaver_SRCS dbusbaseNode.cpp introspectableInterface.cpp screensaverInterface.cpp screensaverNode.cpp screensaverProxy.cpp )

##### ScreenSaver #########################
add_custom_command( 
    OUTPUT  ${ScreenSaver_HDRS} ${ScreenSaver_SRCS}
    COMMAND ${DBUSXML2QT3_EXECUTABLE} ${INTROSPECTIONPATH}/org.freedesktop.ScreenSaver.xml 2>/dev/null
    DEPENDS ${INTROSPECTIONPATH}/org.freedesktop.ScreenSaver.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
)

tde_add_library( screensaverinterfaces STATIC_PIC AUTOMOC
  SOURCES ${ScreenSaver_SRCS}
  LINK ${DBUS_TQT_LIBRARIES} 
)

##### install headers ###################################

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DESTINATION ${INCLUDE_INSTALL_DIR}
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING PATTERN PATTERN "*.h"
    PATTERN "CMakeFiles" EXCLUDE
)
