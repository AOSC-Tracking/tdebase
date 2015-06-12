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

# FIXME this is far from complete!!!

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${DBUS_INCLUDE_DIRS}
)

link_directories(
  ${DBUS_LIBRARY_DIRS}
)

##### tdm (executable) ##########################

add_custom_command( OUTPUT config.ci
  COMMAND perl -w ${CMAKE_SOURCE_DIR}/tdm/confproc.pl ${CMAKE_SOURCE_DIR}/tdm/config.def config.ci
  DEPENDS ${CMAKE_SOURCE_DIR}/tdm/confproc.pl ${CMAKE_SOURCE_DIR}/tdm/config.def )

set_property( SOURCE auth.c APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config.ci )

if( WITH_XDMCP )
  set( XDMCP_LIBRARIES "Xdmcp" )
else()
  set( XDMCP_LIBRARIES "" )
endif()

tde_add_executable( tdm
  SOURCES
    access.c auth.c bootman.c choose.c client.c consolekit.c
    ctrl.c daemon.c dm.c dpylist.c error.c genauth.c getfd.c
    inifile.c krb5auth.c mitauth.c netaddr.c policy.c
    process.c protodpy.c reset.c resource.c rpcauth.c
    server.c session.c sessreg.c socket.c streams.c
    util.c xdmauth.c xdmcp.c
  LINK X11 ${XAU_LIBRARIES} ${DBUS_LIBRARIES} ${CRYPT_LIBRARY} ${PAM_LIBRARY} ${UTIL_LIBRARY} ${XDMCP_LIBRARIES}
  DESTINATION ${BIN_INSTALL_DIR}
)
