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

add_subdirectory( pics )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ksplashml/themeengine
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES Theme.rc DESTINATION ${DATA_INSTALL_DIR}/ksplash/Themes/Standard )
tde_create_translated_desktop(
  SOURCE ksplashstandard.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR ksplash-desktops
)


##### ksplashstandard (module) ##################

tde_add_kpart( ksplashstandard AUTOMOC
  SOURCES themestandardlib.cpp
  LINK themestandard-static
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### themestandard (static) ####################

tde_add_library( themestandard STATIC_PIC AUTOMOC
  SOURCES themestandard.cpp wndicon.cpp wndstatus.cpp
  LINK ksplashthemes-shared
)
