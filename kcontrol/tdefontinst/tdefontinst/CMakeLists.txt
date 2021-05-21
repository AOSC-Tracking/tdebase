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

add_definitions( -DOS_${CMAKE_SYSTEM_NAME} )

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/kcontrol/tdefontinst/lib
  ${CMAKE_SOURCE_DIR}/kcontrol/fonts/
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### tdefontinst (executable) ####################

tde_add_executable( tdefontinst
  SOURCES
    Main.cpp FontEngine.cpp Fontmap.cpp GetPid.c
    XConfig.cpp
  LINK kxftconfig-static tdefontinst-shared tdeio-shared ${FREETYPE_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)
