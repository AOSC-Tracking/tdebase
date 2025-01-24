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

file( GLOB _color_schemes_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.kcsrc )
tde_create_translated_desktop(
  SOURCE ${_color_schemes_files}
  DESTINATION ${DATA_INSTALL_DIR}/tdedisplay/color-schemes
  PO_DIR kcontrol-color-schemes
)
