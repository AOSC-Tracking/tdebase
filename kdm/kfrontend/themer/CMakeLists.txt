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
  ${CMAKE_SOURCE_DIR}/kdm/kfrontend
  ${CMAKE_SOURCE_DIR}/kdmlib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
  ${LIBART_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


# FIXME this must be optimized
##### config.ci (generated) #####################

add_custom_command( OUTPUT config.ci
  COMMAND perl -w ${CMAKE_SOURCE_DIR}/kdm/confproc.pl ${CMAKE_SOURCE_DIR}/kdm/config.def config.ci
  DEPENDS ${CMAKE_SOURCE_DIR}/kdm/confproc.pl ${CMAKE_SOURCE_DIR}/kdm/config.def )
set_property( SOURCE kdmthemer.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.ci )


##### kdmthemer (static) ########################

tde_add_library( kdmthemer STATIC_PIC AUTOMOC
  SOURCES
    kdmthemer.cpp kdmitem.cpp kdmpixmap.cpp
    kdmrect.cpp kdmlabel.cpp kdmlayout.cpp
  LINK ${LIBART_LIBRARIES}
)
