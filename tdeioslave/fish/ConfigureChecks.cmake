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

check_include_file( stropts.h HAVE_STROPTS )
check_include_file( libutil.h HAVE_LIBUTIL_H )
check_include_file( util.h HAVE_UTIL_H )
check_include_file( pty.h HAVE_PTY_H )


tde_save( CMAKE_REQUIRED_LIBRARIES )
set( CMAKE_REQUIRED_LIBRARIES util )

if( HAVE_PTY_H )
  set( USE_OPENPTY_H pty.h )
elseif( HAVE_UTIL_H )
  set( USE_OPENPTY_H util.h )
elseif( HAVE_LIBUTIL_H )
  set( USE_OPENPTY_H libutil.h )
endif( )
if( USE_OPENPTY_H )
  check_c_source_runs("
      #include <${USE_OPENPTY_H}>
      int main(int argc, char* argv[]) {
        int master_fd, slave_fd;
        int result;
        result = openpty(&master_fd, &slave_fd, 0, 0, 0);
        return 0;
    }"
    HAVE_OPENPTY
  )
endif( )
if( HAVE_OPENPTY )
  set( LIB_UTIL util )
endif( )

tde_restore( CMAKE_REQUIRED_LIBRARIES )
