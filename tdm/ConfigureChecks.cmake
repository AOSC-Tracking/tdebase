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

find_library( UTIL_LIBRARY util )
if( ${CMAKE_SYSTEM_NAME} MATCHES "SunOS" )
  set( UTIL_LIBRARY "" )
endif()

check_function_exists( getdomainname HAVE_GETDOMAINNAME )
check_function_exists( initgroups HAVE_INITGROUPS )
check_function_exists( mkstemp HAVE_MKSTEMP )
check_function_exists( setproctitle HAVE_SETPROCTITLE )
check_function_exists( pthread_setname_np HAVE_PTHREAD_SETNAME_NP )
check_function_exists( sysinfo HAVE_SYSINFO )
check_function_exists( strnlen HAVE_STRNLEN )
check_function_exists( getifaddrs HAVE_GETIFADDRS )

tde_save( CMAKE_REQUIRED_LIBRARIES )
set( CMAKE_REQUIRED_LIBRARIES ${UTIL_LIBRARY} )
check_function_exists( setusercontext HAVE_SETUSERCONTEXT )
check_function_exists( getusershell HAVE_GETUSERSHELL )
check_function_exists( login_getclass HAVE_LOGIN_GETCLASS )
check_function_exists( auth_timeok HAVE_AUTH_TIMEOK )
tde_restore( CMAKE_REQUIRED_LIBRARIES )

check_include_file( lastlog.h HAVE_LASTLOG_H )
check_include_file( termio.h HAVE_TERMIO_H )

check_struct_has_member( "struct sockaddr_in" "sin_len" "sys/socket.h;netinet/in.h" HAVE_STRUCT_SOCKADDR_IN_SIN_LEN )
check_struct_has_member( "struct passwd" "pw_expire" "pwd.h" HAVE_STRUCT_PASSWD_PW_EXPIRE )
check_struct_has_member( "struct utmp" "ut_user" "utmp.h" HAVE_STRUCT_UTMP_UT_USER )

check_c_source_runs( "
  #include <errno.h>
  #include <unistd.h>
  int main()
  {
      setlogin(0);
      return errno == ENOSYS;
  }
" HAVE_SETLOGIN )

check_c_source_runs( "
  #include <sys/socket.h>
  #include <sys/un.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <string.h>
  #include <unistd.h>
  #include <errno.h>
  int main()
  {
    int fd, fd2;
    struct sockaddr_un sa;

    if((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
      return 2;
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, \"testsock\");
    unlink(sa.sun_path);
    if(bind(fd, (struct sockaddr *)&sa, sizeof(sa)))
      return 2;
    chmod(sa.sun_path, 0);
    setuid(getuid() + 1000);
    if((fd2 = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
      return 2;
    connect(fd2, (struct sockaddr *)&sa, sizeof(sa));
    return errno != EACCES;
  }
" HONORS_SOCKET_PERMS )

if( CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME MATCHES Darwin OR CMAKE_SYSTEM_NAME MATCHES GNU/FreeBSD )
    unset( HAVE_UTMPX )
    unset( HAVE_LASTLOGX )
else( )
    check_function_exists( getutxent HAVE_UTMPX )
    check_function_exists( updlastlogx HAVE_LASTLOGX )
endif( )

unset( BSD_UTMP )
if( NOT HAVE_UTMPX )
    check_function_exists( getutent have_getutent )
    if( NOT have_getutent )
        set( BSD_UTMP 1 CACHE INTERNAL "" FORCE )
    endif( )
endif( )

check_function_exists( arc4random HAVE_ARC4RANDOM )
if( NOT HAVE_ARC4RANDOM )
    # assume that /dev/random is non-blocking if /dev/urandom does not exist
    if( EXISTS /dev/urandom )
      set( DEV_RANDOM "/dev/urandom" CACHE INTERNAL "" FORCE )
    elseif( EXISTS /dev/random )
      set( DEV_RANDOM "/dev/random" CACHE INTERNAL "" FORCE )
    endif( )
endif (NOT HAVE_ARC4RANDOM)
check_function_exists( arc4random_buf HAVE_ARC4RANDOM_BUF )

# Xau
pkg_search_module( XAU xau )
if( NOT XAU_FOUND )
  tde_message_fatal( "Xau are required, but not found on your system" )
endif()


# xdmcp
if( WITH_XDMCP )
  pkg_search_module( XDMCP xdmcp )
  if( XDMCP_FOUND )
    set( XDMCP 1 CACHE INTERNAL "" FORCE )
  else()
    tde_message_fatal( "xdmcp is requested, but was not found on your system" )
  endif()
endif()


if( WITH_PAM )

  set( USE_PAM 1 CACHE INTERNAL "" FORCE )

elseif( WITH_SHADOW )

  set( HAVE_SHADOW 1 CACHE INTERNAL "" FORCE )
  set( USESHADOW 1 CACHE INTERNAL "" FORCE )

endif( )


# If a tdm.service file is wanted, find systemd, then work out which
# distribution is running, select an appropriate template and create the file.
# When it is not possible to identify the distribution or there is no specific
# template is available, use the default of 'tde.service.cmake'. The template
# can also be set from the command line.

if( BUILD_TDM_SYSTEMD_UNIT_FILE )

  if( NOT SYSTEMDSYSTEMUNITDIR )
    pkg_search_module( SYSTEMD systemd )
    if( SYSTEMD_FOUND )
      execute_process(
        COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=systemdsystemunitdir systemd
        OUTPUT_VARIABLE SYSTEMDSYSTEMUNITDIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif( )
  endif( )

  if( "${SYSTEMDSYSTEMUNITDIR}" STREQUAL "" )
    set( SYSTEMDSYSTEMUNITDIR "/usr/lib/systemd/system" )
  endif( )

  if( NOT TDM_SERVICE_FILE_TEMPLATE )

    find_program( LSB_RELEASE lsb_release HINTS ENV PATH )
    unset( _DIST_ID_LIST )

    if( EXISTS "/etc/os-release" )

      file( READ "/etc/os-release" _OS_RELEASE )

      if( "\n${_OS_RELEASE}" MATCHES "\nID=\"*([^\"\n]*)" )
        set( _DISTRIBUTION "${CMAKE_MATCH_1}" )
      endif( )
      if( "\n${_OS_RELEASE}" MATCHES "\nVERSION_ID=\"*([^\\.\"\n]*)" )
        set( _DIST_VERSION "${CMAKE_MATCH_1}" )
      endif( )
      if( "\n${_OS_RELEASE}" MATCHES "\nID_LIKE=\"*([^\\.\"\n]*)" )
        string( REGEX REPLACE " " ";" _DIST_ID_LIST "${CMAKE_MATCH_1}" )
      endif( )

      if( _DISTRIBUTION )
        message( STATUS "Running ${_DISTRIBUTION} distribution, version ${_DIST_VERSION}" )
        string( TOLOWER "${_DISTRIBUTION}" _DISTRIBUTION )
        list( INSERT _DIST_ID_LIST 0 "${_DISTRIBUTION}" )
      endif( )

      foreach( _DIST_ID IN LISTS _DIST_ID_LIST )
        if( NOT TDM_SERVICE_FILE_TEMPLATE )
          if( _DIST_VERSION AND
              EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tdm.service.${_DIST_ID}-${_DIST_VERSION}.cmake" )
            set( TDM_SERVICE_FILE_TEMPLATE "tdm.service.${_DIST_ID}-${_DIST_VERSION}.cmake" )
          elseif( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tdm.service.${_DIST_ID}.cmake" )
            set( TDM_SERVICE_FILE_TEMPLATE "tdm.service.${_DIST_ID}.cmake" )
          endif( )
        endif( )
      endforeach( )

    else( )

      message(STATUS "**WARNING** /etc/os-release was not found. The default template for tdm.service will be used.")

    endif( )

    if( NOT TDM_SERVICE_FILE_TEMPLATE )
      set( TDM_SERVICE_FILE_TEMPLATE "tdm.service.cmake" )
    endif( )

    message( STATUS "tdm.service template file is ${TDM_SERVICE_FILE_TEMPLATE}" )

    configure_file( "${TDM_SERVICE_FILE_TEMPLATE}" tdm.service @ONLY )

  endif( )

endif( )
