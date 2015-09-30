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
  ${CMAKE_SOURCE_DIR}/tdm/kfrontend
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)

if( BUILD_TDM )


##### headers ###################################

install( FILES kgreeterplugin.h DESTINATION ${INCLUDE_INSTALL_DIR} )


##### kgreet_classic (module) ###################

tde_add_kpart( kgreet_classic AUTOMOC
  SOURCES kgreet_classic.cpp
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kgreet_pam (module) #######################

tde_add_kpart( kgreet_pam AUTOMOC
  SOURCES kgreet_pam.cpp
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)


##### kgreet_winbind (module) ###################

tde_add_kpart( kgreet_winbind AUTOMOC
  SOURCES kgreet_winbind.cpp
  LINK tdeui-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)



endif( BUILD_TDM )


##### dmctl (static) ############################

if( BUILD_KICKER OR BUILD_KDESKTOP OR BUILD_TDM OR 
    BUILD_KSMSERVER OR BUILD_TDEIOSLAVES )

  tde_add_library( dmctl STATIC_PIC
    SOURCES dmctl.cpp
    LINK Xau
  )

endif( )

##### tdmtsak (executable) #######################

if( BUILD_TSAK )
  tde_add_executable( tdmtsak
    SOURCES tdmtsak.cpp
    LINK ${TQT_LIBRARIES} tdecore-shared dmctl-static
    DESTINATION ${BIN_INSTALL_DIR}
    SETUID
    DESCRIPTION "Secure Attention Key interface for TDM"
    AUTHORS "Timothy Pearson"
  )
endif( BUILD_TSAK )
