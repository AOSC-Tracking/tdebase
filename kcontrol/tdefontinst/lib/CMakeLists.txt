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
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  ${FONTCONFIG_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### tdefontinst (shared) ########################

tde_add_library( tdefontinst SHARED
  SOURCES Misc.cpp FcEngine.cpp
  VERSION 0.0.0
  LINK tdeio-shared ${FONTCONFIG_LIBRARIES} ${XFT_LIBRARIES}
  DESTINATION ${LIB_INSTALL_DIR}
)
