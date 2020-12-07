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
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE konq_iconview.desktop konq_multicolumnview.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR konqueror-desktops
)

install( FILES konq_iconview.rc konq_multicolumnview.rc DESTINATION ${DATA_INSTALL_DIR}/konqiconview )


##### konq_iconview (module) ####################

set( target konq_iconview )

tde_add_kpart( ${target} AUTOMOC
  SOURCES konq_iconview.cpp
  LINK konq-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
