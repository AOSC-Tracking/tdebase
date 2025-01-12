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
  ${TDE_INCLUDE_DIR}
  ${DBUS_TQT_INCLUDE_DIRS}
)

set( ScreenSaver_HDRS dbusbaseNode.h introspectableInterface.h screensaverInterface.h screensaverNode.h screensaverProxy.h)
set( ScreenSaver_SRCS dbusbaseNode.cpp introspectableInterface.cpp screensaverInterface.cpp screensaverNode.cpp screensaverProxy.cpp )

##### DbusScreenSaver #########################
add_custom_command( 
    OUTPUT  ${ScreenSaver_HDRS} ${ScreenSaver_SRCS}
    COMMAND ${DBUSXML2QT3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.ScreenSaver.xml 2>/dev/null
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} 
)

tde_add_library( dbusscreensaveriface STATIC_PIC AUTOMOC
  SOURCES ${ScreenSaver_SRCS}
  LINK ${DBUS_TQT_LIBRARIES} 
)
