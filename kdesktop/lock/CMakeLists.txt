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
  ${CMAKE_BINARY_DIR}/kdesktop
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/kdmlib
  ${CMAKE_SOURCE_DIR}/kcheckpass
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### kdesktop_lock (executable) ################

set( target kdesktop_lock )

set( ${target}_SRCS
  lockprocess.cc lockdlg.cc infodlg.cc querydlg.cc sakdlg.cc
  securedlg.cc autologout.cc main.cc
)

tde_add_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK kdesktopsettings-static dmctl-static kio-shared Xext ${GL_LIBRARY} "-z\ now"
  DESTINATION ${BIN_INSTALL_DIR}
)
