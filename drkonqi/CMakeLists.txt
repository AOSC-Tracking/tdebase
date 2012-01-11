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

add_subdirectory( presets )
add_subdirectory( pics )
add_subdirectory( debuggers )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### drkonqi (executable) ######################

tde_add_executable( drkonqi AUTOMOC
  SOURCES
    krashdcopinterface.skel main.cpp debugger.cpp
    krashconf.cpp drbugreport.cpp backtrace.cpp toplevel.cpp
  LINK kio-shared
  DESTINATION ${BIN_INSTALL_DIR}
)

##### crashtest (executable) ######################

if( BUILD_CRASHTEST )
  tde_add_executable( crashtest AUTOMOC
    SOURCES
      crashtest.cpp
    LINK kio-shared
    DESTINATION ${BIN_INSTALL_DIR}
  )
endif( BUILD_CRASHTEST )
