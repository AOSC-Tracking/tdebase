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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE localdomainurifilter.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR kcontrol-desktops
)


##### liblocaldomainurifilter (module) ##########

tde_add_kpart( liblocaldomainurifilter AUTOMOC
  SOURCES localdomainurifilter.cpp localdomainurifilter.skel
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### klocaldomainurifilterhelper (executable ) #

tde_add_executable( klocaldomainurifilterhelper
  SOURCES klocaldomainurifilterhelper.c
  LINK
  DESTINATION ${BIN_INSTALL_DIR}
)
