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

add_subdirectory( remote )
add_subdirectory( services )


##### other data ################################

tde_create_translated_desktop(
  SOURCE
    home.desktop root.desktop history.desktop bookmarks.desktop
    services.desktop remote.desktop system.desktop trash.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/entries
  PO_DIR konqueror-desktops
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/home.desktop
    ${CMAKE_CURRENT_BINARY_DIR}/root.desktop
    ${CMAKE_CURRENT_BINARY_DIR}/history.desktop
    ${CMAKE_CURRENT_BINARY_DIR}/bookmarks.desktop
    ${CMAKE_CURRENT_BINARY_DIR}/services.desktop
    ${CMAKE_CURRENT_BINARY_DIR}/remote.desktop
    ${CMAKE_CURRENT_BINARY_DIR}/trash.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/kicker_entries
)

tde_create_translated_desktop(
  SOURCE virtualfolderadd.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/add
  PO_DIR konqueror-desktops
)

tde_create_translated_desktop(
  SOURCE
    history_module.desktop dirtree_module.desktop bookmarks_module.desktop
  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/dirtree
  PO_DIR konqueror-desktops
)
