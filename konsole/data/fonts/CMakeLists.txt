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

install( FILES
    ${CMAKE_CURRENT_BINARY_DIR}/console8x16.pcf.gz
    ${CMAKE_CURRENT_BINARY_DIR}/9x15.pcf.gz
  DESTINATION ${DATA_INSTALL_DIR}/konsole/fonts )

# FIXME this should be improved, to guarantee that mkfontdir is executed after fonts are installed
install( CODE "execute_process( COMMAND mkfontdir \$ENV{DESTDIR}${DATA_INSTALL_DIR}/konsole/fonts )" )

# FIXME generating fonts can be improved

add_custom_target( konsole-font-8x16 ALL
  DEPENDS console8x16.pcf.gz )
add_custom_command( OUTPUT console8x16.pcf.gz
  COMMAND bdftopcf -o console8x16.pcf ${CMAKE_CURRENT_SOURCE_DIR}/console8x16.bdf
  COMMAND gzip -f --best console8x16.pcf
  DEPENDS console8x16.bdf )

add_custom_target( konsole-font-9x15 ALL
  DEPENDS 9x15.pcf.gz )
add_custom_command( OUTPUT 9x15.pcf.gz
  COMMAND bdftopcf -o 9x15.pcf ${CMAKE_CURRENT_SOURCE_DIR}/9x15.bdf
  COMMAND gzip -f --best 9x15.pcf
  DEPENDS 9x15.bdf )
