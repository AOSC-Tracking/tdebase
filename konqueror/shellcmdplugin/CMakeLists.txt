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
  ${CMAKE_SOURCE_DIR}/libkonq
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TDE_LIBRARY_DIRS}
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqiconview/kpartplugins )

install( FILES kshellcmdplugin.rc kshellcmdplugin.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqlistview/kpartplugins )


##### konq_shellcmdplugin (module) ##############

set( target konq_shellcmdplugin )

set( ${target}_SRCS
  kshellcmdexecutor.cpp kshellcmddialog.cpp
  kshellcmdplugin.cpp
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK konq-shared tdesu-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
