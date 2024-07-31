###############################################################################
#                                                                             #
# Copyright © 2024 Mavridis Philippe <mavridisf@gmail.com                     #
#                                                                             #
# Licensed under the terms of GNU General Public License v2 or later.         #
# Improvements are welcome!                                                   # 
#                                                                             #
###############################################################################

install(
  FILES remote_folder_icon.upd
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR}
)

install(
  PROGRAMS remote_folder_icon_upd.sh
  DESTINATION ${KCONF_UPDATE_INSTALL_DIR}
)