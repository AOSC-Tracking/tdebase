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

add_subdirectory( kdedmodule )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### other data ################################

tde_create_translated_desktop(
  SOURCE remote.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdeioremote (static) ########################

set( target tdeioremote )

tde_add_library( ${target} STATIC_PIC AUTOMOC
  SOURCES tdeio_remote.cpp remoteimpl.cpp
)


##### tdeio_remote (module) #######################

set( target tdeio_remote )

configure_file( ${TDE_CMAKE_TEMPLATES}/tde_dummy_cpp.cmake dummy.cpp COPYONLY )

tde_add_kpart( ${target}
  SOURCES dummy.cpp
  EMBED tdeioremote-static
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
