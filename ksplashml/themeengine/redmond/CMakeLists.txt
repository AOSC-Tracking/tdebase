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
  ${CMAKE_SOURCE_DIR}/ksplashml/themeengine
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES Theme.rc DESTINATION ${DATA_INSTALL_DIR}/ksplash/Themes/Redmond )
tde_create_translated_desktop(
  SOURCE ksplashredmond.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR ksplash-desktops
)


##### ksplashredmond (module) ###################

tde_add_kpart( ksplashredmond AUTOMOC
  SOURCES previewredmond.cpp themeredmond.cpp
  LINK ksplashthemes-shared tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
