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

add_subdirectory( twinoptions )
add_subdirectory( twindecoration )
add_subdirectory( twinrules )

tde_create_translated_desktop(
  SOURCE showdesktop.desktop
  PO_DIR twin-desktops
)
