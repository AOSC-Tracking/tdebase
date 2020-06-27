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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE tdeshorturifilter.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)
install( FILES tdeshorturifilterrc DESTINATION ${CONFIG_INSTALL_DIR} )


##### libtdeshorturifilter (module) ###############

tde_add_kpart( libtdeshorturifilter AUTOMOC
  SOURCES tdeshorturifilter.cpp tdeshorturifilter.skel
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
