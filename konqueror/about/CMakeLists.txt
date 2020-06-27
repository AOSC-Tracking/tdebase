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
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE konq_aboutpage.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE konqaboutpage.desktop
  DESTINATION ${SERVICETYPES_INSTALL_DIR}
  PO_DIR konqueror-desktops
)

install( FILES
    box-centre-konqueror.png top-right-konqueror.png
    lightning.png launch.html intro.html specs.html
    tips.html plugins.html plugins_rtl.html konq.css
  DESTINATION ${DATA_INSTALL_DIR}/konqueror/about )


##### konq_aboutpage (module) ###################

set( target konq_aboutpage )

tde_add_kpart( ${target} AUTOMOC
  SOURCES konq_aboutpage.cc
  LINK tdehtml-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
