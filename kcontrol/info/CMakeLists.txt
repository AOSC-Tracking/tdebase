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

if( WITH_OPENGL )
  add_definitions( -DINFO_OPENGL_AVAILABLE )
  set( OPENGL_DESKTOP opengl.desktop )
  set( OPENGL_SOURCES opengl.cpp )
endif( WITH_OPENGL )

##### other data ################################

tde_create_translated_desktop(
  SOURCE
    memory.desktop processor.desktop dma.desktop
    interrupts.desktop ioports.desktop pci.desktop
    sound.desktop devices.desktop scsi.desktop
    partitions.desktop xserver.desktop cdinfo.desktop
    ${OPENGL_DESKTOP}
  PO_DIR kcontrol-desktops
)


##### kcm_info (module) #########################

tde_add_kpart( kcm_info AUTOMOC
  SOURCES main.cpp memory.cpp ${OPENGL_SOURCES}
  LINK tdeui-shared ${GL_LIBRARIES} ${GLU_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
