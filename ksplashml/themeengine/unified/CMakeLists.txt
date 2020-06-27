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
  #${CMAKE_CURRENT_SOURCE_DIR}
  #${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/ksplashml/themeengine
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE ksplashunified.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR ksplash-desktops
)


##### ksplashunified (module) ###################

tde_add_kpart( ksplashunified AUTOMOC
  SOURCES main.cpp
  LINK themeunified-static ksplashthemes-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### themeunified (static) #####################

tde_add_library( themeunified STATIC_PIC AUTOMOC
  SOURCES themeunified.cpp
  LINK ksplashthemes-shared
)
