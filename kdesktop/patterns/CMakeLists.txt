#################################################
#
#  (C) 2010-2011 Serghei Amelian
#  serghei (DOT) amelian (AT) gmail.com
#
#  (C) 2020 Slávek Banko
#  slavek.banko (AT) axis.cz
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

file( GLOB _patterns RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.png *.jpg )
list( SORT _patterns )

foreach( _pattern IN LISTS _patterns )
  get_filename_component( _pattern_name ${_pattern} NAME_WE )
  if( EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_pattern_name}.desktop )
    install(
      FILES ${_pattern}
      DESTINATION ${DATA_INSTALL_DIR}/kdesktop/patterns
    )
    tde_create_translated_desktop(
      SOURCE ${_pattern_name}.desktop
      DESTINATION ${DATA_INSTALL_DIR}/kdesktop/patterns
      PO_DIR kdesktop-desktops
    )
  endif()
endforeach()
