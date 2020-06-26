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

add_subdirectory( themeengine )
add_subdirectory( pics )
add_subdirectory( kcmksplash )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ksplashml/themeengine
  ${CMAKE_SOURCE_DIR}/ksplashml/themeengine/default
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE ksplash.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR ksplash-desktops
)


##### ksplash (executable) ######################

tde_add_executable( ksplash AUTOMOC
  SOURCES wndmain.cpp ksplashiface.skel main.cpp
  LINK themedefault-static tdeparts-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
