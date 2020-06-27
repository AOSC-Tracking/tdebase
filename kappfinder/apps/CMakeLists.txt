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

file( GLOB_RECURSE _desktops RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.desktop )
list( SORT _desktops )

foreach( _desktop IN LISTS _desktops )
  get_filename_component( _category ${_desktop} PATH )
  tde_create_translated_desktop(
    SOURCE ${_desktop}
    DESTINATION ${DATA_INSTALL_DIR}/kappfinder/apps/${_category}
    PO_DIR kappfinder-data
  )
endforeach()
