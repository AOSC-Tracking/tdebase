#################################################
#
#  (C) 2015 Timothy Pearson
#  kb9vqf (AT) pearsoncomputing.net
#
#  Improvements and feedback are welcome
#
#  This file is released under GPL >= 2
#
#################################################

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/tdmlib
  ${TDE_INCLUDE_DIR}
  ${TQT_INCLUDE_DIRS}
)

link_directories(
  ${TQT_LIBRARY_DIRS}
)


##### tdecryptocardwatcher (executable) #########

tde_add_executable( tdecryptocardwatcher AUTOMOC
  SOURCES main.cpp watcher.cpp
  LINK tdecore-shared tdeio-shared dmctl-static
  DESTINATION ${BIN_INSTALL_DIR}
  SETUID
)

