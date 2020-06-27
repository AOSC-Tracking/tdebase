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

# FIXME not built: man2html, tdeio_man_test


##### system checks #############################

check_include_file( "unistd.h" HAVE_UNISTD_H )
check_include_file( "string.h" HAVE_STRING_H )


##### compiler/linker settings ##################

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

tde_create_translated_desktop(
  SOURCE man.protocol kmanpart.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)

install( FILES tdeio_man.css DESTINATION ${DATA_INSTALL_DIR}/tdeio_man )


##### tdeio_man (module) ##########################

set( target tdeio_man )

tde_add_kpart( ${target} AUTOMOC
  SOURCES man2html.cpp tdeio_man.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### libkmanpart (module) ######################

set( target libkmanpart )

tde_add_kpart( ${target} AUTOMOC
  SOURCES kmanpart.cpp
  LINK tdehtml-shared tdeparts-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
