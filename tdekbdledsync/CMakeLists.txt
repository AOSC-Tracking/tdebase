#################################################
#
#  (C) 2013 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include_directories(
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### tdekbdledsync (executable) ################

tde_add_executable( tdekbdledsync
  SOURCES getfd.c main.cpp
  LINK ${UDEV_LIBRARIES} X11
  DESTINATION ${BIN_INSTALL_DIR}
  SETUID
)

