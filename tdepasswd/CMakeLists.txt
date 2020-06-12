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

add_subdirectory( kcm )

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

install( FILES tdepasswd.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )


##### tdepasswd (executable) ####################

tde_add_executable( tdepasswd AUTOMOC
  SOURCES tdepasswd.cpp passwd.cpp passwddlg.cpp
  LINK tdeio-shared tdesu-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
