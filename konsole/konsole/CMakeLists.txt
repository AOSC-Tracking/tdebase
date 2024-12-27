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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

##### other data ################################

tde_create_translated_desktop(
  SOURCE x-konsole.desktop
  DESTINATION ${MIME_INSTALL_DIR}/application
  PO_DIR konsole-desktops
)


##### libkonsolepart (module) ###################

set( target libkonsolepart )

set( ${target}_SRCS
  TEPty.cpp BlockArray.cpp konsole_part.cpp schema.cpp
  session.cpp zmodem_dialog.cpp TEWidget.cpp TEmuVt102.cpp
  TEScreen.cpp konsole_wcwidth.cpp TEmulation.cpp
  TEHistory.cpp keytrans.cpp konsoleiface.skel sessioniface.skel
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeparts-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kded_kwrited (module) #####################

set( target kded_kwrited )

set( ${target}_SRCS
  kwrited.cpp TEPty.cpp kwrited.skel
)

tde_add_kpart( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeinit_kded-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### konsole (tdeinit) #########################

set( target konsole )

set( ${target}_SRCS
  TEPty.cpp BlockArray.cpp main.cpp konsole.cpp
  schema.cpp session.cpp TEWidget.cpp TEmuVt102.cpp
  TEScreen.cpp TEmulation.cpp TEHistory.cpp keytrans.cpp
  konsoleiface.skel sessioniface.skel konsole_wcwidth.cpp
  konsolebookmarkhandler.cpp konsolebookmarkmenu.cpp
  zmodem_dialog.cpp printsettings.cpp
)

tde_add_tdeinit_executable( ${target} AUTOMOC
  SOURCES ${${target}_SRCS}
  LINK tdeprint-shared tdeio-shared ${XRENDER_LIBRARIES} ${CMAKE_DL_LIBS}
)
