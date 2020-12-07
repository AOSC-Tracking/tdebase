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
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES kfmclient_3_2.upd DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update )
install( PROGRAMS kfmclient_3_2_update.sh DESTINATION ${DATA_INSTALL_DIR}/tdeconf_update )


##### kfmclient (tdeinit) #######################

set( target kfmclient )

set( ${target}_SRCS
  kfmclient.cpp
  ${CMAKE_SOURCE_DIR}/konqueror/KonquerorIface.stub
  ${CMAKE_SOURCE_DIR}/kdesktop/KDesktopIface.stub
)

tde_add_tdeinit_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeio-shared
)
