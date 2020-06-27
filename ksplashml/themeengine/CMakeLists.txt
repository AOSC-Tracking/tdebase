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

add_subdirectory( default )
add_subdirectory( unified )
add_subdirectory( standard )
add_subdirectory( redmond )
add_subdirectory( simple )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES themeengine.h objkstheme.h DESTINATION ${INCLUDE_INSTALL_DIR}/ksplash )
tde_create_translated_desktop(
  SOURCE ksplashplugins.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR ksplash-desktops
)


##### ksplashthemes (shared) ####################

tde_add_library( ksplashthemes SHARED AUTOMOC
  SOURCES themeengine.cpp objkstheme.cpp
  VERSION 0.0.0
  LINK tdeui-shared
  DESTINATION ${LIB_INSTALL_DIR}
)
