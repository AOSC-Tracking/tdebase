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


##### kateinterfaces (shared lib) ###############

set( target kateinterfaces )

set( ${target}_SRCS
  kateapp.cpp kateconfigdialog.cpp kateconfigplugindialogpage.cpp
  kateconsole.cpp katedocmanager.cpp katefilelist.cpp
  katefileselector.cpp katemainwindow.cpp katepluginmanager.cpp
  kateviewmanager.cpp kateviewspace.cpp katemainwindowiface.skel
  katemainwindowiface.cpp kategrepdialog.cpp katemailfilesdialog.cpp
  kbookmarkhandler.cpp katedocmanageriface.skel kateappIface.cpp
  kateappIface.skel katedocmanageriface.cpp kateexternaltools.cpp
  katesavemodifieddialog.cpp kateviewspacecontainer.cpp
  katemwmodonhddialog.cpp katesession.cpp katesessionpanel.cpp
  katemdi.cpp katetabwidget.cpp
)

tde_add_library( ${target} SHARED AUTOMOC
  SOURCES ${${target}_SRCS}
  VERSION 0.0.0
  EMBED kateinterfacesprivate-static
  LINK tdeutils-shared
  DESTINATION ${LIB_INSTALL_DIR}
)


##### kate (tdeinit) ############################

set( target kate )

tde_add_tdeinit_executable( ${target}
  SOURCES katemain.cpp
  LINK kateinterfaces-shared
)


##### kwrite (tdeinit) ##########################

set( target kwrite )

tde_add_tdeinit_executable( ${target} AUTOMOC
  SOURCES kwritemain.cpp
  LINK kateinterfaces-shared katepartinterfaces-shared
)
