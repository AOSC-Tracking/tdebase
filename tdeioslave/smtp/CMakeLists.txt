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

tde_create_translated_desktop(
  SOURCE smtp.protocol smtps.protocol
  DESTINATION ${SERVICES_INSTALL_DIR}
  PO_DIR tdeioslave-desktops
)


##### tdeio_smtp (module) #########################

set( target tdeio_smtp )

tde_add_kpart( ${target} AUTOMOC
  SOURCES smtp.cpp request.cpp response.cpp capabilities.cpp command.cpp transactionstate.cpp
  LINK tdeio-shared ${SASL_LIBRARIES}
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
