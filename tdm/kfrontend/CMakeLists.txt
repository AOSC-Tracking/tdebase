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

add_subdirectory( themer )
add_subdirectory( themes )
add_subdirectory( pics )
add_subdirectory( sessions )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/tdm/backend
  ${CMAKE_SOURCE_DIR}/tdmlib
  ${CMAKE_SOURCE_DIR}/kcontrol/background
  ${TDE_INCLUDE_DIR}
  ${TDE_INCLUDE_DIR}/tde
  ${TQT_INCLUDE_DIRS}
  ${KRB5_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
  ${LIBART_LIBRARY_DIRS}
  ${TDE_LIB_DIR}
)


##### other data ################################

if( NOT DEFINED GENTDMCONF_FLAGS )
  set( GENTDMCONF_FLAGS "--no-old" )
endif( )

install( CODE "execute_process( COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gentdmconf --in \$ENV{DESTDIR}${CONFIG_INSTALL_DIR}/tdm --no-in-notice --face-src ${CMAKE_CURRENT_SOURCE_DIR}/pics ${GENTDMCONF_FLAGS} )" )


##### config.ci (generated) #####################

add_custom_command( OUTPUT config.ci
  COMMAND perl -w ${CMAKE_SOURCE_DIR}/tdm/confproc.pl ${CMAKE_SOURCE_DIR}/tdm/config.def config.ci
  DEPENDS ${CMAKE_SOURCE_DIR}/tdm/confproc.pl ${CMAKE_SOURCE_DIR}/tdm/config.def )


##### tdm_config (executable) ###################

set_property( SOURCE tdm_config.c tdm_greet.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.ci )

tde_add_executable( tdm_config
  SOURCES tdm_config.c
  LINK
  DESTINATION ${BIN_INSTALL_DIR}
)


##### tdm_greet (executable) ####################
if( WITH_XRANDR )
  set( TDMGREET_OPTIONAL_LINK "tderandr-shared" )
endif ( )

tde_add_executable( tdm_greet AUTOMOC
  SOURCES
    tdm_greet.c tdmconfig.cpp tdmclock.cpp kconsole.cpp
    kfdialog.cpp kgdialog.cpp kchooser.cpp kgverify.cpp
    tdmshutdown.cpp tdmadmindialog.cpp kgreeter.cpp
    kgapp.cpp sakdlg.cpp
  LINK tdmthemer-static tdeui-shared tdeio-shared dmctl-static Xtst ${LIBTDELDAP_LIBRARIES} ${TDMGREET_OPTIONAL_LINK}
  DESTINATION ${BIN_INSTALL_DIR}
)


##### krootimage (executable) ###################

tde_add_executable( krootimage AUTOMOC
  SOURCES krootimage.cpp
  LINK bgnd-static tdeio-shared
  DESTINATION ${BIN_INSTALL_DIR}
)


##### gentdmconf (executable) ###################

set_property( SOURCE gentdmconf.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.ci )

tde_add_executable( gentdmconf AUTOMOC
  SOURCES gentdmconf.c
  LINK X11
  DESTINATION ${BIN_INSTALL_DIR}
)


##### tdmctl (executable) #######################

tde_add_executable( tdmctl
  SOURCES tdmctl.c
  LINK
  DESTINATION ${BIN_INSTALL_DIR}
)
