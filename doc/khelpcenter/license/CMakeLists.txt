#################################################
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

install( FILES tde_license_info.desktop DESTINATION ${AUTOSTART_INSTALL_DIR} )


##### tde_license_info (executable) #########################

tde_add_executable( tde_license_info AUTOMOC
  SOURCES mainWindow.cpp TDELicenseDlg.cpp
  LINK tdeui-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
