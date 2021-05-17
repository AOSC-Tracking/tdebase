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
  ${CMAKE_SOURCE_DIR}/tdeioslave/media/libmediacommon
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### tdeio_media_mounthelper (executable) ########

set( target tdeio_media_mounthelper )

set( ${target}_SRCS
  tdeio_media_mounthelper.cpp decryptdialog.ui dialog.cpp
)

tde_add_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK mediacommon-static tdeio-shared
  DESTINATION ${BIN_INSTALL_DIR}
)
