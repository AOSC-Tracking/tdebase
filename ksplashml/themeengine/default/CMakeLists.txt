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
  SOURCE ksplashdefault.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR ksplash-desktops
)


##### ksplashdefault (module) ###################

tde_add_kpart( ksplashdefault AUTOMOC
  SOURCES main.cpp
  LINK themedefault-static ksplashthemes-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### themedefault (static) #####################

tde_add_library( themedefault STATIC_PIC AUTOMOC
  SOURCES themelegacy.cpp
  LINK ksplashthemes-shared
)
