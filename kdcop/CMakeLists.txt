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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES kdcopui.rc DESTINATION ${DATA_INSTALL_DIR}/kdcop )
install( FILES kdcop.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )


##### kdcop (executable) ########################

tde_add_executable( kdcop AUTOMOC
  SOURCES
    kdcop.cpp kdcopwindow.cpp kdcoplistview.cpp kdcopview.ui
  LINK tdeio-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
