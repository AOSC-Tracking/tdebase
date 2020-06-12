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


##### other data ################################

install( FILES sftp.protocol DESTINATION ${SERVICES_INSTALL_DIR} )


##### tdeio_sftp (module) #########################

set( target tdeio_sftp )

tde_add_kpart( ${target} AUTOMOC
  SOURCES process.cpp atomicio.cpp tdeio_sftp.cpp sftpfileattr.cpp ksshprocess.cpp
  LINK tdeio-shared tdesu-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
