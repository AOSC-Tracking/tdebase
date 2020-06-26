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

add_subdirectory( default )
add_subdirectory( pics )

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
  SOURCE language.desktop
  PO_DIR kcontrol-desktops
)


##### kcm_locale (module) #######################

tde_add_kpart( kcm_locale AUTOMOC
  SOURCES
    main.cpp toplevel.cpp kcmlocale.cpp klocalesample.cpp
    localenum.cpp localemon.cpp localetime.cpp localeother.cpp
  LINK tdeio-shared
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
