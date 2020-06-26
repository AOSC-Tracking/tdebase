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

add_definitions( -D_TDEPRINT_COMPILE )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES tdeprint_part.rc DESTINATION ${DATA_INSTALL_DIR}/tdeprint_part )

tde_create_translated_desktop(
  SOURCE tdeprint_part.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeprint-desktops
)


##### libtdeprint_part (module) #################

tde_add_kpart( libtdeprint_part AUTOMOC
  SOURCES printpart.cpp
  LINK tdeprint_management-shared tdeparts-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
