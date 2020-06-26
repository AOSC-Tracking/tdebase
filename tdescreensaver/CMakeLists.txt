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
  SOURCE KRandom.desktop KBlankscreen.desktop
  DESTINATION ${APPS_INSTALL_DIR}/System/ScreenSavers
  PO_DIR tdescreensaver-desktops
)


##### krandom (executable) ######################

tde_add_executable( krandom.kss AUTOMOC
  SOURCES random.cpp
  LINK tdeui-shared ${GL_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)


##### kblankscrn (executable) ###################

tde_add_executable( kblankscrn.kss AUTOMOC
  SOURCES blankscrn.cpp
  LINK tdescreensaver-shared tdeui-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
