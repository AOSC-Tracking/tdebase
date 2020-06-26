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
  SOURCE kcmview1394.desktop
  PO_DIR kcontrol-desktops
)
install( FILES oui.db DESTINATION ${DATA_INSTALL_DIR}/kcmview1394 )


##### kcm_view1394 (module) #####################

tde_add_kpart( kcm_view1394 AUTOMOC
  SOURCES view1394widget.ui view1394.cpp
  LINK tdeui-shared ${LIBRAW1394_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
